#include "drs/engine/RuntimePresetState.h"
#include "drs/engine/PublishedMacroBinding.h"

#include "drs/engine/RuntimeLoadProfile.h"

#include <json/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

constexpr auto phase1PresetSchemaName = "drs.presetState";
constexpr auto phase1PresetSchemaVersion = 1;

bool isPublishedHostRuntimeMacroId(const std::string& macroId)
{
    constexpr std::string_view prefix { "macro." };
    for (const auto& slot : publishedMacroHostTopology())
    {
        const std::string_view hostId { slot.hostParameterId };
        if (hostId.rfind(prefix, 0) == 0
            && macroId == hostId.substr(prefix.size()))
            return true;
    }
    return false;
}

template <typename TResult>
void addIssue(TResult& result, const std::string& issue)
{
    result.issues.push_back(issue);
}

std::string readTextFile(const fs::path& filePath)
{
    std::ifstream input(filePath, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

template <typename TValue, typename TResult>
std::optional<TValue> readRequired(const json& object,
                                   TResult& result,
                                   const char* propertyName,
                                   const char* context)
{
    const auto iterator = object.find(propertyName);

    if (iterator == object.end())
    {
        addIssue(result, std::string(context) + " is missing required field '" + propertyName + "'.");
        return std::nullopt;
    }

    try
    {
        return iterator->get<TValue>();
    }
    catch (const json::exception&)
    {
        addIssue(result, std::string(context) + " has invalid type for field '" + propertyName + "'.");
        return std::nullopt;
    }
}

template <typename TResult>
std::vector<std::string> readOptionalStringArray(const json& object,
                                                 TResult& result,
                                                 const char* propertyName,
                                                 const char* context)
{
    std::vector<std::string> values;
    const auto iterator = object.find(propertyName);

    if (iterator == object.end())
        return values;

    if (!iterator->is_array())
    {
        addIssue(result, std::string(context) + " field '" + propertyName + "' must be an array.");
        return values;
    }

    values.reserve(iterator->size());
    for (const auto& entry : *iterator)
    {
        if (!entry.is_string())
        {
            addIssue(result, std::string(context) + " field '" + propertyName + "' must contain only strings.");
            continue;
        }

        values.push_back(entry.get<std::string>());
    }

    return values;
}

ordered_json serializeStringArray(const std::vector<std::string>& values)
{
    ordered_json array = ordered_json::array();

    for (const auto& value : values)
        array.push_back(value);

    return array;
}

ordered_json serializeMacroValues(const std::vector<RuntimePresetMacroValue>& macroValues)
{
    ordered_json array = ordered_json::array();

    for (const auto& macroValue : macroValues)
    {
        ordered_json macroObject;
        macroObject["id"] = macroValue.id;
        macroObject["value"] = macroValue.value;
        array.push_back(std::move(macroObject));
    }

    return array;
}

ordered_json serializeDspMacroTargets(const std::vector<RuntimePresetDspMacroTarget>& targets)
{
    ordered_json array = ordered_json::array();
    for (const auto& target : targets)
    {
        array.push_back({ { "macroId", target.macroId }, { "dspSlotId", target.dspSlotId },
                          { "dspParameterId", target.dspParameterId } });
    }
    return array;
}

std::optional<std::string> describeForbiddenPresetField(const std::string& fieldName)
{
    static const std::unordered_map<std::string, std::string> forbiddenFields {
        {"compiledStreamAssetPath", "is project content and must stay in the checked-in runtime manifest, not inside preset data."},
        {"sourceProjectPath", "is project content and must stay in the checked-in runtime manifest, not inside preset data."},
        {"streamAssetPath", "is project content and must stay in the checked-in runtime manifest, not inside preset data."},
        {"samplePath", "is project content and must stay in the checked-in runtime manifest, not inside preset data."},
        {"zones", "is project content and must stay in the checked-in runtime manifest, not inside preset data."},
        {"groups", "is project content and must stay in the checked-in runtime manifest, not inside preset data."},
        {"articulations", "is project content and must stay in the checked-in runtime manifest, not inside preset data."},
        {"validationNotes", "belongs to the authored instrument contract, not to user recall state."},
        {"streamingMetrics", "is transient diagnostics data and must not be serialized into preset state."},
        {"pageMissCount", "is transient diagnostics data and must not be serialized into preset state."},
        {"activeVoiceCount", "is transient diagnostics data and must not be serialized into preset state."},
        {"cachedPageCount", "is transient cache telemetry and must not be serialized into preset state."},
        {"integrationState", "is a transient shell diagnostic and must not be serialized into preset state."},
        {"lastFailure", "is a transient failure snapshot and must not be serialized into preset state."}
    };

    const auto iterator = forbiddenFields.find(fieldName);
    if (iterator == forbiddenFields.end())
        return std::nullopt;

    return iterator->second;
}

template <typename TResult>
void validateAllowedFields(const json& object,
                           TResult& result,
                           const std::unordered_set<std::string>& allowedFields,
                           const char* context)
{
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator)
    {
        if (allowedFields.find(iterator.key()) != allowedFields.end())
            continue;

        if (const auto description = describeForbiddenPresetField(iterator.key()); description.has_value())
        {
            addIssue(result, std::string(context) + " field '" + iterator.key() + "' " + *description);
            continue;
        }

        addIssue(result, std::string(context) + " field '" + iterator.key() + "' is not part of the Phase 1 preset-state contract.");
    }
}

std::vector<RuntimePresetMacroValue> readMacroValues(const json& object,
                                                     RuntimePresetStateLoadResult& result)
{
    std::vector<RuntimePresetMacroValue> macroValues;
    const auto iterator = object.find("macroValues");

    if (iterator == object.end())
    {
        addIssue(result, "Preset state is missing required field 'macroValues'.");
        return macroValues;
    }

    if (!iterator->is_array())
    {
        addIssue(result, "Preset state field 'macroValues' must be an array.");
        return macroValues;
    }

    std::unordered_set<std::string> seenMacroIds;
    macroValues.reserve(iterator->size());

    for (std::size_t index = 0; index < iterator->size(); ++index)
    {
        const auto& entry = (*iterator)[index];
        const auto context = "Preset state macroValues[" + std::to_string(index) + "]";

        if (!entry.is_object())
        {
            addIssue(result, context + " must be an object.");
            continue;
        }

        static const std::unordered_set<std::string> allowedFields {
            "id",
            "value"
        };
        validateAllowedFields(entry, result, allowedFields, context.c_str());

        const auto id = readRequired<std::string>(entry, result, "id", context.c_str());
        const auto value = readRequired<double>(entry, result, "value", context.c_str());

        if (!id.has_value() || !value.has_value())
            continue;

        if (!seenMacroIds.insert(*id).second)
        {
            addIssue(result, context + " duplicates macro id '" + *id + "'.");
            continue;
        }

        macroValues.push_back({*id, *value});
    }

    return macroValues;
}

std::vector<RuntimePresetDspMacroTarget> readDspMacroTargets(const json& object,
                                                             RuntimePresetStateLoadResult& result)
{
    std::vector<RuntimePresetDspMacroTarget> targets;
    const auto iterator = object.find("dspMacroTargets");
    if (iterator == object.end())
        return targets;
    if (!iterator->is_array())
    {
        addIssue(result, "Preset state field 'dspMacroTargets' must be an array.");
        return targets;
    }

    std::unordered_set<std::string> seenMacroIds;
    targets.reserve(iterator->size());
    for (std::size_t index = 0; index < iterator->size(); ++index)
    {
        const auto& entry = (*iterator)[index];
        const auto context = "Preset state dspMacroTargets[" + std::to_string(index) + "]";
        if (!entry.is_object())
        {
            addIssue(result, context + " must be an object.");
            continue;
        }
        static const std::unordered_set<std::string> allowed { "macroId", "dspSlotId", "dspParameterId" };
        validateAllowedFields(entry, result, allowed, context.c_str());
        const auto macroId = readRequired<std::string>(entry, result, "macroId", context.c_str());
        const auto slotId = readRequired<std::string>(entry, result, "dspSlotId", context.c_str());
        const auto parameterId = readRequired<std::string>(entry, result, "dspParameterId", context.c_str());
        if (!macroId.has_value() || !slotId.has_value() || !parameterId.has_value())
            continue;
        if (macroId->empty() || slotId->empty() || parameterId->empty()
            || !seenMacroIds.insert(*macroId).second)
        {
            addIssue(result, context + " must contain unique non-empty stable DSP target identities.");
            continue;
        }
        targets.push_back({ *macroId, *slotId, *parameterId });
    }
    return targets;
}

std::string findDefaultArticulationId(const RuntimeInstrumentModel& instrument)
{
    const auto iterator = std::find_if(instrument.articulations.begin(),
                                       instrument.articulations.end(),
                                       [](const RuntimeArticulationDefinition& articulation)
                                       {
                                           return articulation.isDefault;
                                       });

    if (iterator != instrument.articulations.end())
        return iterator->id;

    return instrument.articulations.empty() ? std::string {} : instrument.articulations.front().id;
}
} // namespace

