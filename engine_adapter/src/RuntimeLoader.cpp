#include "drs/engine/RuntimeLoader.h"

#include "drs/engine/WorkspacePaths.generated.h"

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
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

template <typename TResult>
void addIssue(TResult& result, const std::string& issue)
{
    result.issues.push_back(issue);
}

std::string toDisplayPath(const fs::path& path)
{
    return path.lexically_normal().generic_string();
}

std::string readTextFile(const fs::path& filePath)
{
    std::ifstream input(filePath, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

fs::path resolveRelativePath(const fs::path& manifestPath, const std::string& rawPath)
{
    const fs::path candidate(rawPath);

    if (candidate.is_absolute())
        return candidate.lexically_normal();

    return (manifestPath.parent_path() / candidate).lexically_normal();
}

std::string toManifestRelativePath(const fs::path& manifestPath, const std::string& storedPath)
{
    const fs::path candidate(storedPath);

    if (!candidate.is_absolute())
        return candidate.generic_string();

    const auto relativePath = candidate.lexically_relative(manifestPath.parent_path());

    if (!relativePath.empty())
        return relativePath.generic_string();

    return candidate.generic_string();
}

template <typename TResult>
std::optional<fs::path> validateRequiredFile(TResult& result,
                                             const fs::path& manifestPath,
                                             const std::string& rawPath,
                                             const char* context)
{
    const auto resolvedPath = resolveRelativePath(manifestPath, rawPath);

    std::error_code errorCode;
    if (!fs::exists(resolvedPath, errorCode))
    {
        addIssue(result, std::string(context) + " does not exist: " + toDisplayPath(resolvedPath));
        return std::nullopt;
    }

    return resolvedPath;
}

template <typename TResult>
std::optional<fs::path> validateRequiredDirectory(TResult& result,
                                                  const fs::path& manifestPath,
                                                  const std::string& rawPath,
                                                  const char* context)
{
    const auto resolvedPath = resolveRelativePath(manifestPath, rawPath);

    std::error_code errorCode;
    if (!fs::exists(resolvedPath, errorCode) || !fs::is_directory(resolvedPath, errorCode))
    {
        addIssue(result, std::string(context) + " directory does not exist: " + toDisplayPath(resolvedPath));
        return std::nullopt;
    }

    return resolvedPath;
}

template <typename TResult, typename TValue>
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

template <typename TResult, typename TValue>
std::optional<TValue> readOptional(const json& object,
                                   TResult& result,
                                   const char* propertyName,
                                   const char* context)
{
    const auto iterator = object.find(propertyName);

    if (iterator == object.end())
        return std::nullopt;

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
std::vector<std::string> readRequiredStringArray(const json& object,
                                                 TResult& result,
                                                 const char* propertyName,
                                                 const char* context)
{
    std::vector<std::string> values;

    const auto iterator = object.find(propertyName);
    if (iterator == object.end())
    {
        addIssue(result, std::string(context) + " is missing required field '" + propertyName + "'.");
        return values;
    }

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

bool isObjectArray(const json& value)
{
    return value.is_array()
        && std::all_of(value.begin(), value.end(), [](const auto& entry) { return entry.is_object(); });
}

ordered_json serializeStringArray(const std::vector<std::string>& values)
{
    ordered_json array = ordered_json::array();

    for (const auto& value : values)
        array.push_back(value);

    return array;
}

template <typename TItem>
bool hasDuplicateIds(const std::vector<TItem>& items)
{
    std::unordered_set<std::string> ids;

    for (const auto& item : items)
    {
        if (item.id.empty())
            continue;

        if (!ids.insert(item.id).second)
            return true;
    }

    return false;
}

RuntimeProjectAuthoringState buildDefaultPhase2AuthoringState()
{
    RuntimeProjectAuthoringState authoring;
    authoring.schemaName = "drs.authoring";
    authoring.schemaVersion = 1;
    return authoring;
}

ordered_json serializeMacroTargets(const std::vector<RuntimeProjectMacroTargetDefinition>& targets)
{
    ordered_json array = ordered_json::array();

    for (const auto& target : targets)
    {
        ordered_json targetObject;
        targetObject["parameterId"] = target.parameterId;
        targetObject["parameterPath"] = target.parameterPath;
        targetObject["role"] = target.role;
        array.push_back(std::move(targetObject));
    }

    return array;
}

ordered_json serializeProjectMacros(const std::vector<RuntimeProjectMacroDefinition>& macros)
{
    ordered_json array = ordered_json::array();

    for (const auto& macro : macros)
    {
        ordered_json macroObject;
        macroObject["id"] = macro.id;
        macroObject["name"] = macro.name;
        macroObject["defaultValue"] = macro.defaultValue;
        macroObject["minValue"] = macro.minValue;
        macroObject["maxValue"] = macro.maxValue;
        macroObject["targets"] = serializeMacroTargets(macro.targets);
        array.push_back(std::move(macroObject));
    }

    return array;
}

ordered_json serializeProjectZones(const std::vector<RuntimeProjectZoneDefinition>& zones)
{
    ordered_json array = ordered_json::array();

    for (const auto& zone : zones)
    {
        ordered_json zoneObject;
        zoneObject["id"] = zone.id;
        zoneObject["sampleSourceId"] = zone.sampleSourceId;
        zoneObject["displayName"] = zone.displayName;
        zoneObject["groupId"] = zone.groupId;
        zoneObject["articulationId"] = zone.articulationId;
        zoneObject["rootKey"] = zone.rootKey;
        zoneObject["keyLow"] = zone.keyLow;
        zoneObject["keyHigh"] = zone.keyHigh;
        zoneObject["velocityLow"] = zone.velocityLow;
        zoneObject["velocityHigh"] = zone.velocityHigh;
        zoneObject["gainDb"] = zone.gainDb;
        zoneObject["pan"] = zone.pan;
        zoneObject["sampleStartFrame"] = zone.sampleStartFrame;
        zoneObject["loopEnabled"] = zone.loopEnabled;
        zoneObject["loopStartFrame"] = zone.loopStartFrame;
        zoneObject["loopEndFrame"] = zone.loopEndFrame;
        array.push_back(std::move(zoneObject));
    }

    return array;
}

ordered_json serializeFxSlots(const std::vector<RuntimeProjectFxSlotDefinition>& fxSlots)
{
    ordered_json array = ordered_json::array();

    for (const auto& fxSlot : fxSlots)
    {
        ordered_json fxObject;
        fxObject["id"] = fxSlot.id;
        fxObject["displayName"] = fxSlot.displayName;
        fxObject["effectType"] = fxSlot.effectType;
        fxObject["bypassed"] = fxSlot.bypassed;
        array.push_back(std::move(fxObject));
    }

    return array;
}

ordered_json serializeRoutingBuses(const std::vector<RuntimeProjectRoutingBusDefinition>& routingBuses)
{
    ordered_json array = ordered_json::array();

    for (const auto& bus : routingBuses)
    {
        ordered_json busObject;
        busObject["id"] = bus.id;
        busObject["displayName"] = bus.displayName;
        busObject["inputSourceId"] = bus.inputSourceId;
        busObject["fxSlotIds"] = serializeStringArray(bus.fxSlotIds);
        array.push_back(std::move(busObject));
    }

    return array;
}

ordered_json serializeTriggerSlots(const std::vector<RuntimeProjectTriggerSlotDefinition>& triggerSlots)
{
    ordered_json array = ordered_json::array();

    for (const auto& slot : triggerSlots)
    {
        ordered_json slotObject;
        slotObject["id"] = slot.id;
        slotObject["displayName"] = slot.displayName;
        slotObject["triggerEvent"] = slot.triggerEvent;
        slotObject["targetArticulationId"] = slot.targetArticulationId;
        array.push_back(std::move(slotObject));
    }

    return array;
}

ordered_json serializePerformanceBanks(const std::vector<RuntimeProjectPerformanceBankDefinition>& banks)
{
    ordered_json array = ordered_json::array();

    for (const auto& bank : banks)
    {
        ordered_json bankObject;
        bankObject["id"] = bank.id;
        bankObject["displayName"] = bank.displayName;
        bankObject["triggerSlots"] = serializeTriggerSlots(bank.triggerSlots);
        bankObject["notes"] = serializeStringArray(bank.notes);
        array.push_back(std::move(bankObject));
    }

    return array;
}
} // namespace

std::string getPhase1RuntimeRootPath()
{
    return generated::workspacePhase1RuntimeRoot;
}

std::string getPhase1ReferenceCorpusIndexPath()
{
    return generated::workspacePhase1ReferenceCorpusIndex;
}

std::string getPhase1ReferenceBenchmarkScenePath()
{
    return generated::workspacePhase1ReferenceBenchmarkScene;
}

std::string getPhase1ReferenceBaselinePath()
{
    return generated::workspacePhase1ReferenceBaseline;
}

std::string getPhase1ReferenceProjectManifestPath()
{
    return generated::workspacePhase1ReferenceProject;
}

std::string getPhase1ReferenceInstrumentManifestPath()
{
    return generated::workspacePhase1ReferenceManifest;
}

std::string getPhase1ReferencePackageManifestPath()
{
    return generated::workspacePhase1ReferencePackageManifest;
}

std::string getPhase2RuntimeRootPath()
{
    return generated::workspacePhase2RuntimeRoot;
}

std::string getPhase2ReferenceProjectManifestPath()
{
    return generated::workspacePhase2ReferenceProject;
}

RuntimeProjectLoadResult loadRuntimeProjectManifest(const std::string& manifestPath)
{
    RuntimeProjectLoadResult result;
    result.manifestPath = manifestPath;
    result.state = "Project load not attempted";

    const fs::path manifestFsPath(manifestPath);
    std::error_code errorCode;

    if (!fs::exists(manifestFsPath, errorCode))
    {
        result.state = "Project missing";
        addIssue(result, "Project file was not found at " + manifestPath + ".");
        return result;
    }

    result.manifestFound = true;

    const auto rawText = readTextFile(manifestFsPath);
    if (rawText.empty())
    {
        result.state = "Project unreadable";
        addIssue(result, "Project file was empty or unreadable.");
        return result;
    }

    json root;
    try
    {
        root = json::parse(rawText);
    }
    catch (const json::exception& exception)
    {
        result.state = "Project parse failed";
        addIssue(result, "Project JSON parse failed: " + std::string(exception.what()));
        return result;
    }

    if (!root.is_object())
    {
        result.state = "Project root invalid";
        addIssue(result, "Project root must be a JSON object.");
        return result;
    }

    auto& project = result.project;

    if (const auto schemaName = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "schemaName", "Project"))
        project.schemaName = *schemaName;

    if (const auto schemaVersion = readRequired<RuntimeProjectLoadResult, int>(root, result, "schemaVersion", "Project"))
        project.schemaVersion = *schemaVersion;

    if (const auto projectId = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "projectId", "Project"))
        project.projectId = *projectId;

    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "displayName", "Project"))
        project.displayName = *displayName;

    if (const auto contentRoot = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "contentRoot", "Project"))
    {
        const auto resolved = validateRequiredDirectory(result, manifestFsPath, *contentRoot, "Project content root");
        project.contentRootPath = resolved ? toDisplayPath(*resolved) : *contentRoot;
    }

    if (const auto defaultInstrument = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "defaultInstrumentManifest", "Project"))
    {
        const auto resolved = validateRequiredFile(result, manifestFsPath, *defaultInstrument, "Default instrument manifest");
        project.defaultInstrumentManifestPath = resolved ? toDisplayPath(*resolved) : *defaultInstrument;
    }

    const auto sampleSourcesIterator = root.find("sampleSources");
    if (sampleSourcesIterator == root.end() || !isObjectArray(*sampleSourcesIterator))
    {
        addIssue(result, "Project field 'sampleSources' must be an array of objects.");
    }
    else
    {
        project.sampleSources.reserve(sampleSourcesIterator->size());

        for (std::size_t index = 0; index < sampleSourcesIterator->size(); ++index)
        {
            const auto& sampleObject = sampleSourcesIterator->at(index);
            const auto context = "SampleSource[" + std::to_string(index) + "]";
            RuntimeProjectSampleSource sampleSource;

            if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(sampleObject, result, "id", context.c_str()))
                sampleSource.id = *id;

            if (const auto path = readRequired<RuntimeProjectLoadResult, std::string>(sampleObject, result, "path", context.c_str()))
            {
                const auto resolved = validateRequiredFile(result, manifestFsPath, *path, "Sample source");
                sampleSource.path = resolved ? toDisplayPath(*resolved) : *path;
            }

            if (const auto role = readRequired<RuntimeProjectLoadResult, std::string>(sampleObject, result, "role", context.c_str()))
                sampleSource.role = *role;

            project.sampleSources.push_back(std::move(sampleSource));
        }
    }

    project.notes = readRequiredStringArray(root, result, "notes", "Project");

    if (project.schemaVersion == 2)
    {
        const auto authoringIterator = root.find("authoring");
        if (authoringIterator == root.end() || !authoringIterator->is_object())
        {
            addIssue(result, "Project schemaVersion 2 requires an 'authoring' object.");
        }
        else
        {
            auto& authoring = project.authoring;

            if (const auto schemaName = readRequired<RuntimeProjectLoadResult, std::string>(*authoringIterator, result, "schemaName", "Project authoring"))
                authoring.schemaName = *schemaName;

            if (const auto schemaVersion = readRequired<RuntimeProjectLoadResult, int>(*authoringIterator, result, "schemaVersion", "Project authoring"))
                authoring.schemaVersion = *schemaVersion;

            if (const auto selectedZoneId = readOptional<RuntimeProjectLoadResult, std::string>(*authoringIterator, result, "selectedZoneId", "Project authoring"))
                authoring.selectedZoneId = *selectedZoneId;

            if (const auto selectedPerformanceBankId = readOptional<RuntimeProjectLoadResult, std::string>(*authoringIterator, result, "selectedPerformanceBankId", "Project authoring"))
                authoring.selectedPerformanceBankId = *selectedPerformanceBankId;

            const auto zonesIterator = authoringIterator->find("zones");
            if (zonesIterator == authoringIterator->end() || !isObjectArray(*zonesIterator))
            {
                addIssue(result, "Project authoring field 'zones' must be an array of objects.");
            }
            else
            {
                authoring.zones.reserve(zonesIterator->size());

                for (std::size_t index = 0; index < zonesIterator->size(); ++index)
                {
                    const auto& zoneObject = zonesIterator->at(index);
                    const auto context = "ProjectZone[" + std::to_string(index) + "]";
                    RuntimeProjectZoneDefinition zone;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "id", context.c_str()))
                        zone.id = *id;
                    if (const auto sampleSourceId = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "sampleSourceId", context.c_str()))
                        zone.sampleSourceId = *sampleSourceId;
                    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "displayName", context.c_str()))
                        zone.displayName = *displayName;
                    if (const auto groupId = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "groupId", context.c_str()))
                        zone.groupId = *groupId;
                    if (const auto articulationId = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "articulationId", context.c_str()))
                        zone.articulationId = *articulationId;
                    if (const auto rootKey = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "rootKey", context.c_str()))
                        zone.rootKey = *rootKey;
                    if (const auto keyLow = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "keyLow", context.c_str()))
                        zone.keyLow = *keyLow;
                    if (const auto keyHigh = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "keyHigh", context.c_str()))
                        zone.keyHigh = *keyHigh;
                    if (const auto velocityLow = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "velocityLow", context.c_str()))
                        zone.velocityLow = *velocityLow;
                    if (const auto velocityHigh = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "velocityHigh", context.c_str()))
                        zone.velocityHigh = *velocityHigh;
                    if (const auto gainDb = readRequired<RuntimeProjectLoadResult, double>(zoneObject, result, "gainDb", context.c_str()))
                        zone.gainDb = *gainDb;
                    if (const auto pan = readRequired<RuntimeProjectLoadResult, double>(zoneObject, result, "pan", context.c_str()))
                        zone.pan = *pan;
                    if (const auto sampleStartFrame = readRequired<RuntimeProjectLoadResult, std::uint64_t>(zoneObject, result, "sampleStartFrame", context.c_str()))
                        zone.sampleStartFrame = *sampleStartFrame;
                    if (const auto loopEnabled = readRequired<RuntimeProjectLoadResult, bool>(zoneObject, result, "loopEnabled", context.c_str()))
                        zone.loopEnabled = *loopEnabled;
                    if (const auto loopStartFrame = readRequired<RuntimeProjectLoadResult, std::uint64_t>(zoneObject, result, "loopStartFrame", context.c_str()))
                        zone.loopStartFrame = *loopStartFrame;
                    if (const auto loopEndFrame = readRequired<RuntimeProjectLoadResult, std::uint64_t>(zoneObject, result, "loopEndFrame", context.c_str()))
                        zone.loopEndFrame = *loopEndFrame;

                    authoring.zones.push_back(std::move(zone));
                }
            }

            const auto macrosIterator = authoringIterator->find("macros");
            if (macrosIterator == authoringIterator->end() || !isObjectArray(*macrosIterator))
            {
                addIssue(result, "Project authoring field 'macros' must be an array of objects.");
            }
            else
            {
                authoring.macros.reserve(macrosIterator->size());

                for (std::size_t index = 0; index < macrosIterator->size(); ++index)
                {
                    const auto& macroObject = macrosIterator->at(index);
                    const auto context = "ProjectMacro[" + std::to_string(index) + "]";
                    RuntimeProjectMacroDefinition macro;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(macroObject, result, "id", context.c_str()))
                        macro.id = *id;
                    if (const auto name = readRequired<RuntimeProjectLoadResult, std::string>(macroObject, result, "name", context.c_str()))
                        macro.name = *name;
                    if (const auto defaultValue = readRequired<RuntimeProjectLoadResult, double>(macroObject, result, "defaultValue", context.c_str()))
                        macro.defaultValue = *defaultValue;
                    if (const auto minValue = readRequired<RuntimeProjectLoadResult, double>(macroObject, result, "minValue", context.c_str()))
                        macro.minValue = *minValue;
                    if (const auto maxValue = readRequired<RuntimeProjectLoadResult, double>(macroObject, result, "maxValue", context.c_str()))
                        macro.maxValue = *maxValue;

                    const auto targetsIterator = macroObject.find("targets");
                    if (targetsIterator == macroObject.end() || !isObjectArray(*targetsIterator))
                    {
                        addIssue(result, context + " field 'targets' must be an array of objects.");
                    }
                    else
                    {
                        macro.targets.reserve(targetsIterator->size());

                        for (std::size_t targetIndex = 0; targetIndex < targetsIterator->size(); ++targetIndex)
                        {
                            const auto& targetObject = targetsIterator->at(targetIndex);
                            const auto targetContext = context + ".Target[" + std::to_string(targetIndex) + "]";
                            RuntimeProjectMacroTargetDefinition target;

                            if (const auto parameterId = readRequired<RuntimeProjectLoadResult, std::string>(targetObject, result, "parameterId", targetContext.c_str()))
                                target.parameterId = *parameterId;
                            if (const auto parameterPath = readRequired<RuntimeProjectLoadResult, std::string>(targetObject, result, "parameterPath", targetContext.c_str()))
                                target.parameterPath = *parameterPath;
                            if (const auto role = readRequired<RuntimeProjectLoadResult, std::string>(targetObject, result, "role", targetContext.c_str()))
                                target.role = *role;

                            macro.targets.push_back(std::move(target));
                        }
                    }

                    authoring.macros.push_back(std::move(macro));
                }
            }

            const auto fxSlotsIterator = authoringIterator->find("fxSlots");
            if (fxSlotsIterator == authoringIterator->end() || !isObjectArray(*fxSlotsIterator))
            {
                addIssue(result, "Project authoring field 'fxSlots' must be an array of objects.");
            }
            else
            {
                authoring.fxSlots.reserve(fxSlotsIterator->size());

                for (std::size_t index = 0; index < fxSlotsIterator->size(); ++index)
                {
                    const auto& fxObject = fxSlotsIterator->at(index);
                    const auto context = "ProjectFxSlot[" + std::to_string(index) + "]";
                    RuntimeProjectFxSlotDefinition fxSlot;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(fxObject, result, "id", context.c_str()))
                        fxSlot.id = *id;
                    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(fxObject, result, "displayName", context.c_str()))
                        fxSlot.displayName = *displayName;
                    if (const auto effectType = readRequired<RuntimeProjectLoadResult, std::string>(fxObject, result, "effectType", context.c_str()))
                        fxSlot.effectType = *effectType;
                    if (const auto bypassed = readRequired<RuntimeProjectLoadResult, bool>(fxObject, result, "bypassed", context.c_str()))
                        fxSlot.bypassed = *bypassed;

                    authoring.fxSlots.push_back(std::move(fxSlot));
                }
            }

            const auto routingBusesIterator = authoringIterator->find("routingBuses");
            if (routingBusesIterator == authoringIterator->end() || !isObjectArray(*routingBusesIterator))
            {
                addIssue(result, "Project authoring field 'routingBuses' must be an array of objects.");
            }
            else
            {
                authoring.routingBuses.reserve(routingBusesIterator->size());

                for (std::size_t index = 0; index < routingBusesIterator->size(); ++index)
                {
                    const auto& busObject = routingBusesIterator->at(index);
                    const auto context = "ProjectRoutingBus[" + std::to_string(index) + "]";
                    RuntimeProjectRoutingBusDefinition bus;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(busObject, result, "id", context.c_str()))
                        bus.id = *id;
                    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(busObject, result, "displayName", context.c_str()))
                        bus.displayName = *displayName;
                    if (const auto inputSourceId = readRequired<RuntimeProjectLoadResult, std::string>(busObject, result, "inputSourceId", context.c_str()))
                        bus.inputSourceId = *inputSourceId;
                    bus.fxSlotIds = readRequiredStringArray(busObject, result, "fxSlotIds", context.c_str());

                    authoring.routingBuses.push_back(std::move(bus));
                }
            }

            const auto performanceBanksIterator = authoringIterator->find("performanceBanks");
            if (performanceBanksIterator == authoringIterator->end() || !isObjectArray(*performanceBanksIterator))
            {
                addIssue(result, "Project authoring field 'performanceBanks' must be an array of objects.");
            }
            else
            {
                authoring.performanceBanks.reserve(performanceBanksIterator->size());

                for (std::size_t index = 0; index < performanceBanksIterator->size(); ++index)
                {
                    const auto& bankObject = performanceBanksIterator->at(index);
                    const auto context = "ProjectPerformanceBank[" + std::to_string(index) + "]";
                    RuntimeProjectPerformanceBankDefinition bank;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(bankObject, result, "id", context.c_str()))
                        bank.id = *id;
                    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(bankObject, result, "displayName", context.c_str()))
                        bank.displayName = *displayName;

                    const auto triggerSlotsIterator = bankObject.find("triggerSlots");
                    if (triggerSlotsIterator == bankObject.end() || !isObjectArray(*triggerSlotsIterator))
                    {
                        addIssue(result, context + " field 'triggerSlots' must be an array of objects.");
                    }
                    else
                    {
                        bank.triggerSlots.reserve(triggerSlotsIterator->size());

                        for (std::size_t triggerIndex = 0; triggerIndex < triggerSlotsIterator->size(); ++triggerIndex)
                        {
                            const auto& slotObject = triggerSlotsIterator->at(triggerIndex);
                            const auto slotContext = context + ".TriggerSlot[" + std::to_string(triggerIndex) + "]";
                            RuntimeProjectTriggerSlotDefinition slot;

                            if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(slotObject, result, "id", slotContext.c_str()))
                                slot.id = *id;
                            if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(slotObject, result, "displayName", slotContext.c_str()))
                                slot.displayName = *displayName;
                            if (const auto triggerEvent = readRequired<RuntimeProjectLoadResult, std::string>(slotObject, result, "triggerEvent", slotContext.c_str()))
                                slot.triggerEvent = *triggerEvent;
                            if (const auto targetArticulationId = readRequired<RuntimeProjectLoadResult, std::string>(slotObject, result, "targetArticulationId", slotContext.c_str()))
                                slot.targetArticulationId = *targetArticulationId;

                            bank.triggerSlots.push_back(std::move(slot));
                        }
                    }

                    bank.notes = readRequiredStringArray(bankObject, result, "notes", context.c_str());
                    authoring.performanceBanks.push_back(std::move(bank));
                }
            }

            authoring.notes = readRequiredStringArray(*authoringIterator, result, "notes", "Project authoring");
        }
    }

    const auto validation = validateRuntimeProjectModel(project);
    result.issues.insert(result.issues.end(), validation.issues.begin(), validation.issues.end());

    result.loaded = result.issues.empty();
    result.state = result.loaded ? "Project loaded" : "Project invalid";
    return result;
}

