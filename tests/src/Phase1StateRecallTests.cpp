#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"
#include "drs/engine/RuntimeLoader.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

drs::engine::EngineMacroDescriptor findMacroDescriptor(const drs::engine::EngineFacade& engineFacade,
                                                       const std::string& macroId)
{
    for (const auto& macro : engineFacade.getMacroDescriptors())
    {
        if (macro.id == macroId)
            return macro;
    }

    throw std::runtime_error("Macro descriptor '" + macroId + "' was not found.");
}

void serviceRestore(drs::plugin::Processor& processor, const std::string& context)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        const auto restore = processor.getProjectRestoreSnapshot();
        if (restore != nullptr
            && (restore->state == drs::engine::ProjectRestoreState::active
                || restore->state == drs::engine::ProjectRestoreState::needsLocation
                || restore->state == drs::engine::ProjectRestoreState::failed))
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    throw std::runtime_error(context + " timed out.");
}

void drainProcessorBackgroundWork(drs::plugin::Processor& processor)
{
    processor.getEngineFacade().waitForPreparedPlaybackIdle(std::chrono::seconds(10));
    for (auto pass = 0; pass < 8; ++pass)
    {
        processor.serviceMessageThreadWork();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    processor.getEngineFacade().waitForPreparedPlaybackIdle(std::chrono::seconds(10));
    processor.serviceMessageThreadWork();
    processor.waitForHostStatePublication();
    processor.serviceMessageThreadWork();
}

void requireSessionMatchesLeadPerformance(const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                          const std::string& context)
{
    require(sessionState.targetInstrumentId == "drs.phase1.tiny-open-instrument",
            context + " target instrument changed unexpectedly.");
    require(sessionState.targetInstrumentSchemaName == "drs.instrument",
            context + " target instrument schema changed unexpectedly.");
    require(sessionState.targetInstrumentSchemaVersion == 1,
            context + " target instrument schema version changed unexpectedly.");
    require(sessionState.loadProfileId == "performance",
            context + " load profile did not round-trip.");
    require(sessionState.selectedArticulationId == "lead",
            context + " articulation id did not round-trip.");
    require(sessionState.macroValues.size() == 2,
            context + " macro count changed unexpectedly.");
    require(sessionState.macroValues[0].id == "tone" && sessionState.macroValues[0].value == 0.62,
            context + " tone macro did not round-trip.");
    require(sessionState.macroValues[1].id == "motion" && sessionState.macroValues[1].value == 0.78,
            context + " motion macro did not round-trip.");
}

void requirePresetMatchesLeadPerformance(const drs::engine::RuntimePresetState& preset,
                                         const std::string& context)
{
    require(preset.schemaName == "drs.presetState" && preset.schemaVersion == 1,
            context + " preset schema changed unexpectedly.");
    require(preset.targetInstrumentId == "drs.phase1.tiny-open-instrument"
                && preset.selectedArticulationId == "lead"
                && preset.loadProfileId == "performance",
            context + " preset target or selection changed unexpectedly.");
    require(preset.macroValues.size() == 2
                && preset.macroValues[0].id == "tone"
                && preset.macroValues[0].value == 0.62
                && preset.macroValues[1].id == "motion"
                && preset.macroValues[1].value == 0.78,
            context + " preset macros changed unexpectedly.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto presetRoot = fs::path(drs::engine::getPhase1RuntimeRootPath()) / "preset-state";
        const auto leadPresetPath = presetRoot / "reference" / "lead-performance-state.drpreset.json";
        const auto negativePresetPath = presetRoot / "negative" / "transient-diagnostics-leak.drpreset.json";

        const auto leadPresetJson = readTextFile(leadPresetPath);
        const auto negativePresetJson = readTextFile(negativePresetPath);

        const auto phase2ProjectPath = drs::engine::getPhase2ReferenceProjectManifestPath();
        const auto phase2ProjectLoad = drs::engine::loadRuntimeProjectManifest(phase2ProjectPath);
        require(phase2ProjectLoad.loaded,
                "The authored-project binding test requires the Phase 2 reference project.");

        auto bindingProcessor = std::make_unique<drs::plugin::Processor>();
        require(bindingProcessor->replaceAuthoringProject(
                    phase2ProjectLoad.project,
                    juce::File(juce::String::fromUTF8(phase2ProjectPath.c_str()))),
                "The processor must accept a manifest that matches the authored project.");
        const auto acceptedBinding = bindingProcessor->getAuthoringProjectBinding();
        require(acceptedBinding.projectId == phase2ProjectLoad.project.projectId,
                "The accepted binding must preserve the authored project identity.");
        require(juce::File(juce::String::fromUTF8(acceptedBinding.manifestPath.c_str()))
                    == juce::File(juce::String::fromUTF8(phase2ProjectPath.c_str())),
                "The accepted binding must preserve the resolved manifest path.");
        require(acceptedBinding.manifestDigest
                    == drs::engine::computeHostProjectManifestDigest(
                        phase2ProjectLoad.project,
                        acceptedBinding.manifestPath),
                "The accepted binding must preserve the canonical manifest digest.");

        const auto previousBoundProject = bindingProcessor->getAuthoringSession().getProject();
        const auto previousBinding = bindingProcessor->getAuthoringProjectBinding();
        const auto phase1ProjectPath = drs::engine::getPhase1ReferenceProjectManifestPath();
        require(!bindingProcessor->bindAuthoringProjectFile(
                    juce::File(juce::String::fromUTF8(phase1ProjectPath.c_str()))),
                "A manifest with a different project identity must be rejected.");
        require(bindingProcessor->getAuthoringProjectBinding().manifestPath
                    == previousBinding.manifestPath,
                "A rejected identity mismatch must preserve the previous binding.");

        const auto missingProjectPath = (fs::temp_directory_path()
                                         / "drs-host-state-missing-project.drsproj").string();
        require(!bindingProcessor->bindAuthoringProjectFile(
                    juce::File(juce::String::fromUTF8(missingProjectPath.c_str()))),
                "A missing manifest must be rejected.");
        require(bindingProcessor->getAuthoringProjectBinding().manifestDigest
                    == previousBinding.manifestDigest,
                "A rejected missing manifest must preserve the previous binding.");

        auto mismatchedContent = phase2ProjectLoad.project;
        mismatchedContent.displayName += " mismatched";
        require(!bindingProcessor->replaceAuthoringProject(
                    mismatchedContent,
                    juce::File(juce::String::fromUTF8(phase2ProjectPath.c_str()))),
                "A manifest whose canonical content differs from the candidate project must be rejected.");
        require(bindingProcessor->getAuthoringSession().getProject().displayName
                    == previousBoundProject.displayName,
                "A rejected content mismatch must preserve the authored project.");
        require(bindingProcessor->getAuthoringProjectBinding().manifestDigest
                    == previousBinding.manifestDigest,
                "A rejected content mismatch must preserve the authored-project binding.");

        auto standaloneSource = std::make_unique<drs::standalone::MainComponent>(false);
        const auto standaloneRestore = standaloneSource->restoreStateJson(leadPresetJson);
        require(standaloneRestore.restored, "Standalone shell must restore the lead/performance preset fixture.");
        serviceRestore(standaloneSource->getProcessor(), "Standalone legacy restore");
        require(standaloneSource->getProcessor().waitForHostStatePublication(),
                "Standalone legacy state did not reach background host-state publication.");

        const auto exportedStandaloneState = standaloneSource->exportStateJson();
        const auto parsedStandaloneState = drs::engine::parseHostSessionState(exportedStandaloneState);
        require(parsedStandaloneState.isLegacyPreset(),
                "An unloaded standalone shell must preserve the unbound legacy preset contract.");
        requirePresetMatchesLeadPerformance(*parsedStandaloneState.legacyPreset,
                                            "Standalone unbound state");

        auto standaloneReloaded = std::make_unique<drs::standalone::MainComponent>(false);
        const auto standaloneReload = standaloneReloaded->restoreStateJson(exportedStandaloneState);
        require(standaloneReload.restored, "Standalone shell must restore its exported state.");
        serviceRestore(standaloneReloaded->getProcessor(), "Standalone host-state restore");
        requireSessionMatchesLeadPerformance(standaloneReloaded->getEngineFacade().getCurrentSessionState(),
                                            "Standalone shell");
        require(findMacroDescriptor(standaloneReloaded->getEngineFacade(), "tone").currentEffect == "Balanced attack",
                "Standalone shell tone macro effect did not restore with the lead fixture.");
        require(findMacroDescriptor(standaloneReloaded->getEngineFacade(), "motion").currentEffect == "+7 st",
                "Standalone shell motion macro effect did not restore with the lead fixture.");

        const auto standaloneRejected = standaloneReloaded->restoreStateJson(negativePresetJson);
        require(!standaloneRejected.restored, "Standalone shell must reject the transient-diagnostics leak fixture.");
        requireSessionMatchesLeadPerformance(standaloneReloaded->getEngineFacade().getCurrentSessionState(),
                                            "Standalone shell after rejected restore");

        auto sourceProcessor = std::make_unique<drs::plugin::Processor>();
        sourceProcessor->setStateInformation(leadPresetJson.data(), static_cast<int>(leadPresetJson.size()));
        serviceRestore(*sourceProcessor, "Plugin legacy restore");
        requireSessionMatchesLeadPerformance(sourceProcessor->getEngineFacade().getCurrentSessionState(),
                                            "Plugin processor source state");

        juce::MemoryBlock processorState;
        require(sourceProcessor->waitForHostStatePublication(),
                "Plugin legacy state did not reach the background host-state publication.");
        sourceProcessor->getStateInformation(processorState);
        const auto processorStateText = std::string(
            static_cast<const char*>(processorState.getData()), processorState.getSize());
        const auto parsedProcessorState = drs::engine::parseHostSessionState(processorStateText);
        require(parsedProcessorState.isLegacyPreset(),
                "An unloaded plug-in must preserve the unbound legacy preset contract.");
        requirePresetMatchesLeadPerformance(*parsedProcessorState.legacyPreset,
                                            "Plugin unbound state");

        auto restoredProcessor = std::make_unique<drs::plugin::Processor>();
        restoredProcessor->setStateInformation(processorState.getData(), static_cast<int>(processorState.getSize()));
        serviceRestore(*restoredProcessor, "Plugin host-state restore");
        requireSessionMatchesLeadPerformance(restoredProcessor->getEngineFacade().getCurrentSessionState(),
                                            "Plugin processor");
        require(findMacroDescriptor(restoredProcessor->getEngineFacade(), "tone").currentEffect == "Balanced attack",
                "Plugin tone macro effect did not restore with the lead fixture.");
        require(findMacroDescriptor(restoredProcessor->getEngineFacade(), "motion").currentEffect == "+7 st",
                "Plugin motion macro effect did not restore with the lead fixture.");
        require(restoredProcessor->getEngineFacade().getCurrentSessionState().transientMetrics.lastFailure.empty(),
                "Plugin processor must not retain a restore failure after a valid round-trip.");

        const auto previousPluginState = restoredProcessor->getEngineFacade().exportPresetStateJson();
        restoredProcessor->setStateInformation(negativePresetJson.data(), static_cast<int>(negativePresetJson.size()));
        serviceRestore(*restoredProcessor, "Plugin rejected restore");
        require(restoredProcessor->getEngineFacade().exportPresetStateJson() == previousPluginState,
                "Plugin processor must preserve the previous session state when restore input is invalid.");
        require(restoredProcessor->getProjectRestoreSnapshot()->state
                    == drs::engine::ProjectRestoreState::failed,
                "Plugin processor must publish a typed coordinator failure for invalid state.");

        bindingProcessor->closeAuthoringProject({});
        require(bindingProcessor->getAuthoringProjectFile() == juce::File(),
                "Closing an authored project must clear its validated file binding.");

        drainProcessorBackgroundWork(*restoredProcessor);
        restoredProcessor.reset();
        drainProcessorBackgroundWork(*sourceProcessor);
        sourceProcessor.reset();
        drainProcessorBackgroundWork(standaloneReloaded->getProcessor());
        standaloneReloaded.reset();
        drainProcessorBackgroundWork(standaloneSource->getProcessor());
        standaloneSource.reset();
        drainProcessorBackgroundWork(*bindingProcessor);
        bindingProcessor.reset();

        std::cout << "Phase 1 state recall tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 state recall tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