RuntimeSessionStateSnapshot buildDefaultRuntimeSessionState(const RuntimeManifestLoadResult& manifest)
{
    RuntimeSessionStateSnapshot snapshot;
    snapshot.presetId = manifest.instrument.instrumentId + ".default";
    snapshot.targetInstrumentId = manifest.instrument.instrumentId;
    snapshot.targetInstrumentSchemaName = manifest.instrument.schemaName;
    snapshot.targetInstrumentSchemaVersion = manifest.instrument.schemaVersion;
    snapshot.loadProfileId = manifest.instrument.defaultLoadProfile;
    snapshot.selectedArticulationId = findDefaultArticulationId(manifest.instrument);
    snapshot.macroValues.reserve(manifest.instrument.macros.size());

    for (const auto& macro : manifest.instrument.macros)
        snapshot.macroValues.push_back({macro.id, macro.defaultValue});

    return snapshot;
}

RuntimePresetState captureRuntimePresetState(const RuntimeSessionStateSnapshot& snapshot)
{
    RuntimePresetState preset;
    preset.schemaName = phase1PresetSchemaName;
    preset.schemaVersion = phase1PresetSchemaVersion;
    preset.presetId = snapshot.presetId;
    preset.targetInstrumentId = snapshot.targetInstrumentId;
    preset.targetInstrumentSchemaName = snapshot.targetInstrumentSchemaName;
    preset.targetInstrumentSchemaVersion = snapshot.targetInstrumentSchemaVersion;
    preset.loadProfileId = snapshot.loadProfileId;
    preset.selectedArticulationId = snapshot.selectedArticulationId;
    preset.macroValues = snapshot.macroValues;
    preset.dspGraphDigest = snapshot.dspGraphDigest;
    preset.dspMacroTargets = snapshot.dspMacroTargets;
    preset.notes = snapshot.notes;
    return preset;
}

