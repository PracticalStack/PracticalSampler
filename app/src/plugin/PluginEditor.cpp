#include "plugin/PluginEditor.h"

#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"
#include "shared/PlayableInstrumentLicenseViewer.h"
#include "shared/ProjectStorage.h"
#include "shared/MessageThreadMetrics.h"
#include "shared/SfzImportWorkflow.h"
#include "shared/WorkspaceMenuPolicy.h"
#include "shared/WavImportWorkflow.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>

namespace drs::plugin
{
namespace
{
namespace fs = std::filesystem;
const auto performTabColour = drs::app::authoring::visual::surfaceSubtle;
const auto mapTabColour = drs::app::authoring::visual::surface;

constexpr int saveButtonResult = 1;
constexpr int discardButtonResult = 2;
constexpr int cancelButtonResult = 0;
constexpr auto libraryLocationPropertyKey = "libraryLocation";
constexpr auto projectDirectoryPropertyKey = "projectDirectory";
constexpr auto recentProjectDirectoryPropertyKey = "recentProjectDirectory";
constexpr int menuRowHeight = 32;
constexpr int menuButtonHeight = 24;
constexpr int menuButtonSpacing = 8;
constexpr int menuButtonYInset = 4;
constexpr int menuButtonWidth = 72;
constexpr int settingsButtonWidth = 92;

std::string buildPerformancePackageTimingSummary(
    const drs::engine::PerformancePackagePreparationTimings& timings)
{
    if (timings.packageLoadMicros == 0
        && timings.snapshotBuildMicros == 0
        && timings.preparedBuildMicros == 0
        && timings.activationPayloadMicros == 0
        && timings.totalMicros == 0)
    {
        return {};
    }

    const auto toMillis = [](const std::uint64_t micros)
    {
        return static_cast<double>(micros) / 1000.0;
    };

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1)
           << "Package-open timings (ms): load=" << toMillis(timings.packageLoadMicros)
           << ", snapshot=" << toMillis(timings.snapshotBuildMicros)
           << ", prepared=" << toMillis(timings.preparedBuildMicros)
           << ", payload=" << toMillis(timings.activationPayloadMicros)
           << ", total=" << toMillis(timings.totalMicros);
    return stream.str();
}

juce::String formatMidiNoteLabel(int midiNote)
{
    return juce::String(midiNote) + " - " + juce::MidiMessage::getMidiNoteName(midiNote, true, true, 4);
}

const drs::engine::RuntimeProjectSampleSource* findSampleSource(const drs::engine::RuntimeProjectModel& project,
                                                                const std::string& sampleSourceId)
{
    const auto iterator = std::find_if(project.sampleSources.begin(),
                                       project.sampleSources.end(),
                                       [&](const drs::engine::RuntimeProjectSampleSource& sampleSource)
                                       {
                                           return sampleSource.id == sampleSourceId;
                                       });
    return iterator == project.sampleSources.end() ? nullptr : &*iterator;
}

bool ensureDirectoryExists(const juce::File& directory)
{
    if (directory == juce::File())
        return false;

    if (directory.exists())
        return directory.isDirectory();

    return directory.createDirectory();
}

std::string makeProjectId()
{
    return "project-" + juce::Uuid().toString().toLowerCase().toStdString();
}

std::optional<drs::engine::RuntimeProjectModel> upgradeLoadedProjectToLatestSchema(
    const drs::engine::RuntimeProjectModel& project,
    std::vector<std::string>& issues)
{
    auto upgradedProject = project;

    if (upgradedProject.schemaVersion == 1)
    {
        const auto phase2Migration = drs::engine::migrateRuntimeProjectToPhase2Authoring(upgradedProject);
        if (!phase2Migration.valid)
        {
            issues = phase2Migration.issues;
            return std::nullopt;
        }

        upgradedProject = phase2Migration.project;
    }

    if (upgradedProject.schemaVersion == 2 && upgradedProject.authoring.schemaVersion == 1)
    {
        const auto phase3Migration = drs::engine::migrateRuntimeProjectToPhase3RoundRobinSchema(upgradedProject);
        if (!phase3Migration.valid)
        {
            issues = phase3Migration.issues;
            return std::nullopt;
        }

        upgradedProject = phase3Migration.project;
    }

    if (upgradedProject.schemaVersion == 3 && upgradedProject.authoring.schemaVersion == 2)
    {
        const auto zoneGroupMigration = drs::engine::migrateRuntimeProjectToZoneGroupsSchema(upgradedProject);
        if (!zoneGroupMigration.valid)
        {
            issues = zoneGroupMigration.issues;
            return std::nullopt;
        }

        upgradedProject = zoneGroupMigration.project;
    }

    if (upgradedProject.schemaVersion == 4 && upgradedProject.authoring.schemaVersion == 3)
    {
        const auto dspMigration = drs::engine::migrateRuntimeProjectToCuratedDspSchema(upgradedProject);
        if (!dspMigration.valid)
        {
            issues = dspMigration.issues;
            return std::nullopt;
        }

        upgradedProject = dspMigration.project;
    }

    if (upgradedProject.schemaVersion == 5 && upgradedProject.authoring.schemaVersion == 4)
    {
        const auto articulationMigration = drs::engine::migrateRuntimeProjectToPerformanceArticulationSchema(upgradedProject);
        if (!articulationMigration.valid)
        {
            issues = articulationMigration.issues;
            return std::nullopt;
        }

        upgradedProject = articulationMigration.project;
    }

    if (upgradedProject.schemaVersion == 6 && upgradedProject.authoring.schemaVersion == 5)
    {
        const auto damperMigration = drs::engine::migrateRuntimeProjectToContinuousDamperSchema(upgradedProject);
        if (!damperMigration.valid)
        {
            issues = damperMigration.issues;
            return std::nullopt;
        }

        upgradedProject = damperMigration.project;
    }

    if (upgradedProject.schemaVersion == drs::engine::continuousDamperProjectSchemaVersion
        && upgradedProject.authoring.schemaVersion == drs::engine::continuousDamperAuthoringSchemaVersion)
    {
        const auto playbackRegionMigration =
            drs::engine::migrateRuntimeProjectToPlaybackRegionSchema(upgradedProject);
        if (!playbackRegionMigration.valid)
        {
            issues = playbackRegionMigration.issues;
            return std::nullopt;
        }
        upgradedProject = playbackRegionMigration.project;
    }

    if (upgradedProject.schemaVersion == drs::engine::playbackRegionProjectSchemaVersion
        && upgradedProject.authoring.schemaVersion == drs::engine::playbackRegionAuthoringSchemaVersion)
    {
        const auto loopCrossfadeMigration =
            drs::engine::migrateRuntimeProjectToLoopCrossfadeSchema(upgradedProject);
        if (!loopCrossfadeMigration.valid)
        {
            issues = loopCrossfadeMigration.issues;
            return std::nullopt;
        }
        upgradedProject = loopCrossfadeMigration.project;
    }

    if (upgradedProject.schemaVersion == drs::engine::loopCrossfadeProjectSchemaVersion
        && upgradedProject.authoring.schemaVersion == drs::engine::loopCrossfadeAuthoringSchemaVersion)
    {
        const auto layerMigration = drs::engine::migrateRuntimeProjectToLayerSchema(upgradedProject);
        if (!layerMigration.valid)
        {
            issues = layerMigration.issues;
            return std::nullopt;
        }
        upgradedProject = layerMigration.project;
    }

    return upgradedProject;
}

class PreferencesComponent final : public juce::Component
{
public:
    PreferencesComponent(juce::File initialLibraryLocation,
                         juce::File initialProjectDirectory,
                         std::function<void(juce::File, juce::File)> onSave)
        : onSaveCallback(std::move(onSave))
    {
        titleLabel.setText("Preferences", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));

        libraryLocationLabel.setText("Library Location", juce::dontSendNotification);
        libraryLocationLabel.setJustificationType(juce::Justification::centredLeft);

        projectDirectoryLabel.setText("Default Project Directory", juce::dontSendNotification);
        projectDirectoryLabel.setJustificationType(juce::Justification::centredLeft);

        libraryHelpLabel.setText("Used as the default starting folder for WAV imports.",
                                 juce::dontSendNotification);
        libraryHelpLabel.setJustificationType(juce::Justification::centredLeft);

        projectHelpLabel.setText(drs::app::projectDirectoryHelpText,
                                 juce::dontSendNotification);
        projectHelpLabel.setJustificationType(juce::Justification::centredLeft);

        libraryLocationEditor.setText(initialLibraryLocation.getFullPathName(), juce::dontSendNotification);
        projectDirectoryEditor.setText(initialProjectDirectory.getFullPathName(), juce::dontSendNotification);

        libraryBrowseButton.setButtonText("Browse...");
        libraryBrowseButton.onClick = [this]
        {
            browseForDirectory(libraryLocationEditor, "Choose library location");
        };

        projectBrowseButton.setButtonText("Browse...");
        projectBrowseButton.onClick = [this]
        {
            browseForDirectory(projectDirectoryEditor, "Choose default project directory");
        };

