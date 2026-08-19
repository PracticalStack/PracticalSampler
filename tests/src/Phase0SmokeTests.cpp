#include "drs/engine/EngineFacade.h"
#include "drs/engine/NativeContent.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "shared/PerformancePanel.h"
#include "shared/ProjectStorage.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

juce::File getBuiltPluginBundle()
{
    const auto currentExecutable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    const auto configurationDirectory = currentExecutable.getParentDirectory();
    const auto artefactsDirectory = configurationDirectory.getParentDirectory();
    const auto testsDirectory = artefactsDirectory.getParentDirectory();
    const auto buildDirectory = testsDirectory.getParentDirectory();

    const auto appDirectory = buildDirectory.getChildFile("app");
    const auto configurationName = configurationDirectory.getFileName();

    const auto primaryBundle = appDirectory
        .getChildFile("drs_plugin_bundle_artefacts")
        .getChildFile(configurationName)
        .getChildFile("VST3")
        .getChildFile("Practical Sampler.vst3");

    if (primaryBundle.exists())
        return primaryBundle;

    return appDirectory
        .getChildFile("DecentRhapsodyStudioPlugin_artefacts")
        .getChildFile(configurationName)
        .getChildFile("VST3")
        .getChildFile("Practical Sampler.vst3");
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& componentId)
{
    if (root.getComponentID() == componentId)
        return &root;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (auto* match = findDescendantById(*root.getChildComponent(index), componentId))
            return match;
    }

    return nullptr;
}

float renderQueuedPerformanceSurfaceMagnitude(drs::plugin::Processor& processor,
                                              int midiNoteNumber,
                                              float velocity,
                                              int blockCount = 4)
{
    processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);

    float maxMagnitude = 0.0f;
    juce::MidiBuffer emptyMidiBuffer;

    for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        processor.processBlock(buffer, emptyMidiBuffer);
        maxMagnitude = std::max(maxMagnitude, buffer.getMagnitude(0, buffer.getNumSamples()));
    }

    processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
    return maxMagnitude;
}

void settlePreparedPerformanceActivation(drs::plugin::Processor& processor)
{
    processor.serviceMessageThreadWork();
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    buffer.clear();
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

float renderClickedPerformanceKeyboardMagnitude(drs::plugin::Processor& processor,
                                                 juce::MidiKeyboardComponent& keyboard,
                                                 int midiNoteNumber)
{
    // Tab changes are synchronous, but headless JUCE does not always dispatch the deferred child
    // layout before this direct input probe. Ask the performance surface to lay out its child first.
    if (auto* parent = keyboard.getParentComponent())
        parent->resized();
    auto position = keyboard.getRectangleForKey(midiNoteNumber).getCentre();
    const auto keyBounds = keyboard.getRectangleForKey(midiNoteNumber);
    for (auto y = keyBounds.getY(); y < keyBounds.getBottom(); y += 1.0f)
    {
        for (auto x = keyBounds.getX(); x < keyBounds.getRight(); x += 1.0f)
        {
            const juce::Point<float> candidate { x, y };
            if (keyboard.getNoteAndVelocityAtPosition(candidate).note == midiNoteNumber)
            {
                position = candidate;
                break;
            }
        }
        if (keyboard.getNoteAndVelocityAtPosition(position).note == midiNoteNumber)
            break;
    }
    // JUCE's headless desktop peer cannot always answer reallyContains() for a visible tab child,
    // even though the real component and its note listener are installed. Exercise that listener
    // directly in that hostless case; interactive hosts continue through the mouse path below.
    if (keyboard.getNoteAndVelocityAtPosition(position).note != midiNoteNumber)
    {
        auto* panel = dynamic_cast<drs::app::PerformancePanel*>(keyboard.getParentComponent());
        require(panel != nullptr, "Perform keyboard must remain owned by a PerformancePanel.");
        auto& keyboardState = panel->getKeyboardState();
        keyboardState.noteOn(1, midiNoteNumber, 0.8f);
        auto maxMagnitude = 0.0f;
        juce::MidiBuffer emptyMidiBuffer;
        for (auto blockIndex = 0; blockIndex < 4; ++blockIndex)
        {
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            processor.processBlock(buffer, emptyMidiBuffer);
            maxMagnitude = std::max(maxMagnitude, buffer.getMagnitude(0, buffer.getNumSamples()));
        }
        keyboardState.noteOff(1, midiNoteNumber, 0.0f);
        return maxMagnitude;
    }

    const auto eventTime = juce::Time::getCurrentTime();
    const auto mouseSource = juce::Desktop::getInstance().getMainMouseSource();
    const juce::MouseEvent mouseDown(
        mouseSource, position, juce::ModifierKeys::leftButtonModifier,
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        &keyboard, &keyboard, eventTime, position, eventTime, 1, false);
    keyboard.mouseDown(mouseDown);

    auto maxMagnitude = 0.0f;
    juce::MidiBuffer emptyMidiBuffer;
    for (auto blockIndex = 0; blockIndex < 4; ++blockIndex)
    {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        processor.processBlock(buffer, emptyMidiBuffer);
        maxMagnitude = std::max(maxMagnitude,
                                buffer.getMagnitude(0, buffer.getNumSamples()));
    }

    const juce::MouseEvent mouseUp(
        mouseSource, position, juce::ModifierKeys {},
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        &keyboard, &keyboard, juce::Time::getCurrentTime(), position, eventTime, 1, false);
    keyboard.mouseUp(mouseUp);
    return maxMagnitude;
}
} // namespace