RuntimePresetStateLoadResult parseRuntimePresetState(const std::string& text)
{
    RuntimePresetStateLoadResult result;
    result.state = "Preset state parse failed";

    json root;

    try
    {
        root = json::parse(text);
    }
    catch (const json::exception& exception)
    {
        addIssue(result, std::string("Preset state JSON parse failed: ") + exception.what());
        return result;
    }

    if (!root.is_object())
    {
        addIssue(result, "Preset state root must be an object.");
        return result;
    }

    static const std::unordered_set<std::string> allowedFields {
        "schemaName",
        "schemaVersion",
        "presetId",
        "targetInstrumentId",
        "targetInstrumentSchemaName",
        "targetInstrumentSchemaVersion",
        "loadProfileId",
        "selectedArticulationId",
        "macroValues",
        "dspGraphDigest",
        "dspMacroTargets",
        "notes"
    };
    validateAllowedFields(root, result, allowedFields, "Preset state");

    if (const auto schemaName = readRequired<std::string>(root, result, "schemaName", "Preset state"))
        result.preset.schemaName = *schemaName;

    if (const auto schemaVersion = readRequired<int>(root, result, "schemaVersion", "Preset state"))
        result.preset.schemaVersion = *schemaVersion;

    if (const auto presetId = readRequired<std::string>(root, result, "presetId", "Preset state"))
        result.preset.presetId = *presetId;

    if (const auto targetInstrumentId = readRequired<std::string>(root, result, "targetInstrumentId", "Preset state"))
        result.preset.targetInstrumentId = *targetInstrumentId;

    if (const auto targetInstrumentSchemaName = readRequired<std::string>(root, result, "targetInstrumentSchemaName", "Preset state"))
        result.preset.targetInstrumentSchemaName = *targetInstrumentSchemaName;

    if (const auto targetInstrumentSchemaVersion = readRequired<int>(root, result, "targetInstrumentSchemaVersion", "Preset state"))
        result.preset.targetInstrumentSchemaVersion = *targetInstrumentSchemaVersion;

    if (const auto loadProfileId = readRequired<std::string>(root, result, "loadProfileId", "Preset state"))
        result.preset.loadProfileId = *loadProfileId;

    if (const auto selectedArticulationId = readRequired<std::string>(root, result, "selectedArticulationId", "Preset state"))
        result.preset.selectedArticulationId = *selectedArticulationId;

    result.preset.macroValues = readMacroValues(root, result);
    if (const auto digest = root.find("dspGraphDigest"); digest != root.end())
    {
        if (!digest->is_string())
            addIssue(result, "Preset state field 'dspGraphDigest' must be a string.");
        else
            result.preset.dspGraphDigest = digest->get<std::string>();
    }
    result.preset.dspMacroTargets = readDspMacroTargets(root, result);
    result.preset.notes = readOptionalStringArray(root, result, "notes", "Preset state");

    if (result.preset.schemaName != phase1PresetSchemaName)
        addIssue(result, "Preset state schemaName must be 'drs.presetState'.");

    if (result.preset.schemaVersion != phase1PresetSchemaVersion)
        addIssue(result, "Preset state schemaVersion must be 1 for the Phase 1 contract.");

    if (result.preset.presetId.empty())
        addIssue(result, "Preset state presetId must not be empty.");

    if (result.preset.targetInstrumentId.empty())
        addIssue(result, "Preset state targetInstrumentId must not be empty.");

    if (result.preset.targetInstrumentSchemaName.empty())
        addIssue(result, "Preset state targetInstrumentSchemaName must not be empty.");

    if (result.preset.loadProfileId.empty())
        addIssue(result, "Preset state loadProfileId must not be empty.");

    if (result.preset.selectedArticulationId.empty())
        addIssue(result, "Preset state selectedArticulationId must not be empty.");

    if (result.issues.empty())
    {
        result.loaded = true;
        result.state = "Preset state loaded";
    }

    return result;
}

