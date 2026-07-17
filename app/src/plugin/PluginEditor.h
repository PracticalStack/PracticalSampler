#pragma once

#include "shared/AuthoringPanel.h"
#include "plugin/PluginProcessor.h"
#include "shared/PerformancePanel.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace drs::plugin
{
class Editor final : public juce::AudioProcessorEditor
{
public:
    explicit Editor(Processor&);
    ~Editor() override;

    void resized() override;

private:
    enum MenuCommandId
    {
        newProjectCommandId = 1,
        openProjectCommandId,
        closeProjectCommandId,
        saveProjectCommandId,
        saveProjectAsCommandId,
        importWavCommandId,
        preferencesCommandId
    };

    void showFileMenu();
    void showSettingsMenu();
    void handleMenuCommand(int menuItemId);
    void createNewProject();
    void openProject();
    void closeProject();
    void saveProject(std::function<void(bool)> completion = {});
    void saveProjectAs(std::function<void(bool)> completion = {});
    void importWavFiles();
    void showPreferencesDialog();
    void restoreSelectedZoneRootKey();
    bool saveProjectToFile(const juce::File& file);
    bool loadProjectFromFile(const juce::File& file);
    void confirmSafeToDiscardChanges(const juce::String& nextAction, std::function<void(bool)> completion);
    void refreshProjectViews();
    void updateProjectStatusLabel();
    drs::engine::RuntimeProjectModel buildUnloadedProjectState() const;
    drs::engine::RuntimeProjectModel buildEmptyProjectTemplate() const;
    juce::String buildProjectIssueSummary(const std::vector<std::string>& issues) const;
    juce::String buildImportSummaryMessage(std::size_t importedCount,
                                           std::size_t warningCount,
                                           std::size_t skippedCount,
                                           const std::vector<std::string>& details) const;
    juce::File getLibraryLocation() const;
    void setLibraryLocation(const juce::File& folder);
    juce::File getProjectDirectory() const;
    void setProjectDirectory(const juce::File& folder);
    juce::File getRecentProjectDirectory() const;
    void setRecentProjectDirectory(const juce::File& folder);
    juce::File buildChooserBaseDirectory() const;
    juce::File buildDefaultSaveTarget() const;
    void launchOpenProjectChooser(std::function<void(juce::File)> completion);
    void launchSaveProjectChooser(std::function<void(juce::File)> completion);
    void launchImportWavChooser(std::function<void(std::vector<juce::File>)> completion);
    void promptForRootKeySelection(const juce::String& title,
                                   const juce::String& message,
                                   int initialRootKey,
                                   std::function<void(std::optional<int>)> completion) const;

    Processor& processor;
    juce::Component workspaceShell;
    juce::TextButton fileMenuButton { "File" };
    juce::TextButton settingsMenuButton { "Settings" };
    juce::Label projectStatusLabel;
    juce::TabbedComponent workspaceTabs { juce::TabbedButtonBar::TabsAtTop };
    drs::app::PerformancePanel performancePanel;
    drs::app::AuthoringPanel authoringPanel;
    mutable juce::ApplicationProperties appProperties;
    std::unique_ptr<juce::FileChooser> activeFileChooser;
};
} // namespace drs::plugin