RuntimeProjectLoadResult loadPhase1ReferenceProjectManifest()
{
    return loadRuntimeProjectManifest(getPhase1ReferenceProjectManifestPath());
}

RuntimeProjectLoadResult loadPhase2ReferenceProjectManifest()
{
    return loadRuntimeProjectManifest(getPhase2ReferenceProjectManifestPath());
}

RuntimeProjectValidationResult validateRuntimeProjectModel(const RuntimeProjectModel& project)
{
    RuntimeProjectValidationResult result;
    result.state = "Project validation failed";

    if (project.schemaName != "drs.project")
        addIssue(result, "Project schemaName must be 'drs.project'.");

    if (project.schemaVersion != 1 && project.schemaVersion != 2)
        addIssue(result, "Project schemaVersion must be 1 or 2.");

    if (project.projectId.empty())
        addIssue(result, "Project projectId must not be empty.");

    if (project.displayName.empty())
        addIssue(result, "Project displayName must not be empty.");

    if (project.contentRootPath.empty())
        addIssue(result, "Project contentRootPath must not be empty.");

    if (project.defaultInstrumentManifestPath.empty())
        addIssue(result, "Project defaultInstrumentManifestPath must not be empty.");

    if (project.sampleSources.empty())
        addIssue(result, "Project must declare at least one sample source.");

    {
        std::unordered_set<std::string> sampleSourceIds;
        for (const auto& sampleSource : project.sampleSources)
        {
            if (sampleSource.id.empty())
                addIssue(result, "Project sample sources must have non-empty ids.");
            else if (!sampleSourceIds.insert(sampleSource.id).second)
                addIssue(result, "Project sample source ids must be unique; duplicate '" + sampleSource.id + "'.");

            if (sampleSource.path.empty())
                addIssue(result, "Project sample source '" + sampleSource.id + "' must have a non-empty path.");

            if (sampleSource.role.empty())
                addIssue(result, "Project sample source '" + sampleSource.id + "' must have a non-empty role.");
        }
    }

    if (project.schemaVersion == 2)
    {
        const auto& authoring = project.authoring;

        if (authoring.schemaName != "drs.authoring")
            addIssue(result, "Project authoring schemaName must be 'drs.authoring'.");

        if (authoring.schemaVersion != 1)
            addIssue(result, "Project authoring schemaVersion must be 1.");

        if (hasDuplicateIds(authoring.zones))
            addIssue(result, "Project authoring zone ids must be unique.");

        if (hasDuplicateIds(authoring.macros))
            addIssue(result, "Project authoring macro ids must be unique.");

        if (hasDuplicateIds(authoring.fxSlots))
            addIssue(result, "Project authoring FX slot ids must be unique.");

        if (hasDuplicateIds(authoring.routingBuses))
            addIssue(result, "Project authoring routing bus ids must be unique.");

        if (hasDuplicateIds(authoring.performanceBanks))
            addIssue(result, "Project authoring performance bank ids must be unique.");

        std::unordered_set<std::string> sampleSourceIds;
        for (const auto& sampleSource : project.sampleSources)
            sampleSourceIds.insert(sampleSource.id);

        std::unordered_set<std::string> zoneIds;
        for (const auto& zone : authoring.zones)
        {
            if (!zone.id.empty())
                zoneIds.insert(zone.id);

            if (zone.sampleSourceId.empty())
                addIssue(result, "Project zone '" + zone.id + "' must reference a sampleSourceId.");
            else if (!sampleSourceIds.count(zone.sampleSourceId))
                addIssue(result, "Project zone '" + zone.id + "' references unknown sampleSourceId '" + zone.sampleSourceId + "'.");

            if (zone.displayName.empty())
                addIssue(result, "Project zone '" + zone.id + "' must have a displayName.");

            if (zone.groupId.empty())
                addIssue(result, "Project zone '" + zone.id + "' must have a groupId.");

            if (zone.articulationId.empty())
                addIssue(result, "Project zone '" + zone.id + "' must have an articulationId.");

            if (zone.keyLow > zone.keyHigh)
                addIssue(result, "Project zone '" + zone.id + "' has keyLow greater than keyHigh.");

            if (zone.velocityLow > zone.velocityHigh)
                addIssue(result, "Project zone '" + zone.id + "' has velocityLow greater than velocityHigh.");

            if (zone.loopEnabled && zone.loopStartFrame > zone.loopEndFrame)
                addIssue(result, "Project zone '" + zone.id + "' has loopStartFrame greater than loopEndFrame.");
        }

        if (!authoring.selectedZoneId.empty() && !zoneIds.count(authoring.selectedZoneId))
            addIssue(result, "Project authoring selectedZoneId references unknown zone '" + authoring.selectedZoneId + "'.");

        std::unordered_set<std::string> fxSlotIds;
        for (const auto& fxSlot : authoring.fxSlots)
        {
            if (fxSlot.id.empty())
                addIssue(result, "Project FX slots must have non-empty ids.");
            else
                fxSlotIds.insert(fxSlot.id);

            if (fxSlot.displayName.empty())
                addIssue(result, "Project FX slot '" + fxSlot.id + "' must have a displayName.");

            if (fxSlot.effectType.empty())
                addIssue(result, "Project FX slot '" + fxSlot.id + "' must have an effectType.");
        }

        for (const auto& macro : authoring.macros)
        {
            if (macro.id.empty())
                addIssue(result, "Project macros must have non-empty ids.");

            if (macro.name.empty())
                addIssue(result, "Project macro '" + macro.id + "' must have a name.");

            if (macro.minValue > macro.maxValue)
                addIssue(result, "Project macro '" + macro.id + "' has minValue greater than maxValue.");

            for (const auto& target : macro.targets)
            {
                if (target.parameterId.empty())
                    addIssue(result, "Project macro '" + macro.id + "' contains a target without parameterId.");

                if (target.parameterPath.empty())
                    addIssue(result, "Project macro '" + macro.id + "' contains a target without parameterPath.");

                if (target.role.empty())
                    addIssue(result, "Project macro '" + macro.id + "' contains a target without role.");
            }
        }

        std::unordered_set<std::string> performanceBankIds;
        for (const auto& bank : authoring.performanceBanks)
        {
            if (!bank.id.empty())
                performanceBankIds.insert(bank.id);

            if (bank.displayName.empty())
                addIssue(result, "Project performance bank '" + bank.id + "' must have a displayName.");

            std::unordered_set<std::string> triggerSlotIds;
            for (const auto& triggerSlot : bank.triggerSlots)
            {
                if (triggerSlot.id.empty())
                    addIssue(result, "Project performance bank '" + bank.id + "' contains a trigger slot without id.");
                else if (!triggerSlotIds.insert(triggerSlot.id).second)
                    addIssue(result, "Project performance bank '" + bank.id + "' contains duplicate trigger slot id '" + triggerSlot.id + "'.");

                if (triggerSlot.displayName.empty())
                    addIssue(result, "Project trigger slot '" + triggerSlot.id + "' must have a displayName.");

                if (triggerSlot.triggerEvent.empty())
                    addIssue(result, "Project trigger slot '" + triggerSlot.id + "' must have a triggerEvent.");

                if (triggerSlot.targetArticulationId.empty())
                    addIssue(result, "Project trigger slot '" + triggerSlot.id + "' must have a targetArticulationId.");
            }
        }

        if (!authoring.selectedPerformanceBankId.empty() && !performanceBankIds.count(authoring.selectedPerformanceBankId))
            addIssue(result,
                     "Project authoring selectedPerformanceBankId references unknown bank '"
                         + authoring.selectedPerformanceBankId + "'.");

        for (const auto& bus : authoring.routingBuses)
        {
            if (bus.id.empty())
                addIssue(result, "Project routing buses must have non-empty ids.");

            if (bus.displayName.empty())
                addIssue(result, "Project routing bus '" + bus.id + "' must have a displayName.");

            if (bus.inputSourceId.empty())
                addIssue(result, "Project routing bus '" + bus.id + "' must have an inputSourceId.");

            for (const auto& fxSlotId : bus.fxSlotIds)
            {
                if (!fxSlotIds.count(fxSlotId))
                    addIssue(result, "Project routing bus '" + bus.id + "' references unknown FX slot '" + fxSlotId + "'.");
            }
        }
    }

    if (result.issues.empty())
    {
        result.valid = true;
        result.state = "Project validated";
    }

    return result;
}

