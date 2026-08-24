#pragma once

#include "drs/engine/RuntimeModel.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct RuntimePresetMacroValue
{
    std::string id;
    double value = 0.0;
};

struct RuntimePresetInstrumentControlValue
{
    std::string id;
    double normalizedValue = 0.0;
};

struct RuntimePresetDspMacroTarget
{
    std::string macroId;
    std::string dspSlotId;
    std::string dspParameterId;
};

struct RuntimePresetState
{
    std::string schemaName;
    int schemaVersion = 0;
    std::string presetId;
    std::string targetInstrumentId;
    std::string targetInstrumentSchemaName;
    int targetInstrumentSchemaVersion = 0;
    std::string loadProfileId;
    std::string selectedArticulationId;
    std::vector<RuntimePresetMacroValue> macroValues;
    std::vector<RuntimePresetInstrumentControlValue> instrumentControlValues;
    std::string dspGraphDigest;
    std::vector<RuntimePresetDspMacroTarget> dspMacroTargets;
    std::vector<std::string> notes;
};

struct RuntimeStateTransientMetrics
{
    std::uint64_t pageMissCount = 0;
    std::uint64_t activeVoiceCount = 0;
    std::size_t cachedPageCount = 0;
    std::string integrationState;
    std::string lastFailure;
};

struct RuntimeSessionStateSnapshot
{
    std::string presetId;
    std::string targetInstrumentId;
    std::string targetInstrumentSchemaName;
    int targetInstrumentSchemaVersion = 0;
    std::string loadProfileId;
    std::string selectedArticulationId;
    std::vector<RuntimePresetMacroValue> macroValues;
    std::vector<RuntimePresetInstrumentControlValue> instrumentControlValues;
    std::string dspGraphDigest;
    std::vector<RuntimePresetDspMacroTarget> dspMacroTargets;
    std::vector<std::string> notes;
    RuntimeStateTransientMetrics transientMetrics;
};

struct RuntimePresetStateLoadResult
{
    bool loaded = false;
    std::string state;
    std::vector<std::string> issues;
    RuntimePresetState preset;
};

struct RuntimePresetStateValidationResult
{
    bool valid = false;
    std::string state;
    std::vector<std::string> issues;
};

RuntimeSessionStateSnapshot buildDefaultRuntimeSessionState(const RuntimeManifestLoadResult& manifest);
RuntimePresetState captureRuntimePresetState(const RuntimeSessionStateSnapshot& snapshot);
RuntimePresetStateLoadResult parseRuntimePresetState(const std::string& text);
RuntimePresetStateLoadResult loadRuntimePresetStateFile(const std::string& path);
RuntimePresetStateValidationResult validateRuntimePresetState(const RuntimePresetState& preset,
                                                              const RuntimeInstrumentModel& instrument);
std::string serializeRuntimePresetState(const RuntimePresetState& preset);
} // namespace drs::engine
