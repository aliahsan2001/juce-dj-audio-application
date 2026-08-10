/*
  ==============================================================================

    ChannelStrip.cpp
    Author:  Syed Ali Ahsan
    Deck control strip – implementation for PulseStation

  ==============================================================================
*/

// --- WRITTEN BY SYED ALI AHSAN (R4: Equaliser, R3: Hot Cues UI) ---

#include "../JuceLibraryCode/JuceHeader.h"
#include "ChannelStrip.h"
#include <cmath>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ChannelStrip::ChannelStrip(PlaybackCore* _core,
                           TrackVault* _vault,
                           AudioFormatManager& fmtMgr,
                           AudioThumbnailCache& thumbCache,
                           int _deckIdx)
    : core(_core),
      vault(_vault),
      waveView(fmtMgr, thumbCache),
      deckIdx(_deckIdx)
{
    // --- Transport buttons ---
    for (auto* b : { &engageBtn, &haltBtn, &browseBtn, &wipeCuesBtn })
    {
        addAndMakeVisible(b);
        b->addListener(this);
        b->setColour(TextButton::buttonColourId, Colour(0xff2d2440));
        b->setColour(TextButton::textColourOffId, Colour(0xffddccee));
    }

    // --- WRITTEN BY SYED ALI AHSAN (R3A, R3B: Hot-cue buttons UI limit to 8 slots) ---
    for (int i = 0; i < 8; ++i)
    {
        cueSlots[i].setButtonText(juce::String(i + 1));
        cueSlots[i].addListener(this);
        addAndMakeVisible(cueSlots[i]);
    }

    // --- Gain fader ---
    addAndMakeVisible(gainFader);
    gainFader.setSliderStyle(Slider::LinearVertical);
    gainFader.setTextBoxStyle(Slider::TextBoxBelow, false, 55, 18);
    gainFader.setRange(0.0, 1.2, 0.01);
    gainFader.setValue(1.0);
    gainFader.addListener(this);

    // --- Rate (speed) fader ---
    addAndMakeVisible(rateFader);
    rateFader.setSliderStyle(Slider::LinearVertical);
    rateFader.setTextBoxStyle(Slider::TextBoxBelow, false, 55, 18);
    rateFader.setRange(0.1, 3.0, 0.01);
    rateFader.setValue(1.0);
    rateFader.addListener(this);

    // --- Seek bar ---
    addAndMakeVisible(seekBar);
    seekBar.setSliderStyle(Slider::LinearHorizontal);
    seekBar.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    seekBar.setRange(0.0, 1.0, 0.001);
    seekBar.addListener(this);

    // --- WRITTEN BY SYED ALI AHSAN (R4A: 3 band EQ knobs for high, medium, low) ---
    for (auto* knob : { &bassKnob, &midKnob, &trebleKnob })
    {
        addAndMakeVisible(knob);
        knob->setSliderStyle(Slider::RotaryVerticalDrag);
        knob->setTextBoxStyle(Slider::TextBoxBelow, false, 48, 18);
        knob->setRange(0.0, 2.0, 0.01);
        knob->setValue(1.0);
        knob->addListener(this);
    }

    // --- Labels ---
    addAndMakeVisible(gainTag);
    addAndMakeVisible(rateTag);
    addAndMakeVisible(tempoTag);

    gainTag.setText("Gain", dontSendNotification);
    gainTag.setJustificationType(Justification::centred);
    rateTag.setText("Rate", dontSendNotification);
    rateTag.setJustificationType(Justification::centred);

    // --- Waveform view ---
    addAndMakeVisible(waveView);

    startTimer(80);  // ~12.5 fps refresh
}

