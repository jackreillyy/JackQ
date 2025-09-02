#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "graphics.h"
#include "Colors.h"
#include "BinaryData.h"
#include "PresetManager.h"
#include "PresetPanel.h"


//slider LnF
class MeterLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MeterLookAndFeel() {}
    
    //no ugly ass thumb on my gain meters
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float, float, float, const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        g.setColour(slider.findColour(juce::Slider::backgroundColourId));
        g.fillRect(x,y,width,height);
        
        float proportion = slider.valueToProportionOfLength(slider.getValue());
        int fillH = juce::jlimit(0, height, (int)(proportion*height));
        
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillRect(x, y + (height - fillH), width, fillH);
    }
};

//-------------------------------------------------------------------

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel() {} //for knobs like gain freq Q
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle,
        float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(x, y, width, height);
        auto center = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;
        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        
        //dark grey fill for all
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(center.x, center.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(slider.findColour(juce::Slider::backgroundColourId));
        g.strokePath(backgroundArc, juce::PathStrokeType(7.0f));
        
        //white fill for current value shit
        juce::Path valueArc;
        valueArc.addCentredArc(center.x, center.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.strokePath(valueArc, juce::PathStrokeType(7.0f));
    }
};

//-------------------------------------------------------------------
//split button like    | |  type  | V |     for bypass and filter menu
class FilterButton : public juce::TextButton
{
public:
    using TypeChangeCallback = std::function<void(int)>;
    
    FilterButton(int bandIndex, juce::AudioProcessorValueTreeState& state, TypeChangeCallback onTypeChange) : TextButton(""), idx(bandIndex), apvts(state), typeChangeCallback(onTypeChange)
    {
        setClickingTogglesState(false);

        bypassAttachment.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(
            apvts,
            "Filter" + juce::String(idx+1) + "Bypass",
            *this));
        
        updateLabel();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        const int bypassZoneWidth = 20;
        const int arrowZoneStart = getWidth() - 20;
        
        if(e.x < bypassZoneWidth)
        {
            setToggleState(! getToggleState(), juce::NotificationType::sendNotification);
        }
        else if(e.x > arrowZoneStart)
        {
            showTypeMenu();
        }
        else
        {
            TextButton::mouseUp(e);
        }
    }
    
    struct FilterButtonLookAndFeel : public JackGraphics::SimpleLnF
    {
        juce::Font getTextButtonFont (juce::TextButton&, int /*buttonHeight*/) override
        {
            return juce::Font (10.0f);
        }
    };
    
    void paintButton(juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        //get standard button that we will write over later #owned
        juce::TextButton::paintButton(g, isMouseOver, isMouseDown);
        
        auto bounds = getLocalBounds();
        const int iconSize = 12;
        const int padding = 4;
        
        //bypass square hoe
        auto bypassArea = bounds.removeFromLeft(20).reduced(padding);
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(bypassArea, 2);
        if(! getToggleState())
            g.fillRect(bypassArea.reduced(2));
        
        //triangle dropdown bih
        auto dropArea = getLocalBounds().removeFromRight(20);
        juce::Path triangle;
        auto cx = dropArea.getCentreX();
        auto cy = dropArea.getCentreY();
        triangle.addTriangle(cx - iconSize/2, cy - iconSize/4,
                             cx + iconSize/2, cy - iconSize/4,
                             cx, cy + iconSize/2);
        g.setColour(isMouseOver ? juce::Colours::grey : juce::Colours::darkgrey);
        g.fillPath(triangle);
        
    }

private:
    int idx;
    juce::AudioProcessorValueTreeState& apvts;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    TypeChangeCallback typeChangeCallback;
    
    void showTypeMenu()
    {
        juce::PopupMenu m;
        m.addItem(1, "HighPass");
        m.addItem(2, "LowPass");
        m.addItem(3, "HighShelf");
        m.addItem(4, "LowShelf");
        m.addItem(5, "Bell");
        m.addItem(6, "Notch");
        
        m.setLookAndFeel(&getLookAndFeel());

        auto* parent = getTopLevelComponent(); // array parent in top (current)

        auto options = juce::PopupMenu::Options()
            .withTargetComponent(this)
            .withParentComponent(parent)
            .withMinimumWidth(160) // avoid tiny menus
            .withStandardItemHeight(20);

        m.showMenuAsync(options,
            [this](int result)
            {
                if (result > 0)
                {
                    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(
                            apvts.getParameter("Filter" + juce::String(idx+1) + "Type")))
                    {
                        float norm = (result - 1) / float(choiceParam->choices.size() - 1);
                        choiceParam->beginChangeGesture();
                        choiceParam->setValueNotifyingHost(norm);
                        choiceParam->endChangeGesture();
                        updateLabel();
                        typeChangeCallback(idx);

                    }
                }
            });
    }

    void updateLabel()
    {
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(
                apvts.getParameter("Filter" + juce::String(idx+1) + "Type")))
        {
            auto typeText = choiceParam->getCurrentValueAsText();
            setButtonText(juce::String(idx+1) + ". " + typeText);
            
        }
    }
};

//-------------------------------------------------------------------

class JackQAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer, public juce::Button::Listener
{
public:
    JackQAudioProcessorEditor (JackQAudioProcessor&);
    ~JackQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void timerCallback() override;
    
    // Button handler for both buttons, norm and overridden
    void buttonClicked (juce::Button*) override;
    
private:

    JackQAudioProcessor& processorRef;
    juce::AudioProcessorValueTreeState& apvts;
    
    int selectedFilter = 0;
    juce::OwnedArray<FilterButton> filterButtons;
    void updateFilterButtonStyles();
    
    juce::Slider freqSlider, gainSlider, qSlider;
    juce:: Label freqLabel, gainLabel, qLabel;

    // Attachments for the *currently selected* node so we know which node the sliders respond and adjust to
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> freqAttachment, gainAttachment, qAttachment;
    
    void refreshAttachments();

    //gain meter shit
    juce::Slider leftMeter, rightMeter;
    juce::Label leftMeterLabel, rightMeterLabel;
    MeterLookAndFeel meterLookAndFeel;
    
    std::unique_ptr<JackGraphics::SimpleLnF> lnf;
    
    //knob look n feel type
    KnobLookAndFeel knobLookAndFeel;

    FilterButton::FilterButtonLookAndFeel filterBtnLf;
    
    juce::Rectangle<int> spectrumArea;
    juce::Rectangle<int> leftMarginArea;
    
    juce::Rectangle<int> leftCol, mainCol, rightCol;
    juce::Rectangle<int> spectrumRow, controlsRow;
    
    static constexpr int leftMarginPx = 45;
    static constexpr int rightMarginPx = 175;
    
    juce::Image facePic;
    static constexpr int picMargin = 10;
    
    std::unique_ptr<Service::PresetManager> presetManager;
    std::unique_ptr<Gui::PresetPanel> presetPanel;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JackQAudioProcessorEditor)
};