        saveButton.setButtonText("Save");
        saveButton.onClick = [this]
        {
            auto chosenDirectory = juce::File(libraryLocationEditor.getText().trim());
            if (libraryLocationEditor.getText().trim().isEmpty())
            {
                chosenDirectory = {};
            }
            else if (!ensureDirectoryExists(chosenDirectory))
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Invalid Library Location",
                                                       "The selected library folder could not be created.");
                return;
            }
            else if (!chosenDirectory.isDirectory())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Invalid Library Location",
                                                       "The selected path is not a folder.");
                return;
            }

            auto chosenProjectDirectory = juce::File(projectDirectoryEditor.getText().trim());
            if (chosenProjectDirectory == juce::File())
                chosenProjectDirectory = drs::app::getDefaultStudioProjectDirectory();

            if (!ensureDirectoryExists(chosenProjectDirectory))
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Invalid Project Directory",
                                                       "The selected project folder could not be created.");
                return;
            }

            if (!chosenProjectDirectory.isDirectory())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Invalid Project Directory",
                                                       "The selected path is not a folder.");
                return;
            }

            if (onSaveCallback)
                onSaveCallback(chosenDirectory, chosenProjectDirectory);

            closeDialog(saveButtonResult);
        };

        cancelButton.setButtonText("Cancel");
        cancelButton.onClick = [this]
        {
            closeDialog(cancelButtonResult);
        };

        for (auto* component : {
                 static_cast<juce::Component*>(&titleLabel),
                 static_cast<juce::Component*>(&libraryLocationLabel),
                 static_cast<juce::Component*>(&projectDirectoryLabel),
                 static_cast<juce::Component*>(&libraryHelpLabel),
                 static_cast<juce::Component*>(&projectHelpLabel),
                 static_cast<juce::Component*>(&libraryLocationEditor),
                 static_cast<juce::Component*>(&projectDirectoryEditor),
                 static_cast<juce::Component*>(&libraryBrowseButton),
                 static_cast<juce::Component*>(&projectBrowseButton),
                 static_cast<juce::Component*>(&saveButton),
                 static_cast<juce::Component*>(&cancelButton)
             })
        {
            addAndMakeVisible(component);
        }

        setSize(720, 230);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(16);
        titleLabel.setBounds(area.removeFromTop(28));
        area.removeFromTop(8);
        libraryLocationLabel.setBounds(area.removeFromTop(24));
        auto editorRow = area.removeFromTop(28);
        libraryLocationEditor.setBounds(editorRow.removeFromLeft(editorRow.proportionOfWidth(0.8f)));
        editorRow.removeFromLeft(8);
        libraryBrowseButton.setBounds(editorRow);
        area.removeFromTop(8);
        libraryHelpLabel.setBounds(area.removeFromTop(22));
        area.removeFromTop(12);
        projectDirectoryLabel.setBounds(area.removeFromTop(24));
        auto projectRow = area.removeFromTop(28);
        projectDirectoryEditor.setBounds(projectRow.removeFromLeft(projectRow.proportionOfWidth(0.8f)));
        projectRow.removeFromLeft(8);
        projectBrowseButton.setBounds(projectRow);
        area.removeFromTop(8);
        projectHelpLabel.setBounds(area.removeFromTop(22));
        area.removeFromTop(16);
        auto buttonRow = area.removeFromBottom(28);
        cancelButton.setBounds(buttonRow.removeFromRight(100));
        buttonRow.removeFromRight(8);
        saveButton.setBounds(buttonRow.removeFromRight(100));
    }

private:
    void browseForDirectory(juce::TextEditor& targetEditor, const juce::String& dialogTitle)
    {
        auto initialDirectory = juce::File(targetEditor.getText().trim());
        if (initialDirectory == juce::File())
            initialDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

        directoryChooser = std::make_unique<juce::FileChooser>(dialogTitle,
                                                               initialDirectory,
                                                               "*",
                                                               true,
                                                               false,
                                                               this);
        auto safeThis = juce::Component::SafePointer<PreferencesComponent>(this);
        directoryChooser->launchAsync(juce::FileBrowserComponent::openMode
                                          | juce::FileBrowserComponent::canSelectDirectories,
                                      [safeThis, target = juce::Component::SafePointer<juce::TextEditor>(&targetEditor)]
                                      (const juce::FileChooser& chooser)
                                      {
                                          if (safeThis == nullptr || target == nullptr)
                                              return;

                                          const auto selectedDirectory = chooser.getResult();
                                          safeThis->directoryChooser.reset();
                                          if (selectedDirectory != juce::File())
                                              target->setText(selectedDirectory.getFullPathName(),
                                                              juce::dontSendNotification);
                                      });
    }

    void closeDialog(int result)
    {
        if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
            dialog->exitModalState(result);
    }

    std::function<void(juce::File, juce::File)> onSaveCallback;
    std::unique_ptr<juce::FileChooser> directoryChooser;
    juce::Label titleLabel;
    juce::Label libraryLocationLabel;
    juce::Label projectDirectoryLabel;
    juce::Label libraryHelpLabel;
    juce::Label projectHelpLabel;
    juce::TextEditor libraryLocationEditor;
    juce::TextEditor projectDirectoryEditor;
    juce::TextButton libraryBrowseButton;
    juce::TextButton projectBrowseButton;
    juce::TextButton saveButton;
    juce::TextButton cancelButton;
};

class RootKeySelectionComponent final : public juce::Component
{
public:
    RootKeySelectionComponent(const juce::String& message,
                              int initialRootKey,
                              std::optional<int>& selectedRootKeyOut)
        : selectedRootKeyResult(selectedRootKeyOut)
    {
        messageLabel.setText(message, juce::dontSendNotification);
        messageLabel.setJustificationType(juce::Justification::topLeft);

        rootKeyLabel.setText("Root Key", juce::dontSendNotification);
        rootKeyLabel.setJustificationType(juce::Justification::centredLeft);

        juce::StringArray rootKeyChoices;
        rootKeyChoices.ensureStorageAllocated(128);
        for (int midiNote = 0; midiNote < 128; ++midiNote)
            rootKeyChoices.add(formatMidiNoteLabel(midiNote));

        rootKeySelector.addItemList(rootKeyChoices, 1);
        rootKeySelector.setSelectedItemIndex(juce::jlimit(0, 127, initialRootKey), juce::dontSendNotification);

        useButton.setButtonText("Use Root Key");
        useButton.onClick = [this]
        {
            selectedRootKeyResult = rootKeySelector.getSelectedItemIndex();
            closeDialog(1);
        };

        cancelButton.setButtonText("Cancel");
        cancelButton.onClick = [this]
        {
            selectedRootKeyResult.reset();
            closeDialog(0);
        };

        for (auto* component : {
                 static_cast<juce::Component*>(&messageLabel),
                 static_cast<juce::Component*>(&rootKeyLabel),
                 static_cast<juce::Component*>(&rootKeySelector),
                 static_cast<juce::Component*>(&useButton),
                 static_cast<juce::Component*>(&cancelButton)
             })
        {
            addAndMakeVisible(component);
        }

        setSize(520, 170);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(16);
        messageLabel.setBounds(area.removeFromTop(64));
        area.removeFromTop(8);

        auto selectorRow = area.removeFromTop(28);
        rootKeyLabel.setBounds(selectorRow.removeFromLeft(72));
        selectorRow.removeFromLeft(8);
        rootKeySelector.setBounds(selectorRow);

        area.removeFromTop(18);
        auto buttonRow = area.removeFromBottom(28);
        cancelButton.setBounds(buttonRow.removeFromRight(100));
        buttonRow.removeFromRight(8);
        useButton.setBounds(buttonRow.removeFromRight(120));
    }

private:
    void closeDialog(int result)
    {
        if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
            dialog->exitModalState(result);
    }

    std::optional<int>& selectedRootKeyResult;
    juce::Label messageLabel;
    juce::Label rootKeyLabel;
    juce::ComboBox rootKeySelector;
    juce::TextButton useButton;
    juce::TextButton cancelButton;
};
} // namespace

