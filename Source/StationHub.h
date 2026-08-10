/*
  ==============================================================================

    StationHub.h
    Author:  Syed Ali Ahsan
    Root mixing panel that wires decks, library, and the audio graph

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PlaybackCore.h"
#include "ChannelStrip.h"
#include "TrackVault.h"

/**
 * StationHub is the top-level AudioAppComponent for PulseStation.
 * It owns two PlaybackCore instances (left/right deck), the TrackVault
 * (music library), and the MixerAudioSource that merges both decks.
 * Layout: decks side-by-side at the top, library spanning full width below.
 */
class StationHub : public AudioAppComponent
{
public:

    StationHub();
    ~StationHub();

    // ---- AudioAppComponent overrides ----

    /** Wire both playback cores and the mixer for the given sample rate. */
    void prepareToPlay(int samplesPerBlock, double sampleRate) override;

    /** Fetch combined audio from both decks. */
    void getNextAudioBlock(const AudioSourceChannelInfo& buffer) override;

    /** Release audio resources for both cores. */
    void releaseResources() override;

    // ---- Component overrides ----

    /** Draw the app header and separator lines. */
    void paint(Graphics& g) override;

    /** Lay out decks (side-by-side) and the track vault (bottom). */
    void resized() override;

private:

    AudioFormatManager formatMgr;
    AudioThumbnailCache thumbCache{ 100 };

    TrackVault trackVault{ formatMgr };

    PlaybackCore coreLeft{ formatMgr };
    ChannelStrip deckLeft{ &coreLeft, &trackVault, formatMgr, thumbCache, 0 };

    PlaybackCore coreRight{ formatMgr };
    ChannelStrip deckRight{ &coreRight, &trackVault, formatMgr, thumbCache, 1 };

    MixerAudioSource mixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StationHub)
};
