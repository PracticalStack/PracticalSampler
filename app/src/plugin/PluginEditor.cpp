#include "plugin/PluginEditor.h"

#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"
#include "shared/ProjectStorage.h"
#include "shared/SfzImportWorkflow.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_set>

namespace drs::plugin
{
namespace
{
namespace fs = std::filesystem;

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

std::string slugifyText(const std::string& text)
{
    std::string slug;
    bool previousWasDash = false;

    for (const auto character : text)
    {
        if (std::isalnum(static_cast<unsigned char>(character)) != 0)
        {
            slug.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
            previousWasDash = false;
        }
        else if (!previousWasDash)
        {
            slug.push_back('-');
            previousWasDash = true;
        }
    }

    while (!slug.empty() && slug.back() == '-')
        slug.pop_back();

    return slug.empty() ? "imported-sample" : slug;
}

std::string makeUniqueId(const std::string& preferredId, std::unordered_set<std::string>& usedIds)
{
    auto candidate = slugifyText(preferredId);
    auto suffix = 2;

    while (!usedIds.insert(candidate).second)
        candidate = slugifyText(preferredId) + "-" + std::to_string(suffix++);

    return candidate;
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
                     })
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
    fileMenuButton.onClick = [this]
    {
        showFileMenu();
    };
    workspaceShell.addAndMakeVisible(fileMenuButton);

    settingsMenuButton.setComponentID("pluginSettingsMenuButton");
    settingsMenuButton.onClick = [this]
    {
        showSettingsMenu();
    };
    workspaceShell.addAndMakeVisible(settingsMenuButton);

    projectStatusLabel.setComponentID("pluginProjectStatusLabel");
    projectStatusLabel.setJustificationType(juce::Justification::centredRight);
    workspaceShell.addAndMakeVisible(projectStatusLabel);

    workspaceTabs.setComponentID("workspaceTabs");
    workspaceTabs.addTab("Perform", juce::Colour::fromRGB(28, 126, 214), &performancePanel, false);
    workspaceTabs.addTab("Map", juce::Colour::fromRGB(181, 96, 21), &authoringPanel, false);
    workspaceShell.addAndMakeVisible(workspaceTabs);

    setSize(drs::app::authoring::compactShellWidth, drs::app::authoring::compactShellHeight);
    refreshProjectViews();
    startTimerHz(4);
}

Editor::~Editor()
{
    stopTimer();
    appProperties.saveIfNeeded();
}

void Editor::resized()
{
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

    workspaceTabs.setBounds(area);
}

void Editor::showFileMenu()
{
    juce::PopupMenu menu;
    menu.addItem(newProjectCommandId, "New Project");
    menu.addItem(openProjectCommandId, "Open Project...");
    menu.addItem(closeProjectCommandId, "Close");
    menu.addSeparator();
    menu.addItem(saveProjectCommandId, "Save");
    menu.addItem(saveProjectAsCommandId, "Save As...");
    menu.addItem(importWavCommandId, "Import WAV...");
    menu.addItem(importSfzCommandId, "Import SFZ...");

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
        case preferencesCommandId:
            showPreferencesDialog();
            break;
        default:
            break;
    }
}

void Editor::createNewProject()
{
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
                                            safeThis->processor.setAuthoringProjectFile({});
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

void Editor::closeProject()
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    confirmSafeToDiscardChanges("closing the current project",
                                [safeThis](bool shouldProceed)
                                {
                                    if (!shouldProceed || safeThis == nullptr)
                                        return;

                                    safeThis->processor.setAuthoringProjectFile({});
                                    safeThis->processor.closeAuthoringProject(safeThis->buildUnloadedProjectState());
                                    safeThis->refreshProjectViews();
                                });
}

void Editor::saveProject(std::function<void(bool)> completion)
{
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

void Editor::importWavFiles()
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    launchImportWavChooser([safeThis](std::vector<juce::File> selectedFiles)
    {
        if (safeThis != nullptr && !selectedFiles.empty())
            safeThis->importSampleFiles(std::move(selectedFiles));
    });
}

void Editor::importSfzFile()
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    launchImportSfzChooser([safeThis](juce::File selectedFile)
    {
        if (safeThis != nullptr && selectedFile != juce::File())
            safeThis->reviewSfzImportFile(selectedFile);
    });
}