Editor::Editor(Processor& owner)
    : juce::AudioProcessorEditor(owner),
      processor(owner),
      performancePanel(owner.getEngineFacade(),
                       [&owner](const std::string& macroId, double value)
                       {
                           owner.setMacroValueFromShell(macroId, value);
                       },
                       [&owner](int midiNoteNumber, float velocity)
                       {
                           owner.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);
                       },
                       [&owner](int midiNoteNumber)
                       {
                           owner.queuePerformanceSurfaceNoteOff(midiNoteNumber);
                       },
                       [&owner](const drs::engine::PerformancePublishCommand& command,
                                drs::engine::PerformancePublishCommandSource source)
                       {
                           return owner.submitPerformancePublishCommand(command, source);
                       },
                       [&owner]()
                       {
                           return owner.getPerformancePublishPresentationSnapshot();
                       },
                       [&owner]()
                       {
                           return owner.hasRecentAudioCallback();
                       },
                       [&owner]()
                       {
                           return owner.getInstrumentControlsExpandedChoice();
                       },
                       [&owner](const bool expanded)
                       {
                           owner.setInstrumentControlsExpandedChoice(expanded);
                       },
                       [&owner]()
                       {
                           const auto& document = owner.getWorkspaceDocumentState();
                           return document.displayName == "No Project Loaded"
                               ? juce::String {}
                               : juce::String::fromUTF8(document.displayName.c_str());
                       }),
      authoringPanel(owner.getAuthoringSession(),
                     [&owner]()
                     {
                         return owner.getAuthoringWaveformPreview();
                     },
                     [&owner]()
                     {
                         return owner.getAuthoringPreviewStatusSnapshot();
                     },
                     [&owner]()
                     {
                         return owner.getAuthoringImportResponsivenessSnapshot();
                     },
                     drs::app::AuthoringPanel::LayoutMode::compact,
                     [this]
                     {
                         restoreSelectedZoneRootKey();
                     },
                     [&owner]()
                     {
                         return owner.getEngineFacade().getDraftPlaybackStatus();
                     },
                     [&owner]()
                     {
                         owner.requestAuthoringPreview(
                             drs::engine::AuthoringPreviewScope::currentDraft);
                     },
                     [&owner]()
                     {
                         owner.submitPerformancePublishCommand(
                             {}, drs::engine::PerformancePublishCommandSource::authoringWorkspace);
                     },
                     [&owner](const drs::engine::AuthoringPreviewCommand& command)
                     {
                         owner.submitAuthoringPreviewCommand(command);
                     },
                     [this](std::vector<juce::File> files)
                     {
                         importSampleFiles(std::move(files));
                     },
                     [&owner]()
                     {
                         owner.authorizeAuthoringWaveformPreviewLoad();
                     },
                     [&owner]()
                     {
                         return owner.getAuthoringSourceValidationSnapshot();
                     },
                     [&owner]()
                     {
                         owner.requestAuthoringSourceValidation();
                     },
                     [&owner]()
                     {
                         owner.cancelAuthoringSourceValidation();
                     },
                     [&owner](const std::uint64_t startFrame,
                              const std::uint64_t endFrameExclusive,
                              const std::size_t displayPointCount)
                     {
                         owner.requestAuthoringWaveformDetail(startFrame,
                                                              endFrameExclusive,
                                                              displayPointCount);
                     }),
      restoreBanner([this] { locateProjectForRestore(); },
                    [&owner] { owner.retryProjectRestore(); })
{
    juce::PropertiesFile::Options appSettingsOptions;
    appSettingsOptions.applicationName = "DecentRhapsodyStudio";
    appSettingsOptions.filenameSuffix = "settings";
    appSettingsOptions.folderName = "DecentRhapsodyStudio";
    appSettingsOptions.osxLibrarySubFolder = "Application Support";
    appSettingsOptions.commonToAllUsers = false;
    appProperties.setStorageParameters(appSettingsOptions);

    workspaceShell.setComponentID("pluginWorkspaceShell");
    addAndMakeVisible(workspaceShell);

    fileMenuButton.setComponentID("pluginFileMenuButton");
    fileMenuButton.setColour(juce::TextButton::buttonColourId, drs::app::authoring::visual::surfaceRaised);
    fileMenuButton.setColour(juce::TextButton::textColourOffId, drs::app::authoring::visual::text);
    fileMenuButton.onClick = [this]
    {
        showFileMenu();
    };
    workspaceShell.addAndMakeVisible(fileMenuButton);

    settingsMenuButton.setComponentID("pluginSettingsMenuButton");
    settingsMenuButton.setColour(juce::TextButton::buttonColourId, drs::app::authoring::visual::surfaceRaised);
    settingsMenuButton.setColour(juce::TextButton::textColourOffId, drs::app::authoring::visual::text);
    settingsMenuButton.onClick = [this]
    {
        showSettingsMenu();
    };
    workspaceShell.addAndMakeVisible(settingsMenuButton);

    projectStatusLabel.setComponentID("pluginProjectStatusLabel");
    projectStatusLabel.setJustificationType(juce::Justification::centredRight);
    projectStatusLabel.setColour(juce::Label::textColourId, drs::app::authoring::visual::textMuted);
    workspaceShell.addAndMakeVisible(projectStatusLabel);

    workspaceTabs.setComponentID("workspaceTabs");
    workspaceTabs.setColour(juce::TabbedComponent::backgroundColourId,
                            drs::app::authoring::visual::shell);
    workspaceTabs.setColour(juce::TabbedComponent::outlineColourId,
                            drs::app::authoring::visual::border);
    workspaceTabs.setColour(juce::TabbedButtonBar::tabOutlineColourId,
                            drs::app::authoring::visual::border);
    workspaceTabs.setColour(juce::TabbedButtonBar::frontOutlineColourId,
                            drs::app::authoring::visual::selection);
    workspaceTabs.setColour(juce::TabbedButtonBar::tabTextColourId,
                            drs::app::authoring::visual::textMuted);
    workspaceTabs.setColour(juce::TabbedButtonBar::frontTextColourId,
                            drs::app::authoring::visual::text);
    workspaceShell.addAndMakeVisible(workspaceTabs);
    workspaceShell.addAndMakeVisible(restoreBanner);
    performancePackageExportProgress.setCancelCallback([this]
    {
        if (performancePackageExportClient.has_value())
            performancePackageExportClient->cancel("Canceled by user");
    });
    performancePackageExportProgress.setVisible(false);
    workspaceShell.addChildComponent(performancePackageExportProgress);
    wavImportProgress.setCancelCallback([this]
    {
        if (wavImportClient.has_value())
            wavImportClient->cancel("Canceled by user");
    });
    workspaceShell.addChildComponent(wavImportProgress);
    sfzImportProgress.setCancelCallback([this]
    {
        if (sfzImportClient.has_value())
            sfzImportClient->cancel("Canceled by user");
    });
    sfzImportProgress.setVisible(false);
    workspaceShell.addAndMakeVisible(sfzImportProgress);
    restoreBanner.update(processor.getProjectRestoreSnapshot());
    synchronizeWorkspacePresentation();

    setSize(drs::app::authoring::compactShellWidth, drs::app::authoring::compactShellHeight);
    refreshProjectViews();
    startTimerHz(4);
}

Editor::~Editor()
{
    stopTimer();
    if (performancePackageExportClient.has_value())
    {
        performancePackageExportClient->cancel("Editor closed");
        performancePackageExportClient->waitForTerminal(std::chrono::seconds(10));
    }
    if (sfzImportClient.has_value())
    {
        sfzImportClient->cancel("Editor closed");
        sfzImportClient->waitForTerminal(std::chrono::seconds(10));
    }
    appProperties.saveIfNeeded();
}

void Editor::paint(juce::Graphics& g)
{
    g.fillAll(drs::app::authoring::visual::shell);
}

void Editor::resized()
{
    synchronizeWorkspacePresentation();
    workspaceShell.setBounds(getLocalBounds());

    auto area = workspaceShell.getLocalBounds();
    auto menuRow = area.removeFromTop(menuRowHeight).reduced(8, 0);

    fileMenuButton.setBounds(menuRow.removeFromLeft(menuButtonWidth).withTrimmedTop(menuButtonYInset)
                                 .withTrimmedBottom(menuButtonYInset));
    menuRow.removeFromLeft(menuButtonSpacing);
    settingsMenuButton.setBounds(menuRow.removeFromLeft(settingsButtonWidth).withTrimmedTop(menuButtonYInset)
                                     .withTrimmedBottom(menuButtonYInset));
    menuRow.removeFromLeft(menuButtonSpacing);
    projectStatusLabel.setBounds(menuRow);
    updateProjectStatusLabel();
    if (performancePackageExportProgress.isVisible())
        performancePackageExportProgress.setBounds(area.removeFromTop(72).reduced(8, 2));
    else
        performancePackageExportProgress.setBounds({});

    if (wavImportProgress.isVisible())
        wavImportProgress.setBounds(area.removeFromTop(58).reduced(8, 2));
    else
        wavImportProgress.setBounds({});

    if (sfzImportProgress.isVisible())
        sfzImportProgress.setBounds(area.removeFromTop(42).reduced(8, 2));
    else
        sfzImportProgress.setBounds({});

    if (restoreBanner.isVisible())
        restoreBanner.setBounds(area.removeFromTop(42));
    else
        restoreBanner.setBounds({});
    workspaceTabs.setBounds(area);
}

void Editor::showFileMenu()
{
    juce::PopupMenu menu;
    const auto authoringAvailable = processor.getWorkspaceDocumentState().authoringAvailable;
    const auto packageSession
        = processor.getWorkspaceDocumentState().kind == drs::engine::WorkspaceDocumentKind::performancePackage;
    menu.addItem(newProjectCommandId, drs::app::newProjectMenuLabel);
    menu.addItem(openProjectCommandId, drs::app::openProjectMenuLabel);
    menu.addItem(openPerformancePackageCommandId, drs::app::openPerformancePackageMenuLabel);
    menu.addItem(closeProjectCommandId, packageSession ? drs::app::closePackageMenuLabel
                                                       : drs::app::closeWorkspaceMenuLabel);
    if (drs::app::shouldShowViewLicenseMenuItem(
            packageSession,
            processor.getEngineFacade().getPerformancePackageLicenseText() != nullptr))
    {
        menu.addSeparator();
        menu.addItem(viewLicenseCommandId, drs::app::viewLicenseMenuLabel);
    }
    if (authoringAvailable)
    {
        menu.addSeparator();
        menu.addItem(saveProjectCommandId, drs::app::saveProjectMenuLabel);
        menu.addItem(saveProjectAsCommandId, drs::app::saveProjectAsMenuLabel);
        menu.addItem(exportPerformancePackageCommandId, drs::app::exportPerformancePackageMenuLabel);
        menu.addItem(importWavCommandId, drs::app::importWavMenuLabel);
        menu.addItem(importSfzCommandId, drs::app::importSfzMenuLabel);
        menu.addItem(importBackgroundImageCommandId, drs::app::importBackgroundImageMenuLabel);
        menu.addItem(importLicenseFileCommandId, drs::app::importLicenseFileMenuLabel);
    }

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&fileMenuButton),
                       [safeThis](int menuItemId)
                       {
                           if (safeThis == nullptr || menuItemId == 0)
                               return;

                           safeThis->handleMenuCommand(menuItemId);
                       });
}

void Editor::showSettingsMenu()
{
    juce::PopupMenu menu;
    menu.addItem(preferencesCommandId, "Preferences...");

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settingsMenuButton),
                       [safeThis](int menuItemId)
                       {
                           if (safeThis == nullptr || menuItemId == 0)
                               return;

                           safeThis->handleMenuCommand(menuItemId);
                       });
}

void Editor::handleMenuCommand(int menuItemId)
{
    switch (menuItemId)
    {
        case newProjectCommandId:
            createNewProject();
            break;
        case openProjectCommandId:
            openProject();
            break;
        case openPerformancePackageCommandId:
            openPerformancePackage();
            break;
        case closeProjectCommandId:
            closeProject();
            break;
        case saveProjectCommandId:
            saveProject({});
            break;
        case saveProjectAsCommandId:
            saveProjectAs({});
            break;
        case exportPerformancePackageCommandId:
            exportPerformancePackage();
            break;
        case importWavCommandId:
            importWavFiles();
            break;
        case importSfzCommandId:
            importSfzFile();
            break;
        case importBackgroundImageCommandId:
            importBackgroundImage();
            break;
        case importLicenseFileCommandId:
            importLicenseFile();
            break;
        case viewLicenseCommandId:
            viewLicense();
            break;
        case preferencesCommandId:
            showPreferencesDialog();
            break;
        default:
            break;
    }
}

void Editor::createNewProject()
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
        return;

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    confirmSafeToDiscardChanges("creating a new project",
                                [safeThis](bool shouldProceed)
                                {
                                    if (!shouldProceed || safeThis == nullptr)
                                        return;

                                    safeThis->launchNewProjectChooser(
                                        [safeThis](juce::File selectedFile)
                                        {
                                            if (safeThis == nullptr || selectedFile == juce::File())
                                                return;

                                            const auto projectFile = drs::app::makeSelfContainedProjectFile(selectedFile);
                                            safeThis->pendingPerformancePackageOpenTask.reset();
                                            safeThis->processor.replaceAuthoringProject(safeThis->buildEmptyProjectTemplate());
                                            safeThis->saveProjectToFile(projectFile);
                                        });
                                });
}

