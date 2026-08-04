#include "standalone/MainComponent.h"

#include "drs/engine/SampleImport.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/ProjectStorage.h"
#include "shared/SfzImportWorkflow.h"
#include "shared/WavImportWorkflow.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <thread>

namespace drs::standalone
{
namespace
{
namespace fs = std::filesystem;
const auto performTabColour = juce::Colour::fromRGB(28, 126, 214);
const auto mapTabColour = juce::Colour::fromRGB(181, 96, 21);

constexpr int saveButtonResult = 1;
constexpr int discardButtonResult = 2;
constexpr int cancelButtonResult = 0;
constexpr auto libraryLocationPropertyKey = "libraryLocation";
constexpr auto projectDirectoryPropertyKey = "projectDirectory";
constexpr auto recentProjectDirectoryPropertyKey = "recentProjectDirectory";
constexpr auto audioDeviceStatePropertyKey = "audioDeviceState";

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

        projectHelpLabel.setText("Open/Save Project uses the most recent project folder first, then this default folder.",
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

MainComponent::MainComponent(bool enableAudioOutput)
    : performancePanel(processor.getEngineFacade(),
                       [this](const std::string& macroId, double value)
                       {
                           processor.setMacroValueFromShell(macroId, value);
                       },
                       [this](int midiNoteNumber, float velocity)
                       {
                           processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);
                       },
                       [this](int midiNoteNumber)
                       {
                           processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
                       },
                       [this](const drs::engine::PerformancePublishCommand& command,
                              drs::engine::PerformancePublishCommandSource source)
                       {
                           return processor.submitPerformancePublishCommand(command, source);
                       },
                       [this]()
                       {
                           return processor.getPerformancePublishPresentationSnapshot();
                       },
                       [this]()
                       {
                           return processor.hasRecentAudioCallback();
                       }),
      authoringPanel(processor.getAuthoringSession(),
                     [this]()
                     {
                         return processor.getAuthoringWaveformPreview();
                     },
                     [this]()
                     {
                         return processor.getAuthoringPreviewStatusSnapshot();
                     },
                     [this]()
                     {
                         return processor.getAuthoringImportResponsivenessSnapshot();
                     },
                     drs::app::AuthoringPanel::LayoutMode::expanded,
                     [this]
                     {
                         restoreSelectedZoneRootKey();
                     },
                     [this]()
                     {
                         return processor.getEngineFacade().getDraftPlaybackStatus();
                     },
                     [this]()
                     {
                         processor.requestAuthoringPreview(
                             drs::engine::AuthoringPreviewScope::currentDraft);
                     },
                     [this]()
                     {
                         processor.submitPerformancePublishCommand(
                             {}, drs::engine::PerformancePublishCommandSource::authoringWorkspace);
                     },
                     [this](const drs::engine::AuthoringPreviewCommand& command)
                     {
                         processor.submitAuthoringPreviewCommand(command);
                     },
                     [this](std::vector<juce::File> files)
                     {
                         importSampleFiles(std::move(files));
                     },
                     [this]()
                     {
                         processor.authorizeAuthoringWaveformPreviewLoad();
                     },
                     [this]()
                     {
                         return processor.getAuthoringSourceValidationSnapshot();
                     },
                     [this]()
                     {
                         processor.requestAuthoringSourceValidation();
                     },
                     [this]()
                     {
                         processor.cancelAuthoringSourceValidation();
                     }),
      restoreBanner([this] { locateProjectForRestore(); },
                    [this] { processor.retryProjectRestore(); })
{
    juce::PropertiesFile::Options appSettingsOptions;
    appSettingsOptions.applicationName = "DecentRhapsodyStudio";
    appSettingsOptions.filenameSuffix = "settings";
    appSettingsOptions.folderName = "DecentRhapsodyStudio";
    appSettingsOptions.osxLibrarySubFolder = "Application Support";
    appSettingsOptions.commonToAllUsers = false;
    appProperties.setStorageParameters(appSettingsOptions);

    menuBar.setComponentID("mainMenuBar");
    addAndMakeVisible(menuBar);

    sessionStatusLabel.setComponentID("standaloneWorkspaceStatusLabel");
    sessionStatusLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(sessionStatusLabel);

    workspaceTabs.setComponentID("workspaceTabs");
    addAndMakeVisible(workspaceTabs);
    addAndMakeVisible(restoreBanner);
    wavImportProgress.setCancelCallback([this]
    {
        if (wavImportClient.has_value())
            wavImportClient->cancel("Canceled by user");
    });
    addChildComponent(wavImportProgress);
    sfzImportProgress.setCancelCallback([this]
    {
        if (sfzImportClient.has_value())
            sfzImportClient->cancel("Canceled by user");
    });
    sfzImportProgress.setVisible(false);
    addAndMakeVisible(sfzImportProgress);
    restoreBanner.update(processor.getProjectRestoreSnapshot());
    synchronizeWorkspacePresentation();
    setSize(drs::app::authoring::expandedTargetShellWidth,
            drs::app::authoring::expandedTargetShellHeight);

    if (enableAudioOutput)
        initializeAudioOutput();

    startTimerHz(4);
    updateWindowTitle();
}

MainComponent::~MainComponent()
{
    stopTimer();
    if (sfzImportClient.has_value())
    {
        sfzImportClient->cancel("Standalone shell closed");
        sfzImportClient->waitForTerminal(std::chrono::seconds(10));
    }
    menuBar.setModel(nullptr);
    appProperties.saveIfNeeded();
    shutdownAudioOutput();
}

void MainComponent::resized()
{
    synchronizeWorkspacePresentation();
    auto area = getLocalBounds();
    menuBar.setBounds(area.removeFromTop(28));
    sessionStatusLabel.setBounds(area.removeFromTop(24).reduced(8, 0));
    updateWorkspaceStatusLabel();
    if (restoreBanner.isVisible())
        restoreBanner.setBounds(area.removeFromTop(42));
    else
        restoreBanner.setBounds({});
    if (wavImportProgress.isVisible())
        wavImportProgress.setBounds(area.removeFromTop(58).reduced(8, 2));
    else
        wavImportProgress.setBounds({});

    if (sfzImportProgress.isVisible())
        sfzImportProgress.setBounds(area.removeFromTop(42).reduced(8, 2));
    else
        sfzImportProgress.setBounds({});
    workspaceTabs.setBounds(area);
}

std::string MainComponent::exportStateJson() const
{
    juce::MemoryBlock stateBlock;
    const_cast<drs::plugin::Processor&>(processor).getStateInformation(stateBlock);
    return std::string(static_cast<const char*>(stateBlock.getData()), stateBlock.getSize());
}

drs::engine::EnginePresetStateRestoreResult MainComponent::restoreStateJson(const std::string& stateJson)
{
    const auto parsed = drs::engine::parseHostSessionState(stateJson);
    if (!parsed.isValidHostState() && !parsed.isLegacyPreset())
    {
        drs::engine::EnginePresetStateRestoreResult rejected;
        rejected.state = parsed.state;
        for (const auto& finding : parsed.findings)
            rejected.issues.push_back(finding.message);
        return rejected;
    }

    processor.setStateInformation(stateJson.data(), static_cast<int>(stateJson.size()));
    drs::engine::EnginePresetStateRestoreResult queued;
    queued.restored = true;
    queued.state = "State restore queued";
    return queued;
}

bool MainComponent::setMacroValue(const std::string& macroId, double value)
{
    const auto parameterId = "macro." + juce::String::fromUTF8(macroId.c_str());
    if (processor.getParameterState().getParameter(parameterId) == nullptr)
        return false;

    processor.setMacroValueFromShell(macroId, value);
    processor.serviceMessageThreadWork();
    return true;
}

void MainComponent::handleCloseRequest(std::function<void(bool)> completion)
{
    confirmSafeToDiscardChanges("closing the current project", std::move(completion));
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Settings" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int topLevelMenuIndex, const juce::String&)
{
    juce::PopupMenu menu;
    const auto authoringAvailable = processor.getWorkspaceDocumentState().authoringAvailable;
    if (topLevelMenuIndex == 0)
    {
        const auto packageSession
            = processor.getWorkspaceDocumentState().kind == drs::engine::WorkspaceDocumentKind::performancePackage;
        menu.addItem(newProjectCommandId, "New Project");
        menu.addItem(openProjectCommandId, "Open Project...");
        menu.addItem(openPerformancePackageCommandId, "Open Playable Package...");
        menu.addItem(closeProjectCommandId, packageSession ? "Close Package" : "Close");
        if (authoringAvailable)
        {
            menu.addSeparator();
            menu.addItem(saveProjectCommandId, "Save");
            menu.addItem(saveProjectAsCommandId, "Save As...");
            menu.addItem(importWavCommandId, "Import WAV...");
            menu.addItem(importSfzCommandId, "Import SFZ...");
        }
        menu.addSeparator();
        menu.addItem(exitApplicationCommandId, "Exit");
    }
    else if (topLevelMenuIndex == 1)
    {
        menu.addItem(audioDeviceSettingsCommandId, "Audio Device Settings...");
        menu.addSeparator();
        menu.addItem(preferencesCommandId, "Preferences...");
    }

    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int)
{
    switch (menuItemID)
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
        case importWavCommandId:
            importWavFiles();
            break;
        case importSfzCommandId:
            importSfzFile();
            break;
        case audioDeviceSettingsCommandId:
            showAudioDeviceSettingsDialog();
            break;
        case preferencesCommandId:
            showPreferencesDialog();
            break;
        case exitApplicationCommandId:
            handleCloseRequest([](bool shouldClose)
            {
                if (shouldClose)
                    juce::JUCEApplication::getInstance()->quit();
            });
            break;
        default:
            break;
    }
}

void MainComponent::timerCallback()
{
    processor.serviceMessageThreadWork();
    if (restoreBanner.update(processor.getProjectRestoreSnapshot()))
        resized();
    performancePanel.refreshNow();
    if (processor.getWorkspaceDocumentState().authoringAvailable)
        authoringPanel.refreshNow();
    updateWindowTitle();
    pollWavImportService();
    pollSfzImportReviewService();
}

void MainComponent::locateProjectForRestore()
{
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
    launchOpenProjectChooser(
        [safeThis](juce::File selectedFile)
        {
            if (safeThis == nullptr || selectedFile == juce::File())
                return;
            safeThis->processor.retryProjectRestoreWithFile(selectedFile);
        });
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source != &audioDeviceManager)
        return;