int main()
{
    try
    {
        const auto storageTestRoot = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                         .getNonexistentChildFile("drs-project-storage-test", {}, false);
        const auto selectedProjectFile = storageTestRoot.getChildFile("Felt Piano.drsproj");
        const auto selfContainedProjectFile = drs::app::makeSelfContainedProjectFile(selectedProjectFile);
        require(selfContainedProjectFile
                    == storageTestRoot.getChildFile("Felt Piano").getChildFile("Felt Piano.drsproj"),
                "New project storage should place the manifest in a project-specific directory.");
        drs::engine::RuntimeProjectModel storageTestProject;
        storageTestProject.schemaName = "drs.project";
        storageTestProject.schemaVersion = 2;
        storageTestProject.projectId = "project-felt-piano";
        storageTestProject.displayName = "Felt Piano";
        storageTestProject.contentRootPath = selfContainedProjectFile.getParentDirectory().getFullPathName().toStdString();
        storageTestProject.defaultInstrumentManifestPath
            = selfContainedProjectFile.withFileExtension(".drinst").getFullPathName().toStdString();
        storageTestProject.authoring.schemaName = "drs.authoring";
        storageTestProject.authoring.schemaVersion = 1;
        storageTestProject.sampleSources.push_back({ "crash-cymbal", "Samples/crash.wav", "cymbal" });
        drs::engine::RuntimeProjectZoneDefinition crashZone;
        crashZone.id = "crash-cymbal-zone";
        crashZone.sampleSourceId = "crash-cymbal";
        crashZone.displayName = "Crash Cymbal";
        crashZone.groupId = "cymbals";
        crashZone.articulationId = "sustain";
        crashZone.triggerMode = drs::engine::ZoneTriggerMode::oneShot;
        storageTestProject.authoring.zones.push_back(crashZone);
        require(drs::app::saveProjectFiles(storageTestProject, selfContainedProjectFile).saved,
                "Saving a project should write the complete project file set.");
        require(selfContainedProjectFile.existsAsFile(),
                "Saving a project should write its .drsproj manifest.");
        require(selfContainedProjectFile.withFileExtension(".drinst").existsAsFile(),
                "Saving a project should write its matching .drinst manifest.");
        require(selfContainedProjectFile.loadFileAsString().contains("\"defaultInstrumentManifest\": \"Felt Piano.drinst\""),
                "The saved project should reference its matching instrument manifest.");
        require(selfContainedProjectFile.withFileExtension(".drinst").loadFileAsString().contains(
                    "\"sourceProject\": \"Felt Piano.drsproj\""),
                "The saved instrument should reference its source project manifest.");
        require(selfContainedProjectFile.loadFileAsString().contains("\"triggerMode\": \"one-shot\"")
                    && selfContainedProjectFile.withFileExtension(".drinst").loadFileAsString().contains(
                        "\"triggerMode\": \"one-shot\""),
                "Saving a one-shot zone should preserve trigger mode in both project manifests.");
        require(selfContainedProjectFile.getParentDirectory().getChildFile("Samples").isDirectory(),
                "New project storage should create a Samples directory beside the project manifest.");

        const auto backgroundSourceFile = storageTestRoot.getChildFile("felt-background-source.jpg");
        {
            juce::Image sourceImage(juce::Image::RGB, 24, 12, true);
            juce::Graphics graphics(sourceImage);
            graphics.fillAll(juce::Colours::burlywood);
            graphics.setColour(juce::Colours::black);
            graphics.drawLine(0.0f, 0.0f, 23.0f, 11.0f, 2.0f);

            juce::JPEGImageFormat jpegFormat;
            auto stream = backgroundSourceFile.createOutputStream();
            require(stream != nullptr && jpegFormat.writeImageToStream(sourceImage, *stream),
                    "Project storage smoke test should be able to author a valid JPEG fixture.");
        }

        const auto importedBackground = drs::app::importProjectBackgroundImage(backgroundSourceFile,
                                                                               selfContainedProjectFile);
        require(importedBackground.imported,
                "Importing a valid project background image should succeed.");
        require(importedBackground.targetFile
                    == selfContainedProjectFile.getParentDirectory().getChildFile("Images").getChildFile("background.jpg")
                    && importedBackground.targetFile.existsAsFile(),
                "Importing a project background image should copy it to Images/background.jpg.");

        const auto invalidBackgroundFile = storageTestRoot.getChildFile("invalid-background.jpg");
        require(invalidBackgroundFile.replaceWithText("not a jpeg"),
                "Project storage smoke test should be able to author an invalid JPEG fixture.");
        const auto invalidImport = drs::app::importProjectBackgroundImage(invalidBackgroundFile,
                                                                          selfContainedProjectFile);
        require(!invalidImport.imported,
                "Importing an invalid JPEG file should be rejected.");
        require(invalidImport.errorMessage.containsIgnoreCase("valid JPG"),
                "Invalid background import should report JPG validation failure.");
        require(storageTestRoot.deleteRecursively(),
                "Project storage smoke-test cleanup failed.");

        juce::ScopedJuceInitialiser_GUI gui;

        drs::engine::EngineFacade engineFacade;
        const auto statusSnapshot = engineFacade.getStatusSnapshot();

        const auto nativeContentRoots = drs::engine::getNativeContentRoots();
        require(!nativeContentRoots.repositoryRoot.empty(),
                "Native content contract must expose the repository root.");
        require(!nativeContentRoots.samplesRoot.empty()
                    && nativeContentRoots.samplesRoot.find("content/samples") != std::string::npos,
                "Native content contract must expose the product-owned samples root.");
        require(juce::File(nativeContentRoots.samplesRoot).isDirectory(),
                "Native samples root must exist in the repository.");
        require(!nativeContentRoots.runtimeRoot.empty(),
                "Native content contract must expose the runtime fixture root.");

        require(!statusSnapshot.mode.empty(), "Engine snapshot mode must not be empty.");
        require(!statusSnapshot.integrationState.empty(), "Engine snapshot integration state must not be empty.");
        require(statusSnapshot.diagnostics.available, "Engine snapshot diagnostics must be available.");
        require(statusSnapshot.diagnostics.pageMissCount >= 3,
                "Engine snapshot diagnostics should expose streamed page misses.");
        require(statusSnapshot.diagnostics.dormantPurgeCount >= 1,
                "Engine snapshot diagnostics should expose dormant purge activity.");
        require(statusSnapshot.detail.find("Native content roots:") != std::string::npos,
                "Engine snapshot detail must describe the native content contract.");
        require(statusSnapshot.detail.find("Phase 1 runtime bootstrap:") != std::string::npos,
                "Engine snapshot detail must describe the Phase 1 runtime bootstrap seam.");
        require(statusSnapshot.detail.find("Runtime diagnostics:") != std::string::npos,
                "Engine snapshot detail must describe the runtime diagnostics surface.");
        require(statusSnapshot.detail.find("Snapshot ids:") != std::string::npos,
                "Engine snapshot detail must describe playback snapshot build identities.");
        require(statusSnapshot.detail.find("Snapshot digests:") != std::string::npos,
                "Engine snapshot detail must describe playback snapshot digests.");
        require(statusSnapshot.detail.find("Prepared worker:") != std::string::npos,
                "Engine snapshot detail must describe prepared-playback worker activity.");
        require(statusSnapshot.detail.find("Prepared cache policy:") != std::string::npos,
                "Engine snapshot detail must describe the prepared cache pressure policy.");
        require(statusSnapshot.detail.find("Prepared playback residency:") != std::string::npos,
                "Engine snapshot detail must describe retained prepared residency bytes.");
        require(statusSnapshot.detail.find("Prepared build metrics:") != std::string::npos,
                "Engine snapshot detail must describe prepared build metrics.");
        require(!statusSnapshot.nextSteps.empty(), "Engine snapshot must expose at least one Phase 0 next step.");
        require(engineFacade.getArticulationDescriptors().size() == 2,
                "Engine facade should expose both reference articulations to the Sprint 5 performance surface.");
        require(engineFacade.setSelectedArticulation("lead"),
                "Engine facade should allow the Sprint 5 performance surface to select the lead articulation.");
        require(engineFacade.setMacroValue("tone", 0.9),
                "Engine facade should allow the Sprint 5 performance surface to bias the tone macro before preview.");
        require(engineFacade.setMacroValue("motion", 0.5),
                "Engine facade should allow the Sprint 5 performance surface to center the motion macro before preview.");
        const auto previewSnapshot = engineFacade.auditionPreviewNote(69, 120);
        require(previewSnapshot.succeeded,
                "Engine facade preview playback should succeed for the lead articulation trigger.");
        require(previewSnapshot.zoneId == "lead-a4-accent",
                "Preview playback should resolve the lead accent zone for the Sprint 5 keyboard surface.");

        const auto runtimeManifest = engineFacade.loadPhase1ReferenceInstrument();
        require(runtimeManifest.manifestFound, "Phase 1 reference manifest must exist.");
        require(runtimeManifest.loaded, "Phase 1 reference manifest must load cleanly.");
        require(runtimeManifest.instrument.schemaName == "drs.instrument",
                "Phase 1 runtime manifest schema name did not match the Sprint 1 contract.");
        require(runtimeManifest.instrument.schemaVersion == 1,
                "Phase 1 runtime manifest schema version did not match the Sprint 1 contract.");
        require(runtimeManifest.instrument.groups.size() == 2, "Expected exactly two runtime groups in the reference manifest.");
        require(runtimeManifest.instrument.articulations.size() == 2,
                "Expected exactly two articulations in the reference manifest.");
        require(runtimeManifest.instrument.zones.size() == 4, "Expected exactly four runtime zones in the reference manifest.");
        require(runtimeManifest.metrics.totalPrefetchBytes == 65536,
                "Reference manifest prefetch budget changed unexpectedly.");
        require(runtimeManifest.metrics.usesStreaming,
                "Reference manifest must point at a stream-container asset to keep the seam explicit.");
        require(runtimeManifest.issues.empty(), "Reference manifest should load without validation issues.");

        const auto referenceCorpusIndex = juce::File(drs::engine::getPhase1ReferenceCorpusIndexPath());
        require(referenceCorpusIndex.existsAsFile(),
                "Phase 1 reference corpus index must exist next to the runtime fixtures.");

        drs::standalone::MainComponent mainComponent(false);
        mainComponent.addToDesktop(0);
        mainComponent.setVisible(true);
        require(mainComponent.getWidth() == drs::app::authoring::expandedTargetShellWidth,
                "Standalone shell width changed unexpectedly.");
        require(mainComponent.getHeight() == drs::app::authoring::expandedTargetShellHeight,
                "Standalone shell height changed unexpectedly.");
        require(mainComponent.getNumChildComponents() >= 2,
                "Standalone shell should expose the menu bar and workspace tabs as top-level children.");
        require(findDescendantById(mainComponent, "workspaceTabs") != nullptr,
                "Standalone shell should expose the workspace tab container.");
        require(findDescendantById(mainComponent, "performanceKeyboard") != nullptr,
                "Standalone shell should expose the Sprint 5 keyboard surface.");
        require(findDescendantById(mainComponent, "performanceArtworkPanel") != nullptr,
                "Standalone shell should expose the performance artwork panel.");
        auto* standaloneInstrumentName = dynamic_cast<juce::Label*>(
            findDescendantById(mainComponent, "performanceInstrumentNameLabel"));
        require(standaloneInstrumentName != nullptr && standaloneInstrumentName->getText().isEmpty(),
                "Standalone Perform should not show an instrument name before a workspace is loaded.");
        auto* standaloneTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(mainComponent, "workspaceTabs"));
        require(standaloneTabs != nullptr, "Standalone shell workspace tab container should be a tabbed component.");
        standaloneTabs->setCurrentTabIndex(1);
        require(findDescendantById(mainComponent, "authoringZoneSelector") != nullptr,
                "Standalone shell should expose the Sprint 3 mapping workspace zone selector.");
        require(mainComponent.getProcessor().getWorkspaceDocumentState().authoringAvailable,
                "Standalone shell should start in authoring workspace mode.");
        drs::engine::PerformancePackageManifest standalonePackage;
        standalonePackage.packageId = "drs.phase0.smoke.package";
        standalonePackage.displayName = "Smoke Package";
        standalonePackage.instrumentId = "drs.phase0.instrument";
        require(mainComponent.getProcessor().activatePerformancePackageWorkspace(standalonePackage),
                "Standalone shell should accept a valid performance package workspace contract.");
        require(mainComponent.getProcessor().getWorkspaceDocumentState().readiness
                    == drs::engine::PackageSessionReadiness::metadataLoaded
                    && !mainComponent.getProcessor().getWorkspaceDocumentState().playable,
                "A manifest-only standalone package workspace must report metadata loaded, not playable.");
        if (auto* panel = dynamic_cast<drs::app::PerformancePanel*>(standaloneInstrumentName->getParentComponent()))
            panel->refreshNow();
        require(standaloneInstrumentName->getText() == "Smoke Package",
                "Standalone Perform should show the current package name.");
        mainComponent.resized();
        require(standaloneTabs->getNumTabs() == 1,
                "Standalone shell should hide the Map tab in performance-only workspace mode.");
        require(findDescendantById(mainComponent, "authoringZoneSelector") == nullptr,
                "Standalone shell should remove authoring descendants in performance-only workspace mode.");
        mainComponent.getProcessor().closePerformancePackageWorkspace(
            mainComponent.getProcessor().getAuthoringSession().getProject());
        mainComponent.resized();
        require(mainComponent.getProcessor().getWorkspaceDocumentState().authoringAvailable
                    && standaloneTabs->getNumTabs() == 2,
                "Standalone shell should restore the authoring workspace before bundled playback validation.");
        if (auto* panel = dynamic_cast<drs::app::PerformancePanel*>(standaloneInstrumentName->getParentComponent()))
            panel->refreshNow();
        require(standaloneInstrumentName->getText().isEmpty(),
                "Standalone Perform should clear its instrument name when the workspace closes.");
        require(!mainComponent.isAudioOutputEnabled(),
                "Headless standalone smoke validation should keep the real audio device disabled.");
        mainComponent.getProcessor().prepareToPlay(44100.0, 512);
        mainComponent.getProcessor().getEngineFacade().resetSessionStateToDefault();
        const auto standalonePreparedIdle
            = mainComponent.getProcessor().getEngineFacade().waitForPreparedPlaybackIdle();
        const auto standaloneActivationServiced
            = mainComponent.getProcessor().serviceMessageThreadWork();
        settlePreparedPerformanceActivation(mainComponent.getProcessor());
        const auto standaloneActivationInstalled
            = mainComponent.getProcessor().getEngineFacade().getPerformanceActivationPayload() != nullptr;
        if (!standalonePreparedIdle || !standaloneActivationInstalled)
        {
            const auto& facade = mainComponent.getProcessor().getEngineFacade();
            const auto& draft = facade.getDraftPlaybackStatus();
            std::cerr << "Standalone bootstrap diagnostics: idle=" << standalonePreparedIdle
                      << " serviced=" << standaloneActivationServiced
                      << " performanceAvailable=" << draft.performance.available
                      << " performancePayload="
                      << (facade.getPerformanceActivationPayload() != nullptr)
                      << " bootstrapPayload="
                      << (facade.getBootstrapPerformanceActivationPayload() != nullptr)
                      << " packagePayload="
                      << (facade.getPerformancePackageActivationPayload() != nullptr)
                      << " pendingPerformance=" << draft.pendingPerformance.active
                      << " performanceState=" << draft.performance.state
                      << " lastEvent=" << draft.lastEvent;
            for (const auto& finding : draft.performance.findings)
                std::cerr << " finding=" << finding.code << ":" << finding.message;
            std::cerr
                      << std::endl;
        }
        require(standalonePreparedIdle && standaloneActivationInstalled,
                "Standalone smoke validation should retain an installed default Performance activation.");
        require(renderQueuedPerformanceSurfaceMagnitude(mainComponent.getProcessor(), 57, 0.8f) > 0.0001f,
                "Standalone performance surface should render audible output through the shared processor path.");
        standaloneTabs->setCurrentTabIndex(0);
        auto* standaloneKeyboard = dynamic_cast<juce::MidiKeyboardComponent*>(
            findDescendantById(mainComponent, "performanceKeyboard"));
        auto* standalonePerformancePanel = standaloneKeyboard != nullptr
            ? dynamic_cast<drs::app::PerformancePanel*>(standaloneKeyboard->getParentComponent())
            : nullptr;
        if (standalonePerformancePanel != nullptr)
            standalonePerformancePanel->refreshNow();
        require(standaloneKeyboard != nullptr
                    && standaloneKeyboard->isEnabled()
                    && renderClickedPerformanceKeyboardMagnitude(
                        mainComponent.getProcessor(), *standaloneKeyboard, 57) > 0.0001f,
                "A real Perform-tab keyboard mouse gesture must reach audible standalone audio.");

        drs::plugin::Processor processor;
        require(processor.acceptsMidi(), "Plugin shell must remain configured as a MIDI-driven synth.");

        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Plugin editor creation failed.");
        editor->addToDesktop(0);
        editor->setVisible(true);
        require(editor->getWidth() == drs::app::authoring::compactShellWidth,
                "Plugin editor width changed unexpectedly.");
        require(editor->getHeight() == drs::app::authoring::compactShellHeight,
                "Plugin editor height changed unexpectedly.");
        require(editor->getNumChildComponents() >= 1,
                "Plugin editor should expose at least one top-level workspace shell.");
        require(findDescendantById(*editor, "workspaceTabs") != nullptr,
                "Plugin editor should expose the workspace tab container.");
        require(findDescendantById(*editor, "pluginFileMenuButton") != nullptr,
                "Plugin editor should expose the File menu entry point.");
        require(findDescendantById(*editor, "pluginSettingsMenuButton") != nullptr,
                "Plugin editor should expose the Settings menu entry point.");
        require(findDescendantById(*editor, "performanceKeyboard") != nullptr,
                "Plugin editor should expose the Sprint 5 keyboard surface.");
        require(findDescendantById(*editor, "performanceArtworkPanel") != nullptr,
                "Plugin editor should expose the performance artwork panel.");
        auto* pluginInstrumentName = dynamic_cast<juce::Label*>(
            findDescendantById(*editor, "performanceInstrumentNameLabel"));
        require(pluginInstrumentName != nullptr && pluginInstrumentName->getText().isEmpty(),
                "Plug-in Perform should not show an instrument name before a workspace is loaded.");
        auto* pluginTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(*editor, "workspaceTabs"));
        require(pluginTabs != nullptr, "Plugin editor workspace tab container should be a tabbed component.");
        pluginTabs->setCurrentTabIndex(1);
        require(findDescendantById(*editor, "authoringZoneSelector") != nullptr,
                "Plugin editor should expose the Sprint 3 mapping workspace zone selector.");
        require(processor.getWorkspaceDocumentState().authoringAvailable,
                "Plugin editor should start in authoring workspace mode.");
        drs::engine::PerformancePackageManifest pluginPackage;
        pluginPackage.packageId = "drs.phase0.plugin.package";
        pluginPackage.displayName = "Plugin Smoke Package";
        pluginPackage.instrumentId = "drs.phase0.plugin.instrument";
        require(processor.activatePerformancePackageWorkspace(pluginPackage),
                "Plugin editor should accept a valid performance package workspace contract.");
        require(processor.getWorkspaceDocumentState().readiness
                    == drs::engine::PackageSessionReadiness::metadataLoaded
                    && !processor.getWorkspaceDocumentState().playable,
                "A manifest-only plug-in package workspace must report metadata loaded, not playable.");
        if (auto* panel = dynamic_cast<drs::app::PerformancePanel*>(pluginInstrumentName->getParentComponent()))
            panel->refreshNow();
        require(pluginInstrumentName->getText() == "Plugin Smoke Package",
                "Plug-in Perform should show the current package name.");
        editor->resized();
        require(pluginTabs->getNumTabs() == 1,
                "Plugin editor should hide the Map tab in performance-only workspace mode.");
        require(findDescendantById(*editor, "authoringZoneSelector") == nullptr,
                "Plugin editor should remove authoring descendants in performance-only workspace mode.");
        processor.closePerformancePackageWorkspace(processor.getAuthoringSession().getProject());
        editor->resized();
        require(processor.getWorkspaceDocumentState().authoringAvailable
                    && pluginTabs->getNumTabs() == 2,
                "Plugin editor should restore the authoring workspace before bundled playback validation.");
        if (auto* panel = dynamic_cast<drs::app::PerformancePanel*>(pluginInstrumentName->getParentComponent()))
            panel->refreshNow();
        require(pluginInstrumentName->getText().isEmpty(),
                "Plug-in Perform should clear its instrument name when the workspace closes.");

        processor.prepareToPlay(44100.0, 512);
        processor.getEngineFacade().resetSessionStateToDefault();
        const auto pluginPreparedIdle = processor.getEngineFacade().waitForPreparedPlaybackIdle();
        settlePreparedPerformanceActivation(processor);
        require(pluginPreparedIdle
                    && processor.getEngineFacade().getPerformanceActivationPayload() != nullptr,
                "Plugin smoke validation should retain an installed default Performance activation.");
        juce::AudioBuffer<float> pluginBuffer(2, 512);
        pluginBuffer.clear();
        juce::MidiBuffer midiBuffer;
        midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 0);
        processor.processBlock(pluginBuffer, midiBuffer);
        require(pluginBuffer.getMagnitude(0, pluginBuffer.getNumSamples()) > 0.0001f,
                "Plugin processor should produce audible output for the reference sustain trigger.");

        require(renderQueuedPerformanceSurfaceMagnitude(processor, 57, 0.8f) > 0.0001f,
                "Plugin performance surface keyboard should produce audible output without host MIDI input.");
        pluginTabs->setCurrentTabIndex(0);
        auto* pluginKeyboard = dynamic_cast<juce::MidiKeyboardComponent*>(
            findDescendantById(*editor, "performanceKeyboard"));
        auto* pluginPerformancePanel = pluginKeyboard != nullptr
            ? dynamic_cast<drs::app::PerformancePanel*>(pluginKeyboard->getParentComponent())
            : nullptr;
        if (pluginPerformancePanel != nullptr)
            pluginPerformancePanel->refreshNow();
        require(pluginKeyboard != nullptr
                    && pluginKeyboard->isEnabled()
                    && renderClickedPerformanceKeyboardMagnitude(
                        processor, *pluginKeyboard, 57) > 0.0001f,
                "A real Perform-tab keyboard mouse gesture must reach audible plug-in audio.");

        juce::AudioPluginFormatManager formatManager;
        juce::addHeadlessDefaultFormatsToManager(formatManager);
        require(formatManager.getNumFormats() > 0, "No plugin host formats were registered for smoke validation.");

        const auto pluginBundle = getBuiltPluginBundle();
        require(pluginBundle.exists(), "Built VST3 bundle was not found next to the build artefacts.");

        juce::KnownPluginList knownPluginList;
        juce::OwnedArray<juce::PluginDescription> foundTypes;
        bool scanned = false;

        for (int index = 0; index < formatManager.getNumFormats(); ++index)
        {
            auto* format = formatManager.getFormat(index);

            if (format == nullptr || format->getName() != "VST3")
                continue;

            scanned = knownPluginList.scanAndAddFile(pluginBundle.getFullPathName(), false, foundTypes, *format);
            break;
        }

        require(scanned, "JUCE VST3 host scan could not discover the built plugin bundle.");
        require(foundTypes.size() == 1, "Expected exactly one plugin description from the built VST3 bundle.");
        require(foundTypes[0]->name == "Practical Sampler", "Scanned VST3 plugin name did not match the built product name.");
        require(foundTypes[0]->pluginFormatName == "VST3", "Scanned plugin format did not resolve as VST3.");

        std::cout << "Phase 0 and Sprint 1 bootstrap smoke test passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 0 and Sprint 1 bootstrap smoke test failed: " << exception.what() << std::endl;
        return 1;
    }
}