void Editor::openProject()
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    confirmSafeToDiscardChanges("opening another project",
                                [safeThis](bool shouldProceed)
                                {
                                    if (!shouldProceed || safeThis == nullptr)
                                        return;

                                    safeThis->launchOpenProjectChooser(
                                        [safeThis](juce::File selectedFile)
                                        {
                                            if (safeThis == nullptr || selectedFile == juce::File())
                                                return;

                                            safeThis->loadProjectFromFile(selectedFile);
                                        });
                                });
}

void Editor::openPerformancePackage()
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    confirmSafeToDiscardChanges("opening a playable package",
                                [safeThis](bool shouldProceed)
                                {
                                    if (!shouldProceed || safeThis == nullptr)
                                        return;

                                    safeThis->launchOpenPerformancePackageChooser(
                                        [safeThis](juce::File selectedFile)
                                        {
                                            if (safeThis == nullptr || selectedFile == juce::File())
                                                return;

                                            safeThis->loadPerformancePackageFromFile(selectedFile);
                                        });
                                });
}

void Editor::closeProject()
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    const auto nextAction
        = processor.getWorkspaceDocumentState().kind == drs::engine::WorkspaceDocumentKind::performancePackage
        ? juce::String("closing the current package")
        : juce::String("closing the current project");
    confirmSafeToDiscardChanges(nextAction,
                                [safeThis](bool shouldProceed)
                                {
                                    if (!shouldProceed || safeThis == nullptr)
                                        return;

                                    safeThis->pendingPerformancePackageOpenTask.reset();
                                    if (safeThis->processor.getWorkspaceDocumentState().kind
                                        == drs::engine::WorkspaceDocumentKind::performancePackage)
                                    {
                                        safeThis->processor.closePerformancePackageWorkspace(
                                            safeThis->buildUnloadedProjectState());
                                    }
                                    else
                                    {
                                        safeThis->processor.closeAuthoringProject(
                                            safeThis->buildUnloadedProjectState());
                                    }
                                    safeThis->refreshProjectViews();
                                });
}

void Editor::saveProject(std::function<void(bool)> completion)
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
    {
        if (completion)
            completion(false);
        return;
    }

    if (processor.getAuthoringProjectFile() == juce::File())
    {
        saveProjectAs(std::move(completion));
        return;
    }

    const auto saved = saveProjectToFile(processor.getAuthoringProjectFile());
    if (completion)
        completion(saved);
}

void Editor::saveProjectAs(std::function<void(bool)> completion)
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
    {
        if (completion)
            completion(false);
        return;
    }

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    launchSaveProjectChooser(
        [safeThis, completion = std::move(completion)](juce::File selectedFile) mutable
        {
            if (safeThis == nullptr)
                return;

            if (selectedFile == juce::File())
            {
                if (completion)
                    completion(false);
                return;
            }

            const auto targetFile = safeThis->processor.getAuthoringProjectFile() == juce::File()
                ? drs::app::makeSelfContainedProjectFile(selectedFile)
                : selectedFile;
            const auto saved = safeThis->saveProjectToFile(targetFile);
            if (completion)
                completion(saved);
        });
}

void Editor::exportPerformancePackage()
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
        return;

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    launchExportPerformancePackageChooser([safeThis](juce::File selectedFile)
    {
        if (safeThis == nullptr || selectedFile == juce::File())
            return;

        const auto& session = safeThis->processor.getAuthoringSession();
        auto request = drs::app::PerformancePackageExportRequest {};
        request.project = session.getProject();
        request.sessionState = safeThis->processor.getEngineFacade().getCurrentSessionState();
        request.projectId = safeThis->processor.getAuthoringProjectFile() != juce::File()
            ? safeThis->processor.getAuthoringProjectFile().getFullPathName().toStdString()
            : session.getProject().displayName;
        request.baseRevision = session.getDocumentState().revision;
        request.packagePath = selectedFile.getFullPathName().toStdString();
        safeThis->performancePackageExportClient.emplace(
            safeThis->processor.getPerformancePackageExportService().openClient());
        const auto submitted = safeThis->performancePackageExportClient->submit(std::move(request));
        if (submitted.disposition == drs::app::PerformancePackageExportSubmitDisposition::accepted)
            return;

        safeThis->performancePackageExportClient.reset();
        if (safeThis->performancePackageExportProgress.isVisible())
        {
            safeThis->performancePackageExportProgress.setVisible(false);
            safeThis->resized();
        }
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Export Playable Instrument Failed",
            submitted.disposition == drs::app::PerformancePackageExportSubmitDisposition::busy
                ? "A playable package export is already in progress."
                : "Playable package export could not be started.");
    });
}

void Editor::pollPerformancePackageExportService()
{
    if (!performancePackageExportClient.has_value())
        return;

    const auto snapshot = performancePackageExportClient->getSnapshot();
    if (!snapshot)
        return;

    const auto progressWasVisible = performancePackageExportProgress.isVisible();
    performancePackageExportProgress.update(*snapshot);
    if (progressWasVisible != performancePackageExportProgress.isVisible())
        resized();

    const auto clearState = [this]
    {
        const auto progressVisible = performancePackageExportProgress.isVisible();
        performancePackageExportClient.reset();
        performancePackageExportProgress.setVisible(false);
        if (progressVisible)
            resized();
    };

    if (snapshot->stage == drs::app::PerformancePackageExportStage::failed)
    {
        const auto message = snapshot->result != nullptr
            ? buildProjectIssueSummary(snapshot->result->issues)
            : juce::String(snapshot->detail);
        performancePackageExportClient->consume();
        clearState();
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Export Playable Instrument Failed",
                                               message);
        return;
    }

    if (snapshot->stage == drs::app::PerformancePackageExportStage::canceled)
    {
        performancePackageExportClient->consume();
        clearState();
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                               "Export Playable Instrument Canceled",
                                               juce::String(snapshot->detail.empty()
                                                                ? snapshot->status
                                                                : snapshot->detail));
        return;
    }

    if (snapshot->stage == drs::app::PerformancePackageExportStage::consumed)
    {
        clearState();
        return;
    }

    if (snapshot->stage != drs::app::PerformancePackageExportStage::completed || snapshot->result == nullptr)
        return;

    const auto exportResult = snapshot->result;
    performancePackageExportClient->consume();
    clearState();
    setRecentProjectDirectory(juce::File(exportResult->packagePath).getParentDirectory());
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "Playable Instrument Exported",
        "Exported playable package:\n"
            + juce::String::fromUTF8(exportResult->packagePath.c_str())
            + "\n\nPackage size: "
            + juce::File::descriptionOfSizeInBytes(static_cast<int64_t>(exportResult->packageBytes))
            + "\nPayload count: " + juce::String(static_cast<int>(exportResult->payloadCount)));
}

void Editor::importWavFiles()
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
        return;

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    launchImportWavChooser([safeThis](std::vector<juce::File> selectedFiles)
    {
        if (safeThis != nullptr && !selectedFiles.empty())
            safeThis->importSampleFiles(std::move(selectedFiles));
    });
}

void Editor::importSfzFile()
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
        return;

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    launchImportSfzChooser([safeThis](juce::File selectedFile)
    {
        if (safeThis != nullptr && selectedFile != juce::File())
            safeThis->reviewSfzImportFile(selectedFile);
    });
}

void Editor::importBackgroundImage()
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
        return;

    if (processor.getAuthoringProjectFile() == juce::File())
    {
        auto safeThis = juce::Component::SafePointer<Editor>(this);
        saveProjectAs([safeThis](bool saved)
        {
            if (saved && safeThis != nullptr)
                safeThis->importBackgroundImage();
        });
        return;
    }

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    launchImportBackgroundImageChooser([safeThis](juce::File selectedFile)
    {
        if (safeThis == nullptr || selectedFile == juce::File())
            return;

        const auto importResult = drs::app::importProjectBackgroundImage(
            selectedFile, safeThis->processor.getAuthoringProjectFile());
        if (!importResult.imported)
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Import Background Image Failed",
                                                   importResult.errorMessage);
            return;
        }

        safeThis->performancePanel.refreshArtworkNow();
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                               "Background Image Imported",
                                               "Imported background image to:\n"
                                                   + importResult.targetFile.getFullPathName());
    });
}

void Editor::importLicenseFile()
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
        return;

    if (processor.getAuthoringProjectFile() == juce::File())
    {
        auto safeThis = juce::Component::SafePointer<Editor>(this);
        saveProjectAs([safeThis](bool saved)
        {
            if (saved && safeThis != nullptr)
                safeThis->importLicenseFile();
        });
        return;
    }

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    launchImportLicenseFileChooser([safeThis](juce::File selectedFile)
    {
        if (safeThis == nullptr || selectedFile == juce::File())
            return;

        const auto projectFile = safeThis->processor.getAuthoringProjectFile();
        const auto targetFile = drs::app::getProjectLicenseFile(projectFile);
        auto performImport = [safeThis, selectedFile, projectFile]()
        {
            if (safeThis == nullptr)
                return;

            const auto importResult = drs::app::importProjectLicenseFile(selectedFile, projectFile);
            if (!importResult.imported)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Import License File Failed",
                                                       importResult.errorMessage);
                return;
            }

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                   "License File Imported",
                                                   "Imported license file to:\n"
                                                       + importResult.targetFile.getFullPathName());
        };

        if (targetFile.existsAsFile() && selectedFile != targetFile)
        {
            const auto options = juce::MessageBoxOptions::makeOptionsOkCancel(
                juce::MessageBoxIconType::WarningIcon,
                "Replace License File?",
                "This project already contains LICENSE.txt. Replace it with the selected file?",
                "Replace",
                "Cancel",
                safeThis.getComponent());
            juce::AlertWindow::showAsync(options,
                                         [performImport = std::move(performImport)](int result) mutable
                                         {
                                             if (result == saveButtonResult)
                                                 performImport();
                                         });
            return;
        }

        performImport();
    });
}

void Editor::viewLicense()
{
    if (processor.getWorkspaceDocumentState().kind
        != drs::engine::WorkspaceDocumentKind::performancePackage)
    {
        return;
    }

    const auto licenseText = processor.getEngineFacade().getPerformancePackageLicenseText();
    if (licenseText != nullptr)
        drs::app::showPlayableInstrumentLicenseViewerDialog(this, licenseText);
}