void Editor::importSampleFiles(std::vector<juce::File> selectedFiles)
{
    auto safeThis = juce::Component::SafePointer<Editor>(this);
    auto beginImport = [safeThis, selectedFiles = std::move(selectedFiles)](bool ready) mutable
    {
        if (!ready || safeThis == nullptr || selectedFiles.empty())
            return;

                const auto samplesDirectory = safeThis->processor.getAuthoringProjectFile().getParentDirectory()
                                                  .getChildFile("Samples");
                samplesDirectory.createDirectory();

                struct PendingImportState
                {
                    drs::engine::RuntimeProjectModel currentProject;
                    drs::engine::AuthoringImportQueue importQueue;
                    std::unordered_set<std::string> usedSampleSourceIds;
                    std::unordered_set<std::string> usedZoneIds;
                    std::vector<drs::engine::RuntimeProjectSampleSource> importedSampleSources;
                    std::vector<drs::engine::RuntimeProjectZoneDefinition> importedZones;
                    std::size_t warningCount = 0;
                    std::size_t skippedCount = 0;
                    std::size_t itemIndex = 0;
                    std::vector<std::string> details;
                };

                auto state = std::make_shared<PendingImportState>();

                for (const auto& sourceFile : selectedFiles)
                {
                    if (!sourceFile.existsAsFile())
                    {
                        ++state->skippedCount;
                        state->details.push_back("Skipped missing file: " + sourceFile.getFullPathName().toStdString());
                        continue;
                    }

                    juce::File managedCopy = sourceFile;
                    if (sourceFile.getParentDirectory() != samplesDirectory)
                    {
                        managedCopy = samplesDirectory.getNonexistentChildFile(sourceFile.getFileNameWithoutExtension(),
                                                                              sourceFile.getFileExtension(),
                                                                              false);
                        if (!sourceFile.copyFileTo(managedCopy))
                        {
                            ++state->skippedCount;
                            state->details.push_back("Could not copy " + sourceFile.getFileName().toStdString()
                                                     + " into the project Samples folder.");
                            continue;
                        }
                    }

                    drs::engine::AuthoringImportQueueItem queuedItem;
                    queuedItem.sourcePath = managedCopy.getFullPathName().toStdString();
                    state->importQueue.items.push_back(std::move(queuedItem));
                }

                if (state->importQueue.items.empty())
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                           "Import WAV Failed",
                                                           safeThis->buildImportSummaryMessage(0, 0, state->skippedCount, state->details));
                    return;
                }

                std::vector<std::string> copiedPaths;
                copiedPaths.reserve(state->importQueue.items.size());
                for (const auto& item : state->importQueue.items)
                    copiedPaths.push_back(item.sourcePath);

                state->currentProject = safeThis->processor.getAuthoringSession().getProject();
                state->importQueue = drs::engine::createAuthoringImportQueue(copiedPaths, state->currentProject.contentRootPath);
                while (drs::engine::processNextAuthoringImportQueueItem(state->importQueue).processed)
                {
                }

                for (const auto& sampleSource : state->currentProject.sampleSources)
                    state->usedSampleSourceIds.insert(sampleSource.id);

                for (const auto& zone : state->currentProject.authoring.zones)
                    state->usedZoneIds.insert(zone.id);

                auto processNextItem = std::make_shared<std::function<void()>>();
                *processNextItem = [safeThis, state, processNextItem]()
                {
                    if (safeThis == nullptr)
                        return;

                    auto appendImportedItem = [state](const drs::engine::AuthoringImportQueueItem& item,
                                                      drs::engine::RuntimeProjectZoneDefinition zone)
                    {
                        auto sampleSourceId = makeUniqueId(
                            item.suggestedZone.sourceSampleId.empty()
                                ? fs::path(item.sourcePath).stem().generic_string()
                                : item.suggestedZone.sourceSampleId,
                            state->usedSampleSourceIds);

                        drs::engine::RuntimeProjectSampleSource sampleSource;
                        sampleSource.id = sampleSourceId;
                        sampleSource.path = item.sourcePath;
                        sampleSource.role = item.suggestedZone.zone.articulationId.empty()
                            ? "imported"
                            : "imported-" + item.suggestedZone.zone.articulationId;

                        zone.id = makeUniqueId(zone.id.empty() ? sampleSourceId : zone.id, state->usedZoneIds);
                        zone.sampleSourceId = sampleSourceId;

                        state->importedSampleSources.push_back(std::move(sampleSource));
                        state->importedZones.push_back(std::move(zone));
                    };

                    while (state->itemIndex < state->importQueue.items.size())
                    {
                        const auto& item = state->importQueue.items[state->itemIndex++];

                        if (item.state != drs::engine::AuthoringImportItemState::inferred
                            && item.state != drs::engine::AuthoringImportItemState::warning)
                        {
                            ++state->skippedCount;
                            if (!item.importResult.issues.empty())
                                state->details.push_back(item.importResult.issues.front());
                            else
                                state->details.push_back("Skipped " + fs::path(item.sourcePath).filename().generic_string() + ".");
                            continue;
                        }

                        auto zone = item.suggestedZone.zone;
                        if (item.suggestedZone.rootKeySource == "manual")
                        {
                            safeThis->promptForRootKeySelection(
                                "Root Key Required",
                                "Could not infer a root key for '"
                                    + juce::String::fromUTF8(fs::path(item.sourcePath).filename().generic_string().c_str())
                                    + "'. Select the sample's native pitch to continue importing.",
                                zone.rootKey,
                                [safeThis, state, processNextItem, item, zone, appendImportedItem](std::optional<int> selectedRootKey) mutable
                                {
                                    if (safeThis == nullptr)
                                        return;

                                    if (!selectedRootKey.has_value())
                                    {
                                        ++state->skippedCount;
                                        state->details.push_back("Skipped " + fs::path(item.sourcePath).filename().generic_string()
                                                                 + " because its root key was not confirmed.");
                                        (*processNextItem)();
                                        return;
                                    }

                                    zone.rootKey = *selectedRootKey;
                                    appendImportedItem(item, zone);
                                    ++state->warningCount;
                                    state->details.push_back("Selected root key "
                                                             + juce::MidiMessage::getMidiNoteName(zone.rootKey, true, true, 4).toStdString()
                                                             + " for " + fs::path(item.sourcePath).filename().generic_string() + ".");
                                    (*processNextItem)();
                                });
                            return;
                        }

                        appendImportedItem(item, zone);

                        if (item.state == drs::engine::AuthoringImportItemState::warning)
                        {
                            ++state->warningCount;
                            if (!item.findings.empty())
                                state->details.push_back(item.findings.front().summary + ": " + item.findings.front().detail);
                        }
                    }

                    if (state->importedSampleSources.empty() || state->importedZones.empty())
                    {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                               "Import WAV Failed",
                                                               safeThis->buildImportSummaryMessage(0,
                                                                                                  state->warningCount,
                                                                                                  state->skippedCount,
                                                                                                  state->details));
                        return;
                    }

                    const auto importedCount = state->importedSampleSources.size();
                    drs::engine::reconcileBatchInferredRoundRobinDescriptors(state->importedZones);
                    const auto importResult = safeThis->processor.getAuthoringSession().appendImportedContent(
                        std::move(state->importedSampleSources),
                        std::move(state->importedZones),
                        "Import WAV files");
                    if (!importResult.applied)
                    {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                               "Import WAV Failed",
                                                               safeThis->buildProjectIssueSummary(importResult.issues));
                        return;
                    }

                    safeThis->refreshProjectViews();

                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                           "Import WAV Complete",
                                                           safeThis->buildImportSummaryMessage(importedCount,
                                                                                              state->warningCount,
                                                                                              state->skippedCount,
                                                                                              state->details));
                };

                (*processNextItem)();
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

        const auto baseProject = safeThis->processor.getAuthoringSession().getProject();
        const auto sfzPath = selectedFile.getFullPathName().toStdString();

        std::thread([safeThis, baseProject, sfzPath]()
        {
            auto review = drs::app::prepareSfzImportReview(baseProject, sfzPath);
            juce::MessageManager::callAsync([safeThis, review = std::move(review)]() mutable
            {
                if (safeThis == nullptr)
                    return;

                if (!review.prepared)
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                           "Import SFZ Failed",
                                                           drs::app::buildSfzImportIssueSummary(review));
                    return;
                }

                auto reviewState = std::make_shared<drs::app::SfzImportReviewPreparationResult>(std::move(review));
                drs::app::showSfzImportReviewDialog(
                    safeThis.getComponent(),
                    *reviewState,
                    [safeThis, reviewState](bool accepted) mutable
                    {
                        if (!accepted || safeThis == nullptr)
                            return;

                        const auto appliedSummary = drs::app::buildSfzImportAppliedSummary(*reviewState);
                        const auto importResult = drs::engine::applySfzImportProjection(
                            safeThis->processor.getAuthoringSession(),
                            std::move(reviewState->projection),
                            "Import SFZ document");
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
            });
        }).detach();
    };

    if (processor.getAuthoringProjectFile() == juce::File())
    {
        saveProjectAs(std::move(beginReview));
        return;
    }

    beginReview(true);
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

    const auto importResult = drs::engine::importSampleFile(sampleSource->path);
    if (!importResult.imported)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Restore Root Key Failed",
                                               buildProjectIssueSummary(importResult.issues));
        return;
    }

    const auto inference = drs::engine::inferSampleRootKey(sampleSource->path, &importResult.sample.metadata);
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

    processor.setAuthoringProjectFile(targetFile);
    if (savingUnsavedProject)
        processor.replaceAuthoringProject(project);
    setRecentProjectDirectory(targetFile.getParentDirectory());
    processor.getAuthoringSession().markSaved();
    refreshProjectViews();
    return true;
}