RuntimeProjectMigrationResult migrateRuntimeProjectToPhase2Authoring(const RuntimeProjectModel& project)
{
    RuntimeProjectMigrationResult result;
    result.state = "Project migration failed";

    if (project.schemaVersion == 2)
    {
        result.project = project;
        const auto validation = validateRuntimeProjectModel(result.project);
        result.issues = validation.issues;
        result.valid = validation.valid;
        result.state = validation.valid ? "Project already uses the Phase 2 authoring schema" : validation.state;
        return result;
    }

    if (project.schemaVersion != 1)
    {
        addIssue(result, "Only Project schemaVersion 1 can be migrated into the Phase 2 authoring schema.");
        return result;
    }

    result.project = project;
    result.project.schemaVersion = 2;
    result.project.authoring = buildDefaultPhase2AuthoringState();
    result.project.authoring.notes = {
        "Migrated from the Phase 1 runtime project manifest into the Phase 2 authoring schema.",
        "Authoring zones, macro targets, routing, and performance-bank placeholders can now be edited and saved in-project."
    };

    const auto validation = validateRuntimeProjectModel(result.project);
    result.issues = validation.issues;
    result.valid = validation.valid;
    result.migrated = validation.valid;
    result.state = validation.valid ? "Project migrated to the Phase 2 authoring schema" : validation.state;
    return result;
}

