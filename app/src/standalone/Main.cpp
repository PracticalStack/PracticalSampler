#include "standalone/MainComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace
{
class DecentRhapsodyStudioApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
            mainWindow->requestClose();
        else
            quit();
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : juce::DocumentWindow(name,
                                   juce::Desktop::getInstance().getDefaultLookAndFeel()
                                       .findColour(juce::ResizableWindow::backgroundColourId),
                                   juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new drs::standalone::MainComponent(), true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            requestClose();
        }

        void requestClose()
        {
            if (auto* mainComponent = dynamic_cast<drs::standalone::MainComponent*>(getContentComponent()))
            {
                mainComponent->handleCloseRequest([](bool shouldClose)
                {
                    if (shouldClose)
                        juce::JUCEApplication::getInstance()->quit();
                });
                return;
            }

            juce::JUCEApplication::getInstance()->quit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};
} // namespace

START_JUCE_APPLICATION(DecentRhapsodyStudioApplication)