    synchronizeAudioOutputRegistration();
    saveAudioDeviceSettings();
}

void MainComponent::initializeAudioOutput()
{
    audioDeviceManager.addChangeListener(this);
    audioProcessorPlayer.setProcessor(&processor);

    auto savedAudioDeviceState = loadSavedAudioDeviceState();
    audioDeviceError = audioDeviceManager.initialise(0, 2, savedAudioDeviceState.get(), true);
    synchronizeAudioOutputRegistration();
    saveAudioDeviceSettings();
}

void MainComponent::synchronizeAudioOutputRegistration()
{
    const auto hasCurrentAudioDevice = audioDeviceManager.getCurrentAudioDevice() != nullptr;

    if (hasCurrentAudioDevice && !audioOutputEnabled)
    {
        audioDeviceManager.addAudioCallback(&audioProcessorPlayer);
        audioOutputEnabled = true;
    }
    else if (!hasCurrentAudioDevice && audioOutputEnabled)
    {
        audioDeviceManager.removeAudioCallback(&audioProcessorPlayer);
        audioOutputEnabled = false;
    }

    if (hasCurrentAudioDevice)
        audioDeviceError = {};
}

void MainComponent::shutdownAudioOutput()
{
    saveAudioDeviceSettings();

    if (audioOutputEnabled)
        audioDeviceManager.removeAudioCallback(&audioProcessorPlayer);

    audioDeviceManager.removeChangeListener(this);
    audioDeviceManager.closeAudioDevice();
    audioProcessorPlayer.setProcessor(nullptr);
    audioOutputEnabled = false;
}

void MainComponent::createNewProject()
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
        return;

    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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
                                            safeThis->currentProjectFile = {};
                                            safeThis->processor.replaceAuthoringProject(safeThis->buildEmptyProjectTemplate());
                                            safeThis->saveProjectToFile(projectFile);
                                        });
                                });
}

