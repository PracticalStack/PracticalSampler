#include "standalone/MainComponent.h"

#include "drs/engine/SampleImport.h"
#include "drs/engine/RuntimeLoader.h"

#include <cctype>
#include <filesystem>
#include <unordered_set>

namespace drs::standalone
{
namespace
{
namespace fs = std::filesystem;

constexpr int saveButtonResult = 1;
constexpr int discardButtonResult = 2;
constexpr int cancelButtonResult = 0;
constexpr auto libraryLocationPropertyKey = "libraryLocation";

juce::File ensureProjectExtension(juce::File file)
{
    if (file.hasFileExtension(".drsproj"))
        return file;

    return file.withFileExtension(".drsproj");
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

class PreferencesComponent final : public juce::Component
{
public:
    PreferencesComponent(juce::File initialLibraryLocation,
                         std::function<void(juce::File)> onSave)
        : onSaveCallback(std::move(onSave))
    {
        titleLabel.setText("Preferences", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));

        libraryLocationLabel.setText("Library Location", juce::dontSendNotification);
        libraryLocationLabel.setJustificationType(juce::Justification::centredLeft);

        helpLabel.setText("Choose the default folder for browsing projects and sample imports.",
                          juce::dontSendNotification);
        helpLabel.setJustificationType(juce::Justification::centredLeft);

        libraryLocationEditor.setText(initialLibraryLocation.getFullPathName(), juce::dontSendNotification);

        browseButton.setButtonText("Browse...");
        browseButton.onClick = [this]
        {
            auto initialDirectory = juce::File(libraryLocationEditor.getText().trim());
            if (initialDirectory == juce::File())
                initialDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

            directoryChooser = std::make_unique<juce::FileChooser>("Choose library location",
                                                                   initialDirectory,
                                                                   "*",
                                                                   true,
                                                                   false,
                                                                   this);
            auto safeThis = juce::Component::SafePointer<PreferencesComponent>(this);
            directoryChooser->launchAsync(juce::FileBrowserComponent::openMode
                                              | juce::FileBrowserComponent::canSelectDirectories,
                                          [safeThis](const juce::FileChooser& chooser)
                                          {
                                              if (safeThis == nullptr)
                                                  return;

                                              const auto selectedDirectory = chooser.getResult();
                                              safeThis->directoryChooser.reset();
                                              if (selectedDirectory != juce::File())
                                                  safeThis->libraryLocationEditor.setText(selectedDirectory.getFullPathName(),
                                                                                         juce::dontSendNotification);
                                          });
        };

        saveButton.setButtonText("Save");
        saveButton.onClick = [this]
        {
            auto chosenDirectory = juce::File(libraryLocationEditor.getText().trim());
            if (libraryLocationEditor.getText().trim().isEmpty())
            {
                chosenDirectory = {};
            }
            else if (!chosenDirectory.exists())
            {
                if (!chosenDirectory.createDirectory())
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                           "Invalid Library Location",
                                                           "The selected library folder could not be created.");
                    return;
                }
            }
            else if (!chosenDirectory.isDirectory())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Invalid Library Location",
                                                       "The selected path is not a folder.");
                return;
            }

            if (onSaveCallback)
                onSaveCallback(chosenDirectory);

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
                 static_cast<juce::Component*>(&helpLabel),
                 static_cast<juce::Component*>(&libraryLocationEditor),
                 static_cast<juce::Component*>(&browseButton),
                 static_cast<juce::Component*>(&saveButton),
                 static_cast<juce::Component*>(&cancelButton)
             })
        {
            addAndMakeVisible(component);
        }

        setSize(560, 170);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(16);
        titleLabel.setBounds(area.removeFromTop(28));
        area.removeFromTop(8);
        libraryLocationLabel.setBounds(area.removeFromTop(24));
        auto editorRow = area.removeFromTop(28);
        libraryLocationEditor.setBounds(editorRow.removeFromLeft(editorRow.proportionOfWidth(0.78f)));
        editorRow.removeFromLeft(8);
        browseButton.setBounds(editorRow);
        area.removeFromTop(8);
        helpLabel.setBounds(area.removeFromTop(22));
        area.removeFromTop(16);
        auto buttonRow = area.removeFromBottom(28);
        cancelButton.setBounds(buttonRow.removeFromRight(100));
        buttonRow.removeFromRight(8);
        saveButton.setBounds(buttonRow.removeFromRight(100));
    }

