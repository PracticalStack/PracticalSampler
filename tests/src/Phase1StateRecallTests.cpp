#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"
#include "drs/engine/RuntimeLoader.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
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

        drs::standalone::MainComponent standaloneSource;
        const auto standaloneRestore = standaloneSource.restoreStateJson(leadPresetJson);
        require(standaloneRestore.restored, "Standalone shell must restore the lead/performance preset fixture.");

        const auto exportedStandaloneState = standaloneSource.exportStateJson();
        require(exportedStandaloneState == leadPresetJson,
                "Standalone shell exported state must match the checked-in lead/performance preset fixture.");

        drs::standalone::MainComponent standaloneReloaded;
        const auto standaloneReload = standaloneReloaded.restoreStateJson(exportedStandaloneState);
        require(standaloneReload.restored, "Standalone shell must restore its exported state.");
        requireSessionMatchesLeadPerformance(standaloneReloaded.getEngineFacade().getCurrentSessionState(),
                                            "Standalone shell");
        require(findMacroDescriptor(standaloneReloaded.getEngineFacade(), "tone").currentEffect == "Balanced attack",
                "Standalone shell tone macro effect did not restore with the lead fixture.");
        require(findMacroDescriptor(standaloneReloaded.getEngineFacade(), "motion").currentEffect == "+7 st",
                "Standalone shell motion macro effect did not restore with the lead fixture.");

        const auto standaloneRejected = standaloneReloaded.restoreStateJson(negativePresetJson);
        require(!standaloneRejected.restored, "Standalone shell must reject the transient-diagnostics leak fixture.");
        requireSessionMatchesLeadPerformance(standaloneReloaded.getEngineFacade().getCurrentSessionState(),
                                            "Standalone shell after rejected restore");

        drs::plugin::Processor sourceProcessor;
        sourceProcessor.setStateInformation(leadPresetJson.data(), static_cast<int>(leadPresetJson.size()));
        requireSessionMatchesLeadPerformance(sourceProcessor.getEngineFacade().getCurrentSessionState(),
                                            "Plugin processor source state");

        juce::MemoryBlock processorState;
        sourceProcessor.getStateInformation(processorState);
        require(processorState.getSize() == leadPresetJson.size(),
                "Plugin processor state chunk size must match the serialized preset fixture size.");
        require(std::string(static_cast<const char*>(processorState.getData()), processorState.getSize()) == leadPresetJson,
                "Plugin processor exported state must match the checked-in lead/performance preset fixture.");

        drs::plugin::Processor restoredProcessor;
        restoredProcessor.setStateInformation(processorState.getData(), static_cast<int>(processorState.getSize()));
        requireSessionMatchesLeadPerformance(restoredProcessor.getEngineFacade().getCurrentSessionState(),
                                            "Plugin processor");
        require(findMacroDescriptor(restoredProcessor.getEngineFacade(), "tone").currentEffect == "Balanced attack",
                "Plugin tone macro effect did not restore with the lead fixture.");
        require(findMacroDescriptor(restoredProcessor.getEngineFacade(), "motion").currentEffect == "+7 st",
                "Plugin motion macro effect did not restore with the lead fixture.");
        require(restoredProcessor.getEngineFacade().getCurrentSessionState().transientMetrics.lastFailure.empty(),
                "Plugin processor must not retain a restore failure after a valid round-trip.");

        const auto previousPluginState = restoredProcessor.getEngineFacade().exportPresetStateJson();
        restoredProcessor.setStateInformation(negativePresetJson.data(), static_cast<int>(negativePresetJson.size()));
        require(restoredProcessor.getEngineFacade().exportPresetStateJson() == previousPluginState,
                "Plugin processor must preserve the previous session state when restore input is invalid.");
        require(!restoredProcessor.getEngineFacade().getCurrentSessionState().transientMetrics.lastFailure.empty(),
                "Plugin processor must record a restore failure when invalid state is supplied.");

        std::cout << "Phase 1 state recall tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 state recall tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
