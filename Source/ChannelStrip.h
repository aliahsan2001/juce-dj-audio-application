/*
  ==============================================================================

    ChannelStrip.h
    Author:  Syed Ali Ahsan
    Individual deck control strip for PulseStation

  ==============================================================================
*/

// --- WRITTEN BY SYED ALI AHSAN (R4: Equaliser, R3: Hot Cues UI) ---

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PlaybackCore.h"
#include "WaveRenderer.h"
#include "TrackVault.h"

/**
 * ChannelStrip is a self-contained deck UI.  It provides transport buttons,
 * a waveform view, volume/speed faders, a 3-band EQ section with rotary
 * knobs, and an 8-slot hot-cue grid.  The layout is organised in a
 * horizontal strip: [waveform + transport] | [faders] | [EQ] | [cues].
 */
class ChannelStrip : public Component,
                     public Button::Listener,
                     public Slider::Listener,
                     public FileDragAndDropTarget,
                     public Timer
{
public:

    ChannelStrip(PlaybackCore* core,
                 TrackVault* vault,
                 AudioFormatManager& fmtMgr,
                 AudioThumbnailCache& thumbCache,
                 int deckIndex);

    ~ChannelStrip();

    /** 
     * Renders the deck interface background and custom labels.
     * @param g  The graphics context for drawing.
     */
    void paint(Graphics& g) override;

    /** 
     * Lays out all deck components strictly based on available bounds.
     */
    void resized() override;

    /** 
     * Handles transport and hot-cue button clicks.
     * @param btn  The button that was clicked.
     */
    void buttonClicked(Button* btn) override;

    /** 
     * Handles value changes for faders and EQ knobs.
     * @param slider  The slider that moved.
     */
    void sliderValueChanged(Slider* slider) override;

    /** 
     * Checks if dropped files are valid audio files for this deck.
     * @param files  Array of dropped file paths.
     * @return bool  True if we accept the drop, false otherwise.
     */
    bool isInterestedInFileDrag(const StringArray& files) override;

    /** 
     * Handles file drops by loading the primary dropped file.
     * @param files  Array of dropped file paths.
     * @param x      X drop coordinate.
     * @param y      Y drop coordinate.
     */
    void filesDropped(const StringArray& files, int x, int y) override;

    /** 
     * Periodic timer to update waveform playhead and hot-cue illumination.
     */
    void timerCallback() override;

    /** 
     * Loads a track into this deck, optionally displaying its BPM.
     * @param trackURL  The file URL of the audio track.
     * @param tempo     Optional precalculated BPM value.
     */
    void loadTrack(juce::URL trackURL, double tempo = 0.0);

private:

    // Transport controls
    TextButton engageBtn{ "PLAY" };
    TextButton haltBtn{ "STOP" };
    TextButton browseBtn{ "OPEN" };

    // Hot-cue grid
    TextButton cueSlots[8];
    TextButton wipeCuesBtn{ "Wipe Cues" };

    // Faders
    Slider gainFader;
    Slider rateFader;
    Slider seekBar;

    // EQ knobs
    Slider bassKnob;
    Slider midKnob;
    Slider trebleKnob;

    // Labels
    Label gainTag;
    Label rateTag;
    Label tempoTag;
    Label bassTag, midTag, trebleTag;

    // Core references
    PlaybackCore* core;
    TrackVault*   vault;
    WaveRenderer  waveView;

    int deckIdx;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
};
