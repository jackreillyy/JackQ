#pragma once
#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <atomic>
#include "jackFilters.h"

// Alias for parameter layout
using EQParams = juce::AudioProcessorValueTreeState::ParameterLayout;

class JackQAudioProcessor  : public juce::AudioProcessor
{
public:
    JackQAudioProcessor();
    ~JackQAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "JackQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    
    //APVTS da GOAT
    juce::AudioProcessorValueTreeState apvts;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    
    //for updating coefficients
    juce::dsp::IIR::Coefficients<float>::Ptr getFilterCoefficients (int index) const
    {
        return filterBands[index]->filter.state;
    }
    
    // FFT helpers for the editor
    bool getFFTReadyAndReset() noexcept;
    const std::vector<float>& getScopeData() const noexcept;

    //for gain metering
    float getLeftLevelDb() const noexcept { return leftLevelDb.load();}
    float getRightLevelDb() const noexcept { return rightLevelDb.load();}
    
    static EQParams createParameterLayout();

private:

    juce::OwnedArray<jackFilters::filterBand> filterBands;
    void updateFilters();

    // FFT configuration
    static constexpr int fftOrder = 13;               // 2^10 = 1024 //changed to 12 so its not 4096 (get down to 11 hz resolution but more cpu)
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int numBins = fftSize / 2 + 1; //513 bins (512+1)
    static constexpr int overlap = 4; //75 % windowin overlap
    static constexpr int hopSize = fftSize / overlap; //256 sample
    static constexpr float windowCorrection = 2.0f / 3.0f; //hann

    juce::dsp::FFT forwardFFT{ fftOrder };
    juce::dsp::WindowingFunction<float> window
    { fftSize + 1,
          juce::dsp::WindowingFunction<float>::WindowingMethod::hann,
    false };
    std::vector<float> inputFifo = std::vector<float>(fftSize, 0.0f);
    std::vector<float> fftData = std::vector<float>(fftSize*2, 0.0f);
    int fifoIndex = 0;
    int fftOverlapCount = 0;
    
    std::vector<float> scopeData = std::vector<float>(numBins,  -100.0f);
    std::atomic<bool> nextFFTReady = false;
    
    void pushNextSample (float sample) noexcept;
    void processFrame() noexcept;
    
    //for gain level metering
    std::atomic<float> leftLevelDb { -100.0f};
    std::atomic<float> rightLevelDb { -100.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JackQAudioProcessor)
};

