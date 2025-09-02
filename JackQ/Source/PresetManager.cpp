#include "PresetManager.h"

namespace Service
{
    const juce::File PresetManager::defaultDirectory { juce::File::getSpecialLocation(juce::File::commonDocumentsDirectory)
        .getChildFile(ProjectInfo::companyName)
        .getChildFile(ProjectInfo::projectName)
    };
    const juce::String PresetManager::extension { "preset" };
    const juce::String PresetManager::presetNameProperty { "presetName" };

    PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvts)
        : valueTreeState(apvts)
    {
        if (!defaultDirectory.exists())
        {
            auto result = defaultDirectory.createDirectory();
            if (result.failed())
            {
                DBG("Could not create preset directory: " + result.getErrorMessage());
                jassertfalse;
            }
        }

        apvts.state.addListener(this);
        currentPreset.referTo(apvts.state.getPropertyAsValue(presetNameProperty, nullptr));
    }

    void PresetManager::savePreset(const juce::String& presetName)
    {
        if (presetName.isEmpty())
            return;

        currentPreset.setValue(presetName);

        auto xml = valueTreeState.copyState().createXml();
        auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
        if (!xml->writeTo(presetFile))
        {
            DBG("Could not create preset file: " + presetFile.getFullPathName());
            jassertfalse;
        }
    }

    void PresetManager::deletePreset(const juce::String& presetName)
    {
        if (presetName.isEmpty())
            return;

        auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
        if (!presetFile.existsAsFile())
        {
            DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
            jassertfalse;
            return;
        }

        if (!presetFile.deleteFile())
        {
            DBG("Preset file " + presetFile.getFullPathName() + " could not be deleted");
            jassertfalse;
            return;
        }

        currentPreset = "";
    }

    void PresetManager::loadPreset(const juce::String& presetName)
    {
        if (presetName.isEmpty())
            return;

        auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
        if (!presetFile.existsAsFile())
        {
            DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
            jassertfalse;
            return;
        }

        juce::XmlDocument xmlDocument { presetFile };
        auto valueTreeToLoad = juce::ValueTree::fromXml(*xmlDocument.getDocumentElement());

        valueTreeState.replaceState(valueTreeToLoad);

        // ✅ Properly rebind to the new tree after loading
        currentPreset.referTo(valueTreeState.state.getPropertyAsValue(presetNameProperty, nullptr));
    }

    int PresetManager::loadNextPreset()
    {
        auto allPresets = getAllPresets();
        if (allPresets.isEmpty())
            return -1;

        int currentIndex = allPresets.indexOf(currentPreset.toString());
        int nextIndex = (currentIndex + 1 > (allPresets.size() - 1)) ? 0 : currentIndex + 1;
        loadPreset(allPresets.getReference(nextIndex));
        return nextIndex;
    }

    int PresetManager::loadPreviousPreset()
    {
        auto allPresets = getAllPresets();
        if (allPresets.isEmpty())
            return -1;

        int currentIndex = allPresets.indexOf(currentPreset.toString());
        int previousIndex = (currentIndex - 1 < 0) ? allPresets.size() - 1 : currentIndex - 1;
        loadPreset(allPresets.getReference(previousIndex));
        return previousIndex;
    }

    juce::StringArray PresetManager::getAllPresets() const
    {
        juce::StringArray presets;
        auto fileArray = defaultDirectory.findChildFiles(juce::File::findFiles, false, "*." + extension);
        for (auto& file : fileArray)
        {
            presets.add(file.getFileNameWithoutExtension());
        }
        return presets;
    }

    juce::String PresetManager::getCurrentPreset() const
    {
        return currentPreset.toString();
    }

    void PresetManager::valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged)
    {
        currentPreset.referTo(treeWhichHasBeenChanged.getPropertyAsValue(presetNameProperty, nullptr));
    }

}

