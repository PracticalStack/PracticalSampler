#pragma once

#include "plugin/PluginProcessor.h"
#include "shared/AuthoringPanel.h"
#include "shared/HostStateRecoveryBanner.h"
#include "shared/PerformancePanel.h"
#include "shared/WavImportWorkflow.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <functional>
#include <memory>
#include <optional>

namespace drs::standalone
{
class MainComponent final : public juce::Component,
                            private juce::MenuBarModel,
                            private juce::Timer,
                            private juce::ChangeListener
{
public:
    explicit MainComponent(bool enableAudioOutput = true);
    ~MainComponent() override;

    void resized() override;

    std::string exportStateJson() const;
    drs::engine::EnginePresetStateRestoreResult restoreStateJson(const std::string& stateJson);
    bool setMacroValue(const std::string& macroId, double value);
    void handleCloseRequest(std::function<void(bool)> completion);
    drs::engine::EngineFacade& getEngineFacade() { return processor.getEngineFacade(); }
    const drs::engine::EngineFacade& getEngineFacade() const { return processor.getEngineFacade(); }
    drs::plugin::Processor& getProcessor() { return processor; }
    const drs::plugin::Processor& getProcessor() const { return processor; }
    bool isAudioOutputEnabled() const { return audioOutputEnabled; }
    bool hasAudioDeviceError() const { return !audioDeviceError.isEmpty(); }
    const juce::String& getAudioDeviceError() const { return audioDeviceError; }

private:
    enum MenuCommandId
    {
        newProjectCommandId = 1,
        openProjectCommandId,
        openPerformancePackageCommandId,
        closeProjectCommandId,
        saveProjectCommandId,
        saveProjectAsCommandId,
        exportPerformancePackageCommandId,
        importWavCommandId,
        importSfzCommandId,
        audioDeviceSettingsCommandId,
        preferencesCommandId,
        exitApplicationCommandId
    };

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void initializeAudioOutput();
    void synchronizeAudioOutputRegistration();
    void shutdownAudioOutput();
    void createNewProject();
    void openProject();
    void openPerformancePackage();
    void closeProject();
    void saveProject(std::function<void(bool)> completion = {});
    void saveProjectAs(std::function<void(bool)> completion = {});
    void exportPerformancePackage();
    void importWavFiles();
    void importSfzFile();
    void importSampleFiles(std::vector<juce::File> selectedFiles);
    void reviewSfzImportFile(const juce::File& selectedFile);
    void showAudioDeviceSettingsDialog();
    void showPreferencesDialog();
    void locateProjectForRestore();
    void restoreSelectedZoneRootKey();
    bool saveProjectToFile(const juce::File& file);
    bool loadProjectFromFile(const juce::File& file);
    bool loadPerformancePackageFromFile(const juce::File& file);
    void confirmSafeToDiscardChanges(const juce::String& nextAction, std::function<void(bool)> completion);
    void refreshProjectViews();
    void synchronizeWorkspacePresentation();
    void updateWorkspaceStatusLabel();
    void updateWindowTitle();
    void pollPerformancePackageExportService();
    void pollWavImportService();
    void pollSfzImportReviewService();
    drs::engine::RuntimeProjectModel buildUnloadedProjectState() const;
    drs::engine::RuntimeProjectModel buildEmptyProjectTemplate() const;
    juce::String buildWorkspaceDisplayName() const;
    juce::String buildWorkspaceStatusText() const;
    juce::String buildWorkspaceStatusTooltip() const;
    juce::String buildWindowTitle() const;
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
    std::unique_ptr<juce::XmlElement> loadSavedAudioDeviceState() const;
    void saveAudioDeviceSettings();
    juce::File buildChooserBaseDirectory() const;
    juce::File buildDefaultSaveTarget() const;
    juce::File buildDefaultPackageExportTarget() const;
    void launchOpenProjectChooser(std::function<void(juce::File)> completion);
    void launchOpenPerformancePackageChooser(std::function<void(juce::File)> completion);
    void launchNewProjectChooser(std::function<void(juce::File)> completion);
    void launchSaveProjectChooser(std::function<void(juce::File)> completion);
    void launchExportPerformancePackageChooser(std::function<void(juce::File)> completion);
    void launchImportWavChooser(std::function<void(std::vector<juce::File>)> completion);
    void launchImportSfzChooser(std::function<void(juce::File)> completion);
    void promptForRootKeySelection(const juce::String& title,
                                   const juce::String& message,
                                   int initialRootKey,
                                   std::function<void(std::optional<int>)> completion) const;

    drs::plugin::Processor processor;
    juce::MenuBarComponent menuBar { this };
    juce::Label sessionStatusLabel;
    juce::TabbedComponent workspaceTabs { juce::TabbedButtonBar::TabsAtTop };
    drs::app::PerformancePanel performancePanel;
    drs::app::AuthoringPanel authoringPanel;
    drs::app::HostStateRecoveryBanner restoreBanner;
    drs::app::PerformancePackageExportProgressComponent performancePackageExportProgress;
    drs::app::WavImportProgressComponent wavImportProgress;
    drs::app::SfzImportProgressComponent sfzImportProgress;
    juce::AudioDeviceManager audioDeviceManager;
    juce::AudioProcessorPlayer audioProcessorPlayer;
    mutable juce::ApplicationProperties appProperties;
    std::unique_ptr<juce::FileChooser> activeFileChooser;
    juce::File currentProjectFile;
    bool audioOutputEnabled = false;
    juce::String audioDeviceError;
    std::optional<drs::app::PerformancePackageExportService::Client> performancePackageExportClient;
    std::optional<drs::app::WavImportService::Client> wavImportClient;
    std::shared_ptr<drs::app::PreparedWavImportBatch> wavImportPreparedBatch;
    std::string wavImportProjectId;
    std::size_t wavImportBaseRevision = 0;
    std::string wavImportContentRootPath;
    std::string wavImportSelectedGroupId;
    std::uint64_t wavImportOwnerId = 0;
    std::uint64_t wavImportGeneration = 0;
    bool wavImportManualRootDialogOpen = false;
    std::optional<drs::app::SfzImportReviewService::Client> sfzImportClient;
    std::string sfzImportProjectId;
    std::size_t sfzImportBaseRevision = 0;
    bool sfzImportReviewDialogOpen = false;
};
} // namespace drs::standalone