void Editor::importSampleFiles(std::vector<juce::File> selectedFiles)
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    auto beginImport = [safeThis, selectedFiles = std::move(selectedFiles)](bool ready) mutable
    {
        if (!ready || safeThis == nullptr || selectedFiles.empty())
            return;

        std::vector<std::string> selectedPaths;
        selectedPaths.reserve(selectedFiles.size());
        for (const auto& sourceFile : selectedFiles)
            selectedPaths.push_back(sourceFile.getFullPathName().toStdString());

        const auto& session = safeThis->processor.getAuthoringSession();
        safeThis->wavImportProjectId = safeThis->processor.getAuthoringProjectFile() != juce::File()
            ? safeThis->processor.getAuthoringProjectFile().getFullPathName().toStdString()
            : session.getProject().displayName;
        safeThis->wavImportBaseRevision = session.getDocumentState().revision;
        safeThis->wavImportContentRootPath = session.getProject().contentRootPath;
        safeThis->wavImportSelectedGroupId.clear();
        safeThis->wavImportPreparedBatch.reset();
        safeThis->wavImportManualRootDialogOpen = false;
        safeThis->wavImportClient.emplace(safeThis->processor.getWavImportService().openClient());

        drs::app::WavImportRequest request;
        request.sourcePaths = std::move(selectedPaths);
        request.projectId = safeThis->wavImportProjectId;
        request.baseRevision = safeThis->wavImportBaseRevision;
        request.contentRootPath = safeThis->wavImportContentRootPath;
        if (const auto selectedGroup = session.getSelectedGroup(); selectedGroup.has_value())
        {
            request.selectedGroupId = selectedGroup->id;
            safeThis->wavImportSelectedGroupId = selectedGroup->id;
        }

        const auto submitted = safeThis->wavImportClient->submit(std::move(request));
        if (submitted.disposition != drs::app::WavImportSubmitDisposition::accepted)
        {
            const auto progressWasVisible = safeThis->wavImportProgress.isVisible();
            safeThis->wavImportClient.reset();
            safeThis->wavImportProjectId.clear();
            safeThis->wavImportBaseRevision = 0;
            safeThis->wavImportContentRootPath.clear();
            safeThis->wavImportSelectedGroupId.clear();
            safeThis->wavImportOwnerId = 0;
            safeThis->wavImportGeneration = 0;
            safeThis->wavImportProgress.setVisible(false);
            if (progressWasVisible)
                safeThis->resized();
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Import WAV",
                submitted.disposition == drs::app::WavImportSubmitDisposition::busy
                    ? "A WAV import is already in progress."
                    : "WAV import could not be started.");
            return;
        }

        safeThis->wavImportOwnerId = safeThis->wavImportClient->ownerId();
        safeThis->wavImportGeneration = submitted.identity.generation;
    };

    if (processor.getAuthoringProjectFile() == juce::File())
    {
        saveProjectAs(std::move(beginImport));
        return;
    }

    beginImport(true);
}

void Editor::reviewSfzImportFile(const juce::File& selectedFile)
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    auto beginReview = [safeThis, selectedFile](bool ready) mutable
    {
        if (!ready || safeThis == nullptr || selectedFile == juce::File())
            return;

        const auto& session = safeThis->processor.getAuthoringSession();
        safeThis->sfzImportProjectId = safeThis->processor.getAuthoringProjectFile() != juce::File()
            ? safeThis->processor.getAuthoringProjectFile().getFullPathName().toStdString()
            : session.getProject().displayName;
        safeThis->sfzImportBaseRevision = session.getDocumentState().revision;
        safeThis->sfzImportClient.emplace(safeThis->processor.getSfzImportReviewService().openClient());
        auto request = drs::app::SfzImportReviewRequest { session.getProject(),
                                                          selectedFile.getFullPathName().toStdString(),
                                                          safeThis->sfzImportProjectId,
                                                          safeThis->sfzImportBaseRevision };
        const auto submitted = safeThis->sfzImportClient->submit(std::move(request));
        if (submitted.disposition != drs::app::SfzImportReviewSubmitDisposition::accepted)
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Import SFZ",
                                                   submitted.disposition == drs::app::SfzImportReviewSubmitDisposition::busy
                                                       ? "An SFZ import is already in progress."
                                                       : "SFZ import could not be started.");
    };

    if (processor.getAuthoringProjectFile() == juce::File())
    {
        saveProjectAs(std::move(beginReview));
        return;
    }

    beginReview(true);
}

void Editor::pollSfzImportReviewService()
{
    if (!sfzImportClient.has_value() || sfzImportReviewDialogOpen)
        return;
    const auto snapshot = sfzImportClient->getSnapshot();
    if (!snapshot)
        return;
    sfzImportProgress.update(*snapshot);
    if (snapshot->stage == drs::app::SfzImportReviewServiceStage::failed
        || snapshot->stage == drs::app::SfzImportReviewServiceStage::canceled)
    {
        if (snapshot->stage == drs::app::SfzImportReviewServiceStage::failed)
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Import SFZ Failed",
                                                   juce::String(snapshot->status));
        sfzImportClient->consume();
        return;
    }
    if (snapshot->stage != drs::app::SfzImportReviewServiceStage::reviewReady || !snapshot->result)
        return;

    const auto currentProjectId = processor.getAuthoringProjectFile() != juce::File()
        ? processor.getAuthoringProjectFile().getFullPathName().toStdString()
        : processor.getAuthoringSession().getProject().displayName;
    if (currentProjectId != sfzImportProjectId
        || processor.getAuthoringSession().getDocumentState().revision != sfzImportBaseRevision)
    {
        sfzImportClient->cancel("Project changed while SFZ import was preparing");
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Import SFZ",
                                               "The project changed while the import was preparing. Please retry.");
        return;
    }

    sfzImportReviewDialogOpen = true;
    auto reviewState = snapshot->result;
    drs::app::showSfzImportReviewDialog(
        this,
        *reviewState,
        [safeThis = juce::Component::SafePointer<Editor>(this), reviewState](bool accepted) mutable
        {
            if (safeThis == nullptr)
                return;
            safeThis->sfzImportReviewDialogOpen = false;
            if (!accepted)
            {
                safeThis->sfzImportClient->consume();
                return;
            }
            const auto applyProjectId = safeThis->processor.getAuthoringProjectFile() != juce::File()
                ? safeThis->processor.getAuthoringProjectFile().getFullPathName().toStdString()
                : safeThis->processor.getAuthoringSession().getProject().displayName;
            if (applyProjectId != safeThis->sfzImportProjectId
                || safeThis->processor.getAuthoringSession().getDocumentState().revision
                    != safeThis->sfzImportBaseRevision)
            {
                safeThis->sfzImportClient->consume();
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Import SFZ",
                                                       "The project changed before apply. Please retry.");
                return;
            }
            const auto appliedSummary = drs::app::buildSfzImportAppliedSummary(*reviewState);
            const auto importResult = drs::engine::applySfzImportProjection(
                safeThis->processor.getAuthoringSession(),
                reviewState->projection,
                "Import SFZ document");
            safeThis->sfzImportClient->consume();
            if (!importResult.applied)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Import SFZ Failed",
                                                       safeThis->buildProjectIssueSummary(importResult.issues));
                return;
            }
            safeThis->refreshProjectViews();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                   "Import SFZ Complete",
                                                   appliedSummary);
        });
}

void Editor::showPreferencesDialog()
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Preferences";
    options.dialogBackgroundColour = juce::Colour::fromRGB(244, 240, 232);
    options.content.setOwned(new PreferencesComponent(getLibraryLocation(),
                                                      getProjectDirectory(),
                                                      [safeThis = juce::Component::SafePointer<Editor>(this)](juce::File libraryFolder,
                                                                                                              juce::File projectFolder)
                                                      {
                                                          if (safeThis == nullptr)
                                                              return;

                                                          safeThis->setLibraryLocation(libraryFolder);
                                                          safeThis->setProjectDirectory(projectFolder);
                                                      }));
    options.componentToCentreAround = this;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.useBottomRightCornerResizer = false;
    options.launchAsync();
}

void Editor::restoreSelectedZoneRootKey()
{
    auto& authoringSession = processor.getAuthoringSession();
    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
        return;

    const auto* sampleSource = findSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (sampleSource == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Restore Root Key Failed",
                                               "The selected zone's sample source could not be found in the current project.");
        return;
    }

    const auto inspectionResult = drs::engine::inspectSampleFile(sampleSource->path);
    if (!inspectionResult.accepted)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Restore Root Key Failed",
                                               buildProjectIssueSummary(inspectionResult.issues));
        return;
    }

    const auto inference = drs::engine::inferSampleRootKey(sampleSource->path, &inspectionResult.metadata);
    auto applyRestoredRootKey = [this, selectedZone](int restoredRootKey)
    {
        if (restoredRootKey == selectedZone->rootKey)
            return;

        auto updatedZone = *selectedZone;
        updatedZone.rootKey = restoredRootKey;

        const auto updateResult = processor.getAuthoringSession().updateSelectedZone(updatedZone, "Restore zone root key");
        if (!updateResult.applied)
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Restore Root Key Failed",
                                                   buildProjectIssueSummary(updateResult.issues));
            return;
        }

        refreshProjectViews();
    };

    if (inference.resolved)
    {
        applyRestoredRootKey(inference.rootKey);
        return;
    }

    promptForRootKeySelection(
        "Restore Root Key",
        "Could not infer a root key for '" + juce::String::fromUTF8(selectedZone->displayName.c_str())
            + "'. Select the sample's native pitch to restore the mapping value.",
        selectedZone->rootKey,
        [applyRestoredRootKey](std::optional<int> selectedRootKey)
        {
            if (!selectedRootKey.has_value())
                return;

            applyRestoredRootKey(*selectedRootKey);
        });
}

