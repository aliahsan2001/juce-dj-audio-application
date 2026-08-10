/*
  ==============================================================================

    WaveRenderer.cpp
    Author:  Syed Ali Ahsan
    Waveform thumbnail rendering implementation for PulseStation

  ==============================================================================
*/

// --- WRITTEN BY SYED ALI AHSAN (R1: Waveform rendering for visual feedback) ---

#include <JuceHeader.h>
#include "WaveRenderer.h"

WaveRenderer::WaveRenderer(AudioFormatManager& fmtMgr,
                           AudioThumbnailCache& thumbCache)
    : thumb(1000, fmtMgr, thumbCache),
      sourceReady(false),
      playheadFraction(0.0)
{
    thumb.addChangeListener(this);
}

WaveRenderer::~WaveRenderer() {}

void WaveRenderer::paint(juce::Graphics& g)
{
    auto region = getLocalBounds();

    // Deep background
    g.setColour(Colour(0xff0d0d14));
    g.fillAll();

    // Thin border
    g.setColour(Colour(0xff2a2030));
    g.drawRect(region.toFloat(), 1.0f);

    if (sourceReady)
    {
        // Draw the waveform in a warm amber tint
        g.setColour(Colour(0xff6e5a3b));
        thumb.drawChannel(g, region.reduced(2), 0.0, thumb.getTotalLength(), 0, 1.0f);

        // Playhead line – bright amber
        g.setColour(Colour(0xddffa94d));
        auto xPos = playheadFraction * region.getWidth();
        g.drawLine(static_cast<float>(xPos), 0.0f,
                   static_cast<float>(xPos),
                   static_cast<float>(region.getHeight()), 2.0f);

        // Track name overlay
        g.setColour(Colours::white.withAlpha(0.55f));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(displayTitle, region.reduced(6),
                   juce::Justification::topRight, true);
    }
    else
    {
        // Empty state
        g.setColour(Colour(0xff444444));
        g.setFont(juce::Font(juce::FontOptions(14.0f)).withStyle(juce::Font::italic));
        g.drawText("Drop a track here", region,
                   juce::Justification::centred, true);
    }
}

void WaveRenderer::resized()
{
    // No child components to lay out
}

void WaveRenderer::assignSource(URL audioURL)
{
    thumb.clear();
    sourceReady = thumb.setSource(new URLInputSource(audioURL));

    if (sourceReady)
    {
        if (audioURL.isLocalFile())
        {
            displayTitle = audioURL.getLocalFile()
                               .getFileNameWithoutExtension()
                               .toStdString();
        }
        else
        {
            // Extract filename from a remote URL
            std::string raw = audioURL.toString(false).toStdString();
            auto slashPos = raw.find_last_of('/');
            auto dotPos   = raw.find_last_of('.');
            if (slashPos != std::string::npos && dotPos != std::string::npos && dotPos > slashPos)
                displayTitle = raw.substr(slashPos + 1, dotPos - slashPos - 1);
            else
                displayTitle = raw;
        }
        repaint();
    }
}

void WaveRenderer::changeListenerCallback(ChangeBroadcaster* origin)
{
    juce::ignoreUnused(origin);
    repaint();
}

void WaveRenderer::updatePlayheadFraction(double fraction)
{
    playheadFraction = fraction;
    repaint();
}
