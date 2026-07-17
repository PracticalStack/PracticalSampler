#include "drs/engine/EngineFacade.h"
#include "drs/engine/HiseProjectContent.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <algorithm>
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

bool hasPresentDirectory(const drs::engine::HiseProjectContentSnapshot& snapshot,
                         const std::string& directoryName,
                         std::size_t minimumMatchingFiles)
{
    return std::any_of(snapshot.repoDirectories.begin(),
                       snapshot.repoDirectories.end(),
                       [&](const auto& directory)
                       {
                           return directory.name == directoryName
                               && directory.exists
                               && directory.matchingFileCount >= minimumMatchingFiles;
                       });
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
        .getChildFile("Decent Rhapsody Studio.vst3");

    if (primaryBundle.exists())
        return primaryBundle;

    return appDirectory
        .getChildFile("DecentRhapsodyStudioPlugin_artefacts")
        .getChildFile(configurationName)
        .getChildFile("VST3")
        .getChildFile("Decent Rhapsody Studio.vst3");
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
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        drs::engine::EngineFacade engineFacade;
        const auto statusSnapshot = engineFacade.getStatusSnapshot();

        require(!statusSnapshot.mode.empty(), "Engine snapshot mode must not be empty.");
        require(!statusSnapshot.integrationState.empty(), "Engine snapshot integration state must not be empty.");
        require(statusSnapshot.diagnostics.available, "Engine snapshot diagnostics must be available.");
        require(statusSnapshot.diagnostics.pageMissCount >= 3,
                "Engine snapshot diagnostics should expose streamed page misses.");
        require(statusSnapshot.diagnostics.dormantPurgeCount >= 1,
                "Engine snapshot diagnostics should expose dormant purge activity.");
        require(statusSnapshot.detail.find("Repo HISE content root:") != std::string::npos,
                "Engine snapshot detail must describe the product-owned HISE content seam.");
        require(statusSnapshot.detail.find("Phase 1 runtime bootstrap:") != std::string::npos,
                "Engine snapshot detail must describe the Phase 1 runtime bootstrap seam.");
        require(statusSnapshot.detail.find("Runtime diagnostics:") != std::string::npos,
                "Engine snapshot detail must describe the runtime diagnostics surface.");
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

        const auto contentSnapshot = drs::engine::getHiseProjectContentSnapshot();

        require(contentSnapshot.repoContentRootExists, "Product-owned HISE content root must exist.");
        require(contentSnapshot.presetFileCount >= 2, "Expected at least two authored factory preset files.");
        require(contentSnapshot.sampleMapFileCount >= 2, "Expected at least two authored sample map files.");
        require(hasPresentDirectory(contentSnapshot, "Scripts", 3), "Expected authored script assets under Scripts/.");
        require(hasPresentDirectory(contentSnapshot, "XmlPresetBackups", 2),
                "Expected authored HISE XML backup assets under XmlPresetBackups/.");

        drs::standalone::MainComponent mainComponent(false);
        require(mainComponent.getWidth() == drs::app::authoring::expandedTargetShellWidth,
                "Standalone shell width changed unexpectedly.");
        require(mainComponent.getHeight() == drs::app::authoring::expandedTargetShellHeight,
                "Standalone shell height changed unexpectedly.");
        require(mainComponent.getNumChildComponents() == 1, "Standalone shell should expose exactly one root workspace container.");
        auto* standaloneRoot = mainComponent.getChildComponent(0);
        require(standaloneRoot != nullptr, "Standalone shell root workspace container was missing.");
        require(findDescendantById(mainComponent, "workspaceTabs") != nullptr,
                "Standalone shell should expose the workspace tab container.");
        require(findDescendantById(mainComponent, "performanceKeyboard") != nullptr,
                "Standalone shell should expose the Sprint 5 keyboard surface.");
        require(findDescendantById(mainComponent, "performanceDiagnosticsToggle") != nullptr,
                "Standalone shell should expose a diagnostics entry point.");
        auto* standaloneTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(mainComponent, "workspaceTabs"));
        require(standaloneTabs != nullptr, "Standalone shell workspace tab container should be a tabbed component.");
        standaloneTabs->setCurrentTabIndex(1);
        require(findDescendantById(mainComponent, "authoringZoneSelector") != nullptr,
                "Standalone shell should expose the Sprint 3 mapping workspace zone selector.");
        require(!mainComponent.isAudioOutputEnabled(),
                "Headless standalone smoke validation should keep the real audio device disabled.");
        mainComponent.getProcessor().prepareToPlay(44100.0, 512);
        juce::AudioBuffer<float> standaloneSurfaceBuffer(2, 512);
        standaloneSurfaceBuffer.clear();
        juce::MidiBuffer standaloneEmptyMidiBuffer;
        mainComponent.getProcessor().queuePerformanceSurfaceNoteOn(57, 0.8f);
        mainComponent.getProcessor().processBlock(standaloneSurfaceBuffer, standaloneEmptyMidiBuffer);
        require(standaloneSurfaceBuffer.getMagnitude(0, standaloneSurfaceBuffer.getNumSamples()) > 0.0001f,
                "Standalone performance surface should render audible output through the shared processor path.");
        mainComponent.getProcessor().queuePerformanceSurfaceNoteOff(57);

        drs::plugin::Processor processor;
        require(processor.acceptsMidi(), "Plugin shell must remain configured as a MIDI-driven synth.");

        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Plugin editor creation failed.");
        require(editor->getWidth() == drs::app::authoring::compactShellWidth,
                "Plugin editor width changed unexpectedly.");
        require(editor->getHeight() == drs::app::authoring::compactShellHeight,
                "Plugin editor height changed unexpectedly.");
        require(editor->getNumChildComponents() == 1, "Plugin editor should expose exactly one root workspace container.");
        auto* pluginRoot = editor->getChildComponent(0);
        require(pluginRoot != nullptr, "Plugin editor root workspace container was missing.");
        require(findDescendantById(*editor, "workspaceTabs") != nullptr,
                "Plugin editor should expose the workspace tab container.");
        require(findDescendantById(*editor, "pluginFileMenuButton") != nullptr,
                "Plugin editor should expose the File menu entry point.");
        require(findDescendantById(*editor, "pluginSettingsMenuButton") != nullptr,
                "Plugin editor should expose the Settings menu entry point.");
        require(findDescendantById(*editor, "performanceKeyboard") != nullptr,
                "Plugin editor should expose the Sprint 5 keyboard surface.");
        require(findDescendantById(*editor, "performanceDiagnosticsToggle") != nullptr,
                "Plugin editor should expose a diagnostics entry point.");
        auto* pluginTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(*editor, "workspaceTabs"));
        require(pluginTabs != nullptr, "Plugin editor workspace tab container should be a tabbed component.");
        pluginTabs->setCurrentTabIndex(1);
        require(findDescendantById(*editor, "authoringZoneSelector") != nullptr,
                "Plugin editor should expose the Sprint 3 mapping workspace zone selector.");

        processor.prepareToPlay(44100.0, 512);
        juce::AudioBuffer<float> pluginBuffer(2, 512);
        pluginBuffer.clear();
        juce::MidiBuffer midiBuffer;
        midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 0);
        processor.processBlock(pluginBuffer, midiBuffer);
        require(pluginBuffer.getMagnitude(0, pluginBuffer.getNumSamples()) > 0.0001f,
                "Plugin processor should produce audible output for the reference sustain trigger.");

        juce::AudioBuffer<float> performanceSurfaceBuffer(2, 512);
        performanceSurfaceBuffer.clear();
        juce::MidiBuffer emptyMidiBuffer;
        processor.queuePerformanceSurfaceNoteOn(57, 0.8f);
        processor.processBlock(performanceSurfaceBuffer, emptyMidiBuffer);
        require(performanceSurfaceBuffer.getMagnitude(0, performanceSurfaceBuffer.getNumSamples()) > 0.0001f,
                "Plugin performance surface keyboard should produce audible output without host MIDI input.");
        processor.queuePerformanceSurfaceNoteOff(57);

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
        require(foundTypes[0]->name == "Decent Rhapsody Studio", "Scanned VST3 plugin name did not match the built product name.");
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