ChannelStrip::~ChannelStrip()
{
    stopTimer();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void ChannelStrip::paint(Graphics& g)
{
    // Deck background with subtle purple-tinted gradient
    ColourGradient deckBg(Colour(0xff140f1c), 0.0f, 0.0f,
                          Colour(0xff1a1326),
                          static_cast<float>(getWidth()), 0.0f, false);
    g.setGradientFill(deckBg);
    g.fillAll();

    // Border
    g.setColour(Colour(0xff3e3356));
    g.drawRect(getLocalBounds(), 2);

    // Deck label (L / R)
    g.setColour(deckIdx == 0 ? Colour(0xffffa94d) : Colour(0xff9b59b6));
    g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    String deckName = (deckIdx == 0) ? "DECK L" : "DECK R";
    g.drawText(deckName, 10, 5, 70, 18, Justification::centredLeft, true);

    // EQ band labels  — drawn manually so they sit above the knobs
    g.setColour(Colours::white);
    g.setFont(13.0f);

    auto work = getLocalBounds().reduced(8);
    work.removeFromTop(static_cast<int>(getHeight() * 0.28f) + 28);

    auto colTransport = work.removeFromLeft(static_cast<int>(work.getWidth() * 0.30f));
    auto colFaders    = work.removeFromLeft(static_cast<int>(work.getWidth() * 0.35f));
    auto colEQ        = work;

    juce::ignoreUnused(colTransport, colFaders);

    int eqW = colEQ.getWidth() / 3;
    g.drawText("Bass",   colEQ.getX(),              colEQ.getY(), eqW, 18, Justification::centred);
    g.drawText("Mid",    colEQ.getX() + eqW,        colEQ.getY(), eqW, 18, Justification::centred);
    g.drawText("Treble", colEQ.getX() + (eqW * 2),  colEQ.getY(), eqW, 18, Justification::centred);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void ChannelStrip::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Row 1: waveform + seek bar
    waveView.setBounds(area.removeFromTop(static_cast<int>(getHeight() * 0.28f)));
    seekBar.setBounds(area.removeFromTop(18));
    area.removeFromTop(10);

    // Row 2 splits into three columns
    auto colTransport = area.removeFromLeft(static_cast<int>(area.getWidth() * 0.30f));
    auto colFaders    = area.removeFromLeft(static_cast<int>(area.getWidth() * 0.35f));
    auto colEQ        = area;

    // --- Column 1: transport + cues + BPM (7 rows) ---
    int rowH = colTransport.getHeight() / 7;
    engageBtn.setBounds(colTransport.removeFromTop(rowH).reduced(2));
    haltBtn.setBounds(colTransport.removeFromTop(rowH).reduced(2));
    browseBtn.setBounds(colTransport.removeFromTop(rowH).reduced(2));

    // 2×4 cue grid
    auto cueZone = colTransport.removeFromTop(rowH * 2).reduced(2);
    int cueW = cueZone.getWidth() / 4;
    auto topRow = cueZone.removeFromTop(cueZone.getHeight() / 2);
    for (int i = 0; i < 4; ++i)
        cueSlots[i].setBounds(topRow.removeFromLeft(cueW).reduced(1));
    for (int i = 4; i < 8; ++i)
        cueSlots[i].setBounds(cueZone.removeFromLeft(cueW).reduced(1));

    wipeCuesBtn.setBounds(colTransport.removeFromTop(rowH).reduced(2));
    tempoTag.setBounds(colTransport.removeFromTop(rowH).reduced(2));

    // --- Column 2: faders ---
    auto volCol   = colFaders.removeFromLeft(colFaders.getWidth() / 2);
    auto speedCol = colFaders;

    gainTag.setBounds(volCol.removeFromTop(18));
    gainFader.setBounds(volCol.reduced(4));

    rateTag.setBounds(speedCol.removeFromTop(18));
    rateFader.setBounds(speedCol.reduced(4));

    // --- Column 3: EQ knobs ---
    colEQ.removeFromTop(18);  // space for painted labels
    int knobW = colEQ.getWidth() / 3;
    bassKnob.setBounds(colEQ.removeFromLeft(knobW).reduced(2));
    midKnob.setBounds(colEQ.removeFromLeft(knobW).reduced(2));
    trebleKnob.setBounds(colEQ.reduced(2));
}

// ---------------------------------------------------------------------------
// Button callbacks
// ---------------------------------------------------------------------------

void ChannelStrip::buttonClicked(Button* btn)
{
    if (btn == &engageBtn)  core->engage();
    if (btn == &haltBtn)    core->disengage();

    if (btn == &browseBtn)
    {
        auto flags = juce::FileBrowserComponent::openMode
                   | juce::FileBrowserComponent::canSelectFiles;

        auto picker = std::make_shared<juce::FileChooser>(
            "Pick a track...", juce::File{},
            "*.mp3;*.wav;*.aiff;*.flac;*.m4a");

        picker->launchAsync(flags, [this, picker](const juce::FileChooser& fc)
        {
            auto chosen = fc.getResult();
            if (chosen.existsAsFile())
                loadTrack(juce::URL{ chosen }, 0.0);
        });
    }

    // --- WRITTEN BY SYED ALI AHSAN (R3C: Clear all hot cues using dedicated button) ---
    if (btn == &wipeCuesBtn)
    {
        for (int i = 0; i < 8; ++i)
            core->eraseCueSlot(i);
    }

    // --- WRITTEN BY SYED ALI AHSAN (R3A: Assign/Trigger hot cues) ---
    for (int i = 0; i < 8; ++i)
    {
        if (btn == &cueSlots[i])
            core->triggerCueSlot(i);
    }
}

// ---------------------------------------------------------------------------
// Slider callbacks
// ---------------------------------------------------------------------------

void ChannelStrip::sliderValueChanged(Slider* slider)
{
    // --- WRITTEN BY SYED ALI AHSAN (R1C: Mix track varying volume, R1D: speed up / slow down) ---
    if (slider == &gainFader)   core->applyGain(slider->getValue());
    if (slider == &rateFader)   core->applySpeedRatio(slider->getValue());
    if (slider == &seekBar)     core->seekRelative(slider->getValue());

    // --- WRITTEN BY SYED ALI AHSAN (R4A: Adjust Equaliser value using sliders) ---
    if (slider == &bassKnob)    core->tweakEqBand(0, slider->getValue());
    if (slider == &midKnob)     core->tweakEqBand(1, slider->getValue());
    if (slider == &trebleKnob)  core->tweakEqBand(2, slider->getValue());
}

// ---------------------------------------------------------------------------
// Drag & drop
// ---------------------------------------------------------------------------

bool ChannelStrip::isInterestedInFileDrag(const StringArray& /*files*/)
{
    return true;
}

void ChannelStrip::filesDropped(const StringArray& files, int /*x*/, int /*y*/)
{
    if (files.size() == 1)
        loadTrack(URL{ File{files[0]} }, 0.0);
}

// ---------------------------------------------------------------------------
// Timer (waveform + cue indicator refresh)
// ---------------------------------------------------------------------------

void ChannelStrip::timerCallback()
{
    waveView.updatePlayheadFraction(core->fetchRelativePosition());

    for (int i = 0; i < 8; ++i)
    {
        if (core->cueSlotOccupied(i))
        {
            cueSlots[i].setColour(TextButton::buttonColourId, Colour(0xff3a2850));
            cueSlots[i].setColour(TextButton::textColourOnId,  Colour(0xffffa94d));
        }
        else
        {
            cueSlots[i].setColour(TextButton::buttonColourId, Colour(0xff1a1326));
            cueSlots[i].setColour(TextButton::textColourOnId,  Colours::white);
        }
    }
}

// ---------------------------------------------------------------------------
// Track loading
// ---------------------------------------------------------------------------

void ChannelStrip::loadTrack(juce::URL trackURL, double tempo)
{
    core->openTrack(trackURL);
    waveView.assignSource(trackURL);

    if (tempo > 0.0)
        tempoTag.setText("BPM: " + juce::String(tempo, 1), dontSendNotification);
    else
        tempoTag.setText("BPM: ---", dontSendNotification);
}
