#include "graphics.h"

namespace JackGraphics
{
    void SimpleLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                      float sliderPos, float startAng, float endAng,
                                      juce::Slider& s)
    {
        LookAndFeel_V4::drawRotarySlider(g, x, y, w, h, sliderPos, startAng, endAng, s);
    }

    void SimpleLnF::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                      float sliderPos, float minPos, float maxPos,
                                      const juce::Slider::SliderStyle style, juce::Slider& s)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minPos, maxPos, style, s);
    }

    void SimpleLnF::drawComboBox (juce::Graphics& g, int width, int height, bool /*isDown*/,
                                  int /*startX*/, int /*startY*/, int /*endX*/, int /*endY*/,
                                  juce::ComboBox& box)
    {
        auto bounds = juce::Rectangle<float>(0.f, 0.f, (float) width, (float) height);
        const float corner = 6.f;

        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(bounds, corner);

        g.setColour(juce::Colours::floralwhite);
        g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1.5f);

        // dropdown arrow
        auto arrowArea = juce::Rectangle<float>(width - 20.f, 0.f, 20.f, (float) height);
        auto cx = arrowArea.getCentreX();
        auto cy = arrowArea.getCentreY();

        juce::Path arrow;
        const float w = 8.f, h = 5.f;
        arrow.addTriangle(cx - w * 0.5f, cy - h * 0.5f,
                          cx + w * 0.5f, cy - h * 0.5f,
                          cx,            cy + h * 0.7f);

        g.setColour(juce::Colours::floralwhite);
        g.fillPath(arrow);

        // helpful defaults so text stays readable
        box.setColour(juce::ComboBox::textColourId, juce::Colours::floralwhite);
        box.setColour(juce::PopupMenu::textColourId, juce::Colours::floralwhite);
    }

    // 🔽 NEW: center label, leave room for arrow, white text
    void SimpleLnF::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        label.setBounds(6, 1, box.getWidth() - 26, box.getHeight() - 2);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(14.0f));
        label.setColour(juce::Label::textColourId, juce::Colours::floralwhite);
        label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    }

    void SimpleLnF::drawPopupMenuBackground(juce::Graphics& g, int w, int h)
    {
        g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
        g.setColour(juce::Colours::floralwhite.withAlpha(0.25f));
        g.drawRect(0, 0, w, h);
    }

    void SimpleLnF::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                       bool isSeparator, bool isActive, bool isHighlighted,
                                       bool isTicked, bool hasSubMenu,
                                       const juce::String& text, const juce::String& shortcut,
                                       const juce::Drawable* icon, const juce::Colour* textColour)
    {
        LookAndFeel_V4::drawPopupMenuItem(g, area, isSeparator, isActive, isHighlighted,
                                          isTicked, hasSubMenu, text, shortcut, icon, textColour);
    }
}