bool Editor::saveProjectToFile(const juce::File& file)
{
    const auto targetFile = drs::app::ensureProjectFileExtension(file);
    auto project = processor.getAuthoringSession().getProject();
    const auto savingUnsavedProject = processor.getAuthoringProjectFile() == juce::File();
    const auto targetDirectory = targetFile.getParentDirectory();
    const auto targetInstrumentFile = targetFile.withFileExtension(".drinst");

    if (savingUnsavedProject || project.contentRootPath.empty())
        project.contentRootPath = targetDirectory.getFullPathName().toStdString();

    project.defaultInstrumentManifestPath = targetInstrumentFile.getFullPathName().toStdString();

    if (project.displayName.empty() || project.displayName == "No Project Loaded"
        || (savingUnsavedProject && project.displayName == "Untitled Project"))
        project.displayName = targetFile.getFileNameWithoutExtension().toStdString();

    const auto saveResult = drs::app::saveProjectFiles(project, targetFile);
    if (!saveResult.saved)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Save Project Failed",
                                               saveResult.errorMessage);
        return false;
    }

    const auto bindingAccepted = savingUnsavedProject
        ? processor.replaceAuthoringProject(project, targetFile)
        : processor.bindAuthoringProjectFile(targetFile);
    if (!bindingAccepted)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Save Project Failed",
            "The project files were written, but the saved manifest did not match the authored project. "
            "The current project binding was left unchanged.");
        return false;
    }

    setRecentProjectDirectory(targetFile.getParentDirectory());
    processor.getAuthoringSession().markSaved();
    refreshProjectViews();
    return true;
}

bool Editor::loadProjectFromFile(const juce::File& file)
{
    pendingPerformancePackageOpenTask.reset();

    const auto targetFile = drs::app::ensureProjectFileExtension(file);
    const auto recovery = drs::app::recoverProjectFilesTransaction(targetFile);
    if (recovery.recoveryNeeded && !recovery.recovered)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Project Recovery Failed",
                                               recovery.errorMessage);
        return false;
    }

    const auto loadResult = drs::engine::loadRuntimeProjectManifest(targetFile.getFullPathName().toStdString());
    if (!loadResult.loaded)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Open Project Failed",
                                               buildProjectIssueSummary(loadResult.issues));
        return false;
    }

    std::vector<std::string> migrationIssues;
    const auto upgradedProject = upgradeLoadedProjectToLatestSchema(loadResult.project, migrationIssues);
    if (!upgradedProject.has_value())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Open Project Failed",
                                               buildProjectIssueSummary(migrationIssues));
        return false;
    }

    const auto projectWasMigrated = upgradedProject->schemaVersion != loadResult.project.schemaVersion
        || upgradedProject->authoring.schemaVersion != loadResult.project.authoring.schemaVersion;
    const auto projectInstalled = projectWasMigrated
        ? processor.replaceAuthoringProject(loadResult.project, targetFile)
            && processor.applyAuthoringProjectMigration(*upgradedProject)
        : processor.replaceAuthoringProject(*upgradedProject, targetFile);
    if (!projectInstalled)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Open Project Failed",
            "The manifest was loaded, but its authored project or schema migration could not be activated.");
        return false;
    }

    setRecentProjectDirectory(targetFile.getParentDirectory());
    refreshProjectViews();
    return true;
}

bool Editor::loadPerformancePackageFromFile(const juce::File& file)
{
    if (file == juce::File())
        return false;

    auto task = PendingPerformancePackageOpenTask {};
    task.file = file;
    task.ready = std::make_shared<std::atomic<bool>>(false);
    task.result = std::make_shared<drs::plugin::PreparedPerformancePackageWorkspaceLoadResult>();
    task.generation = ++nextPerformancePackageOpenGeneration;

    const auto packagePath = file.getFullPathName().toStdString();
    const auto ready = task.ready;
    const auto result = task.result;
    pendingPerformancePackageOpenTask = task;
    std::thread([packagePath, ready, result]()
    {
        *result = drs::plugin::preparePerformancePackageWorkspaceInBackground(packagePath);
        ready->store(true, std::memory_order_release);
    }).detach();
    updateProjectStatusLabel();
    return true;
}

void Editor::pollPerformancePackageOpenTask()
{
    if (!pendingPerformancePackageOpenTask.has_value()
        || pendingPerformancePackageOpenTask->ready == nullptr
        || !pendingPerformancePackageOpenTask->ready->load(std::memory_order_acquire)
        || pendingPerformancePackageOpenTask->result == nullptr)
    {
        return;
    }

    auto task = std::move(*pendingPerformancePackageOpenTask);
    pendingPerformancePackageOpenTask.reset();
    auto prepared = std::move(*task.result);
    auto timingSummary = buildPerformancePackageTimingSummary(prepared.timings);

    if (!prepared.prepared)
    {
        auto issues = prepared.issues;
        if (!timingSummary.empty())
        {
            juce::Logger::writeToLog("Playable package open failed during background preparation. "
                                     + juce::String(timingSummary));
            issues.push_back(timingSummary);
        }
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Open Playable Package Failed",
                                               buildProjectIssueSummary(issues));
        updateProjectStatusLabel();
        return;
    }

    const auto loadResult = processor.activatePreparedPerformancePackageWorkspace(
        std::move(prepared.activation),
        task.file);
    if (!loadResult.loaded)
    {
        auto issues = loadResult.issues;
        if (!timingSummary.empty())
        {
            juce::Logger::writeToLog("Playable package open failed during activation. "
                                     + juce::String(timingSummary));
            issues.push_back(timingSummary);
        }
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Open Playable Package Failed",
                                               buildProjectIssueSummary(issues));
        updateProjectStatusLabel();
        return;
    }

    if (!timingSummary.empty())
        juce::Logger::writeToLog("Playable package opened in the background. " + juce::String(timingSummary));
    setRecentProjectDirectory(task.file.getParentDirectory());
    refreshProjectViews();
}

void Editor::confirmSafeToDiscardChanges(const juce::String& nextAction,
                                         std::function<void(bool)> completion)
{
    if (!processor.getWorkspaceDocumentState().dirty)
    {
        if (completion)
            completion(true);
        return;
    }

    auto safeThis = juce::Component::SafePointer<Editor>(this);
    const auto options = juce::MessageBoxOptions::makeOptionsYesNoCancel(juce::MessageBoxIconType::WarningIcon,
                                                                         "Unsaved Changes",
                                                                         "Save changes before " + nextAction + "?",
                                                                         "Save",
                                                                         "Discard",
                                                                         "Cancel",
                                                                         this);
    juce::AlertWindow::showAsync(options,
                                 [safeThis, completion = std::move(completion)](int result) mutable
                                 {
                                     if (safeThis == nullptr)
                                         return;

                                     if (result == saveButtonResult)
                                     {
                                         safeThis->saveProject(std::move(completion));
                                         return;
                                     }

                                     if (completion)
                                         completion(result == discardButtonResult);
                                 });
}

void Editor::timerCallback()
{
    const drs::app::ScopedMessageThreadSpan timing(
        drs::app::MessageThreadSpanKind::editorTimerWork);
    {
        const drs::app::ScopedMessageThreadSpan section(
            drs::app::MessageThreadSpanKind::editorServiceWork);
        processor.serviceMessageThreadWork();
    }
    {
        const drs::app::ScopedMessageThreadSpan section(
            drs::app::MessageThreadSpanKind::editorRestoreWork);
        if (restoreBanner.update(processor.getProjectRestoreSnapshot()))
            resized();
    }
    {
        const drs::app::ScopedMessageThreadSpan section(
            drs::app::MessageThreadSpanKind::editorPerformanceWork);
        performancePanel.refreshNow();
    }
    {
        const drs::app::ScopedMessageThreadSpan section(
            drs::app::MessageThreadSpanKind::editorAuthoringWork);
        if (processor.getWorkspaceDocumentState().authoringAvailable)
            authoringPanel.refreshNow();
    }
    {
        const drs::app::ScopedMessageThreadSpan section(
            drs::app::MessageThreadSpanKind::editorPackageOpenWork);
        pollPerformancePackageOpenTask();
    }
    {
        const drs::app::ScopedMessageThreadSpan section(
            drs::app::MessageThreadSpanKind::editorStatusWork);
        updateProjectStatusLabel();
    }
    {
        const drs::app::ScopedMessageThreadSpan section(
            drs::app::MessageThreadSpanKind::editorExportWork);
        pollPerformancePackageExportService();
    }
    {
        const drs::app::ScopedMessageThreadSpan section(
            drs::app::MessageThreadSpanKind::editorWavImportWork);
        pollWavImportService();
    }
    {
        const drs::app::ScopedMessageThreadSpan section(
            drs::app::MessageThreadSpanKind::editorSfzImportWork);
        pollSfzImportReviewService();
    }
}