private:
    void closeDialog(int result)
    {
        if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
            dialog->exitModalState(result);
    }

    std::function<void(juce::File)> onSaveCallback;
    std::unique_ptr<juce::FileChooser> directoryChooser;
    juce::Label titleLabel;
    juce::Label libraryLocationLabel;
    juce::Label helpLabel;
    juce::TextEditor libraryLocationEditor;
    juce::TextButton browseButton;
    juce::TextButton saveButton;
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
                       }),
      authoringPanel(processor.getAuthoringSession(),
                     [this]()
                     {
                         return processor.getAuthoringWaveformPreview();
                     },
                     [this]()
                     {
                         return processor.getAuthoringImportResponsivenessSnapshot();
                     },
                     drs::app::AuthoringPanel::LayoutMode::expanded,
                     [this](int midiNoteNumber, float velocity)
                     {
                         processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);
                     },
                     [this](int midiNoteNumber)
                     {
                         processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
                     })
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

    workspaceTabs.setComponentID("workspaceTabs");
    workspaceTabs.addTab("Perform", juce::Colour::fromRGB(28, 126, 214), &performancePanel, false);
    workspaceTabs.addTab("Map", juce::Colour::fromRGB(181, 96, 21), &authoringPanel, false);
    addAndMakeVisible(workspaceTabs);
    setSize(860, 760);

    if (enableAudioOutput)
        initializeAudioOutput();

    startTimerHz(4);
    updateWindowTitle();
}

MainComponent::~MainComponent()
{
    stopTimer();
    menuBar.setModel(nullptr);
    appProperties.saveIfNeeded();
    shutdownAudioOutput();
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    menuBar.setBounds(area.removeFromTop(28));
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
    const auto validationResult = processor.getEngineFacade().restorePresetStateJson(stateJson);
    if (!validationResult.restored)
        return validationResult;

    processor.setStateInformation(stateJson.data(), static_cast<int>(stateJson.size()));
    return validationResult;
}