RuntimeManifestLoadResult loadRuntimeInstrumentManifest(const std::string& manifestPath)
{
    const auto startTime = std::chrono::steady_clock::now();
    RuntimeManifestLoadResult result;
    result.manifestPath = manifestPath;
    result.state = "Manifest load not attempted";

    const fs::path manifestFsPath(manifestPath);
    std::error_code errorCode;

    if (!fs::exists(manifestFsPath, errorCode))
    {
        result.state = "Manifest missing";
        addIssue(result, "Manifest file was not found at " + manifestPath + ".");
        result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
        return result;
    }

    result.manifestFound = true;

    const auto rawText = readTextFile(manifestFsPath);
    if (rawText.empty())
    {
        result.state = "Manifest unreadable";
        addIssue(result, "Manifest file was empty or unreadable.");
        result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
        return result;
    }

    result.metrics.manifestSizeBytes = static_cast<std::uint64_t>(rawText.size());

    json root;
    try
    {
        root = json::parse(rawText);
    }
    catch (const json::exception& exception)
    {
        result.state = "Manifest parse failed";
        addIssue(result, "Manifest JSON parse failed: " + std::string(exception.what()));
        result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
        return result;
    }

    if (!root.is_object())
    {
        result.state = "Manifest root invalid";
        addIssue(result, "Manifest root must be a JSON object.");
        result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
        return result;
    }

    auto& instrument = result.instrument;

    if (const auto schemaName = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "schemaName", "Manifest"))
        instrument.schemaName = *schemaName;

    if (const auto schemaVersion = readRequired<RuntimeManifestLoadResult, int>(root, result, "schemaVersion", "Manifest"))
        instrument.schemaVersion = *schemaVersion;

    if (const auto instrumentId = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "instrumentId", "Manifest"))
        instrument.instrumentId = *instrumentId;

    if (const auto displayName = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "displayName", "Manifest"))
        instrument.displayName = *displayName;

    if (const auto sourceProjectPath = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "sourceProject", "Manifest"))
    {
        const auto resolved = validateRequiredFile(result, manifestFsPath, *sourceProjectPath, "Source project");
        instrument.sourceProjectPath = resolved ? toDisplayPath(*resolved) : *sourceProjectPath;
        result.metrics.sourceProjectResolved = resolved.has_value();
    }

    if (const auto compiledStreamAsset = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "compiledStreamAsset", "Manifest"))
    {
        const auto resolved = validateRequiredFile(result, manifestFsPath, *compiledStreamAsset, "Compiled stream asset");
        instrument.compiledStreamAssetPath = resolved ? toDisplayPath(*resolved) : *compiledStreamAsset;
        result.metrics.usesStreaming = resolved.has_value();
        result.metrics.compiledStreamAssetResolved = resolved.has_value();
    }

    if (const auto loadProfile = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "defaultLoadProfile", "Manifest"))
        instrument.defaultLoadProfile = *loadProfile;

    std::unordered_set<std::string> articulationIds;
    bool defaultArticulationFound = false;

    const auto articulationsIterator = root.find("articulations");
    if (articulationsIterator == root.end() || !isObjectArray(*articulationsIterator))
    {
        addIssue(result, "Manifest field 'articulations' must be an array of objects.");
    }
    else
    {
        instrument.articulations.reserve(articulationsIterator->size());

        for (std::size_t index = 0; index < articulationsIterator->size(); ++index)
        {
            const auto& articulationObject = articulationsIterator->at(index);
            const auto context = "Articulation[" + std::to_string(index) + "]";
            RuntimeArticulationDefinition articulation;

            if (const auto id = readRequired<RuntimeManifestLoadResult, std::string>(articulationObject, result, "id", context.c_str()))
                articulation.id = *id;

            if (const auto name = readRequired<RuntimeManifestLoadResult, std::string>(articulationObject, result, "name", context.c_str()))
                articulation.name = *name;

            articulation.isDefault = articulationObject.value("isDefault", false);
            defaultArticulationFound = defaultArticulationFound || articulation.isDefault;

            if (!articulation.id.empty())
                articulationIds.insert(articulation.id);

            instrument.articulations.push_back(std::move(articulation));
        }
    }

    if (!defaultArticulationFound)
        addIssue(result, "Manifest must declare one default articulation for the Sprint 1 reference load path.");

    std::unordered_set<std::string> groupIds;

    const auto groupsIterator = root.find("groups");
    if (groupsIterator == root.end() || !isObjectArray(*groupsIterator))
    {
        addIssue(result, "Manifest field 'groups' must be an array of objects.");
    }
    else
    {
        instrument.groups.reserve(groupsIterator->size());

        for (std::size_t index = 0; index < groupsIterator->size(); ++index)
        {
            const auto& groupObject = groupsIterator->at(index);
            const auto context = "Group[" + std::to_string(index) + "]";
            RuntimeGroupDefinition group;

            if (const auto id = readRequired<RuntimeManifestLoadResult, std::string>(groupObject, result, "id", context.c_str()))
                group.id = *id;

            if (const auto name = readRequired<RuntimeManifestLoadResult, std::string>(groupObject, result, "name", context.c_str()))
                group.name = *name;

            group.articulationIds = readRequiredStringArray(groupObject, result, "articulationIds", context.c_str());

            for (const auto& articulationId : group.articulationIds)
            {
                if (!articulationIds.count(articulationId))
                    addIssue(result, context + " references unknown articulation '" + articulationId + "'.");
            }

            if (!group.id.empty())
                groupIds.insert(group.id);

            instrument.groups.push_back(std::move(group));
        }
    }

    const auto macrosIterator = root.find("macros");
    if (macrosIterator == root.end() || !isObjectArray(*macrosIterator))
    {
        addIssue(result, "Manifest field 'macros' must be an array of objects.");
    }
    else
    {
        instrument.macros.reserve(macrosIterator->size());

        for (std::size_t index = 0; index < macrosIterator->size(); ++index)
        {
            const auto& macroObject = macrosIterator->at(index);
            const auto context = "Macro[" + std::to_string(index) + "]";
            RuntimeMacroDefinition macro;

            if (const auto id = readRequired<RuntimeManifestLoadResult, std::string>(macroObject, result, "id", context.c_str()))
                macro.id = *id;

            if (const auto name = readRequired<RuntimeManifestLoadResult, std::string>(macroObject, result, "name", context.c_str()))
                macro.name = *name;

            if (const auto defaultValue = readRequired<RuntimeManifestLoadResult, double>(macroObject, result, "defaultValue", context.c_str()))
                macro.defaultValue = *defaultValue;

            if (const auto minValue = readRequired<RuntimeManifestLoadResult, double>(macroObject, result, "minValue", context.c_str()))
                macro.minValue = *minValue;

            if (const auto maxValue = readRequired<RuntimeManifestLoadResult, double>(macroObject, result, "maxValue", context.c_str()))
                macro.maxValue = *maxValue;

            if (macro.minValue > macro.maxValue)
                addIssue(result, context + " has minValue greater than maxValue.");

            instrument.macros.push_back(std::move(macro));
        }
    }

    const auto zonesIterator = root.find("zones");
    if (zonesIterator == root.end() || !isObjectArray(*zonesIterator))
    {
        addIssue(result, "Manifest field 'zones' must be an array of objects.");
    }
    else
    {
        instrument.zones.reserve(zonesIterator->size());

        for (std::size_t index = 0; index < zonesIterator->size(); ++index)
        {
            const auto& zoneObject = zonesIterator->at(index);
            const auto context = "Zone[" + std::to_string(index) + "]";
            RuntimeZoneDefinition zone;

            if (const auto id = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "id", context.c_str()))
                zone.id = *id;

            if (const auto groupId = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "groupId", context.c_str()))
                zone.groupId = *groupId;

            if (const auto articulationId = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "articulationId", context.c_str()))
                zone.articulationId = *articulationId;

            if (const auto samplePath = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "samplePath", context.c_str()))
            {
                const auto resolvedSamplePath = validateRequiredFile(result, manifestFsPath, *samplePath, "Zone sample");
                zone.samplePath = resolvedSamplePath ? toDisplayPath(*resolvedSamplePath) : *samplePath;
            }

            if (const auto streamAssetPath = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "streamAssetPath", context.c_str()))
            {
                const auto resolvedStreamPath = validateRequiredFile(result, manifestFsPath, *streamAssetPath, "Zone stream asset");
                zone.streamAssetPath = resolvedStreamPath ? toDisplayPath(*resolvedStreamPath) : *streamAssetPath;
            }

            if (const auto rootKey = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "rootKey", context.c_str()))
                zone.rootKey = *rootKey;

            if (const auto keyLow = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "keyLow", context.c_str()))
                zone.keyLow = *keyLow;

            if (const auto keyHigh = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "keyHigh", context.c_str()))
                zone.keyHigh = *keyHigh;

            if (const auto velocityLow = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "velocityLow", context.c_str()))
                zone.velocityLow = *velocityLow;

            if (const auto velocityHigh = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "velocityHigh", context.c_str()))
                zone.velocityHigh = *velocityHigh;

            if (const auto streamOffsetBytes = readRequired<RuntimeManifestLoadResult, std::uint64_t>(zoneObject, result, "streamOffsetBytes", context.c_str()))
                zone.streamOffsetBytes = *streamOffsetBytes;

            if (const auto prefetchBytes = readRequired<RuntimeManifestLoadResult, std::uint64_t>(zoneObject, result, "prefetchBytes", context.c_str()))
                zone.prefetchBytes = *prefetchBytes;

            if (!groupIds.count(zone.groupId))
                addIssue(result, context + " references unknown group '" + zone.groupId + "'.");

            if (!articulationIds.count(zone.articulationId))
                addIssue(result, context + " references unknown articulation '" + zone.articulationId + "'.");

            if (zone.keyLow > zone.keyHigh)
                addIssue(result, context + " has keyLow greater than keyHigh.");

            if (zone.velocityLow > zone.velocityHigh)
                addIssue(result, context + " has velocityLow greater than velocityHigh.");

            result.metrics.totalPrefetchBytes += zone.prefetchBytes;
            instrument.zones.push_back(std::move(zone));
        }
    }

    instrument.validationNotes = readRequiredStringArray(root, result, "validationNotes", "Manifest");

    result.metrics.macroCount = instrument.macros.size();
    result.metrics.articulationCount = instrument.articulations.size();
    result.metrics.groupCount = instrument.groups.size();
    result.metrics.zoneCount = instrument.zones.size();
    result.metrics.referencedSampleCount = instrument.zones.size();

    if (instrument.schemaName != "drs.instrument")
        addIssue(result, "Manifest schemaName must be 'drs.instrument' for the Sprint 1 loader.");

    if (instrument.schemaVersion != 1)
        addIssue(result, "Manifest schemaVersion must be 1 for the Sprint 1 loader.");

    if (instrument.zones.empty())
        addIssue(result, "Manifest must declare at least one zone.");

    if (instrument.groups.empty())
        addIssue(result, "Manifest must declare at least one group.");

    if (instrument.articulations.empty())
        addIssue(result, "Manifest must declare at least one articulation.");

    if (!result.metrics.usesStreaming)
        addIssue(result, "Compiled stream asset must exist so the Sprint 1 loader can prove the stream-container seam.");

    result.loaded = result.issues.empty();
    result.state = result.loaded ? "Reference manifest loaded" : "Reference manifest invalid";
    result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
    return result;
}

