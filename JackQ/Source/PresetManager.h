#pragma once

#include <JuceHeader.h>
#include "graphics.h"

namespace Service
{
class PresetManager : public JackGraphics::ValueTree::Listener
    {
    public:
        static const juce::File defaultDirectory;
        static const juce::String extension;
        static const juce::String presetNameProperty;
        
        PresetManager(juce::AudioProcessorValueTreeState& apvts);

        void savePreset(const juce::String& presetName);
        void deletePreset(const juce::String& presetName);
        void loadPreset(const juce::String& presetName);
        int loadNextPreset();
        int loadPreviousPreset();
        juce::StringArray getAllPresets() const;
        juce::String getCurrentPreset() const;
        
    private:
        void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) ;

        juce::AudioProcessorValueTreeState& valueTreeState;
        juce::Value currentPreset;
    };
}
