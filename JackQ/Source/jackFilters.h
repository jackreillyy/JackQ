#pragma once

#include <JuceHeader.h>

namespace jackFilters
{
    enum class filterType
    {
        None,
        HighPass,
        LowPass,
        HighShelf,
        LowShelf,
        Bell,
        Notch
            
    };

    struct filterBand
    {
        //duplicating to have L and R channels not just mono #stereoMoment
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,juce::dsp::IIR::Coefficients<float>> filter;
        
        double currentSampleRate = 48000.0;
        
        //prepare filters input goes into before processing
        void prepare(double sampleRate, int samplesPerBlock, int numChannels) noexcept
        {
            currentSampleRate = sampleRate;

            juce::dsp::ProcessSpec spec {
                sampleRate,static_cast<juce::uint32> (samplesPerBlock),static_cast<juce::uint32> (numChannels)
            };

            // prepare with the host content (input bozo)
            filter.prepare (spec);

            // clear any old delay‐lines for when a filter changes state so it recalculates
            filter.reset();
        }
        
        //none doesnt need to be an option on dropdown so it goes here
        filterType lastType { filterType::None };

        void setParameters (filterType type, float freq, float Q, float gainDb) noexcept
        {
            //stock filters
            using Coeff = juce::dsp::IIR::Coefficients<float>;
            
            if (type == filterType::None)
            {
                lastType = type;
                return;
            }
            
            Coeff::Ptr newCoeffs;

            // convert dB‑gain into linear (log - ln)
            float linearGain = std::pow (10.0f, gainDb * 0.05f);

            switch (type)
            {
                case filterType::HighPass:newCoeffs = Coeff::makeHighPass(currentSampleRate, freq, Q); break;
                case filterType::LowPass:newCoeffs = Coeff::makeLowPass(currentSampleRate, freq, Q); break;
                case filterType::HighShelf:newCoeffs = Coeff::makeHighShelf(currentSampleRate, freq, Q, linearGain); break;
                case filterType::LowShelf:newCoeffs = Coeff::makeLowShelf(currentSampleRate, freq, Q, linearGain); break;
                case filterType::Bell:newCoeffs = Coeff::makePeakFilter(currentSampleRate, freq, Q, linearGain); break;
                case filterType::Notch:newCoeffs = Coeff::makeNotch(currentSampleRate, freq, Q); break;
            }

            // assign the new, normalized coefficients:
            *filter.state = *newCoeffs;
            
            //check for filter changes during loop
            if (type != lastType)
                filter.reset();

            lastType = type; //called only for big changes like filter type change, not every buffer
        }
        
        void process(juce::AudioBuffer<float>& buffer)
        {
            juce::dsp::AudioBlock<float> block (buffer);

            juce::dsp::ProcessContextReplacing<float> ctx (block);
            filter.process (ctx);
            
        }
    };
}



