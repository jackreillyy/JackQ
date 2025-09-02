#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "jackFilters.h"
#include <algorithm>
#include <cmath>
#include <JuceHeader.h>
#include <cstring>


JackQAudioProcessor::JackQAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, juce::Identifier ("PARAMS"), createParameterLayout())
{
    for (int i = 0; i < 8; ++i) //MAKING THIS 8 BANDS NOW BC IMA BIG BOY
        filterBands.add (new jackFilters::filterBand());
}

JackQAudioProcessor::~JackQAudioProcessor() {}

bool JackQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto inLayout = layouts.getMainInputChannelSet();
    auto outLayout = layouts.getMainOutputChannelSet();
    
    return inLayout == outLayout;
}


EQParams JackQAudioProcessor::createParameterLayout()
{
    using juce::ParameterID;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray typeNames { "HighPass", "LowPass", "HighShelf", "LowShelf", "Bell", "Notch" };
    
    for (int i = 1; i <= 8; ++i)
    {
        auto base = "Filter" + juce::String(i);

        // Type choice
        layout.add (std::make_unique<juce::AudioParameterChoice>(
            ParameterID { base + "Type", 1 }, base + " Type",
            typeNames, 4));                            // default to a bell bc its neutral widdit

        // Frequency (20–20k, skewed at 1k)
        auto freqRange = juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f);
        freqRange.setSkewForCentre (1000.0f);
        layout.add (std::make_unique<juce::AudioParameterFloat>(
            ParameterID { base + "Freq", 1 }, base + " Freq",
            freqRange, 1000.0f));

        // Gain
        layout.add (std::make_unique<juce::AudioParameterFloat>(
            ParameterID { base + "Gain", 1 }, base + " Gain",
            juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

        // Q
        layout.add (std::make_unique<juce::AudioParameterFloat>(
            ParameterID { base + "Q", 1 }, base + " Q",
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 1.0f));
        // Bypass on/off
        layout.add (std::make_unique<juce::AudioParameterBool>(
            ParameterID{ base + "Bypass", 1 },
            base + " Bypass",
            true
        ));
    }
    
    return layout;
}

void JackQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    const auto numChannels = getTotalNumOutputChannels(); //mono or stereo
    
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<JackGraphics::uint32>(samplesPerBlock), static_cast<JackGraphics::uint32>(numChannels)};
    
    for(auto* band : filterBands)
        band->prepare(sampleRate, samplesPerBlock, numChannels);

    updateFilters();
    
    // FFT visual preparation
    std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    fifoIndex = 0;
    fftOverlapCount = 0;
    nextFFTReady.store(false);

}

void JackQAudioProcessor::releaseResources() {}

void JackQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateFilters();
    
    for (int i = 0; i < filterBands.size(); ++i)
    {
        auto base = "Filter" + juce::String (i + 1);
        bool isByp = *apvts.getRawParameterValue (base + "Bypass") > 0.5f;

        if (! isByp)
            filterBands[i]->process (buffer);
    }

    //mono use left channel, but if theres 2 channels (>1) also use the right channel
    auto* leftPtr = buffer.getReadPointer(0);
    auto *rightPtr = buffer.getNumChannels() > 1
    ? buffer.getReadPointer(1) : nullptr;
    float lp = 0.0f, rp = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        lp = std::max(lp, std::abs(leftPtr[i]));
        if(rightPtr) rp = std::max(rp, std::abs(rightPtr[i]));
    }
    
    //gain level value storing for plotting
    leftLevelDb.store (juce::Decibels::gainToDecibels(lp, -100.0f));
    rightLevelDb.store (juce::Decibels::gainToDecibels(rp, -100.0f));

    //fft feed
    auto* readPtr = buffer.getReadPointer (0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        pushNextSample (readPtr[i]);
}

void JackQAudioProcessor::updateFilters()
{
    double sr = getSampleRate();

    for (int i = 0; i < filterBands.size(); ++i)
    {
        auto* band = filterBands[i];
        auto  index = i + 1;
        auto  base = "Filter" + juce::String (index);

        float f = *apvts.getRawParameterValue (base + "Freq");
        float g = *apvts.getRawParameterValue (base + "Gain");
        float q = *apvts.getRawParameterValue (base + "Q");
        
        auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(base + "Type"));
        int t = choiceParam != nullptr ? choiceParam->getIndex() : 0;
        //int t = int (*apvts.getRawParameterValue (base + "Type"));

        // clamp
        f = juce::jlimit (20.0f, float (sr * 0.5), f);
        q = juce::jlimit (0.1f, 10.0f, q);

        // type enums are 0…5 → we defined None=0, so offset by +1
        auto typeEnum = static_cast<jackFilters::filterType> (t+1);

        band->setParameters (typeEnum, f, q, g);
    }
}


// Pushes one sample into the FIFO... when full, runs the FFT (circular buffer type shit)
void JackQAudioProcessor::pushNextSample (float sample) noexcept
{
    inputFifo[fifoIndex++] = sample;
    if(fifoIndex >= fftSize) fifoIndex = 0;
    
    if(++fftOverlapCount >= hopSize)
    {
        fftOverlapCount = 0;
        processFrame();
    }
}

void JackQAudioProcessor::processFrame() noexcept
{
    int pos = fifoIndex;
    auto* fftPtr = fftData.data();
    int tail = fftSize - pos;
    
    std::memcpy(fftPtr, inputFifo.data() + pos, tail * sizeof(float));
    std::memcpy(fftPtr + tail, inputFifo.data(), pos * sizeof(float));

    window.multiplyWithWindowingTable(fftPtr, fftSize);

    forwardFFT.performFrequencyOnlyForwardTransform(fftPtr);
    
    const float scale = windowCorrection / (fftSize / 2.0f);

    for (int bin = 0; bin < numBins; ++bin)
    {
        float mag = fftPtr[bin] * scale;
        float magDb = juce::Decibels::gainToDecibels (mag, -100.0f);
        scopeData[bin] = magDb;
    }

    nextFFTReady.store(true);
}

bool JackQAudioProcessor::getFFTReadyAndReset() noexcept
{
    return nextFFTReady.exchange (false);
}

const std::vector<float>& JackQAudioProcessor::getScopeData() const noexcept
{
    return scopeData;
}

juce::AudioProcessorEditor* JackQAudioProcessor::createEditor()
{
    return new JackQAudioProcessorEditor (*this);
}

void JackQAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void JackQAudioProcessor::setStateInformation (const void* data, int size)
{
    // correctly deserialize the binary back into an XmlElement
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, size));
    if (xmlState != nullptr)
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JackQAudioProcessor();
}