void Editor::pollWavImportService()
{
    if (!wavImportClient.has_value())
        return;

    const auto snapshot = wavImportClient->getSnapshot();
    if (!snapshot)
        return;

    const auto progressWasVisible = wavImportProgress.isVisible();
    wavImportProgress.update(*snapshot);
    if (progressWasVisible != wavImportProgress.isVisible())
        resized();

    const auto clearState = [this]
    {
        const auto progressVisible = wavImportProgress.isVisible();
        wavImportPreparedBatch.reset();
        wavImportManualRootDialogOpen = false;
        wavImportClient.reset();
        wavImportProjectId.clear();
        wavImportBaseRevision = 0;
        wavImportContentRootPath.clear();
        wavImportSelectedGroupId.clear();
        wavImportOwnerId = 0;
        wavImportGeneration = 0;
        wavImportProgress.setVisible(false);
        if (progressVisible)
            resized();
    };

    if (snapshot->stage == drs::app::WavImportBatchStage::failed)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Import WAV Failed",
                                               juce::String(snapshot->status));
        clearState();
        return;
    }

    if (snapshot->stage == drs::app::WavImportBatchStage::canceled)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                               "Import WAV Canceled",
                                               juce::String(snapshot->status));
        clearState();
        return;
    }

    if (snapshot->stage == drs::app::WavImportBatchStage::superseded
        || snapshot->stage == drs::app::WavImportBatchStage::consumed)
    {
        clearState();
        return;
    }

    if (snapshot->stage != drs::app::WavImportBatchStage::completed || snapshot->completion == nullptr)
        return;

    const auto currentSelectedGroup = processor.getAuthoringSession().getSelectedGroup();
    const auto currentSelectedGroupId = currentSelectedGroup.has_value() ? currentSelectedGroup->id : std::string {};
    const auto currentProjectId = processor.getAuthoringProjectFile() != juce::File()
        ? processor.getAuthoringProjectFile().getFullPathName().toStdString()
        : processor.getAuthoringSession().getProject().displayName;
    const auto currentContentRootPath = processor.getAuthoringSession().getProject().contentRootPath;
    if (snapshot->identity.ownerId != wavImportOwnerId
        || snapshot->identity.generation != wavImportGeneration
        || snapshot->identity.projectId != wavImportProjectId
        || snapshot->identity.baseRevision != wavImportBaseRevision
        || snapshot->identity.contentRootPath != wavImportContentRootPath
        || snapshot->identity.selectedGroupId != wavImportSelectedGroupId
        || currentProjectId != wavImportProjectId
        || processor.getAuthoringSession().getDocumentState().revision != wavImportBaseRevision
        || currentContentRootPath != wavImportContentRootPath
        || currentSelectedGroupId != wavImportSelectedGroupId)
    {
        wavImportClient->consume();
        clearState();
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Import WAV",
                                               "The project, group, or destination changed before apply. Please retry.");
        return;
    }

    if (wavImportPreparedBatch == nullptr)
    {
        wavImportPreparedBatch = std::make_shared<drs::app::PreparedWavImportBatch>(
            drs::app::prepareWavImportBatchFromCompletion(*snapshot->completion,
                                                          processor.getAuthoringSession().getProject(),
                                                          snapshot->completion->identity.selectedGroupId));
    }

    if (wavImportPreparedBatch->pendingManualRoot.has_value())
    {
        if (wavImportManualRootDialogOpen)
            return;

        const auto prompt = *wavImportPreparedBatch->pendingManualRoot;
        wavImportManualRootDialogOpen = true;
        promptForRootKeySelection(
            "Root Key Required",
            "Could not infer a root key for '" + juce::String::fromUTF8(prompt.sourceDisplayName.c_str())
                + "'. Select the sample's native pitch to continue importing.",
            prompt.initialRootKey,
            [safeThis = juce::Component::SafePointer<Editor>(this)](std::optional<int> selectedRootKey) mutable
            {
                if (safeThis == nullptr)
                    return;

                if (safeThis->wavImportPreparedBatch != nullptr)
                    drs::app::resolvePreparedWavImportManualRoot(*safeThis->wavImportPreparedBatch,
                                                                 selectedRootKey);
                safeThis->wavImportManualRootDialogOpen = false;
            });
        return;
    }

    if (!drs::app::hasPreparedWavImportCommit(*wavImportPreparedBatch))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Import WAV Failed",
                                               buildImportSummaryMessage(0,
                                                                         wavImportPreparedBatch->warningCount,
                                                                         wavImportPreparedBatch->skippedCount,
                                                                         wavImportPreparedBatch->details));
        wavImportClient->consume();
        clearState();
        return;
    }

    auto commit = drs::app::takePreparedWavImportCommit(std::move(*wavImportPreparedBatch));
    wavImportPreparedBatch.reset();
    std::vector<std::string> finalizationIssues;
    if (!drs::app::finalizePreparedWavImportCommit(commit, finalizationIssues))
    {
        wavImportClient->consume();
        clearState();
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Import WAV Failed",
                                               buildProjectIssueSummary(finalizationIssues));
        return;
    }

    const auto importedCount = commit.importedCount;
    const auto importResult = processor.getAuthoringSession().appendImportedContent(
        std::move(commit.sampleSources),
        std::move(commit.zones),
        "Import WAV files");
    if (!importResult.applied)
    {
        drs::app::rollbackPreparedWavImportCommit(commit);
        wavImportClient->consume();
        clearState();
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Import WAV Failed",
                                               buildProjectIssueSummary(importResult.issues));
        return;
    }

    wavImportClient->consume();
    clearState();
    refreshProjectViews();
    const auto partialResult = snapshot->terminalDisposition == drs::app::WavImportTerminalDisposition::partiallyCompleted
        || commit.skippedCount > 0;
    juce::AlertWindow::showMessageBoxAsync(partialResult ? juce::AlertWindow::WarningIcon
                                                         : juce::AlertWindow::InfoIcon,
                                           partialResult ? "Import WAV Partially Complete"
                                                         : "Import WAV Complete",
                                           buildImportSummaryMessage(importedCount,
                                                                     commit.warningCount,
                                                                     commit.skippedCount,
                                                                     commit.details));
}

void Editor::locateProjectForRestore()
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    launchOpenProjectChooser(
        [safeThis](juce::File selectedFile)
        {
            if (safeThis == nullptr || selectedFile == juce::File())
                return;
            safeThis->processor.retryProjectRestoreWithFile(selectedFile);
        });
}

void Editor::refreshProjectViews()
{
    if (processor.getWorkspaceDocumentState().authoringAvailable)
        authoringPanel.reloadFromSession();
    synchronizeWorkspacePresentation();
    updateProjectStatusLabel();
}

void Editor::synchronizeWorkspacePresentation()
{
    const auto performanceOnly
        = processor.getWorkspaceDocumentState().workspaceMode == drs::engine::WorkspaceMode::performanceOnly;
    const auto expectedTabCount = performanceOnly ? 1 : 2;
    if (workspaceTabs.getNumTabs() == expectedTabCount)
        return;

    const auto previousIndex = workspaceTabs.getCurrentTabIndex();
    workspaceTabs.clearTabs();
    workspaceTabs.addTab("Perform", performTabColour, &performancePanel, false);
    if (!performanceOnly)
        workspaceTabs.addTab("Map", mapTabColour, &authoringPanel, false);

    workspaceTabs.setCurrentTabIndex(std::clamp(previousIndex, 0, workspaceTabs.getNumTabs() - 1));
}

void Editor::updateProjectStatusLabel()
{
    auto statusText = buildWorkspaceStatusText();
    if (processor.getWorkspaceDocumentState().dirty)
        statusText += " *";

    projectStatusLabel.setText(statusText, juce::dontSendNotification);
    projectStatusLabel.setTooltip(buildWorkspaceStatusTooltip());
}

juce::String Editor::buildWorkspaceDisplayName() const
{
    const auto& document = processor.getWorkspaceDocumentState();
    if (!document.displayName.empty())
        return juce::String::fromUTF8(document.displayName.c_str());

    return "No Project Loaded";
}

juce::String Editor::buildWorkspaceStatusText() const
{
    const auto& document = processor.getWorkspaceDocumentState();
    auto text = buildWorkspaceDisplayName();

    if (pendingPerformancePackageOpenTask.has_value())
        text += " | Opening playable package...";

    if (document.kind == drs::engine::WorkspaceDocumentKind::performancePackage)
    {
        text += " | " + juce::String::fromUTF8(
            drs::engine::packageWorkspaceStatusText(document.readiness).c_str());
        text += " | Read-only | Reader v";
        text += juce::String(document.minimumReaderSchemaVersion);
    }

    return text;
}

juce::String Editor::buildWorkspaceStatusTooltip() const
{
    if (pendingPerformancePackageOpenTask.has_value())
    {
        auto tooltip = juce::String("Opening playable package in the background.");
        if (pendingPerformancePackageOpenTask->file != juce::File())
        {
            tooltip += "\nSource: "
                + pendingPerformancePackageOpenTask->file.getFullPathName();
        }
        return tooltip;
    }

    const auto& document = processor.getWorkspaceDocumentState();
    if (document.kind != drs::engine::WorkspaceDocumentKind::performancePackage)
        return "Editable authoring workspace.";

    return juce::String::fromUTF8(drs::engine::packageWorkspaceStatusTooltip(document).c_str());
}

drs::engine::RuntimeProjectModel Editor::buildUnloadedProjectState() const
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = drs::engine::layerContractProjectSchemaVersion;
    project.displayName = "No Project Loaded";
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = drs::engine::layerContractAuthoringSchemaVersion;
    project.authoring.articulations = { { "default", "Default", true, 0, std::nullopt } };
    project.authoring.notes = { "Open a project or create a new one to begin authoring." };
    project.notes = { "This session starts without loading the checked-in reference project." };
    return project;
}

drs::engine::RuntimeProjectModel Editor::buildEmptyProjectTemplate() const
{
    const auto defaultProjectDirectory = buildChooserBaseDirectory();
    const auto defaultInstrumentFile = defaultProjectDirectory.getChildFile("Untitled Project.drinst");

    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = drs::engine::layerContractProjectSchemaVersion;
    project.projectId = makeProjectId();
    project.displayName = "Untitled Project";
    project.contentRootPath = defaultProjectDirectory.getFullPathName().toStdString();
    project.defaultInstrumentManifestPath = defaultInstrumentFile.getFullPathName().toStdString();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = drs::engine::layerContractAuthoringSchemaVersion;
    project.authoring.articulations = { { "default", "Default", true, 0, std::nullopt } };
    project.authoring.notes = { "Created in the plug-in authoring shell." };
    project.notes = {
        "Created as a new curated DSP authoring project from the plug-in shell.",
        "Sample sources and zones can be added in later authoring sprints."
    };
    return project;
}

juce::String Editor::buildProjectIssueSummary(const std::vector<std::string>& issues) const
{
    if (issues.empty())
        return "The project could not be opened.";

    juce::String summary("The project could not be opened:\n");
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index >= 8)
        {
            summary += "\n...";
            break;
        }

        summary += "\n- " + juce::String::fromUTF8(issues[index].c_str());
    }

    return summary;
}

juce::String Editor::buildImportSummaryMessage(std::size_t importedCount,
                                               std::size_t warningCount,
                                               std::size_t skippedCount,
                                               const std::vector<std::string>& details) const
{
    return drs::app::buildWavImportSummaryMessage(importedCount, warningCount, skippedCount, details);
}

juce::File Editor::getLibraryLocation() const
{
    if (auto* settings = appProperties.getUserSettings())
    {
        const auto storedPath = settings->getValue(libraryLocationPropertyKey).trim();
        if (storedPath.isNotEmpty())
            return juce::File(storedPath);
    }

    return {};
}

void Editor::setLibraryLocation(const juce::File& folder)
{
    if (auto* settings = appProperties.getUserSettings())
    {
        settings->setValue(libraryLocationPropertyKey, folder.getFullPathName());
        settings->saveIfNeeded();
    }
}

juce::File Editor::getProjectDirectory() const
{
    if (auto* settings = appProperties.getUserSettings())
    {
        const auto storedPath = settings->getValue(projectDirectoryPropertyKey).trim();
        if (storedPath.isNotEmpty())
            return juce::File(storedPath);
    }

    return drs::app::getDefaultStudioProjectDirectory();
}