void MainComponent::openProject()
{
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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

void MainComponent::openPerformancePackage()
{
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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

void MainComponent::closeProject()
{
    const auto nextAction
        = processor.getWorkspaceDocumentState().kind == drs::engine::WorkspaceDocumentKind::performancePackage
        ? juce::String("closing the current package")
        : juce::String("closing the current project");
    confirmSafeToDiscardChanges(nextAction,
                                [safeThis = juce::Component::SafePointer<MainComponent>(this)](bool shouldClose)
                                {
                                    if (!shouldClose || safeThis == nullptr)
                                        return;

                                    safeThis->currentProjectFile = {};
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

void MainComponent::saveProject(std::function<void(bool)> completion)
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
    {
        if (completion)
            completion(false);
        return;
    }

    if (currentProjectFile == juce::File())
    {
        saveProjectAs(std::move(completion));
        return;
    }

    const auto saved = saveProjectToFile(currentProjectFile);
    if (completion)
        completion(saved);
}

void MainComponent::saveProjectAs(std::function<void(bool)> completion)
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
    {
        if (completion)
            completion(false);
        return;
    }

    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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

            const auto targetFile = safeThis->currentProjectFile == juce::File()
                ? drs::app::makeSelfContainedProjectFile(selectedFile)
                : selectedFile;
            const auto saved = safeThis->saveProjectToFile(targetFile);
            if (completion)
                completion(saved);
        });
}

void MainComponent::importWavFiles()
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
        return;

    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
    launchImportWavChooser([safeThis](std::vector<juce::File> selectedFiles)
    {
        if (safeThis != nullptr && !selectedFiles.empty())
            safeThis->importSampleFiles(std::move(selectedFiles));
    });
}