bool Editor::loadProjectFromFile(const juce::File& file)
{
    const auto targetFile = drs::app::ensureProjectFileExtension(file);
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

    processor.setAuthoringProjectFile(targetFile);
    setRecentProjectDirectory(targetFile.getParentDirectory());
    processor.replaceAuthoringProject(*upgradedProject);
    refreshProjectViews();
    return true;
}

void Editor::confirmSafeToDiscardChanges(const juce::String& nextAction,
                                         std::function<void(bool)> completion)
{
    if (!processor.getAuthoringSession().getDocumentState().dirty)
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
    processor.serviceMessageThreadWork();
    performancePanel.refreshNow();
    authoringPanel.refreshNow();
    updateProjectStatusLabel();
}

void Editor::refreshProjectViews()
{
    authoringPanel.reloadFromSession();
    updateProjectStatusLabel();
}

void Editor::updateProjectStatusLabel()
{
    juce::String statusText;
    if (processor.getAuthoringProjectFile() != juce::File())
        statusText = processor.getAuthoringProjectFile().getFileNameWithoutExtension();
    else if (!processor.getAuthoringSession().getProject().displayName.empty())
        statusText = juce::String::fromUTF8(processor.getAuthoringSession().getProject().displayName.c_str());
    else
        statusText = "No Project Loaded";

    if (processor.getAuthoringSession().getDocumentState().dirty)
        statusText += " *";

    projectStatusLabel.setText(statusText, juce::dontSendNotification);
}

drs::engine::RuntimeProjectModel Editor::buildUnloadedProjectState() const
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 3;
    project.displayName = "No Project Loaded";
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 2;
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
    project.schemaVersion = 3;
    project.projectId = makeProjectId();
    project.displayName = "Untitled Project";
    project.contentRootPath = defaultProjectDirectory.getFullPathName().toStdString();
    project.defaultInstrumentManifestPath = defaultInstrumentFile.getFullPathName().toStdString();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 2;
    project.authoring.notes = { "Created in the plug-in authoring shell." };
    project.notes = {
        "Created as a new Phase 3 authoring project from the plug-in shell.",
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
    juce::String summary("Imported " + juce::String(static_cast<int>(importedCount)) + " file");
    if (importedCount != 1)
        summary += "s";

    summary += " into the current project.";
    summary += "\nWarnings: " + juce::String(static_cast<int>(warningCount));
    summary += "\nSkipped: " + juce::String(static_cast<int>(skippedCount));

    if (!details.empty())
    {
        summary += "\n\nDetails:";
        for (std::size_t index = 0; index < details.size() && index < 8; ++index)
            summary += "\n- " + juce::String::fromUTF8(details[index].c_str());
    }

    return summary;
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
