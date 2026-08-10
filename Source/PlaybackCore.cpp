/*
  ==============================================================================

    PlaybackCore.cpp
    Author:  Syed Ali Ahsan
    Implementation of the audio playback and DSP engine for PulseStation

  ==============================================================================
*/

// --- WRITTEN BY SYED ALI AHSAN (R1: Basic Program, R3: Hot Cues, R4: Equaliser) ---

#include "PlaybackCore.h"

// Global snapshot map shared across all PlaybackCore instances
std::unordered_map<juce::String, PlaybackCore::Snapshot> PlaybackCore::snapshotCache;

PlaybackCore::PlaybackCore(AudioFormatManager& _fmtMgr)
    : fmtMgr(_fmtMgr)
{
}

PlaybackCore::~PlaybackCore()
{
    // Persist EQ + cue data for the currently loaded track before destruction
    commitSnapshot();
}

// ---------------------------------------------------------------------------
// AudioSource lifecycle
// ---------------------------------------------------------------------------

void PlaybackCore::prepareToPlay(int expectedBlockSize, double newSampleRate)
{
    activeSampleRate = newSampleRate;

    transport.prepareToPlay(expectedBlockSize, newSampleRate);
    resampler.prepareToPlay(expectedBlockSize, newSampleRate);

    // Zero-out the IIR filter state for clean start
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        bassIIR[ch].reset();
        midIIR[ch].reset();
        trebleIIR[ch].reset();
    }
    refreshEqCoefficients();
}

void PlaybackCore::getNextAudioBlock(const AudioSourceChannelInfo& outputBuffer)
{
    resampler.getNextAudioBlock(outputBuffer);

    // Run the tri-band equaliser across each channel
    int channelCount = juce::jmin(outputBuffer.buffer->getNumChannels(), kMaxChannels);
    for (int ch = 0; ch < channelCount; ++ch)
    {
        float* samples = outputBuffer.buffer->getWritePointer(ch, outputBuffer.startSample);
        int count = outputBuffer.numSamples;

        bassIIR[ch].processSamples(samples, count);
        midIIR[ch].processSamples(samples, count);
        trebleIIR[ch].processSamples(samples, count);
    }
}

void PlaybackCore::releaseResources()
{
    transport.releaseResources();
    resampler.releaseResources();
}

// ---------------------------------------------------------------------------
// Track loading
// ---------------------------------------------------------------------------

void PlaybackCore::openTrack(URL trackURL)
{
    // --- WRITTEN BY SYED ALI AHSAN (R1A: Load audio files into players) ---
    // Flush the EQ state for whatever was previously playing
    commitSnapshot();

    auto* reader = fmtMgr.createReaderFor(
        trackURL.createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)));

    if (reader != nullptr)
    {
        auto fresh = std::make_unique<AudioFormatReaderSource>(reader, true);
        transport.setSource(fresh.get(), 0, nullptr, reader->sampleRate);
        fileSource.reset(fresh.release());

        activeTrackID = trackURL.getLocalFile().getFullPathName();
        restoreSnapshot();
    }
}

// ---------------------------------------------------------------------------
// Transport helpers
// ---------------------------------------------------------------------------

// --- WRITTEN BY SYED ALI AHSAN (R1C: Mix track by varying volume) ---
void PlaybackCore::applyGain(double newGain)
{
    if (newGain >= 0.0 && newGain <= 1.0)
        transport.setGain(static_cast<float>(newGain));
}

// --- WRITTEN BY SYED ALI AHSAN (R1D: Speed up and slow down tracks) ---
void PlaybackCore::applySpeedRatio(double ratio)
{
    if (ratio >= 0.1 && ratio <= 100.0)
        resampler.setResamplingRatio(ratio);
}

void PlaybackCore::seekAbsolute(double positionSecs)
{
    transport.setPosition(positionSecs);
}

void PlaybackCore::seekRelative(double normalisedPos)
{
    if (normalisedPos >= 0.0 && normalisedPos <= 1.0)
    {
        double target = transport.getLengthInSeconds() * normalisedPos;
        seekAbsolute(target);
    }
}