void MainComponent::importSfzFile()
{
    if (!processor.getWorkspaceDocumentState().authoringAvailable)
        return;

    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
    launchImportSfzChooser([safeThis](juce::File selectedFile)
    {
        if (safeThis != nullptr && selectedFile != juce::File())
            safeThis->reviewSfzImportFile(selectedFile);
    });
}

void MainComponent::importSampleFiles(std::vector<juce::File> selectedFiles)
{
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
    auto beginImport = [safeThis, selectedFiles = std::move(selectedFiles)](bool ready) mutable
    {
        if (!ready || safeThis == nullptr || selectedFiles.empty())
            return;

        std::vector<std::string> selectedPaths;
        selectedPaths.reserve(selectedFiles.size());
        for (const auto& sourceFile : selectedFiles)
            selectedPaths.push_back(sourceFile.getFullPathName().toStdString());

        const auto& session = safeThis->processor.getAuthoringSession();
        safeThis->wavImportProjectId = safeThis->currentProjectFile != juce::File()
            ? safeThis->currentProjectFile.getFullPathName().toStdString()
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

    if (currentProjectFile == juce::File())
    {
        saveProjectAs(std::move(beginImport));
        return;
    }

    beginImport(true);
}

void MainComponent::pollWavImportService()
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
    const auto currentProjectId = currentProjectFile != juce::File()
        ? currentProjectFile.getFullPathName().toStdString()
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
            [safeThis = juce::Component::SafePointer<MainComponent>(this)](std::optional<int> selectedRootKey) mutable
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

void MainComponent::reviewSfzImportFile(const juce::File& selectedFile)
{
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
    auto beginReview = [safeThis, selectedFile](bool ready) mutable
    {
        if (!ready || safeThis == nullptr || selectedFile == juce::File())
            return;
        const auto& session = safeThis->processor.getAuthoringSession();
        safeThis->sfzImportProjectId = safeThis->currentProjectFile != juce::File()
            ? safeThis->currentProjectFile.getFullPathName().toStdString()
            : session.getProject().displayName;
        safeThis->sfzImportBaseRevision = session.getDocumentState().revision;
        safeThis->sfzImportClient.emplace(safeThis->processor.getSfzImportReviewService().openClient());
        const auto submitted = safeThis->sfzImportClient->submit(
            drs::app::SfzImportReviewRequest { session.getProject(), selectedFile.getFullPathName().toStdString(),
                                               safeThis->sfzImportProjectId, safeThis->sfzImportBaseRevision });
        if (submitted.disposition != drs::app::SfzImportReviewSubmitDisposition::accepted)
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Import SFZ", "SFZ import could not be started.");
    };

    if (currentProjectFile == juce::File())
    {
        saveProjectAs(std::move(beginReview));
        return;
    }

    beginReview(true);
}

void MainComponent::pollSfzImportReviewService()
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
                                                   "Import SFZ Failed", juce::String(snapshot->status));
        sfzImportClient->consume();
        return;
    }
    if (snapshot->stage != drs::app::SfzImportReviewServiceStage::reviewReady || !snapshot->result)
        return;
    const auto currentProjectId = currentProjectFile != juce::File()
        ? currentProjectFile.getFullPathName().toStdString()
        : processor.getAuthoringSession().getProject().displayName;
    if (currentProjectId != sfzImportProjectId
        || processor.getAuthoringSession().getDocumentState().revision != sfzImportBaseRevision)
    {
        sfzImportClient->cancel("Project changed while SFZ import was preparing");
        return;
    }
    sfzImportReviewDialogOpen = true;
    auto reviewState = snapshot->result;
    drs::app::showSfzImportReviewDialog(
        this, *reviewState,
        [safeThis = juce::Component::SafePointer<MainComponent>(this), reviewState](bool accepted) mutable
        {
            if (safeThis == nullptr)
                return;
            safeThis->sfzImportReviewDialogOpen = false;
            if (!accepted)
            {
                safeThis->sfzImportClient->consume();
                return;
            }
            const auto applyProjectId = safeThis->currentProjectFile != juce::File()
                ? safeThis->currentProjectFile.getFullPathName().toStdString()
                : safeThis->processor.getAuthoringSession().getProject().displayName;
            if (applyProjectId != safeThis->sfzImportProjectId
                || safeThis->processor.getAuthoringSession().getDocumentState().revision
                    != safeThis->sfzImportBaseRevision)
            {
                safeThis->sfzImportClient->consume();
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Import SFZ", "The project changed before apply. Please retry.");
                return;
            }
            const auto appliedSummary = drs::app::buildSfzImportAppliedSummary(*reviewState);
            const auto importResult = drs::engine::applySfzImportProjection(
                safeThis->processor.getAuthoringSession(), reviewState->projection, "Import SFZ document");
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
                                                   "Import SFZ Complete", appliedSummary);
        });
}