bool MainComponent::setMacroValue(const std::string& macroId, double value)
{
    const auto parameterId = "macro." + juce::String::fromUTF8(macroId.c_str());
    if (processor.getParameterState().getParameter(parameterId) == nullptr)
        return false;

    processor.setMacroValueFromShell(macroId, value);
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
    if (topLevelMenuIndex == 0)
    {
        menu.addItem(newProjectCommandId, "New Project");
        menu.addItem(openProjectCommandId, "Open Project...");
        menu.addSeparator();
        menu.addItem(saveProjectCommandId, "Save");
        menu.addItem(saveProjectAsCommandId, "Save As...");
        menu.addItem(importWavCommandId, "Import WAV...");
        menu.addSeparator();
        menu.addItem(exitApplicationCommandId, "Exit");
    }
    else if (topLevelMenuIndex == 1)
    {
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
        case saveProjectCommandId:
            saveProject({});
            break;
        case saveProjectAsCommandId:
            saveProjectAs({});
            break;
        case importWavCommandId:
            importWavFiles();
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
    updateWindowTitle();
}

void MainComponent::initializeAudioOutput()
{
    audioProcessorPlayer.setProcessor(&processor);

    const auto setupError = audioDeviceManager.initialise(0, 2, nullptr, true);
    if (setupError.isNotEmpty())
    {
        audioDeviceError = setupError;
        audioProcessorPlayer.setProcessor(nullptr);
        return;
    }

    audioDeviceManager.addAudioCallback(&audioProcessorPlayer);
    audioOutputEnabled = true;
}

void MainComponent::shutdownAudioOutput()
{
    if (audioOutputEnabled)
        audioDeviceManager.removeAudioCallback(&audioProcessorPlayer);

    audioProcessorPlayer.setProcessor(nullptr);
    audioOutputEnabled = false;
}

void MainComponent::createNewProject()
{
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
    confirmSafeToDiscardChanges("creating a new project",
                                [safeThis](bool shouldProceed)
                                {
                                    if (!shouldProceed || safeThis == nullptr)
                                        return;

                                    safeThis->currentProjectFile = {};
                                    safeThis->processor.replaceAuthoringProject(safeThis->buildEmptyProjectTemplate());
                                    safeThis->refreshProjectViews();
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

void MainComponent::saveProject(std::function<void(bool)> completion)
{
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

            const auto saved = safeThis->saveProjectToFile(selectedFile);
            if (completion)
                completion(saved);
        });
}

void MainComponent::importWavFiles()
{
    auto safeThis = juce::Component::SafePointer<MainComponent>(this);
    auto beginImport = [safeThis](bool ready) mutable
    {
        if (!ready || safeThis == nullptr)
            return;

        safeThis->launchImportWavChooser(
            [safeThis](std::vector<juce::File> selectedFiles)
            {
                if (safeThis == nullptr || selectedFiles.empty())
                    return;

                const auto samplesDirectory = safeThis->currentProjectFile.getParentDirectory().getChildFile("Samples");
                samplesDirectory.createDirectory();

                std::vector<std::string> copiedPaths;
                copiedPaths.reserve(selectedFiles.size());

                std::size_t skippedCount = 0;
                std::vector<std::string> details;

                for (const auto& sourceFile : selectedFiles)
                {
                    if (!sourceFile.existsAsFile())
                    {
                        ++skippedCount;
                        details.push_back("Skipped missing file: " + sourceFile.getFullPathName().toStdString());
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
                            ++skippedCount;
                            details.push_back("Could not copy " + sourceFile.getFileName().toStdString()
                                              + " into the project Samples folder.");
                            continue;
                        }
                    }

                    copiedPaths.push_back(managedCopy.getFullPathName().toStdString());
                }

                if (copiedPaths.empty())
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                           "Import WAV Failed",
                                                           safeThis->buildImportSummaryMessage(0, 0, skippedCount, details));
                    return;
                }

                const auto& currentProject = safeThis->processor.getAuthoringSession().getProject();
                auto importQueue = drs::engine::createAuthoringImportQueue(copiedPaths, currentProject.contentRootPath);
                while (drs::engine::processNextAuthoringImportQueueItem(importQueue).processed)
                {
                }

                std::unordered_set<std::string> usedSampleSourceIds;
                for (const auto& sampleSource : currentProject.sampleSources)
                    usedSampleSourceIds.insert(sampleSource.id);

                std::unordered_set<std::string> usedZoneIds;
                for (const auto& zone : currentProject.authoring.zones)
                    usedZoneIds.insert(zone.id);

                std::vector<drs::engine::RuntimeProjectSampleSource> importedSampleSources;
                std::vector<drs::engine::RuntimeProjectZoneDefinition> importedZones;
                std::size_t warningCount = 0;

                for (const auto& item : importQueue.items)
                {
                    if (item.state != drs::engine::AuthoringImportItemState::inferred
                        && item.state != drs::engine::AuthoringImportItemState::warning)
                    {
                        ++skippedCount;
                        if (!item.importResult.issues.empty())
                            details.push_back(item.importResult.issues.front());
                        else
                            details.push_back("Skipped " + fs::path(item.sourcePath).filename().generic_string() + ".");
                        continue;
                    }

                    auto sampleSourceId = makeUniqueId(
                        item.suggestedZone.sourceSampleId.empty()
                            ? fs::path(item.sourcePath).stem().generic_string()
                            : item.suggestedZone.sourceSampleId,
                        usedSampleSourceIds);

                    drs::engine::RuntimeProjectSampleSource sampleSource;
                    sampleSource.id = sampleSourceId;
                    sampleSource.path = item.sourcePath;
                    sampleSource.role = item.suggestedZone.zone.articulationId.empty()
                        ? "imported"
                        : "imported-" + item.suggestedZone.zone.articulationId;

                    auto zone = item.suggestedZone.zone;
                    zone.id = makeUniqueId(zone.id.empty() ? sampleSourceId : zone.id, usedZoneIds);
                    zone.sampleSourceId = sampleSourceId;

                    importedSampleSources.push_back(std::move(sampleSource));
                    importedZones.push_back(std::move(zone));

                    if (item.state == drs::engine::AuthoringImportItemState::warning)
                    {
                        ++warningCount;
                        if (!item.findings.empty())
                            details.push_back(item.findings.front().summary + ": " + item.findings.front().detail);
                    }
                }

                if (importedSampleSources.empty() || importedZones.empty())
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                           "Import WAV Failed",
                                                           safeThis->buildImportSummaryMessage(0, warningCount, skippedCount, details));
                    return;
                }

                const auto importedCount = importedSampleSources.size();
                const auto importResult = safeThis->processor.getAuthoringSession().appendImportedContent(
                    std::move(importedSampleSources),
                    std::move(importedZones),
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
                                                                                           warningCount,
                                                                                           skippedCount,
                                                                                           details));
            });
    };

    if (currentProjectFile == juce::File())
    {
        saveProjectAs(std::move(beginImport));
        return;
    }

    beginImport(true);
}

