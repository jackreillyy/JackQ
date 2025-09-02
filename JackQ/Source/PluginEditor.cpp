#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "jackFilters.h"
#include "filterComboBox.h"
#include "Colors.h"
#include <algorithm>
#include <cmath>
#include <complex>

using namespace juce;

JackQAudioProcessorEditor::JackQAudioProcessorEditor (JackQAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p), apvts (p.apvts)
{
    // Custom Look&Feel shit stolen from orbit #purple
    lnf = std::make_unique<JackGraphics::SimpleLnF>();
    lnf->setThemeColours (ColorScheme::sliderColour,
                    ColorScheme::backgroundColour1,
                    ColorScheme::backgroundColour2);
    setLookAndFeel (lnf.get());
    
    //FACE PIC SHIT LOLLLLL
    auto raw = juce::ImageCache::getFromMemory (BinaryData::selfiefour_png, BinaryData::selfiefour_pngSize);
    facePic = raw.rescaled (128, 128, juce::Graphics::highResamplingQuality);

    auto w = facePic.getWidth();
    auto h = facePic.getHeight();
    juce::Image mask (juce::Image::SingleChannel, w, h, true);
    {
        juce::Graphics gm (mask);
        // Centre = opaque, edges = transparent
        juce::ColourGradient grad (
            juce::Colours::floralwhite, w * 0.5f, h * 0.5f,
            juce::Colours::floralwhite.withAlpha (0.0f), w * 0.5f + jmax (w, h) * 0.5f, h * 0.5f,
            true
        );
        gm.setFillType (juce::FillType (grad));
        gm.fillRect (0, 0, w, h);
    }

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float m = mask.getPixelAt (x, y).getFloatRed();
            facePic.multiplyAlphaAt (x, y, m);
        }
     
    //FACE PIC SHIT ENDS
    
    
    // Combined FilterButtons
    for (int i = 0; i < 8; ++i)
    {
        auto* fb = new FilterButton(i, apvts,
            [this](int idx)
            {
                selectedFilter = idx;
                refreshAttachments();
                updateFilterButtonStyles();
            
            });
        fb->setLookAndFeel(&filterBtnLf);
        fb->addListener(this);
        filterButtons.add(fb);
        addAndMakeVisible(fb);
    }
    

    auto setupKnob = [&](Slider& s, const String& suf)
    {
        s.setSliderStyle(Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
        s.setTextValueSuffix(suf);
        s.setLookAndFeel(&knobLookAndFeel);
        s.setColour(Slider::trackColourId, Colours::floralwhite);
        addAndMakeVisible(s);

    };
    
        //knobs:
        setupKnob (freqSlider, " Hz");
        setupKnob (gainSlider, " dB");
        setupKnob (qSlider, "");
    
        freqLabel.setText("Freq", juce::dontSendNotification);
        freqLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(freqLabel);

        gainLabel.setText("Gain", juce::dontSendNotification);
        gainLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(gainLabel);

        qLabel.setText("Q", juce::dontSendNotification);
        qLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(qLabel);
    
        freqSlider.setLookAndFeel(&knobLookAndFeel);
        gainSlider.setLookAndFeel(&knobLookAndFeel);
        qSlider.setLookAndFeel(&knobLookAndFeel);
    
        freqSlider.setColour (juce::Slider::trackColourId, Colours::floralwhite);
        gainSlider.setColour (juce::Slider::trackColourId, Colours::floralwhite);
        qSlider.setColour (juce::Slider::trackColourId, Colours::floralwhite);


        //left meter
        addAndMakeVisible (leftMeter);
        leftMeter.setSliderStyle (Slider::LinearVertical);
        leftMeter.setTextBoxStyle (Slider::TextBoxBelow, false, 50, 20);
        leftMeter.setNumDecimalPlacesToDisplay (1);
        leftMeter.setRange (-100.0, 12.0);
        leftMeter.setEnabled (false);
        leftMeter.setColour (Slider::trackColourId, Colours::floralwhite);
        leftMeter.setColour (Slider::backgroundColourId, Colours::transparentBlack);

        addAndMakeVisible (leftMeterLabel);
        leftMeterLabel.setText ("L", dontSendNotification);
        leftMeterLabel.setJustificationType (Justification::centred);
        
        leftMeter.setLookAndFeel (&meterLookAndFeel);

        //right meter
        addAndMakeVisible (rightMeter);
        rightMeter.setSliderStyle (Slider::LinearVertical);
        rightMeter.setTextBoxStyle (Slider::TextBoxBelow, false, 50, 20);
        rightMeter.setNumDecimalPlacesToDisplay (1);
        rightMeter.setRange (-100.0, 12.0);
        rightMeter.setEnabled (false);
        rightMeter.setColour (Slider::trackColourId, Colours::floralwhite);
        rightMeter.setColour (Slider::backgroundColourId, Colours::transparentBlack);

        addAndMakeVisible (rightMeterLabel);
        rightMeterLabel.setText ("R", dontSendNotification);
        rightMeterLabel.setJustificationType (Justification::centred);

        rightMeter.setLookAndFeel (&meterLookAndFeel);
    
        //preset panel
        presetManager = std::make_unique<Service::PresetManager>(apvts);
        presetPanel   = std::make_unique<Gui::PresetPanel>(*presetManager);

        presetPanel->onPresetLoaded = [this](const juce::String&)
        {
            refreshAttachments(); // updates knobs based on pulled values
            repaint();
        };

        addAndMakeVisible(*presetPanel);

        //other shit
        setSize (1100, 700);
        setResizable (false, false);
        startTimerHz (30); //120 caused flashes and glitches i think so maybe keeping at 30 is ok
        
        selectedFilter = 0;
        refreshAttachments();
        updateFilterButtonStyles();

}

