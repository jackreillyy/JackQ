#pragma once

#include <JuceHeader.h>
#include "graphics.h"

class filterComboBox : public juce::ComboBox
{
public:
    filterComboBox();
    ~filterComboBox() override;

    void paint(juce::Graphics& g) override;
    
    std::function<void(int)> onSelectionChanged;

private:
    JackGraphics::SimpleLnF customLNF;

};
