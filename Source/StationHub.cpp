/*
  ==============================================================================

    StationHub.cpp
    Author:  Syed Ali Ahsan
    Root mixing panel – implementation for PulseStation

  ==============================================================================
*/

// --- WRITTEN BY SYED ALI AHSAN (R1B: Initialize mixer to play tracks simultaneously) ---

#include "StationHub.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

StationHub::StationHub()
{
    setSize(1400, 820);

    if (RuntimePermissions::isRequired(RuntimePermissions::recordAudio)
        && !RuntimePermissions::isGranted(RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request(RuntimePermissions::recordAudio,
            [&](bool ok) { if (ok) setAudioChannels(2, 2); });
    }
    else
    {
        setAudioChannels(0, 2);
    }

    // Wire the track vault to both deck strips
    trackVault.wireChannelStrips(&deckLeft, &deckRight);

    addAndMakeVisible(deckLeft);
    addAndMakeVisible(deckRight);
    addAndMakeVisible(trackVault);

    formatMgr.registerBasicFormats();
}

StationHub::~StationHub()
{
    shutdownAudio();
}

// ---------------------------------------------------------------------------
// Audio pipeline
// ---------------------------------------------------------------------------

void StationHub::prepareToPlay(int samplesPerBlock, double sampleRate)
{
    coreLeft.prepareToPlay(samplesPerBlock, sampleRate);
    coreRight.prepareToPlay(samplesPerBlock, sampleRate);
    mixer.prepareToPlay(samplesPerBlock, sampleRate);

    mixer.addInputSource(&coreLeft, false);
    mixer.addInputSource(&coreRight, false);
}

// --- WRITTEN BY SYED ALI AHSAN (R1B: Play two or more tracks simultaneously) ---
void StationHub::getNextAudioBlock(const AudioSourceChannelInfo& buffer)
{
    mixer.getNextAudioBlock(buffer);
}

void StationHub::releaseResources()
{
    coreLeft.releaseResources();
    coreRight.releaseResources();
    mixer.releaseResources();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void StationHub::paint(Graphics& g)
{
    // Deep dark background
    g.fillAll(Colour(0xff0e0b14));

    // App header bar
    int hdrH = 38;
    auto titleBar = getLocalBounds().removeFromTop(hdrH);
    {
        ColourGradient hdrGrad(Colour(0xff1c1528), 0.0f, 0.0f,
                               Colour(0xff251d32),
                               static_cast<float>(getWidth()), 0.0f, false);
        g.setGradientFill(hdrGrad);
        g.fillRect(titleBar.toFloat());
    }

    g.setColour(Colour(0xffffa94d));  // amber title
    g.setFont(juce::FontOptions(22.0f).withStyle("Bold"));
    g.drawText("PULSE STATION", titleBar, Justification::centred, true);

    // Separator accent beneath header
    g.setColour(Colour(0xff9b59b6).withAlpha(0.35f));
    g.drawLine(0.0f, static_cast<float>(hdrH),
               static_cast<float>(getWidth()), static_cast<float>(hdrH), 2.0f);

    // Determine layout geometry for separator lines
    int deckAreaH = static_cast<int>((getHeight() - hdrH) * 0.6f);
    int midX      = getWidth() / 2;

    // Vertical line between the two decks
    g.drawLine(static_cast<float>(midX), static_cast<float>(hdrH),
               static_cast<float>(midX), static_cast<float>(hdrH + deckAreaH), 2.0f);

    // Horizontal line separating decks from the track vault
    g.drawLine(0.0f, static_cast<float>(hdrH + deckAreaH),
               static_cast<float>(getWidth()),
               static_cast<float>(hdrH + deckAreaH), 2.0f);
}

// ---------------------------------------------------------------------------
// Layout – SIDE-BY-SIDE decks on top, full-width TrackVault below
// ---------------------------------------------------------------------------

void StationHub::resized()
{
    auto area = getLocalBounds();

    int hdrH = 38;
    area.removeFromTop(hdrH);

    // Top 60%: two decks side by side
    int deckAreaH = static_cast<int>(area.getHeight() * 0.6f);
    auto decksRow = area.removeFromTop(deckAreaH);

    int halfW = decksRow.getWidth() / 2;
    int pad   = 5;

    deckLeft.setBounds(decksRow.removeFromLeft(halfW).reduced(pad));
    deckRight.setBounds(decksRow.reduced(pad));

    // Bottom 40%: track vault
    trackVault.setBounds(area.reduced(pad));
}
