#pragma once

#include "shared/AuthoringPanel.h"
#include "shared/HostStateRecoveryBanner.h"
#include "plugin/PluginProcessor.h"
#include "shared/PerformancePanel.h"
#include "shared/WavImportWorkflow.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <atomic>
#include <memory>
#include <optional>

namespace drs::plugin
{
class Editor final : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit Editor(Processor&);
    ~Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

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
        importBackgroundImageCommandId,
        importLicenseFileCommandId,
        viewLicenseCommandId,
        preferencesCommandId
    };

    void timerCallback() override;
    void showFileMenu();
    void showSettingsMenu();
    void handleMenuCommand(int menuItemId);
    void createNewProject();
    void openProject();
    void openPerformancePackage();
    void closeProject();
    void saveProject(std::function<void(bool)> completion = {});
    void saveProjectAs(std::function<void(bool)> completion = {});
    void exportPerformancePackage();
    void importWavFiles();
    void importSfzFile();
    void importBackgroundImage();
    void importLicenseFile();
    void viewLicense();
    void importSampleFiles(std::vector<juce::File> selectedFiles);
    void reviewSfzImportFile(const juce::File& selectedFile);
    void showPreferencesDialog();
    void locateProjectForRestore();
    void restoreSelectedZoneRootKey();
    bool saveProjectToFile(const juce::File& file);
    bool loadProjectFromFile(const juce::File& file);
    bool loadPerformancePackageFromFile(const juce::File& file);
    void confirmSafeToDiscardChanges(const juce::String& nextAction, std::function<void(bool)> completion);
    void refreshProjectViews();
    void synchronizeWorkspacePresentation();
    void updateProjectStatusLabel();
    void pollPerformancePackageExportService();
    void pollPerformancePackageOpenTask();
    void pollWavImportService();
    void pollSfzImportReviewService();
    drs::engine::RuntimeProjectModel buildUnloadedProjectState() const;
    drs::engine::RuntimeProjectModel buildEmptyProjectTemplate() const;
    juce::String buildWorkspaceDisplayName() const;
    juce::String buildWorkspaceStatusText() const;
    juce::String buildWorkspaceStatusTooltip() const;
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
    juce::File buildDefaultPackageExportTarget() const;
    void launchOpenProjectChooser(std::function<void(juce::File)> completion);
    void launchOpenPerformancePackageChooser(std::function<void(juce::File)> completion);
    void launchNewProjectChooser(std::function<void(juce::File)> completion);
    void launchSaveProjectChooser(std::function<void(juce::File)> completion);
    void launchExportPerformancePackageChooser(std::function<void(juce::File)> completion);
    void launchImportWavChooser(std::function<void(std::vector<juce::File>)> completion);
    void launchImportSfzChooser(std::function<void(juce::File)> completion);
    void launchImportBackgroundImageChooser(std::function<void(juce::File)> completion);
    void launchImportLicenseFileChooser(std::function<void(juce::File)> completion);
    void promptForRootKeySelection(const juce::String& title,
                                   const juce::String& message,
                                   int initialRootKey,
                                   std::function<void(std::optional<int>)> completion) const;

    struct PendingPerformancePackageOpenTask
    {
        juce::File file;
        std::shared_ptr<std::atomic<bool>> ready;
        std::shared_ptr<drs::plugin::PreparedPerformancePackageWorkspaceLoadResult> result;
        std::uint64_t generation = 0;
    };

    Processor& processor;
    juce::Component workspaceShell;
    juce::TextButton fileMenuButton { "File" };
    juce::TextButton settingsMenuButton { "Settings" };
    juce::Label projectStatusLabel;
    juce::TabbedComponent workspaceTabs { juce::TabbedButtonBar::TabsAtTop };
    drs::app::PerformancePanel performancePanel;
    drs::app::AuthoringPanel authoringPanel;
    drs::app::HostStateRecoveryBanner restoreBanner;
    drs::app::PerformancePackageExportProgressComponent performancePackageExportProgress;
    drs::app::WavImportProgressComponent wavImportProgress;
    drs::app::SfzImportProgressComponent sfzImportProgress;
    mutable juce::ApplicationProperties appProperties;
    std::unique_ptr<juce::FileChooser> activeFileChooser;
    std::optional<PendingPerformancePackageOpenTask> pendingPerformancePackageOpenTask;
    std::uint64_t nextPerformancePackageOpenGeneration = 0;
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
} // namespace drs::plugin
