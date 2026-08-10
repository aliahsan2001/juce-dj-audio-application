/*
  ==============================================================================

    WaveRenderer.h
    Author:  Syed Ali Ahsan
    Real-time waveform thumbnail renderer for PulseStation decks

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/**
 * WaveRenderer draws a scrolling audio thumbnail with a playhead indicator.
 * It listens for thumbnail changes to trigger repaints automatically.
 */
class WaveRenderer : public juce::Component,
                     public ChangeListener
{
public:

    /** Construct with references to shared format manager and cache. */
    WaveRenderer(AudioFormatManager& fmtMgr, AudioThumbnailCache& thumbCache);

    ~WaveRenderer() override;

    // ---- Component overrides ----

    /** Render the waveform or an empty-state placeholder. */
    void paint(juce::Graphics& g) override;

    /** Layout hook (currently a no-op). */
    void resized() override;

    // ---- Public API ----

    /** Load a new audio source and regenerate the thumbnail. */
    void assignSource(URL audioURL);

    /** Called when the thumbnail data changes. */
    void changeListenerCallback(ChangeBroadcaster* origin) override;

    /** Update the playhead indicator position (0.0 – 1.0). */
    void updatePlayheadFraction(double fraction);

private:

    AudioThumbnail thumb;
    bool sourceReady;
    double playheadFraction;
    std::string displayTitle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveRenderer)
};
