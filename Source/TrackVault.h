/*
  ==============================================================================

    TrackVault.h
    Author:  Syed Ali Ahsan
    Persistent music collection manager for PulseStation

  ==============================================================================
*/

// --- WRITTEN BY SYED ALI AHSAN (R2: Music Library & Persisting) ---

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <string>

class ChannelStrip;  // forward declaration

/**
 * TrackVault presents a searchable, persistent table of audio files.
 * Tracks can be loaded via file-chooser dialog or drag-and-drop, and
 * each entry stores title, duration, and estimated BPM.  The entire
 * collection is saved to a pipe-delimited text file in the user's
 * documents folder so it survives application restarts.
 */
class TrackVault : public Component,
                   public TableListBoxModel,
                   public Button::Listener,
                   public TextEditor::Listener,
                   public FileDragAndDropTarget
{
public:

    /** Construct with a reference to the shared format manager. */
    TrackVault(AudioFormatManager& fmtMgr);
    ~TrackVault() override;

    /** 
     * Draws the main interface and background behind the table.
     * @param g  The graphics context for drawing.
     */
    void paint(Graphics& g) override;

    /** 
     * Defines layout proportions for toolbar and table list box.
     */
    void resized() override;

    // ---- TableListBoxModel ----

    /** 
     * Return the number of rows currently visible (filtered). 
     * @return int  Count of visible items in the collection.
     */
    int getNumRows() override;

    /**
     * Fills the bounding box for a specific row to indicate selection or normal state.
     * @param g           The graphics context.
     * @param row         The row index.
     * @param w           Width of the row.
     * @param h           Height of the row.
     * @param isSelected  Whether the row is selected or not.
     */
    void paintRowBackground(Graphics& g, int row, int w, int h,
                            bool isSelected) override;

    /**
     * Renders individual cells inside the row, including text data and load buttons.
     * @param g           The graphics context.
     * @param row         The row index.
     * @param col         The column ID.
     * @param w           Width of the cell.
     * @param h           Height of the cell.
     * @param isSelected  Whether the row is selected or not.
     */
    void paintCell(Graphics& g, int row, int col, int w, int h,
                   bool isSelected) override;

    /** 
     * Handle clicks on the load-to-deck columns.
     * @param row   The row index clicked.
     * @param col   The column index clicked.
     * @param event The mouse event details.
     */
    void cellClicked(int row, int col, const MouseEvent& event) override;

    // ---- Button / TextEditor listeners ----

    /**
     * Handles clicks on the UI buttons (Import and Purge).
     * @param btn  The clicked button.
     */
    void buttonClicked(Button* btn) override;

    /**
     * Refreshes the filtered tracklist as the user types.
     * @param editor  The text editor instance modified.
     */
    void textEditorTextChanged(TextEditor& editor) override;

    // ---- Drag & Drop ----

    /**
     * Determine whether the vault should accept the dragged files.
     * @param files Array of dragged file paths.
     * @return bool True if we accept the drop, false otherwise.
     */
    bool isInterestedInFileDrag(const StringArray& files) override;

    /**
     * Append the dropped files to the track vault and persist them.
     * @param files  Array of dropped file paths.
     * @param x      X coordinate of the drop point.
     * @param y      Y coordinate of the drop point.
     */
    void filesDropped(const StringArray& files, int x, int y) override;

    // ---- Deck wiring ----

    /** Connect this vault to two channel strips for load-to-deck. */
    void wireChannelStrips(ChannelStrip* stripA, ChannelStrip* stripB);

private:
    AudioFormatManager& fmtMgr;
    TableListBox tableView;

    // UI elements
    TextEditor filterBox;
    TextButton importBtn;
    TextButton purgeBtn;

    // Master collection (persistent)
    std::vector<std::string> allPaths;
    std::vector<std::string> allNames;
    std::vector<int>         allLengths;   // seconds
    std::vector<double>      allTempos;    // BPM

    // Filtered subset for display
    std::vector<std::string> visPaths;
    std::vector<std::string> visNames;
    std::vector<int>         visLengths;
    std::vector<double>      visTempos;

    ChannelStrip* stripLeft  = nullptr;
    ChannelStrip* stripRight = nullptr;

    // ---- Helpers ----
    void hydrateFromDisk();
    void flushToDisk();
    void applyFilter(String query);
    std::string formatDuration(int totalSecs);
    double estimateTempo(File audioFile);
    void ingestFile(File audioFile);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackVault)
};
