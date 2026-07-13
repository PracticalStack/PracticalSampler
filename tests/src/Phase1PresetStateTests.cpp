#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimePresetState.h"

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

bool containsText(const std::vector<std::string>& issues, const std::string& needle)
{
    for (const auto& issue : issues)
    {
        if (issue.find(needle) != std::string::npos)
            return true;
    }

    return false;
}
} // namespace

int main()
{
    try
    {
        const auto instrumentResult = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(instrumentResult.loaded, "Reference Phase 1 instrument must load before preset-state tests run.");

        const auto presetRoot = fs::path(drs::engine::getPhase1RuntimeRootPath()) / "preset-state";
        const auto defaultPresetPath = presetRoot / "reference" / "default-state.drpreset.json";
        const auto leadPresetPath = presetRoot / "reference" / "lead-performance-state.drpreset.json";
        const auto negativePresetPath = presetRoot / "negative" / "transient-diagnostics-leak.drpreset.json";

        const auto defaultPreset = drs::engine::loadRuntimePresetStateFile(defaultPresetPath.generic_string());
        require(defaultPreset.loaded, "Default preset-state fixture must load cleanly.");
        require(defaultPreset.preset.selectedArticulationId == "sustain",
                "Default preset-state fixture articulation changed unexpectedly.");
        require(defaultPreset.preset.loadProfileId == "balanced",
                "Default preset-state fixture load profile changed unexpectedly.");
        require(defaultPreset.preset.macroValues.size() == 2,
                "Default preset-state fixture macro count changed unexpectedly.");
        require(drs::engine::serializeRuntimePresetState(defaultPreset.preset) == readTextFile(defaultPresetPath),
                "Default preset-state fixture did not round-trip back to the checked-in golden file.");

        const auto defaultValidation = drs::engine::validateRuntimePresetState(defaultPreset.preset,
                                                                               instrumentResult.instrument);
        require(defaultValidation.valid, "Default preset-state fixture must validate against the reference instrument.");

        const auto leadPreset = drs::engine::loadRuntimePresetStateFile(leadPresetPath.generic_string());
        require(leadPreset.loaded, "Lead/performance preset-state fixture must load cleanly.");
        require(leadPreset.preset.selectedArticulationId == "lead",
                "Lead/performance preset-state fixture articulation changed unexpectedly.");
        require(leadPreset.preset.loadProfileId == "performance",
                "Lead/performance preset-state fixture load profile changed unexpectedly.");
        require(drs::engine::serializeRuntimePresetState(leadPreset.preset) == readTextFile(leadPresetPath),
                "Lead/performance preset-state fixture did not round-trip back to the checked-in golden file.");

        const auto leadValidation = drs::engine::validateRuntimePresetState(leadPreset.preset,
                                                                            instrumentResult.instrument);
        require(leadValidation.valid,
                "Lead/performance preset-state fixture must validate against the reference instrument.");

        const auto negativePreset = drs::engine::loadRuntimePresetStateFile(negativePresetPath.generic_string());
        require(!negativePreset.loaded, "Transient-diagnostics leak fixture must fail validation.");
        require(containsText(negativePreset.issues, "streamingMetrics"),
                "Transient-diagnostics leak fixture must report that streaming metrics are not persistent.");
        require(containsText(negativePreset.issues, "compiledStreamAssetPath"),
                "Transient-diagnostics leak fixture must report that compiled stream asset paths are project content.");

        auto capturedState = drs::engine::buildDefaultRuntimeSessionState(instrumentResult);
        capturedState.presetId = "drs.phase1.tiny-open-instrument.capture";
        capturedState.loadProfileId = "performance";
        capturedState.selectedArticulationId = "lead";
        capturedState.macroValues[0].value = 0.62;
        capturedState.macroValues[1].value = 0.78;
        capturedState.transientMetrics.pageMissCount = 17;
        capturedState.transientMetrics.activeVoiceCount = 3;
        capturedState.transientMetrics.cachedPageCount = 9;
        capturedState.transientMetrics.integrationState = "streaming warm";
        capturedState.transientMetrics.lastFailure = "page miss recovered";

        const auto capturedPreset = drs::engine::captureRuntimePresetState(capturedState);
        const auto serializedCapturedPreset = drs::engine::serializeRuntimePresetState(capturedPreset);

        require(serializedCapturedPreset.find("pageMissCount") == std::string::npos,
                "Captured preset state must not leak transient page-miss counters.");
        require(serializedCapturedPreset.find("activeVoiceCount") == std::string::npos,
                "Captured preset state must not leak transient active-voice counters.");
        require(serializedCapturedPreset.find("cachedPageCount") == std::string::npos,
                "Captured preset state must not leak transient cache counters.");
        require(serializedCapturedPreset.find("lastFailure") == std::string::npos,
                "Captured preset state must not leak transient failure strings.");

        const auto parsedCapturedPreset = drs::engine::parseRuntimePresetState(serializedCapturedPreset);
        require(parsedCapturedPreset.loaded, "Captured preset state must parse cleanly after serialization.");
        require(parsedCapturedPreset.preset.loadProfileId == "performance",
                "Captured preset state load profile did not round-trip.");
        require(parsedCapturedPreset.preset.selectedArticulationId == "lead",
                "Captured preset state articulation did not round-trip.");

        const auto capturedValidation = drs::engine::validateRuntimePresetState(parsedCapturedPreset.preset,
                                                                                instrumentResult.instrument);
        require(capturedValidation.valid, "Captured preset state must validate against the reference instrument.");

        std::cout << "Phase 1 preset-state tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 preset-state tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