RuntimeManifestLoadResult loadPhase1ReferenceInstrumentManifest()
{
    return loadRuntimeInstrumentManifest(getPhase1ReferenceInstrumentManifestPath());
}

std::string serializeRuntimeProjectManifest(const RuntimeProjectModel& project, const std::string& manifestPath)
{
    const fs::path manifestFsPath(manifestPath);
    ordered_json root;
    root["schemaName"] = project.schemaName;
    root["schemaVersion"] = project.schemaVersion;
    root["projectId"] = project.projectId;
    root["displayName"] = project.displayName;
    root["contentRoot"] = toManifestRelativePath(manifestFsPath, project.contentRootPath);
    root["defaultInstrumentManifest"] = toManifestRelativePath(manifestFsPath, project.defaultInstrumentManifestPath);

    ordered_json sampleSources = ordered_json::array();
    for (const auto& sampleSource : project.sampleSources)
    {
        ordered_json sample;
        sample["id"] = sampleSource.id;
        sample["path"] = toManifestRelativePath(manifestFsPath, sampleSource.path);
        sample["role"] = sampleSource.role;
        sampleSources.push_back(std::move(sample));
    }

    root["sampleSources"] = std::move(sampleSources);

    if (project.schemaVersion >= 2)
    {
        ordered_json authoring;
        authoring["schemaName"] = project.authoring.schemaName;
        authoring["schemaVersion"] = project.authoring.schemaVersion;
        authoring["selectedZoneId"] = project.authoring.selectedZoneId;
        authoring["selectedPerformanceBankId"] = project.authoring.selectedPerformanceBankId;
        authoring["zones"] = serializeProjectZones(project.authoring.zones);
        authoring["macros"] = serializeProjectMacros(project.authoring.macros);
        authoring["fxSlots"] = serializeFxSlots(project.authoring.fxSlots);
        authoring["routingBuses"] = serializeRoutingBuses(project.authoring.routingBuses);
        authoring["performanceBanks"] = serializePerformanceBanks(project.authoring.performanceBanks);
        authoring["notes"] = serializeStringArray(project.authoring.notes);
        root["authoring"] = std::move(authoring);
    }

    root["notes"] = serializeStringArray(project.notes);
    return root.dump(2) + "\n";
}