RuntimePresetStateLoadResult loadRuntimePresetStateFile(const std::string& path)
{
    RuntimePresetStateLoadResult result;
    result.state = "Preset state file load failed";

    const fs::path filePath(path);
    std::error_code errorCode;

    if (!fs::exists(filePath, errorCode))
    {
        addIssue(result, "Preset state file was not found at " + filePath.generic_string() + ".");
        return result;
    }

    result = parseRuntimePresetState(readTextFile(filePath));
    if (!result.loaded && result.issues.empty())
        addIssue(result, "Preset state file could not be parsed.");

    return result;
}

RuntimePresetStateValidationResult validateRuntimePresetState(const RuntimePresetState& preset,
                                                              const RuntimeInstrumentModel& instrument)
{
    RuntimePresetStateValidationResult result;
    result.state = "Preset state validation failed";

    if (preset.targetInstrumentId != instrument.instrumentId)
        addIssue(result, "Preset state targetInstrumentId does not match the loaded instrument.");

    if (preset.targetInstrumentSchemaName != instrument.schemaName)
        addIssue(result, "Preset state targetInstrumentSchemaName does not match the loaded instrument schema.");

    if (preset.targetInstrumentSchemaVersion != instrument.schemaVersion)
        addIssue(result, "Preset state targetInstrumentSchemaVersion does not match the loaded instrument schema version.");

    if (!findPhase1RuntimeLoadProfile(preset.loadProfileId).has_value())
        addIssue(result, "Preset state references unknown load profile '" + preset.loadProfileId + "'.");

    const auto articulationIterator = std::find_if(instrument.articulations.begin(),
                                                   instrument.articulations.end(),
                                                   [&](const RuntimeArticulationDefinition& articulation)
                                                   {
                                                       return articulation.id == preset.selectedArticulationId;
                                                   });
    if (articulationIterator == instrument.articulations.end())
        addIssue(result, "Preset state references unknown articulation '" + preset.selectedArticulationId + "'.");

    std::unordered_map<std::string, const RuntimeMacroDefinition*> macroDefinitions;
    macroDefinitions.reserve(instrument.macros.size());

    for (const auto& macro : instrument.macros)
        macroDefinitions.emplace(macro.id, &macro);

    std::unordered_set<std::string> seenMacroIds;
    for (const auto& macroValue : preset.macroValues)
    {
        if (!seenMacroIds.insert(macroValue.id).second)
        {
            addIssue(result, "Preset state duplicates macro id '" + macroValue.id + "'.");
            continue;
        }

        const auto definitionIterator = macroDefinitions.find(macroValue.id);
        if (definitionIterator == macroDefinitions.end())
        {
            if (!isPublishedHostRuntimeMacroId(macroValue.id))
                addIssue(result, "Preset state references unknown macro id '" + macroValue.id + "'.");
            else if (macroValue.value < 0.0 || macroValue.value > 1.0)
                addIssue(result, "Preset state published host macro '" + macroValue.id
                                 + "' is outside the normalized range.");
            continue;
        }

        const auto& definition = *definitionIterator->second;
        if (macroValue.value < definition.minValue || macroValue.value > definition.maxValue)
        {
            addIssue(result, "Preset state macro '" + macroValue.id + "' is outside the authored range.");
        }
    }

    for (const auto& macro : instrument.macros)
    {
        if (seenMacroIds.find(macro.id) == seenMacroIds.end())
            addIssue(result, "Preset state is missing macro id '" + macro.id + "'.");
    }

    if (result.issues.empty())
    {
        result.valid = true;
        result.state = "Preset state valid";
    }

    return result;
}

std::string serializeRuntimePresetState(const RuntimePresetState& preset)
{
    ordered_json root;
    root["schemaName"] = preset.schemaName.empty() ? phase1PresetSchemaName : preset.schemaName;
    root["schemaVersion"] = preset.schemaVersion == 0 ? phase1PresetSchemaVersion : preset.schemaVersion;
    root["presetId"] = preset.presetId;
    root["targetInstrumentId"] = preset.targetInstrumentId;
    root["targetInstrumentSchemaName"] = preset.targetInstrumentSchemaName;
    root["targetInstrumentSchemaVersion"] = preset.targetInstrumentSchemaVersion;
    root["loadProfileId"] = preset.loadProfileId;
    root["selectedArticulationId"] = preset.selectedArticulationId;
    root["macroValues"] = serializeMacroValues(preset.macroValues);
    if (!preset.dspGraphDigest.empty())
        root["dspGraphDigest"] = preset.dspGraphDigest;
    if (!preset.dspMacroTargets.empty())
        root["dspMacroTargets"] = serializeDspMacroTargets(preset.dspMacroTargets);
    root["notes"] = serializeStringArray(preset.notes);
    return root.dump(2) + "\n";
}
} // namespace drs::engine