void MainComponent::restoreSelectedZoneRootKey()
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

void MainComponent::promptForRootKeySelection(const juce::String& title,
                                              const juce::String& message,
                                              int initialRootKey,
                                              std::function<void(std::optional<int>)> completion) const
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = title;
    options.dialogBackgroundColour = juce::Colour::fromRGB(244, 240, 232);
    auto selectedRootKey = std::make_shared<std::optional<int>>();
    options.content.setOwned(new RootKeySelectionComponent(message, initialRootKey, *selectedRootKey));
    options.componentToCentreAround = const_cast<MainComponent*>(this);
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

void MainComponent::showAudioDeviceSettingsDialog()
{
    auto* selector = new juce::AudioDeviceSelectorComponent(audioDeviceManager,
                                                            0,
                                                            0,
                                                            0,
                                                            2,
                                                            false,
                                                            false,
                                                            true,
                                                            false);
    selector->setSize(560, 460);

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Audio Device Settings";
    options.dialogBackgroundColour = juce::Colour::fromRGB(244, 240, 232);
    options.content.setOwned(selector);
    options.componentToCentreAround = this;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.useBottomRightCornerResizer = true;
    options.launchAsync();
}

void MainComponent::showPreferencesDialog()
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Preferences";
    options.dialogBackgroundColour = juce::Colour::fromRGB(244, 240, 232);
    options.content.setOwned(new PreferencesComponent(getLibraryLocation(),
                                                      getProjectDirectory(),
                                                      [safeThis = juce::Component::SafePointer<MainComponent>(this)](juce::File libraryFolder,
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

bool MainComponent::saveProjectToFile(const juce::File& file)
{
    const auto targetFile = drs::app::ensureProjectFileExtension(file);
    auto project = processor.getAuthoringSession().getProject();
    const auto savingUnsavedProject = currentProjectFile == juce::File();
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

    currentProjectFile = targetFile;
    setRecentProjectDirectory(targetFile.getParentDirectory());
    processor.getAuthoringSession().markSaved();
    refreshProjectViews();
    return true;
}

bool MainComponent::loadProjectFromFile(const juce::File& file)
{
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

    currentProjectFile = targetFile;
    setRecentProjectDirectory(targetFile.getParentDirectory());
    refreshProjectViews();
    return true;
}

bool MainComponent::loadPerformancePackageFromFile(const juce::File& file)
{
    const auto loadResult = processor.loadPerformancePackageWorkspace(file);
    if (!loadResult.loaded)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Open Playable Package Failed",
                                               buildProjectIssueSummary(loadResult.issues));
        return false;
    }

    currentProjectFile = {};
    setRecentProjectDirectory(file.getParentDirectory());
    refreshProjectViews();
    return true;
}

void MainComponent::confirmSafeToDiscardChanges(const juce::String& nextAction,
                                                std::function<void(bool)> completion)
{
    if (!processor.getWorkspaceDocumentState().dirty)
    {
        if (completion)
            completion(true);
        return;
    }

    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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

void MainComponent::refreshProjectViews()
{
    if (processor.getWorkspaceDocumentState().authoringAvailable)
        authoringPanel.reloadFromSession();
    synchronizeWorkspacePresentation();
    updateWorkspaceStatusLabel();
    updateWindowTitle();
}

void MainComponent::synchronizeWorkspacePresentation()
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

void MainComponent::updateWindowTitle()
{
    if (auto* window = findParentComponentOfClass<juce::TopLevelWindow>())
        window->setName(buildWindowTitle());
}

void MainComponent::updateWorkspaceStatusLabel()
{
    auto statusText = buildWorkspaceStatusText();
    if (processor.getWorkspaceDocumentState().dirty)
        statusText += " *";

    sessionStatusLabel.setText(statusText, juce::dontSendNotification);
    sessionStatusLabel.setTooltip(buildWorkspaceStatusTooltip());
}

drs::engine::RuntimeProjectModel MainComponent::buildUnloadedProjectState() const
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.displayName = "No Project Loaded";
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    project.authoring.articulations = { { "default", "Default", true, 0, std::nullopt } };
    project.authoring.notes = { "Open a project or create a new one to begin authoring." };
    project.notes = { "This session starts without loading the checked-in reference project." };
    return project;
}

drs::engine::RuntimeProjectModel MainComponent::buildEmptyProjectTemplate() const
{
    const auto defaultProjectDirectory = buildChooserBaseDirectory();
    const auto defaultInstrumentFile = defaultProjectDirectory.getChildFile("Untitled Project.drinst");

    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = makeProjectId();
    project.displayName = "Untitled Project";
    project.contentRootPath = defaultProjectDirectory.getFullPathName().toStdString();
    project.defaultInstrumentManifestPath = defaultInstrumentFile.getFullPathName().toStdString();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    project.authoring.articulations = { { "default", "Default", true, 0, std::nullopt } };
    project.authoring.notes = { "Created in the standalone authoring shell." };
    project.notes = {
        "Created as a new curated DSP authoring project from the standalone shell.",
        "Sample sources and zones can be added in later authoring sprints."
    };
    return project;
}

juce::String MainComponent::buildWindowTitle() const
{
    const auto applicationName = juce::JUCEApplication::getInstance() != nullptr
        ? juce::JUCEApplication::getInstance()->getApplicationName()
        : juce::String("Decent Rhapsody Studio");

    auto title = applicationName;
    title += " - ";

    title += buildWorkspaceDisplayName();
    if (processor.getWorkspaceDocumentState().kind == drs::engine::WorkspaceDocumentKind::performancePackage)
        title += " [Playable Package]";

    if (processor.getWorkspaceDocumentState().dirty)
        title += " *";

    return title;
}

juce::String MainComponent::buildWorkspaceDisplayName() const
{
    const auto& document = processor.getWorkspaceDocumentState();
    if (!document.displayName.empty())
        return juce::String::fromUTF8(document.displayName.c_str());

    return "No Project Loaded";
}

juce::String MainComponent::buildWorkspaceStatusText() const
{
    const auto& document = processor.getWorkspaceDocumentState();
    auto text = buildWorkspaceDisplayName();

    if (document.kind == drs::engine::WorkspaceDocumentKind::performancePackage)
    {
        text += " | Playable package | Read-only | Reader v";
        text += juce::String(document.minimumReaderSchemaVersion);
    }

    return text;
}

juce::String MainComponent::buildWorkspaceStatusTooltip() const
{
    const auto& document = processor.getWorkspaceDocumentState();
    if (document.kind != drs::engine::WorkspaceDocumentKind::performancePackage)
        return "Editable authoring workspace.";

    auto tooltip = juce::String("Read-only playable package session.");
    if (!document.sourcePath.empty())
        tooltip += "\nSource: " + juce::String::fromUTF8(document.sourcePath.c_str());
    tooltip += "\nCompatible reader schema: v" + juce::String(document.minimumReaderSchemaVersion);
    return tooltip;
}

juce::String MainComponent::buildProjectIssueSummary(const std::vector<std::string>& issues) const
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

juce::String MainComponent::buildImportSummaryMessage(std::size_t importedCount,
                                                      std::size_t warningCount,
                                                      std::size_t skippedCount,
                                                      const std::vector<std::string>& details) const
{
    return drs::app::buildWavImportSummaryMessage(importedCount, warningCount, skippedCount, details);
}

juce::File MainComponent::getLibraryLocation() const
{
    if (auto* settings = appProperties.getUserSettings())
    {
        const auto storedPath = settings->getValue(libraryLocationPropertyKey).trim();
        if (storedPath.isNotEmpty())
            return juce::File(storedPath);
    }

    return {};
}

void MainComponent::setLibraryLocation(const juce::File& folder)
{
    if (auto* settings = appProperties.getUserSettings())
    {
        settings->setValue(libraryLocationPropertyKey, folder.getFullPathName());
        settings->saveIfNeeded();
    }
}

juce::File MainComponent::getProjectDirectory() const
{
    if (auto* settings = appProperties.getUserSettings())
    {
        const auto storedPath = settings->getValue(projectDirectoryPropertyKey).trim();
        if (storedPath.isNotEmpty())
            return juce::File(storedPath);
    }

    return drs::app::getDefaultStudioProjectDirectory();
}

void MainComponent::setProjectDirectory(const juce::File& folder)
{
    if (auto* settings = appProperties.getUserSettings())
    {
        settings->setValue(projectDirectoryPropertyKey, folder.getFullPathName());
        settings->saveIfNeeded();
    }
}

juce::File MainComponent::getRecentProjectDirectory() const
{
    if (auto* settings = appProperties.getUserSettings())
    {
        const auto storedPath = settings->getValue(recentProjectDirectoryPropertyKey).trim();
        if (storedPath.isNotEmpty())
            return juce::File(storedPath);
    }

    return {};
}

void MainComponent::setRecentProjectDirectory(const juce::File& folder)
{
    if (auto* settings = appProperties.getUserSettings())
    {
        settings->setValue(recentProjectDirectoryPropertyKey, folder.getFullPathName());
        settings->saveIfNeeded();
    }
}

std::unique_ptr<juce::XmlElement> MainComponent::loadSavedAudioDeviceState() const
{
    if (auto* settings = appProperties.getUserSettings())
    {
        const auto xmlText = settings->getValue(audioDeviceStatePropertyKey).trim();
        if (xmlText.isNotEmpty())
            return juce::parseXML(xmlText);
    }

    return {};
}

void MainComponent::saveAudioDeviceSettings()
{
    if (auto* settings = appProperties.getUserSettings())
    {
        if (auto audioDeviceState = audioDeviceManager.createStateXml())
            settings->setValue(audioDeviceStatePropertyKey, audioDeviceState->toString());
        else
            settings->removeValue(audioDeviceStatePropertyKey);

        settings->saveIfNeeded();
    }
}

juce::File MainComponent::buildChooserBaseDirectory() const
{
    if (currentProjectFile != juce::File())
        return currentProjectFile.getParentDirectory();

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

juce::File MainComponent::buildDefaultSaveTarget() const
{
    if (currentProjectFile != juce::File())
        return currentProjectFile;

    return buildChooserBaseDirectory().getChildFile("Untitled Project.drsproj");
}

void MainComponent::launchOpenProjectChooser(std::function<void(juce::File)> completion)
{
    activeFileChooser = std::make_unique<juce::FileChooser>("Open Decent Rhapsody project",
                                                            buildChooserBaseDirectory(),
                                                            "*.drsproj",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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

void MainComponent::launchOpenPerformancePackageChooser(std::function<void(juce::File)> completion)
{
    activeFileChooser = std::make_unique<juce::FileChooser>("Open Decent Rhapsody playable package",
                                                            buildChooserBaseDirectory(),
                                                            "*.drpkg",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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

void MainComponent::launchNewProjectChooser(std::function<void(juce::File)> completion)
{
    auto projectDirectory = getProjectDirectory();
    ensureDirectoryExists(projectDirectory);
    activeFileChooser = std::make_unique<juce::FileChooser>("Create Decent Rhapsody project",
                                                            projectDirectory.getChildFile("Untitled Project.drsproj"),
                                                            "*.drsproj",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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

void MainComponent::launchImportWavChooser(std::function<void(std::vector<juce::File>)> completion)
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
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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

void MainComponent::launchImportSfzChooser(std::function<void(juce::File)> completion)
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
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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

void MainComponent::launchSaveProjectChooser(std::function<void(juce::File)> completion)
{
    activeFileChooser = std::make_unique<juce::FileChooser>("Save Decent Rhapsody project",
                                                            buildDefaultSaveTarget(),
                                                            "*.drsproj",
                                                            true,
                                                            false,
                                                            this);
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
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
} // namespace drs::standalone
