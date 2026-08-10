/*
  ==============================================================================

    PulseStationApp.cpp
    Author:  Syed Ali Ahsan
    Application entry point for PulseStation DJ mixer

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "StationHub.h"

//==============================================================================
/**
 * PulseStationApp bootstraps the JUCE event loop and creates the main window.
 */
class PulseStationApp : public JUCEApplication
{
public:

    PulseStationApp() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override        { return true; }

    //--------------------------------------------------------------------------
    void initialise(const String& cmdLine) override
    {
        juce::ignoreUnused(cmdLine);
        mainFrame.reset(new MainFrame(getApplicationName()));
    }

    void shutdown() override
    {
        mainFrame = nullptr;
    }

    //--------------------------------------------------------------------------
    void systemRequestedQuit() override          { quit(); }
    void anotherInstanceStarted(const String& c) override { juce::ignoreUnused(c); }

    //--------------------------------------------------------------------------
    /**
     * MainFrame is a simple DocumentWindow wrapper that hosts the StationHub.
     */
    class MainFrame : public DocumentWindow
    {
    public:
        MainFrame(String title)
            : DocumentWindow(title,
                             Desktop::getInstance()
                                 .getDefaultLookAndFeel()
                                 .findColour(ResizableWindow::backgroundColourId),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new StationHub(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
           #else
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
           #endif

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainFrame)
    };

private:
    std::unique_ptr<MainFrame> mainFrame;
};

//==============================================================================
START_JUCE_APPLICATION(PulseStationApp)
