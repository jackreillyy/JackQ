#include "filterComboBox.h"

filterComboBox::filterComboBox()
{
    setLookAndFeel(&customLNF);

    addItem("Low Cut", 1); //aka low cut
    addItem("High Cut", 2); //aka high cut
    addItem("High Shelf", 3);
    addItem("Low Shelf", 4);
    addItem("Bell", 5);
    addItem("Notch", 6); 

    
    setJustificationType(juce::Justification::centred);
    setSelectedId(1, juce::dontSendNotification);
    
    setColour(juce::ComboBox::outlineColourId, juce::Colours::floralwhite.withAlpha(0.8f));
    setColour(juce::ComboBox::textColourId, juce::Colours::floralwhite);
    setColour(juce::ComboBox::arrowColourId, juce::Colours::floralwhite);

    setColour(juce::PopupMenu::textColourId, juce::Colours::floralwhite);
    
    onChange = [this]() {
        if (onSelectionChanged)
            onSelectionChanged(getSelectedId());
    };
}

void filterComboBox::paint(juce::Graphics& g)
{
    juce::ComboBox::paint(g);
}

filterComboBox::~filterComboBox()
{
    setLookAndFeel(nullptr);
}