//==============================================================================
JackQAudioProcessorEditor::~JackQAudioProcessorEditor()
{
    for (auto* fb : filterButtons)
        fb->setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);

    freqSlider.setLookAndFeel(nullptr);
    gainSlider.setLookAndFeel(nullptr);
    qSlider.setLookAndFeel(nullptr);
    leftMeter.setLookAndFeel(nullptr);
    rightMeter.setLookAndFeel(nullptr);
    lnf.reset();
}

void JackQAudioProcessorEditor::buttonClicked (juce::Button* b)
{
    for (int i = 0; i < filterButtons.size(); ++i)
    {
        if (b == filterButtons[i])
        {
            selectedFilter = i;
            refreshAttachments();
            updateFilterButtonStyles();
            return;
        }
    }
}

void JackQAudioProcessorEditor::updateFilterButtonStyles()
{
    for (int i = 0; i < filterButtons.size(); ++i)
    {
        auto* fb = filterButtons[i];
        if (i == selectedFilter)
        {
            fb->setColour(TextButton::buttonColourId, Colours::plum);
        }
        else
        {
            bool isByp = *apvts.getRawParameterValue(
                             "Filter" + String(i+1) + "Bypass") > 0.5f;
            fb->setColour(TextButton::buttonColourId,
                isByp ? Colours::silver : Colours::thistle);
        }
    }
}


void JackQAudioProcessorEditor::refreshAttachments()
{
    auto base = "Filter" + String (selectedFilter + 1);

    // Tear down old attachments
    freqAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();

    // pull the actual parameter values (keep updating and refreshing to be accurate)
    freqSlider.setValue (*apvts.getRawParameterValue (base + "Freq"),
                         juce::dontSendNotification);
    gainSlider.setValue (*apvts.getRawParameterValue (base + "Gain"),
                         juce::dontSendNotification);
    qSlider.setValue (*apvts.getRawParameterValue (base + "Q"),
                         juce::dontSendNotification);


    //listen for changes to reset and get relevant (correct filer #) parameters
    freqAttachment.reset (new SliderAttachment (apvts, base + "Freq", freqSlider));
    gainAttachment.reset(new SliderAttachment (apvts, base + "Gain", gainSlider));
    qAttachment   .reset(new SliderAttachment (apvts, base + "Q",    qSlider));

    // gain knob greyed when not needed
    if (auto* choice = dynamic_cast<AudioParameterChoice*> (
             apvts.getParameter (base + "Type")))
    {
        int t = choice->getIndex();
        bool gainOk = (t != 0 && t != 1 && t != 5);
        gainSlider.setEnabled (gainOk);
        gainSlider.setColour (Slider::trackColourId,
            gainOk ? Colours::floralwhite
                   : Colours::lightslategrey);
    }

    updateFilterButtonStyles();
}




