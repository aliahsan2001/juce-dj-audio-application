/*
  ==============================================================================

    TrackVault.cpp
    Author:  Syed Ali Ahsan
    Persistent music collection – implementation for PulseStation

  ==============================================================================
*/

// --- WRITTEN BY SYED ALI AHSAN (R2: Music Library & Persisting) ---

#include "TrackVault.h"
#include "ChannelStrip.h"
#include <algorithm>
#include <cstdio>

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

TrackVault::TrackVault(AudioFormatManager& _fmtMgr)
    : fmtMgr(_fmtMgr)
{
    // Column layout —  L / R load buttons bookend title, duration, tempo
    tableView.getHeader().addColumn("Title",     1, 180);
    tableView.getHeader().addColumn("Length",    2, 70);
    tableView.getHeader().addColumn("Tempo",     5, 55);
    tableView.getHeader().addColumn("L",         3, 35);
    tableView.getHeader().addColumn("R",         4, 35);

    tableView.setModel(this);
    addAndMakeVisible(tableView);

    // --- Filter / search box ---
    addAndMakeVisible(filterBox);
    filterBox.addListener(this);
    filterBox.setTextToShowWhenEmpty("Filter tracks...", Colour(0xff777777));
    filterBox.setColour(TextEditor::backgroundColourId, Colour(0xff1e1826));
    filterBox.setColour(TextEditor::textColourId,       Colours::white);
    filterBox.setColour(TextEditor::highlightColourId,  Colour(0xff9b59b6));
    filterBox.setColour(TextEditor::outlineColourId,    Colour(0xff3e3356));

    // --- Import tracks button ---
    addAndMakeVisible(importBtn);
    importBtn.setButtonText("+ Import");
    importBtn.addListener(this);
    importBtn.setColour(TextButton::buttonColourId,  Colour(0xff2d2440));
    importBtn.setColour(TextButton::textColourOffId,  Colour(0xffddccff));
    importBtn.setColour(TextButton::textColourOnId,   Colour(0xffbb99ff));

    // --- Purge library button ---
    addAndMakeVisible(purgeBtn);
    purgeBtn.setButtonText("Purge");
    purgeBtn.addListener(this);
    purgeBtn.setColour(TextButton::buttonColourId,  Colour(0xff2d2440));
    purgeBtn.setColour(TextButton::textColourOffId,  Colour(0xffddccff));
    purgeBtn.setColour(TextButton::textColourOnId,   Colour(0xffff7777));

    // Restore collection from the last session
    hydrateFromDisk();
}

