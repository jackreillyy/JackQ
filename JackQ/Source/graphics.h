#pragma once
#include <JuceHeader.h>

namespace JackGraphics
{
    using namespace juce;

    class SimpleLnF : public LookAndFeel_V4
    {
    public:
        void setThemeColours(Colour c) { SliderColour = c; }

        void setThemeColours(Colour c, Colour b1)
        {
            SliderColour        = c;
            BackgroundColourOne = b1;
            BackgroundColourTwo = c;
        }

        void setThemeColours(Colour c, Colour b1, Colour b2)
        {
            SliderColour        = c;
            BackgroundColourOne = b1;
            BackgroundColourTwo = b2;
        }
        
        SimpleLnF()
        {
            SliderColour = Colours::floralwhite;

            juce::LookAndFeel::setColour(juce::PopupMenu::textColourId,                  juce::Colours::floralwhite);
            juce::LookAndFeel::setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::black.withAlpha(0.5f));
            juce::LookAndFeel::setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black.withAlpha(0.5f));
            juce::LookAndFeel::setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colours::floralwhite.withAlpha(0.9f));
        }

        void drawRotarySlider (Graphics&, int, int, int, int,
                               float, float, float, Slider&) override;
        void drawLinearSlider (Graphics&, int, int, int, int,
                               float, float, float,
                               const Slider::SliderStyle, Slider&) override;

        // paint combo boxes
        void drawComboBox     (Graphics&, int width, int height, bool,
                               int, int, int, int, ComboBox&) override;

        // text
        void positionComboBoxText (ComboBox&, Label&) override;

        // menu background
        void drawPopupMenuBackground (Graphics&, int width, int height) override;

        void drawPopupMenuItem(Graphics&, const Rectangle<int>& area,
                               bool isSeparator, bool isActive, bool isHighlighted,
                               bool isTicked, bool hasSubMenu,
                               const String& text, const String& shortcut,
                               const Drawable* icon, const Colour* textColour) override;
        

    private:
        Colour SliderColour, BackgroundColourOne, BackgroundColourTwo;
    };
}