double PlaybackCore::fetchRelativePosition()
{
    double len = transport.getLengthInSeconds();
    return (len > 0.0) ? transport.getCurrentPosition() / len : 0.0;
}

void PlaybackCore::engage()
{
    transport.start();
}

void PlaybackCore::disengage()
{
    transport.stop();
}

// ---------------------------------------------------------------------------
// Tri-band EQ
// ---------------------------------------------------------------------------

// --- WRITTEN BY SYED ALI AHSAN (R4A: Create 3 band equaliser panel) ---
void PlaybackCore::tweakEqBand(int bandIndex, double gainValue)
{
    float clamped = static_cast<float>(gainValue);
    switch (bandIndex)
    {
        case 0:  bassLevel   = clamped; break;
        case 1:  midLevel    = clamped; break;
        case 2:  trebleLevel = clamped; break;
        default: return;
    }
    refreshEqCoefficients();
}

void PlaybackCore::refreshEqCoefficients()
{
    // Bass shelf at 300 Hz, mid peak at 1200 Hz, treble shelf at 3500 Hz
    auto bassCoeffs   = IIRCoefficients::makeLowShelf(activeSampleRate, 300.0, 0.9, bassLevel);
    auto midCoeffs    = IIRCoefficients::makePeakFilter(activeSampleRate, 1200.0, 0.85, midLevel);
    auto trebleCoeffs = IIRCoefficients::makeHighShelf(activeSampleRate, 3500.0, 0.9, trebleLevel);

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        bassIIR[ch].setCoefficients(bassCoeffs);
        midIIR[ch].setCoefficients(midCoeffs);
        trebleIIR[ch].setCoefficients(trebleCoeffs);
    }
}

// ---------------------------------------------------------------------------
// Hot-cue slots
// ---------------------------------------------------------------------------

// --- WRITTEN BY SYED ALI AHSAN (R3A, R3B: Assign/Modify hot cues individually) ---
void PlaybackCore::triggerCueSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= kCueSlots) return;
    if (activeTrackID.isEmpty()) return;

    Snapshot& snap = snapshotCache[activeTrackID];

    if (snap.cueMarkers[slotIndex] < 0.0)
    {
        // Slot is empty — bookmark the current playback position
        snap.cueMarkers[slotIndex] = transport.getCurrentPosition();
    }
    else
    {
        // Slot is occupied — jump to the bookmarked position
        transport.setPosition(snap.cueMarkers[slotIndex]);
    }
}

void PlaybackCore::eraseCueSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= kCueSlots) return;
    if (activeTrackID.isEmpty()) return;

    snapshotCache[activeTrackID].cueMarkers[slotIndex] = -1.0;
}

bool PlaybackCore::cueSlotOccupied(int slotIndex) const
{
    if (activeTrackID.isEmpty() || slotIndex < 0 || slotIndex >= kCueSlots)
        return false;

    auto it = snapshotCache.find(activeTrackID);
    if (it == snapshotCache.end()) return false;
    return it->second.cueMarkers[slotIndex] >= 0.0;
}

// ---------------------------------------------------------------------------
// Snapshot persistence helpers
// ---------------------------------------------------------------------------

// --- WRITTEN BY SYED ALI AHSAN (R4B, R3D: Persist EQ & Hot cues per track) ---
void PlaybackCore::commitSnapshot()
{
    if (activeTrackID.isEmpty()) return;

    Snapshot& snap = snapshotCache[activeTrackID];
    snap.bass   = bassLevel;
    snap.mid    = midLevel;
    snap.treble = trebleLevel;
}

void PlaybackCore::restoreSnapshot()
{
    auto it = snapshotCache.find(activeTrackID);
    if (it != snapshotCache.end())
    {
        tweakEqBand(0, it->second.bass);
        tweakEqBand(1, it->second.mid);
        tweakEqBand(2, it->second.treble);
    }
    else
    {
        // No saved state – reset to unity
        tweakEqBand(0, 1.0);
        tweakEqBand(1, 1.0);
        tweakEqBand(2, 1.0);
    }
}