TrackVault::~TrackVault()
{
    flushToDisk();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void TrackVault::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    // Gradient background – deep purple to near-black
    ColourGradient bg(Colour(0xff16101e), 0.0f, 0.0f,
                      Colour(0xff1d1528), 0.0f,
                      static_cast<float>(getHeight()), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Section header
    auto header = bounds.removeFromTop(32);
    g.setColour(Colour(0xff221a30));
    g.fillRect(header.toFloat());

    g.setColour(Colour(0xffbb99ff));
    g.setFont(juce::FontOptions(15.0f).withStyle("Bold"));
    g.drawText("TRACK VAULT", header, Justification::centred, true);

    // Separator line
    g.setColour(Colour(0xff3e3356).withAlpha(0.5f));
    g.drawLine(static_cast<float>(header.getX()),
               static_cast<float>(header.getBottom()),
               static_cast<float>(header.getRight()),
               static_cast<float>(header.getBottom()), 1.0f);

    // Empty-state guidance
    if (visPaths.empty())
    {
        auto emptyZone = bounds.reduced(18);
        g.setColour(Colour(0xff665580));
        g.setFont(juce::FontOptions(20.0f).withStyle("Italic"));
        g.drawText("Drag audio files here to begin",
                   emptyZone, Justification::centred, true);

        g.setColour(Colour(0xff4d3d66));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText("or click '+ Import' above",
                   emptyZone.removeFromBottom(26), Justification::centred, true);
    }
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void TrackVault::resized()
{
    int headerH  = 32;
    int controlH = 32;
    int gap       = 5;

    auto area = getLocalBounds();
    area.removeFromTop(headerH);                       // painted header

    auto toolbar = area.removeFromTop(controlH);
    toolbar.reduce(gap, gap);

    int filterW = static_cast<int>(toolbar.getWidth() * 0.55f);
    int btnW    = 80;

    filterBox.setBounds(toolbar.removeFromLeft(filterW).reduced(0, 1));
    toolbar.removeFromLeft(gap);
    importBtn.setBounds(toolbar.removeFromLeft(btnW).reduced(0, 1));
    toolbar.removeFromLeft(gap);
    purgeBtn.setBounds(toolbar.removeFromLeft(btnW).reduced(0, 1));

    tableView.setBounds(area.reduced(2));
}

// ---------------------------------------------------------------------------
// TableListBoxModel
// ---------------------------------------------------------------------------

int TrackVault::getNumRows()
{
    return static_cast<int>(visPaths.size());
}

void TrackVault::paintRowBackground(Graphics& g, int row, int w, int h,
                                    bool isSelected)
{
    juce::ignoreUnused(row, w, h);

    if (isSelected)
    {
        g.fillAll(Colour(0xff332a44));
        g.setColour(Colour(0xff4d3d66));
        g.drawRect(Rectangle<int>(0, 0, w, h), 1);
    }
    else
    {
        g.fillAll((row & 1) ? Colour(0xff1a1424) : Colour(0xff201830));
    }
}

void TrackVault::paintCell(Graphics& g, int row, int col,
                           int w, int h, bool /*isSelected*/)
{
    if (row >= static_cast<int>(visPaths.size())) return;

    auto idx = static_cast<size_t>(row);
    Rectangle<int> cell(2, 0, w - 4, h);

    g.setColour(Colours::white);
    g.setFont(juce::FontOptions(13.0f));

    if (col == 1)
    {
        g.drawText(visNames[idx], cell.toFloat(), Justification::centredLeft, true);
    }
    else if (col == 2)
    {
        g.drawText(formatDuration(visLengths[idx]), cell.toFloat(),
                   Justification::centred, true);
    }
    else if (col == 5)
    {
        int rounded = static_cast<int>(std::round(visTempos[idx]));
        g.drawText(String(rounded), cell.toFloat(), Justification::centred, true);
    }
    else if (col == 3)
    {
        // Left-deck load button
        g.setColour(Colour(0xff2d2440));
        g.fillRoundedRectangle(Rectangle<float>(2.0f, 2.0f,
            static_cast<float>(w - 4), static_cast<float>(h - 4)), 3.0f);
        g.setColour(Colour(0xffffa94d));  // amber
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText("L", cell.toFloat(), Justification::centred, true);
    }
    else if (col == 4)
    {
        // Right-deck load button
        g.setColour(Colour(0xff2d2440));
        g.fillRoundedRectangle(Rectangle<float>(2.0f, 2.0f,
            static_cast<float>(w - 4), static_cast<float>(h - 4)), 3.0f);
        g.setColour(Colour(0xff9b59b6));  // purple
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText("R", cell.toFloat(), Justification::centred, true);
    }
}

void TrackVault::cellClicked(int row, int col, const MouseEvent&)
{
    // --- WRITTEN BY SYED ALI AHSAN (R2B: Load music from library to different decks) ---
    if (row >= static_cast<int>(visPaths.size())) return;

    auto idx = static_cast<size_t>(row);
    String path  = visPaths[idx];
    double tempo = visTempos[idx];

    if (col == 3 && stripLeft  != nullptr)
        stripLeft->loadTrack(URL{ File{path} }, tempo);

    if (col == 4 && stripRight != nullptr)
        stripRight->loadTrack(URL{ File{path} }, tempo);
}

// ---------------------------------------------------------------------------
// Buttons & search
// ---------------------------------------------------------------------------

void TrackVault::buttonClicked(Button* btn)
{
    // --- WRITTEN BY SYED ALI AHSAN (R2A: Load multiple files into the deck) ---
    if (btn == &importBtn)
    {
        auto flags = juce::FileBrowserComponent::openMode
                   | juce::FileBrowserComponent::canSelectFiles
                   | juce::FileBrowserComponent::canSelectMultipleItems;

        auto picker = std::make_shared<juce::FileChooser>(
            "Choose audio files...", juce::File{},
            "*.mp3;*.wav;*.aiff;*.flac;*.m4a");

        picker->launchAsync(flags, [this, picker](const juce::FileChooser& fc)
        {
            for (auto& f : fc.getResults())
                ingestFile(f);
            applyFilter("");
            flushToDisk();
            repaint();
        });
    }
    else if (btn == &purgeBtn)
    {
        allPaths.clear();
        allNames.clear();
        allLengths.clear();
        allTempos.clear();
        applyFilter("");

        File store = File::getSpecialLocation(File::userDocumentsDirectory)
                         .getChildFile("PulseStationVault.dat");
        store.deleteFile();
        repaint();
    }
}

void TrackVault::textEditorTextChanged(TextEditor& editor)
{
    applyFilter(editor.getText());
}

// ---------------------------------------------------------------------------
// Drag & Drop
// ---------------------------------------------------------------------------

bool TrackVault::isInterestedInFileDrag(const StringArray& /*files*/)
{
    return true;
}

void TrackVault::filesDropped(const StringArray& files, int /*x*/, int /*y*/)
{
    for (const auto& f : files)
        ingestFile(File{ f });
    applyFilter("");
    flushToDisk();
    repaint();
}

// ---------------------------------------------------------------------------
// Deck wiring
// ---------------------------------------------------------------------------

void TrackVault::wireChannelStrips(ChannelStrip* a, ChannelStrip* b)
{
    stripLeft  = a;
    stripRight = b;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void TrackVault::ingestFile(File audioFile)
{
    String path = audioFile.getFullPathName();

    // Duplicate guard
    for (const auto& existing : allPaths)
        if (existing == path.toStdString()) return;

    String name = audioFile.getFileNameWithoutExtension();

    double secs = 0.0;
    std::unique_ptr<AudioFormatReader> reader(fmtMgr.createReaderFor(audioFile));
    if (reader != nullptr)
        secs = static_cast<double>(reader->lengthInSamples)
             / static_cast<double>(reader->sampleRate);

    double tempo = estimateTempo(audioFile);

    allPaths.push_back(path.toStdString());
    allNames.push_back(name.toStdString());
    allLengths.push_back(static_cast<int>(secs));
    allTempos.push_back(tempo);
}

void TrackVault::applyFilter(String query)
{
    visPaths.clear();
    visNames.clear();
    visLengths.clear();
    visTempos.clear();

    for (size_t i = 0; i < allNames.size(); ++i)
    {
        if (query.isEmpty() || String(allNames[i]).containsIgnoreCase(query))
        {
            visPaths.push_back(allPaths[i]);
            visNames.push_back(allNames[i]);
            visLengths.push_back(allLengths[i]);
            visTempos.push_back(allTempos[i]);
        }
    }

    tableView.setVisible(!visPaths.empty());
    tableView.updateContent();
    repaint();
}

std::string TrackVault::formatDuration(int totalSecs)
{
    int m = totalSecs / 60;
    int s = totalSecs % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// Persistence (pipe-delimited flat file)
// ---------------------------------------------------------------------------

// --- WRITTEN BY SYED ALI AHSAN (R2C: Library state persist after re-opening) ---
void TrackVault::hydrateFromDisk()
{
    allPaths.clear();
    allNames.clear();
    allLengths.clear();
    allTempos.clear();

    File store = File::getSpecialLocation(File::userDocumentsDirectory)
                     .getChildFile("PulseStationVault.dat");
    if (!store.exists()) { applyFilter(""); return; }

    FileInputStream in(store);
    while (!in.isExhausted())
    {
        String line = in.readNextLine();
        StringArray tokens;
        tokens.addTokens(line, "|", "");

        if (tokens.size() >= 4 && tokens[0].isNotEmpty())
        {
            std::string p = tokens[0].toStdString();
            // Skip duplicates
            bool dup = false;
            for (const auto& e : allPaths)
                if (e == p) { dup = true; break; }

            if (!dup)
            {
                allPaths.push_back(p);
                allNames.push_back(tokens[1].toStdString());
                allLengths.push_back(tokens[2].getIntValue());
                allTempos.push_back(tokens[3].getDoubleValue());
            }
        }
    }
    applyFilter("");
}

void TrackVault::flushToDisk()
{
    File store = File::getSpecialLocation(File::userDocumentsDirectory)
                     .getChildFile("PulseStationVault.dat");
    store.create();
    FileOutputStream out(store);
    out.truncate();

    for (size_t i = 0; i < allPaths.size(); ++i)
    {
        String row = String(allPaths[i]) + "|" + String(allNames[i]) + "|"
                   + String(allLengths[i]) + "|" + String(allTempos[i]) + "\n";
        out.writeText(row, false, false, nullptr);
    }
}

// ---------------------------------------------------------------------------
// BPM estimation via energy-peak autocorrelation
// ---------------------------------------------------------------------------

// --- WRITTEN BY SYED ALI AHSAN (R5C, R5D: BPM Calculation Implementation) ---
double TrackVault::estimateTempo(File audioFile)
{
    std::unique_ptr<AudioFormatReader> reader(fmtMgr.createReaderFor(audioFile));
    if (reader == nullptr) return 0.0;

    int channels   = static_cast<int>(reader->numChannels);
    int rate       = static_cast<int>(reader->sampleRate);
    int maxSamples = static_cast<int>(
        std::min(static_cast<int64>(reader->lengthInSamples),
                 static_cast<int64>(30 * rate)));

    AudioBuffer<float> raw(channels, maxSamples);
    reader->read(&raw, 0, maxSamples, 0, true, true);

    // Down-mix to mono
    std::vector<float> mono(static_cast<size_t>(maxSamples), 0.0f);
    const float* chL = raw.getReadPointer(0);
    const float* chR = (channels > 1) ? raw.getReadPointer(1) : nullptr;

    for (int i = 0; i < maxSamples; ++i)
    {
        float left  = chL[i];
        float right = (chR != nullptr) ? chR[i] : left;
        mono[static_cast<size_t>(i)] = (left + right) * 0.5f;
    }

    // Compute RMS energy per block
    constexpr int kBlock = 1024;
    int blockCount = maxSamples / kBlock;
    std::vector<float> rms;
    rms.reserve(static_cast<size_t>(blockCount));

    for (int b = 0; b < blockCount; ++b)
    {
        float acc = 0.0f;
        for (int j = 0; j < kBlock; ++j)
        {
            float s = mono[static_cast<size_t>(b * kBlock + j)];
            acc += s * s;
        }
        rms.push_back(std::sqrt(acc / static_cast<float>(kBlock)));
    }

    // Detect peaks above 1.5x average
    float avg = 0.0f;
    for (float v : rms) avg += v;
    avg /= static_cast<float>(rms.size());

    float peakThreshold = avg * 1.5f;

    std::vector<int> peaks;
    for (size_t i = 1; i + 1 < rms.size(); ++i)
    {
        if (rms[i] > peakThreshold && rms[i] > rms[i - 1] && rms[i] > rms[i + 1])
            peaks.push_back(static_cast<int>(i));
    }

    if (peaks.size() < 2) return 120.0;

    // Measure inter-peak intervals and take the median
    std::vector<float> gaps;
    for (size_t i = 1; i < peaks.size(); ++i)
    {
        int diff = peaks[i] - peaks[i - 1];
        float timeSec = static_cast<float>(diff) * static_cast<float>(kBlock)
                      / static_cast<float>(rate);

        if (timeSec > 0.25f && timeSec < 1.2f)
            gaps.push_back(timeSec);
    }

    if (gaps.empty()) return 120.0;

    std::sort(gaps.begin(), gaps.end());
    float medianGap = gaps[gaps.size() / 2];
    if (medianGap <= 0.0f) return 120.0;

    double bpm = 60.0 / static_cast<double>(medianGap);

    // Normalise into a typical DJ range
    while (bpm < 75.0)  bpm *= 2.0;
    while (bpm > 175.0) bpm /= 2.0;

    return bpm;
}