std::string serializeRuntimeInstrumentManifest(const RuntimeInstrumentModel& instrument, const std::string& manifestPath)
{
    const fs::path manifestFsPath(manifestPath);
    ordered_json root;
    root["schemaName"] = instrument.schemaName;
    root["schemaVersion"] = instrument.schemaVersion;
    root["instrumentId"] = instrument.instrumentId;
    root["displayName"] = instrument.displayName;
    root["sourceProject"] = toManifestRelativePath(manifestFsPath, instrument.sourceProjectPath);
    root["compiledStreamAsset"] = toManifestRelativePath(manifestFsPath, instrument.compiledStreamAssetPath);
    root["defaultLoadProfile"] = instrument.defaultLoadProfile;

    ordered_json macros = ordered_json::array();
    for (const auto& macro : instrument.macros)
    {
        ordered_json macroObject;
        macroObject["id"] = macro.id;
        macroObject["name"] = macro.name;
        macroObject["defaultValue"] = macro.defaultValue;
        macroObject["minValue"] = macro.minValue;
        macroObject["maxValue"] = macro.maxValue;
        macros.push_back(std::move(macroObject));
    }
    root["macros"] = std::move(macros);

    ordered_json articulations = ordered_json::array();
    for (const auto& articulation : instrument.articulations)
    {
        ordered_json articulationObject;
        articulationObject["id"] = articulation.id;
        articulationObject["name"] = articulation.name;
        articulationObject["isDefault"] = articulation.isDefault;
        articulations.push_back(std::move(articulationObject));
    }
    root["articulations"] = std::move(articulations);

    ordered_json groups = ordered_json::array();
    for (const auto& group : instrument.groups)
    {
        ordered_json groupObject;
        groupObject["id"] = group.id;
        groupObject["name"] = group.name;
        groupObject["articulationIds"] = serializeStringArray(group.articulationIds);
        groups.push_back(std::move(groupObject));
    }
    root["groups"] = std::move(groups);

    ordered_json zones = ordered_json::array();
    for (const auto& zone : instrument.zones)
    {
        ordered_json zoneObject;
        zoneObject["id"] = zone.id;
        zoneObject["groupId"] = zone.groupId;
        zoneObject["articulationId"] = zone.articulationId;
        zoneObject["samplePath"] = toManifestRelativePath(manifestFsPath, zone.samplePath);
        zoneObject["streamAssetPath"] = toManifestRelativePath(manifestFsPath, zone.streamAssetPath);
        zoneObject["rootKey"] = zone.rootKey;
        zoneObject["keyLow"] = zone.keyLow;
        zoneObject["keyHigh"] = zone.keyHigh;
        zoneObject["velocityLow"] = zone.velocityLow;
        zoneObject["velocityHigh"] = zone.velocityHigh;
        zoneObject["streamOffsetBytes"] = zone.streamOffsetBytes;
        zoneObject["prefetchBytes"] = zone.prefetchBytes;
        zones.push_back(std::move(zoneObject));
    }
    root["zones"] = std::move(zones);

    root["validationNotes"] = serializeStringArray(instrument.validationNotes);
    return root.dump(2) + "\n";
}
} // namespace drs::engine