void Editor::setProjectDirectory(const juce::File& folder)
{
    if (auto* settings = appProperties.getUserSettings())
    {
        settings->setValue(projectDirectoryPropertyKey, folder.getFullPathName());
        settings->saveIfNeeded();
    }
}

juce::File Editor::getRecentProjectDirectory() const
{
    if (auto* settings = appProperties.getUserSettings())
    {
        const auto storedPath = settings->getValue(recentProjectDirectoryPropertyKey).trim();
        if (storedPath.isNotEmpty())
            return juce::File(storedPath);
    }

    return {};
}

void Editor::setRecentProjectDirectory(const juce::File& folder)
{
    if (auto* settings = appProperties.getUserSettings())
    {
        settings->setValue(recentProjectDirectoryPropertyKey, folder.getFullPathName());
        settings->saveIfNeeded();
    }
}

juce::File Editor::buildChooserBaseDirectory() const
{
    if (processor.getAuthoringProjectFile() != juce::File())
        return processor.getAuthoringProjectFile().getParentDirectory();

    auto recentProjectDirectory = getRecentProjectDirectory();
    if (recentProjectDirectory != juce::File())
    {
        ensureDirectoryExists(recentProjectDirectory);
        return recentProjectDirectory;
    }

    auto projectDirectory = getProjectDirectory();
    if (projectDirectory != juce::File())
    {
        ensureDirectoryExists(projectDirectory);
        return projectDirectory;
    }

    return drs::app::getDefaultStudioProjectDirectory();
}

juce::File Editor::buildDefaultSaveTarget() const
{
    if (processor.getAuthoringProjectFile() != juce::File())
        return processor.getAuthoringProjectFile();

    return buildChooserBaseDirectory().getChildFile("Untitled Project.drsproj");
}

juce::File Editor::buildDefaultPackageExportTarget() const
{
    if (processor.getAuthoringProjectFile() != juce::File())
        return processor.getAuthoringProjectFile().withFileExtension(".drpkg");

    auto baseDirectory = buildChooserBaseDirectory();
    if (baseDirectory == juce::File() || !baseDirectory.isDirectory())
        baseDirectory = getProjectDirectory();

    auto baseName = juce::File::createLegalFileName(buildWorkspaceDisplayName());
    if (baseName.trim().isEmpty() || baseName == "No Project Loaded")
        baseName = "Playable Instrument";

    return baseDirectory.getChildFile(baseName).withFileExtension(".drpkg");
}

void Editor::launchOpenProjectChooser(std::function<void(juce::File)> completion)
{
    activeFileChooser = std::make_unique<juce::FileChooser>("Open Decent Rhapsody project",
                                                            buildChooserBaseDirectory(),
                                                            "*.drsproj",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                       | juce::FileBrowserComponent::canSelectFiles,
                                   [safeThis, completion = std::move(completion)](const juce::FileChooser& chooser) mutable
                                   {
                                       if (safeThis == nullptr)
                                           return;

                                       const auto selectedFile = chooser.getResult();
                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(selectedFile);
                                   });
}

void Editor::launchOpenPerformancePackageChooser(std::function<void(juce::File)> completion)
{
    activeFileChooser = std::make_unique<juce::FileChooser>("Open Decent Rhapsody playable package",
                                                            buildChooserBaseDirectory(),
                                                            "*.drpkg",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                       | juce::FileBrowserComponent::canSelectFiles,
                                   [safeThis, completion = std::move(completion)](const juce::FileChooser& chooser) mutable
                                   {
                                       if (safeThis == nullptr)
                                           return;

                                       const auto selectedFile = chooser.getResult();
                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(selectedFile);
                                   });
}

void Editor::launchNewProjectChooser(std::function<void(juce::File)> completion)
{
    auto projectDirectory = getProjectDirectory();
    ensureDirectoryExists(projectDirectory);
    activeFileChooser = std::make_unique<juce::FileChooser>("Create Decent Rhapsody project",
                                                            projectDirectory.getChildFile("Untitled Project.drsproj"),
                                                            "*.drsproj",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                       | juce::FileBrowserComponent::canSelectFiles
                                       | juce::FileBrowserComponent::warnAboutOverwriting,
                                   [safeThis, completion = std::move(completion)](const juce::FileChooser& chooser) mutable
                                   {
                                       if (safeThis == nullptr)
                                           return;

                                       const auto selectedFile = drs::app::ensureProjectFileExtension(chooser.getResult());
                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(selectedFile);
                                   });
}

void Editor::launchSaveProjectChooser(std::function<void(juce::File)> completion)
{
    activeFileChooser = std::make_unique<juce::FileChooser>("Save Decent Rhapsody project",
                                                            buildDefaultSaveTarget(),
                                                            "*.drsproj",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                       | juce::FileBrowserComponent::canSelectFiles
                                       | juce::FileBrowserComponent::warnAboutOverwriting,
                                   [safeThis, completion = std::move(completion)](const juce::FileChooser& chooser) mutable
                                   {
                                       if (safeThis == nullptr)
                                           return;

                                       const auto selectedFile = drs::app::ensureProjectFileExtension(chooser.getResult());
                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(selectedFile);
                                   });
}

void Editor::launchExportPerformancePackageChooser(std::function<void(juce::File)> completion)
{
    activeFileChooser = std::make_unique<juce::FileChooser>("Export Decent Rhapsody playable instrument",
                                                            buildDefaultPackageExportTarget(),
                                                            "*.drpkg",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                                       | juce::FileBrowserComponent::canSelectFiles
                                       | juce::FileBrowserComponent::warnAboutOverwriting,
                                   [safeThis, completion = std::move(completion)](const juce::FileChooser& chooser) mutable
                                   {
                                       if (safeThis == nullptr)
                                           return;

                                       const auto selectedFile = chooser.getResult().withFileExtension(".drpkg");
                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(selectedFile);
                                   });
}

void Editor::launchImportWavChooser(std::function<void(std::vector<juce::File>)> completion)
{
    auto initialDirectory = getLibraryLocation();
    if (!initialDirectory.isDirectory())
        initialDirectory = buildChooserBaseDirectory();
    activeFileChooser = std::make_unique<juce::FileChooser>("Import WAV files into the current project",
                                                            initialDirectory,
                                                            "*.wav",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                       | juce::FileBrowserComponent::canSelectFiles
                                       | juce::FileBrowserComponent::canSelectMultipleItems,
                                   [safeThis, completion = std::move(completion)](const juce::FileChooser& chooser) mutable
                                   {
                                       if (safeThis == nullptr)
                                           return;

                                       std::vector<juce::File> selectedFiles;
                                       const auto results = chooser.getResults();
                                       selectedFiles.reserve(static_cast<std::size_t>(results.size()));
                                       for (const auto& result : results)
                                           selectedFiles.push_back(result);

                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(std::move(selectedFiles));
                                   });
}

void Editor::launchImportSfzChooser(std::function<void(juce::File)> completion)
{
    auto initialDirectory = getLibraryLocation();
    if (!initialDirectory.isDirectory())
        initialDirectory = buildChooserBaseDirectory();
    activeFileChooser = std::make_unique<juce::FileChooser>("Import SFZ document into the current project",
                                                            initialDirectory,
                                                            "*.sfz",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                       | juce::FileBrowserComponent::canSelectFiles,
                                   [safeThis, completion = std::move(completion)](const juce::FileChooser& chooser) mutable
                                   {
                                       if (safeThis == nullptr)
                                           return;

                                       const auto selectedFile = chooser.getResult();
                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(selectedFile);
                                   });
}

void Editor::launchImportBackgroundImageChooser(std::function<void(juce::File)> completion)
{
    auto initialDirectory = buildChooserBaseDirectory();
    if (processor.getAuthoringProjectFile() != juce::File())
    {
        const auto projectImagesDirectory = processor.getAuthoringProjectFile().getParentDirectory().getChildFile("Images");
        if (projectImagesDirectory.isDirectory())
            initialDirectory = projectImagesDirectory;
    }

    activeFileChooser = std::make_unique<juce::FileChooser>("Import background image into the current project",
                                                            initialDirectory,
                                                            "*.jpg;*.jpeg",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                       | juce::FileBrowserComponent::canSelectFiles,
                                   [safeThis, completion = std::move(completion)](const juce::FileChooser& chooser) mutable
                                   {
                                       if (safeThis == nullptr)
                                           return;

                                       const auto selectedFile = chooser.getResult();
                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(selectedFile);
                                   });
}

void Editor::launchImportLicenseFileChooser(std::function<void(juce::File)> completion)
{
    auto initialDirectory = buildChooserBaseDirectory();
    if (processor.getAuthoringProjectFile() != juce::File())
        initialDirectory = processor.getAuthoringProjectFile().getParentDirectory();

    activeFileChooser = std::make_unique<juce::FileChooser>("Import license file into the current project",
                                                            initialDirectory,
                                                            "*.txt",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    activeFileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                       | juce::FileBrowserComponent::canSelectFiles,
                                   [safeThis, completion = std::move(completion)](const juce::FileChooser& chooser) mutable
                                   {
                                       if (safeThis == nullptr)
                                           return;

                                       const auto selectedFile = chooser.getResult();
                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(selectedFile);
                                   });
}

void Editor::promptForRootKeySelection(const juce::String& title,
                                       const juce::String& message,
                                       int initialRootKey,
                                       std::function<void(std::optional<int>)> completion) const
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = title;
    options.dialogBackgroundColour = juce::Colour::fromRGB(244, 240, 232);
    auto selectedRootKey = std::make_shared<std::optional<int>>();
    options.content.setOwned(new RootKeySelectionComponent(message, initialRootKey, *selectedRootKey));
    options.componentToCentreAround = const_cast<Editor*>(this);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.useBottomRightCornerResizer = false;
    auto* dialog = options.create();
    dialog->enterModalState(true,
                            juce::ModalCallbackFunction::create(
                                [selectedRootKey, completion = std::move(completion)](int result) mutable
                                {
                                    if (result == 0)
                                    {
                                        completion(std::nullopt);
                                        return;
                                    }

                                    completion(*selectedRootKey);
                                }),
                            true);
}
} // namespace drs::plugin