void MainComponent::showPreferencesDialog()
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Preferences";
    options.dialogBackgroundColour = juce::Colour::fromRGB(244, 240, 232);
    options.content.setOwned(new PreferencesComponent(getLibraryLocation(),
                                                      [safeThis = juce::Component::SafePointer<MainComponent>(this)](juce::File folder)
                                                      {
                                                          if (safeThis == nullptr)
                                                              return;

                                                          safeThis->setLibraryLocation(folder);
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
    const auto targetFile = ensureProjectExtension(file);
    const auto& project = processor.getAuthoringSession().getProject();
    const auto serializedProject = drs::engine::serializeRuntimeProjectManifest(project,
                                                                                targetFile.getFullPathName().toStdString());

    std::error_code errorCode;
    fs::create_directories(fs::path(targetFile.getParentDirectory().getFullPathName().toStdString()), errorCode);

    if (!targetFile.replaceWithText(juce::String::fromUTF8(serializedProject.c_str()), false, false, "\n"))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Save Project Failed",
                                               "The project could not be written to:\n"
                                                   + targetFile.getFullPathName());
        return false;
    }

    currentProjectFile = targetFile;
    processor.getAuthoringSession().markSaved();
    refreshProjectViews();
    return true;
}

bool MainComponent::loadProjectFromFile(const juce::File& file)
{
    const auto targetFile = ensureProjectExtension(file);
    const auto loadResult = drs::engine::loadRuntimeProjectManifest(targetFile.getFullPathName().toStdString());
    if (!loadResult.loaded)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Open Project Failed",
                                               buildProjectIssueSummary(loadResult.issues));
        return false;
    }

    currentProjectFile = targetFile;
    processor.replaceAuthoringProject(loadResult.project);
    refreshProjectViews();
    return true;
}

void MainComponent::confirmSafeToDiscardChanges(const juce::String& nextAction,
                                                std::function<void(bool)> completion)
{
    if (!processor.getAuthoringSession().getDocumentState().dirty)
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
    authoringPanel.reloadFromSession();
    updateWindowTitle();
}

void MainComponent::updateWindowTitle()
{
    if (auto* window = findParentComponentOfClass<juce::TopLevelWindow>())
        window->setName(buildWindowTitle());
}

drs::engine::RuntimeProjectModel MainComponent::buildEmptyProjectTemplate() const
{
    const auto& currentProject = processor.getAuthoringSession().getProject();

    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 2;
    project.projectId = makeProjectId();
    project.displayName = "Untitled Project";
    project.contentRootPath = currentProject.contentRootPath;
    project.defaultInstrumentManifestPath = currentProject.defaultInstrumentManifestPath;
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 1;
    project.authoring.notes = { "Created in the standalone authoring shell." };
    project.notes = {
        "Created as a new Phase 2 authoring project from the standalone shell.",
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

    if (currentProjectFile != juce::File())
        title += currentProjectFile.getFileNameWithoutExtension();
    else
        title += juce::String::fromUTF8(processor.getAuthoringSession().getProject().displayName.c_str());

    if (processor.getAuthoringSession().getDocumentState().dirty)
        title += " *";

    return title;
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

juce::File MainComponent::buildChooserBaseDirectory() const
{
    if (currentProjectFile != juce::File())
        return currentProjectFile.getParentDirectory();

    const auto libraryLocation = getLibraryLocation();
    if (libraryLocation.isDirectory())
        return libraryLocation;

    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
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

                                       const auto selectedFile = ensureProjectExtension(chooser.getResult());
                                       safeThis->activeFileChooser.reset();
                                       if (completion)
                                           completion(selectedFile);
                                   });
}
} // namespace drs::standalone