//==============================================================================
void JackQAudioProcessorEditor::timerCallback()
{
    // meters
    leftMeter.setValue  (processorRef.getLeftLevelDb(),  dontSendNotification);
    rightMeter.setValue (processorRef.getRightLevelDb(), dontSendNotification);

    repaint();
}
//==============================================================================
void JackQAudioProcessorEditor::paint (Graphics& g)
{
    static constexpr float minGainDb = -12.0f, maxGainDb = 12.0f;
    static constexpr float minFreq   = 20.0f,   maxFreq   = 20000.0f;
    const int tickSize = 5;

    // background gradient
    ColourGradient grad (ColorScheme::backgroundColour1, 0, 0,
                         ColorScheme::backgroundColour2, 0, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();

    // split off top half for spectrum
    auto area = getLocalBounds();
    area.removeFromLeft  (leftMarginPx);
    area.removeFromRight (rightMarginPx);
    auto topArea = area.removeFromTop (getHeight() * 2 / 3);
    spectrumArea = topArea.reduced (0, 35);

    // draw border
    g.setColour (Colours::darkgrey);
    g.drawRect (spectrumArea, 2);

    // y-axis
    int yAxisX = spectrumArea.getX();
    g.setColour (Colours::floralwhite);
    g.drawLine (yAxisX, spectrumArea.getY(), yAxisX, spectrumArea.getBottom());

    g.setFont (12.0f);

    // horizontal gain lines & labels DB
    for (auto gd : { maxGainDb, 3*maxGainDb/4, maxGainDb/2, maxGainDb/4,
                      0.0f, minGainDb/4, minGainDb/2, 3*minGainDb/4, minGainDb })
    {
        float y = jmap (gd,
                        minGainDb, maxGainDb,
                        (float) spectrumArea.getBottom(),
                        (float) spectrumArea.getY());

        g.setColour (gd == 0.0f ? Colours::floralwhite : Colours::darkgrey);
        g.drawHorizontalLine ((int) y, (float) spectrumArea.getX(), (float) spectrumArea.getRight());

        g.setColour (Colours::floralwhite);
        g.drawLine ((float) spectrumArea.getX(), y,
                    (float) spectrumArea.getX() + tickSize, y);

        // label dB values aligned to left of spectrum
        float textX = spectrumArea.getX() - tickSize - 10;
        g.drawFittedText (String ((int) gd) + "dB", (int) textX - 38, (int) y - 8, leftMarginPx, 16, Justification::right, 1);
    }

    // vertical freq lines & labels HZ
    for (auto f : { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 })
    {
        float norm = (std::log10 (f) - std::log10 (minFreq))
                   / (std::log10 (maxFreq) - std::log10 (minFreq));
        float x = spectrumArea.getX() + norm * spectrumArea.getWidth();

        g.setColour (Colours::darkgrey);
        g.drawLine (x, (float) spectrumArea.getY(), x, (float) spectrumArea.getBottom());
        g.drawLine (x, (float) spectrumArea.getBottom(), x, spectrumArea.getBottom() + tickSize);

        String lbl = f >= 1000 ? String (f / 1000.0f, 1) + "kHz" : String ((int) f) + "Hz";
        g.setColour (Colours::floralwhite);
        g.drawText (lbl, x - 20, spectrumArea.getBottom() + tickSize + 2, 50, 16, Justification::centred);
    }

    // FFT spectrum
    {
        const auto& data = processorRef.getScopeData();
        const size_t n = data.size();
        static constexpr float minFftDb = -80.0f, maxFftDb = 0.0f;

        if (n > 1)
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (spectrumArea);

            const float sr = (float) processorRef.getSampleRate();
            //get min and max for accurate plotting on the spectrum
            const float logMin = std::log10 (minFreq);
            const float logMax = std::log10 (maxFreq);

            Path fftPath;
            bool first = true;
            for (size_t i = 0; i < n; ++i)
            {
                float fHz = (float) i * (sr * 0.5f) / (float) (n - 1);
                if (fHz < minFreq || fHz > maxFreq) continue;

                float normF = (std::log10 (fHz) - logMin) / (logMax - logMin);
                float x = spectrumArea.getX() + normF * spectrumArea.getWidth();
                float db = jlimit (minFftDb, maxFftDb, data[i]);
                float y = jmap (db, minFftDb, maxFftDb, (float) spectrumArea.getBottom(), (float) spectrumArea.getY());

                if (first) { fftPath.startNewSubPath (x, y); first = false; }
                else { fftPath.lineTo(x, y);}
            }

            g.setColour (Colours::thistle);
            g.strokePath (fftPath, PathStrokeType (2.0f));
        }
    }

    // Filter curves (aligned with grid)
    {
        Graphics::ScopedSaveState css (g);
        g.reduceClipRegion (spectrumArea);

        const float logMin = std::log10 (minFreq);
        const float logMax = std::log10 (maxFreq);
        auto freqToX = [&](float fHz)
        {
            auto norm = (std::log10 (fHz) - logMin) / (logMax - logMin);
            return spectrumArea.getX() + norm * spectrumArea.getWidth();
        };

        const int numPts = spectrumArea.getWidth();
        const float sr = (float) processorRef.getSampleRate();

        Path totalResp;
        totalResp.preallocateSpace (numPts * 3);

        for (int i = 0; i < numPts; ++i)
        {
            float normX = (float) i / (float) (numPts - 1);
            float fHz = std::pow (10.0f, logMin + normX * (logMax - logMin));

            // start at unity gain
            float totalMag = 1.0f;

            // multiply each band's response to cascade filters
            constexpr int numBands = 8;
            for (int b = 0; b < numBands; ++b)
            {
                auto base = "Filter" + juce::String(b+1);
                bool isByp = *apvts.getRawParameterValue(base + "Bypass") > 0.5f;
                if(isByp)
                    continue;
                
                auto coeffs = processorRef.getFilterCoefficients (b);
                totalMag *= (float) coeffs->getMagnitudeForFrequency (fHz, sr);
            }

            // guard against zero, convert to dB
            totalMag = jmax (totalMag, 1e-6f);
            float totalDb = Decibels::gainToDecibels (totalMag);
            
            totalDb = juce::jlimit(minGainDb,maxGainDb, totalDb);

            // map to screen coordinates
            float x = freqToX (fHz);
            float y = jmap (totalDb, minGainDb, maxGainDb, (float) spectrumArea.getBottom(), (float) spectrumArea.getY());

            if (i == 0) totalResp.startNewSubPath (x, y);
            else totalResp.lineTo (x, y);
        }

        // draw combined EQ curve
        g.setColour (Colours::white);
        g.strokePath (totalResp, PathStrokeType (3.0f));
    }
    
    //FACE PIC
    if(facePic.isValid())
    {
        constexpr int faceMargin = 20;  // px from edges
        constexpr int faceW = 150;
        constexpr int faceH = 150;

        int x = getWidth()  - faceW - faceMargin;
        int y = getHeight() - faceH - faceMargin;

        g.drawImage ( facePic,
                     x, y, faceW, faceH,          // dest rect
                     0, 0,
                     facePic.getWidth(),
                     facePic.getHeight(),
                     false ); // false = draw full alpha
    
    }
}

