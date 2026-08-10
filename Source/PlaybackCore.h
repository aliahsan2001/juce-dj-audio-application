/*
  ==============================================================================

    PlaybackCore.h
    Author:  Syed Ali Ahsan
    Core audio playback and DSP processing module for PulseStation

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <unordered_map>

/**
 * PlaybackCore wraps JUCE transport, resampling, and tri-band EQ into a
 * single audio source that a ChannelStrip can drive.  It also owns the
 * per-track hot-cue bookmarks (up to 8) and persists EQ/cue state across
 * sessions via an in-memory map that is serialised elsewhere.
 */
class PlaybackCore : public AudioSource
{
public:

    /** Construct with a reference to a shared format manager. */
    PlaybackCore(AudioFormatManager& fmtMgr);

    /** Destructor – saves current track state before teardown. */
    ~PlaybackCore();

    // ---- AudioSource overrides ----

    /** Allocate internal buffers for the given block size and rate. */
    void prepareToPlay(int expectedBlockSize, double newSampleRate) override;

    /** Fill the supplied buffer with processed audio samples. */
    void getNextAudioBlock(const AudioSourceChannelInfo& outputBuffer) override;

    /** Free any allocated audio resources. */
    void releaseResources() override;

    // ---- Transport helpers ----

    /** Open an audio file from the given URL and begin buffering. */
    void openTrack(URL trackURL);

    /** Adjust the output gain (0.0 – 1.0 range, clamped). */
    void applyGain(double newGain);

    /** Change playback speed ratio (0.1 – 100.0, clamped). */
    void applySpeedRatio(double ratio);

    /** Seek to an absolute position in seconds. */
    void seekAbsolute(double positionSecs);

    /** Seek to a relative position (0.0 – 1.0). */
    void seekRelative(double normalisedPos);

    /** Return the current playback position as a 0-1 fraction. */
    double fetchRelativePosition();

    /** Begin playback of the loaded track. */
    void engage();

    /** Pause playback of the loaded track. */
    void disengage();

    // ---- Tri-band EQ ----

    /**
     * Adjust one of the three EQ bands.
     * @param bandIndex  0 = bass, 1 = mid, 2 = treble
     * @param gainValue  linear gain multiplier
     */
    void tweakEqBand(int bandIndex, double gainValue);

    // ---- Hot-cue management (8 slots) ----

    /**
     * If cue slot is empty, store the current position.
     * If cue slot is occupied, jump to the stored position.
     */
    void triggerCueSlot(int slotIndex);

    /** Remove the bookmark stored in the given cue slot. */
    void eraseCueSlot(int slotIndex);

    /** Check whether a cue slot contains a valid bookmark. */
    bool cueSlotOccupied(int slotIndex) const;

private:

    AudioFormatManager& fmtMgr;
    std::unique_ptr<AudioFormatReaderSource> fileSource;
    AudioTransportSource transport;
    ResamplingAudioSource resampler{ &transport, false, 2 };

    // ----- DSP: Tri-band EQ (per-channel stereo) -----
    static constexpr int kMaxChannels = 2;
    IIRFilter bassIIR[kMaxChannels];
    IIRFilter midIIR[kMaxChannels];
    IIRFilter trebleIIR[kMaxChannels];

    double activeSampleRate{ 44100.0 };
    float bassLevel{ 1.0f };
    float midLevel{ 1.0f };
    float trebleLevel{ 1.0f };

    /** Recompute IIR coefficients after any EQ change. */
    void refreshEqCoefficients();

    // ----- Per-track state (EQ + Cues) -----
    static constexpr int kCueSlots = 8;

    struct Snapshot
    {
        float bass{ 1.0f };
        float mid{ 1.0f };
        float treble{ 1.0f };
        double cueMarkers[kCueSlots];

        Snapshot()
        {
            for (int s = 0; s < kCueSlots; ++s)
                cueMarkers[s] = -1.0;
        }
    };

    static std::unordered_map<juce::String, Snapshot> snapshotCache;
    juce::String activeTrackID;

    /** Persist the current EQ values into the snapshot for this track. */
    void commitSnapshot();

    /** Restore EQ values from a previously saved snapshot (or defaults). */
    void restoreSnapshot();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaybackCore)
};
