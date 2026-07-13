#include "drs/engine/EngineFacade.h"
#include "drs/engine/HiseProjectContent.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
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

    return buildDirectory
        .getChildFile("app")
        .getChildFile("DecentRhapsodyStudioPlugin_artefacts")
        .getChildFile(configurationDirectory.getFileName())
        .getChildFile("VST3")
        .getChildFile("Decent Rhapsody Studio.vst3");
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
        require(statusSnapshot.detail.find("Repo HISE content root:") != std::string::npos,
                "Engine snapshot detail must describe the product-owned HISE content seam.");
        require(statusSnapshot.detail.find("Phase 1 runtime bootstrap:") != std::string::npos,
                "Engine snapshot detail must describe the Phase 1 runtime bootstrap seam.");
        require(!statusSnapshot.nextSteps.empty(), "Engine snapshot must expose at least one Phase 0 next step.");

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

        drs::standalone::MainComponent mainComponent;
        require(mainComponent.getWidth() == 820, "Standalone shell width changed unexpectedly.");
        require(mainComponent.getHeight() == 520, "Standalone shell height changed unexpectedly.");
        require(mainComponent.getNumChildComponents() == 1, "Standalone shell should expose exactly one root status panel.");

        drs::plugin::Processor processor;
        require(processor.acceptsMidi(), "Plugin shell must remain configured as a MIDI-driven synth.");

        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Plugin editor creation failed.");
        require(editor->getWidth() == 720, "Plugin editor width changed unexpectedly.");
        require(editor->getHeight() == 420, "Plugin editor height changed unexpectedly.");
        require(editor->getNumChildComponents() == 1, "Plugin editor should expose exactly one root status panel.");

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