//==============================================================================
void JackQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromLeft (leftMarginPx);
    area.removeFromRight (rightMarginPx);

    const int topHeight = getHeight() * 2 / 3;
    auto topArea = area.removeFromTop (topHeight);
    
    //PRESET MANAGER
    const int presetRowH = 40;                        // tweak 30-50 idk
    auto presetRow = topArea.removeFromTop (presetRowH);
    if (presetPanel)
        presetPanel->setBounds (presetRow.reduced (4, 2));

    topArea.reduce(0, 5);
    
    //meters
    const int meterRegionWidth = rightMarginPx - 10;
    auto meterRegion = topArea.removeFromRight (meterRegionWidth).reduced (10, 20);

    spectrumArea = topArea.reduced (0, 25);

    //two meters inside meterRegion
    const int meterGap = 4;
    const int singleW = (meterRegion.getWidth() - meterGap) / 2;
    auto leftArea = meterRegion.removeFromLeft (singleW);
    meterRegion.removeFromLeft (meterGap);
    auto rightArea = meterRegion;

    leftMeter.setBounds (leftArea.translated(175, 0));
    rightMeter.setBounds (rightArea.translated ( 175, 0 ));
    leftMeterLabel.setBounds (leftArea.getX() + 175, leftArea.getBottom()+2, singleW, 20);
    rightMeterLabel.setBounds (rightArea.getX() + 175, rightArea.getBottom()+2, singleW, 20);

    // Bottom 1/3 is for controls like knobs n nodes (filter row too)
    auto controlsArea = area.reduced (0, 10);

    // Filter-button row (fixed)
    const int filterRowH = 30;
    auto filterRow = controlsArea.removeFromTop (filterRowH);
    const int nF = filterButtons.size();
    const int fGap = 5;
    const int fW = (filterRow.getWidth() - (nF - 1) * fGap) / nF;

    for (auto* fb : filterButtons)
    {
        fb->setBounds (filterRow.removeFromLeft (fW).reduced (2));
        filterRow.removeFromLeft (fGap);
    }

    // Knob-row
    auto knobRow = controlsArea.reduced(0, 10);
    const int knobCount = 3;
    const int labelH = 20;
    const int maxKnobSize = knobRow.getHeight() - labelH - 10;  // 10px vertical padding
    const int availW = knobRow.getWidth();
    const int knobSpacing = 20;
    const int knobSize = juce::jmin(maxKnobSize,
                                        (availW - knobSpacing * (knobCount - 1)) / knobCount);
    const int totalKnobsW = knobCount * knobSize + (knobCount - 1) * knobSpacing;
    const int startX = knobRow.getX() + (availW - totalKnobsW) / 2;
    const int startY = knobRow.getY() + (knobRow.getHeight() - knobSize - labelH);

    freqSlider.setBounds(startX, startY, knobSize, knobSize);
    gainSlider.setBounds(startX + (knobSize + knobSpacing), startY, knobSize, knobSize);
    qSlider.setBounds(startX + 2*(knobSize + knobSpacing), startY, knobSize, knobSize);

    freqLabel.setBounds(freqSlider .getX(), freqSlider .getBottom(), knobSize, labelH);
    gainLabel.setBounds(gainSlider .getX(), gainSlider .getBottom(), knobSize, labelH);
    qLabel.setBounds(qSlider.getX(), qSlider.getBottom(), knobSize, labelH);
}
