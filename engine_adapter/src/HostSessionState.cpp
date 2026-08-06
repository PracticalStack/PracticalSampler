#include "drs/engine/HostSessionState.h"

#include "drs/engine/RuntimeLoader.h"

#include <json/json.hpp>

#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace drs::engine
{
namespace
{
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

template <typename TResult>
void addFinding(TResult& result,
                const HostSessionStateFindingCode code,
                std::string path,
                std::string message,
                const HostSessionStateFindingSeverity severity = HostSessionStateFindingSeverity::error)
{
    result.findings.push_back({ severity, code, std::move(path), std::move(message) });
}

template <typename TResult>
void validateAllowedFields(const json& object,
                           TResult& result,
                           const std::unordered_set<std::string>& allowed,
                           const std::string& path)
{
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator)
    {
        if (allowed.find(iterator.key()) == allowed.end())
        {
            addFinding(result,
                       HostSessionStateFindingCode::unknownField,
                       path + "/" + iterator.key(),
                       "Field '" + iterator.key() + "' is not part of the host-state version 1 contract.");
        }
    }
}

template <typename TResult>
std::optional<std::string> readRequiredString(const json& object,
                                              TResult& result,
                                              const char* name,
                                              const std::string& path)
{
    const auto iterator = object.find(name);
    if (iterator == object.end())
    {
        addFinding(result,
                   HostSessionStateFindingCode::requiredFieldMissing,
                   path + "/" + name,
                   "Required string field '" + std::string(name) + "' is missing.");
        return std::nullopt;
    }

    if (!iterator->is_string())
    {
        addFinding(result,
                   HostSessionStateFindingCode::fieldTypeInvalid,
                   path + "/" + name,
                   "Field '" + std::string(name) + "' must be a string.");
        return std::nullopt;
    }

    return iterator->get<std::string>();
}

template <typename TResult>
std::optional<std::string> readOptionalString(const json& object,
                                              TResult& result,
                                              const char* name,
                                              const std::string& path)
{
    const auto iterator = object.find(name);
    if (iterator == object.end())
        return std::string {};

    if (!iterator->is_string())
    {
        addFinding(result,
                   HostSessionStateFindingCode::fieldTypeInvalid,
                   path + "/" + name,
                   "Field '" + std::string(name) + "' must be a string when present.");
        return std::nullopt;
    }

    return iterator->get<std::string>();
}

template <typename TResult>
std::optional<std::size_t> readRequiredSize(const json& object,
                                            TResult& result,
                                            const char* name,
                                            const std::string& path)
{
    const auto iterator = object.find(name);
    if (iterator == object.end())
    {
        addFinding(result,
                   HostSessionStateFindingCode::requiredFieldMissing,
                   path + "/" + name,
                   "Required non-negative integer field '" + std::string(name) + "' is missing.");
        return std::nullopt;
    }

    if (!iterator->is_number_integer() && !iterator->is_number_unsigned())
    {
        addFinding(result,
                   HostSessionStateFindingCode::fieldTypeInvalid,
                   path + "/" + name,
                   "Field '" + std::string(name) + "' must be a non-negative integer.");
        return std::nullopt;
    }

    try
    {
        if (iterator->is_number_integer())
        {
            const auto signedValue = iterator->get<std::int64_t>();
            if (signedValue < 0)
                throw std::out_of_range("negative");

            return static_cast<std::size_t>(signedValue);
        }

        const auto value = iterator->get<std::uint64_t>();
        if (value > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
            throw std::out_of_range("size");

        return static_cast<std::size_t>(value);
    }
    catch (const std::exception&)
    {
        addFinding(result,
                   HostSessionStateFindingCode::fieldValueInvalid,
                   path + "/" + name,
                   "Field '" + std::string(name) + "' is outside the supported integer range.");
        return std::nullopt;
    }
}

template <typename TResult>
std::optional<bool> readRequiredBool(const json& object,
                                     TResult& result,
                                     const char* name,
                                     const std::string& path)
{
    const auto iterator = object.find(name);
    if (iterator == object.end())
    {
        addFinding(result,
                   HostSessionStateFindingCode::requiredFieldMissing,
                   path + "/" + name,
                   "Required boolean field '" + std::string(name) + "' is missing.");
        return std::nullopt;
    }

    if (!iterator->is_boolean())
    {
        addFinding(result,
                   HostSessionStateFindingCode::fieldTypeInvalid,
                   path + "/" + name,
                   "Field '" + std::string(name) + "' must be a boolean.");
        return std::nullopt;
    }

    return iterator->get<bool>();
}

bool hasErrors(const std::vector<HostSessionStateFinding>& findings)
{
    for (const auto& finding : findings)
    {
        if (finding.severity == HostSessionStateFindingSeverity::error)
            return true;
    }

    return false;
}

bool exceedsJsonDepth(const std::string& text) noexcept
{
    std::size_t depth = 0;
    bool insideString = false;
    bool escaped = false;

    for (const auto character : text)
    {
        if (insideString)
        {
            if (escaped)
            {
                escaped = false;
                continue;
            }

            if (character == '\\')
            {
                escaped = true;
                continue;
            }

            if (character == '"')
                insideString = false;

            continue;
        }

        if (character == '"')
        {
            insideString = true;
            continue;
        }

        if (character == '{' || character == '[')
        {
            if (depth == std::numeric_limits<std::size_t>::max())
                return true;

            ++depth;
            if (depth > hostSessionStateMaxJsonDepth)
                return true;
        }
        else if ((character == '}' || character == ']') && depth > 0)
        {
            --depth;
        }
    }

    return false;
}

bool endsWith(const std::string_view value, const std::string_view suffix) noexcept
{
    return value.size() >= suffix.size()
        && value.substr(value.size() - suffix.size()) == suffix;
}

std::size_t stringLimitForField(const std::string_view fieldName) noexcept
{
    if (fieldName == "schemaName"
        || fieldName == "id"
        || endsWith(fieldName, "Id")
        || fieldName.find("Digest") != std::string_view::npos)
    {
        return hostSessionStateMaxIdentityBytes;
    }

    if (fieldName == "manifestFileName"
        || fieldName.find("Path") != std::string_view::npos
        || fieldName.find("path") != std::string_view::npos
        || fieldName == "contentRoot"
        || fieldName == "contentRootHint")
    {
        return hostSessionStateMaxPathBytes;
    }

    return hostSessionStateMaxStringBytes;
}

std::optional<std::size_t> collectionLimitForPath(const std::string& path) noexcept
{
    if (path == "/authoringState/projectSnapshot/sampleSources")
        return hostSessionStateMaxSampleSources;
    if (path == "/authoringState/projectSnapshot/authoring/zones")
        return hostSessionStateMaxZones;
    if (path == "/authoringState/projectSnapshot/authoring/groups")
        return hostSessionStateMaxGroups;
    if (path == "/authoringState/projectSnapshot/authoring/macros")
        return hostSessionStateMaxMacros;
    if (path == "/authoringState/projectSnapshot/authoring/fxSlots")
        return hostSessionStateMaxFxSlots;
    if (path.find("/authoringState/projectSnapshot/authoring/fxSlots[") == 0
        && endsWith(path, "/parameters"))
        return hostSessionStateMaxDspParameters;
    if (path == "/authoringState/projectSnapshot/authoring/routingBuses")
        return hostSessionStateMaxRoutingBuses;
    if (path == "/authoringState/projectSnapshot/authoring/performanceBanks")
        return hostSessionStateMaxPerformanceBanks;
    if (endsWith(path, "/notes") || endsWith(path, "/issues"))
        return hostSessionStateMaxNotesOrIssues;

    return std::nullopt;
}

void validateStructuralBudgets(const json& value,
                               HostSessionStateParseResult& result,
                               const std::string& path,
                               const std::string_view fieldName = {})
{
    if (value.is_string())
    {
        const auto& stringValue = value.get_ref<const std::string&>();
        const auto limit = stringLimitForField(fieldName);
        if (stringValue.size() > limit)
        {
            addFinding(result,
                       HostSessionStateFindingCode::stringTooLong,
                       path,
                       "UTF-8 string exceeds the " + std::to_string(limit)
                           + "-byte limit for this field.");
        }
        return;
    }

    if (value.is_array())
    {
        if (const auto limit = collectionLimitForPath(path);
            limit.has_value() && value.size() > *limit)
        {
            addFinding(result,
                       HostSessionStateFindingCode::collectionTooLarge,
                       path,
                       "Collection exceeds the " + std::to_string(*limit)
                           + "-entry host-state limit.");
        }

        for (std::size_t index = 0; index < value.size(); ++index)
        {
            validateStructuralBudgets(value[index],
                                      result,
                                      path + "/" + std::to_string(index));
        }
        return;
    }

    if (!value.is_object())
        return;

    for (auto iterator = value.begin(); iterator != value.end(); ++iterator)
    {
        if (iterator.key().size() > hostSessionStateMaxIdentityBytes)
        {
            addFinding(result,
                       HostSessionStateFindingCode::stringTooLong,
                       path,
                       "JSON field name exceeds the 512-byte identity limit.");
        }

        validateStructuralBudgets(*iterator,
                                  result,
                                  path + "/" + iterator.key(),
                                  iterator.key());
    }
}

void appendPresetIssues(HostSessionStateParseResult& result,
                        const RuntimePresetStateLoadResult& preset)
{
    for (const auto& issue : preset.issues)
    {
        addFinding(result,
                   HostSessionStateFindingCode::presetInvalid,
                   "/presetState",
                   issue);
    }
}

void parseProjectBinding(const json& object, HostSessionStateParseResult& result)
{
    static const std::unordered_set<std::string> allowed {
        "projectId",
        "manifestPath",
        "manifestFileName",
        "manifestDigest",
        "contentRootHint",
        "portableRelativePath"
    };
    validateAllowedFields(object, result, allowed, "/projectBinding");

    auto& binding = result.hostState->projectBinding;
    if (const auto value = readRequiredString(object, result, "projectId", "/projectBinding"))
        binding.projectId = *value;
    if (const auto value = readOptionalString(object, result, "manifestPath", "/projectBinding"))
        binding.manifestPath = *value;
    if (const auto value = readRequiredString(object, result, "manifestFileName", "/projectBinding"))
        binding.manifestFileName = *value;
    if (const auto value = readRequiredString(object, result, "manifestDigest", "/projectBinding"))
        binding.manifestDigest = *value;
    if (const auto value = readOptionalString(object, result, "contentRootHint", "/projectBinding"))
        binding.contentRootHint = *value;
    if (const auto value = readOptionalString(object, result, "portableRelativePath", "/projectBinding"))
        binding.portableRelativePath = *value;

    if (binding.projectId.empty() || binding.manifestFileName.empty() || binding.manifestDigest.empty())
    {
        addFinding(result,
                   HostSessionStateFindingCode::projectBindingInvalid,
                   "/projectBinding",
                   "Project binding identity, manifest filename, and digest must not be empty.");
    }
}

void parsePerformancePackageBinding(const json& object,
                                    HostSessionStateParseResult& result)
{
    static const std::unordered_set<std::string> allowed {
        "packageId",
        "packagePath",
        "packageFileName"
    };
    validateAllowedFields(object, result, allowed, "/performancePackageBinding");

    HostPerformancePackageBinding binding;
    if (const auto value = readRequiredString(
            object, result, "packageId", "/performancePackageBinding"))
        binding.packageId = *value;
    if (const auto value = readRequiredString(
            object, result, "packagePath", "/performancePackageBinding"))
        binding.packagePath = *value;
    if (const auto value = readRequiredString(
            object, result, "packageFileName", "/performancePackageBinding"))
        binding.packageFileName = *value;
    if (binding.packageId.empty() || binding.packagePath.empty()
        || binding.packageFileName.empty())
    {
        addFinding(result,
                   HostSessionStateFindingCode::performancePackageBindingInvalid,
                   "/performancePackageBinding",
                   "Package identity, path, and filename must not be empty.");
        return;
    }
    result.hostState->performancePackageBinding = std::move(binding);
}

void parseAuthoringState(const json& object,
                         HostSessionStateParseResult& result,
                         const std::string& snapshotManifestPath)
{
    static const std::unordered_set<std::string> allowed {
        "revision",
        "savedRevision",
        "dirty",
        "projectSnapshot"
    };
    validateAllowedFields(object, result, allowed, "/authoringState");

    auto& authoring = result.hostState->authoringState;
    if (const auto value = readRequiredSize(object, result, "revision", "/authoringState"))
        authoring.revision = *value;
    if (const auto value = readRequiredSize(object, result, "savedRevision", "/authoringState"))
        authoring.savedRevision = *value;
    if (const auto value = readRequiredBool(object, result, "dirty", "/authoringState"))
        authoring.dirty = *value;

    if (authoring.savedRevision > authoring.revision
        || authoring.dirty != (authoring.revision != authoring.savedRevision))
    {
        addFinding(result,
                   HostSessionStateFindingCode::authoringStateInvalid,
                   "/authoringState",
                   "savedRevision must not exceed revision and dirty must equal revision != savedRevision.");
    }

    const auto snapshot = object.find("projectSnapshot");
    if (snapshot != object.end())
    {
        if (!snapshot->is_object())
        {
            addFinding(result,
                       HostSessionStateFindingCode::fieldTypeInvalid,
                       "/authoringState/projectSnapshot",
                       "projectSnapshot must be an object.");
        }
        else
        {
            auto path = snapshotManifestPath;
            if (path.empty())
                path = result.hostState->projectBinding.manifestPath;
            if (path.empty())
                path = result.hostState->projectBinding.manifestFileName;

            const auto projectText = snapshot->dump(2) + "\n";
            if (projectText.size() > hostSessionStateMaxProjectSnapshotBytes)
            {
                addFinding(result,
                           HostSessionStateFindingCode::projectSnapshotTooLarge,
                           "/authoringState/projectSnapshot",
                           "Embedded project snapshot exceeds the 1.5 MiB version 1 limit.");
                return;
            }

            const auto project = parseRuntimeProjectManifest(projectText, path, false);
            if (!project.loaded)
            {
                for (const auto& issue : project.issues)
                {
                    addFinding(result,
                               HostSessionStateFindingCode::projectSnapshotInvalid,
                               "/authoringState/projectSnapshot",
                               issue);
                }
            }
            else
            {
                authoring.projectSnapshot = project.project;
            }
        }
    }

    if ((authoring.dirty || result.hostState->projectBinding.manifestPath.empty())
        && !authoring.projectSnapshot.has_value())
    {
        addFinding(result,
                   HostSessionStateFindingCode::projectSnapshotInvalid,
                   "/authoringState/projectSnapshot",
                   "A dirty or never-saved project requires an embedded projectSnapshot.");
    }

    if (authoring.projectSnapshot.has_value())
    {
        auto path = snapshotManifestPath;
        if (path.empty())
            path = result.hostState->projectBinding.manifestPath;
        if (path.empty())
            path = result.hostState->projectBinding.manifestFileName;

        const auto verification = verifyHostProjectBinding(
            result.hostState->projectBinding, *authoring.projectSnapshot, path);
        if (verification.match == HostProjectBindingMatch::projectIdentityMismatch)
        {
            addFinding(result,
                       HostSessionStateFindingCode::projectIdentityMismatch,
                       "/authoringState/projectSnapshot/projectId",
                       "Embedded project ID does not match projectBinding.projectId.");
        }
        else if (verification.match == HostProjectBindingMatch::manifestDigestMismatch)
        {
            addFinding(result,
                       HostSessionStateFindingCode::projectManifestDigestMismatch,
                       "/authoringState/projectSnapshot",
                       "Embedded project digest does not match projectBinding.manifestDigest.");
        }
    }
}

void parsePublishedState(const json& object, HostSessionStateParseResult& result)
{
    if (object.empty())
        return;

    static const std::unordered_set<std::string> allowed {
        "revision",
        "projectGeneration",
        "authoredContentDigest",
        "macroSchemaDigest",
        "preparedContentDigest",
        "dspGraphDigest"
    };
    validateAllowedFields(object, result, allowed, "/publishedState");

    HostPublishedCheckpoint published;
    if (const auto value = readRequiredSize(object, result, "revision", "/publishedState"))
        published.revision = *value;
    if (const auto value = readRequiredSize(object, result, "projectGeneration", "/publishedState"))
        published.projectGeneration = static_cast<std::uint64_t>(*value);
    if (const auto value = readRequiredString(object, result, "authoredContentDigest", "/publishedState"))
        published.authoredContentDigest = *value;
    if (const auto value = readRequiredString(object, result, "macroSchemaDigest", "/publishedState"))
        published.macroSchemaDigest = *value;
    if (const auto value = readRequiredString(object, result, "preparedContentDigest", "/publishedState"))
        published.preparedContentDigest = *value;
    if (const auto iterator = object.find("dspGraphDigest"); iterator != object.end())
    {
        if (!iterator->is_string())
            addFinding(result, HostSessionStateFindingCode::fieldTypeInvalid, "/publishedState/dspGraphDigest",
                       "Published DSP graph digest must be a string.");
        else
            published.dspGraphDigest = iterator->get<std::string>();
    }

    if (published.projectGeneration == 0
        || published.revision > result.hostState->authoringState.revision
        || published.authoredContentDigest.empty()
        || published.macroSchemaDigest.empty()
        || published.preparedContentDigest.empty())
    {
        addFinding(result,
                   HostSessionStateFindingCode::publishedStateInvalid,
                   "/publishedState",
                   "Published state requires a nonzero generation, a reachable revision, and three non-empty digests.");
    }

    result.hostState->publishedState = std::move(published);
}

ordered_json parseSerializedObject(const std::string& text)
{
    return ordered_json::parse(text);
}
} // namespace

HostSessionStateParseResult parseHostSessionState(const std::string& text,
                                                  const std::string& snapshotManifestPath)
{
    HostSessionStateParseResult result;
    result.state = "Host state absent";

    if (text.empty())
    {
        addFinding(result,
                   HostSessionStateFindingCode::payloadEmpty,
                   "/",
                   "Host-state payload is empty.",
                   HostSessionStateFindingSeverity::info);
        return result;
    }

    if (text.size() > hostSessionStateMaxBytes)
    {
        result.disposition = HostSessionStateParseDisposition::invalid;
        result.state = "Host state payload too large";
        addFinding(result,
                   HostSessionStateFindingCode::payloadTooLarge,
                   "/",
                   "Host-state payload exceeds the 2 MiB version 1 limit.");
        return result;
    }

    if (exceedsJsonDepth(text))
    {
        result.disposition = HostSessionStateParseDisposition::invalid;
        result.state = "Host state JSON too deep";
        addFinding(result,
                   HostSessionStateFindingCode::jsonDepthExceeded,
                   "/",
                   "Host-state JSON exceeds the maximum nesting depth of 64.");
        return result;
    }

    json root;
    try
    {
        root = json::parse(text);
    }
    catch (const json::exception& exception)
    {
        result.disposition = HostSessionStateParseDisposition::invalid;
        result.state = "Host state JSON invalid";
        addFinding(result,
                   HostSessionStateFindingCode::jsonInvalid,
                   "/",
                   std::string("Host-state JSON parse failed: ") + exception.what());
        return result;
    }

    if (!root.is_object())
    {
        result.disposition = HostSessionStateParseDisposition::invalid;
        result.state = "Host state root invalid";
        addFinding(result,
                   HostSessionStateFindingCode::rootInvalid,
                   "/",
                   "Host-state root must be an object.");
        return result;
    }

    result.disposition = HostSessionStateParseDisposition::invalid;
    result.state = "Host state invalid";
    validateStructuralBudgets(root, result, "");
    if (hasErrors(result.findings))
        return result;

    const auto schemaName = root.find("schemaName");
    if (schemaName != root.end() && schemaName->is_string()
        && schemaName->get<std::string>() == "drs.presetState")
    {
        const auto preset = parseRuntimePresetState(text);
        if (!preset.loaded)
        {
            result.disposition = HostSessionStateParseDisposition::invalid;
            result.state = "Legacy preset invalid";
            appendPresetIssues(result, preset);
            return result;
        }

        result.disposition = HostSessionStateParseDisposition::legacyPreset;
        result.state = "Legacy preset state loaded without project binding";
        result.legacyPreset = preset.preset;
        addFinding(result,
                   HostSessionStateFindingCode::none,
                   "/",
                   "Legacy preset state is valid but does not identify an authored project.",
                   HostSessionStateFindingSeverity::warning);
        return result;
    }

    result.hostState.emplace();

    static const std::unordered_set<std::string> allowed {
        "schemaName",
        "schemaVersion",
        "presetState",
        "performancePackageBinding",
        "projectBinding",
        "authoringState",
        "publishedState"
    };
    validateAllowedFields(root, result, allowed, "");

    if (const auto value = readRequiredString(root, result, "schemaName", ""))
        result.hostState->schemaName = *value;

    const auto schemaVersion = readRequiredSize(root, result, "schemaVersion", "");
    if (schemaVersion.has_value())
        result.hostState->schemaVersion = static_cast<int>(*schemaVersion);

    if (result.hostState->schemaName != hostSessionStateSchemaName)
    {
        addFinding(result,
                   HostSessionStateFindingCode::schemaNameInvalid,
                   "/schemaName",
                   "Host-state schemaName must be 'drs.hostState'.");
    }

    if (!schemaVersion.has_value())
    {
        addFinding(result,
                   HostSessionStateFindingCode::schemaVersionMissing,
                   "/schemaVersion",
                   "Host-state schemaVersion is required.");
    }
    else if (*schemaVersion != static_cast<std::size_t>(hostSessionStateSchemaVersion))
    {
        addFinding(result,
                   HostSessionStateFindingCode::schemaVersionUnsupported,
                   "/schemaVersion",
                   "Host-state schemaVersion is unsupported.");
    }

    const auto presetState = root.find("presetState");
    if (presetState == root.end())
    {
        addFinding(result,
                   HostSessionStateFindingCode::requiredFieldMissing,
                   "/presetState",
                   "Required object 'presetState' is missing.");
    }
    else if (!presetState->is_object())
    {
        addFinding(result,
                   HostSessionStateFindingCode::fieldTypeInvalid,
                   "/presetState",
                   "presetState must be an object.");
    }
    else
    {
        const auto preset = parseRuntimePresetState(presetState->dump());
        if (!preset.loaded)
            appendPresetIssues(result, preset);
        else
            result.hostState->presetState = preset.preset;
    }

    const auto performancePackageBinding = root.find("performancePackageBinding");
    if (performancePackageBinding != root.end())
    {
        if (!performancePackageBinding->is_object())
        {
            addFinding(result,
                       HostSessionStateFindingCode::fieldTypeInvalid,
                       "/performancePackageBinding",
                       "performancePackageBinding must be an object.");
        }
        else
        {
            parsePerformancePackageBinding(*performancePackageBinding, result);
        }
    }
    const auto packageState = result.hostState->performancePackageBinding.has_value();

    const auto projectBinding = root.find("projectBinding");
    if (projectBinding == root.end() && !packageState)
    {
        addFinding(result,
                   HostSessionStateFindingCode::requiredFieldMissing,
                   "/projectBinding",
                   "Required object 'projectBinding' is missing.");
    }
    else if (projectBinding != root.end() && !projectBinding->is_object())
    {
        addFinding(result,
                   HostSessionStateFindingCode::fieldTypeInvalid,
                   "/projectBinding",
                   "projectBinding must be an object.");
    }
    else if (projectBinding != root.end())
    {
        parseProjectBinding(*projectBinding, result);
    }

    const auto authoringState = root.find("authoringState");
    if (authoringState == root.end() && !packageState)
    {
        addFinding(result,
                   HostSessionStateFindingCode::requiredFieldMissing,
                   "/authoringState",
                   "Required object 'authoringState' is missing.");
    }
    else if (authoringState != root.end() && !authoringState->is_object())
    {
        addFinding(result,
                   HostSessionStateFindingCode::fieldTypeInvalid,
                   "/authoringState",
                   "authoringState must be an object.");
    }
    else if (authoringState != root.end())
    {
        parseAuthoringState(*authoringState, result, snapshotManifestPath);
    }

    const auto publishedState = root.find("publishedState");
    if (publishedState == root.end())
    {
        addFinding(result,
                   HostSessionStateFindingCode::requiredFieldMissing,
                   "/publishedState",
                   "Required object 'publishedState' is missing.");
    }
    else if (!publishedState->is_object())
    {
        addFinding(result,
                   HostSessionStateFindingCode::fieldTypeInvalid,
                   "/publishedState",
                   "publishedState must be an object.");
    }
    else
    {
        parsePublishedState(*publishedState, result);
    }

    if (hasErrors(result.findings))
    {
        result.hostState.reset();
        return result;
    }

    result.disposition = HostSessionStateParseDisposition::valid;
    result.state = "Host state loaded";
    return result;
}

HostSessionStateSerializeResult serializeHostSessionState(
    const HostSessionState& hostState,
    const std::string& snapshotManifestPath)
{
    HostSessionStateSerializeResult result;
    result.state = "Host state serialization failed";

    try
    {
        ordered_json root;
        root["schemaName"] = hostState.schemaName.empty()
            ? hostSessionStateSchemaName : hostState.schemaName;
        root["schemaVersion"] = hostState.schemaVersion == 0
            ? hostSessionStateSchemaVersion : hostState.schemaVersion;
        root["presetState"] = parseSerializedObject(serializeRuntimePresetState(hostState.presetState));
        if (hostState.performancePackageBinding.has_value())
        {
            ordered_json binding;
            binding["packageId"] = hostState.performancePackageBinding->packageId;
            binding["packagePath"] = hostState.performancePackageBinding->packagePath;
            binding["packageFileName"] = hostState.performancePackageBinding->packageFileName;
            root["performancePackageBinding"] = std::move(binding);
        }
        else
        {
            ordered_json binding;
            binding["projectId"] = hostState.projectBinding.projectId;
            if (!hostState.projectBinding.manifestPath.empty())
                binding["manifestPath"] = hostState.projectBinding.manifestPath;
            binding["manifestFileName"] = hostState.projectBinding.manifestFileName;
            binding["manifestDigest"] = hostState.projectBinding.manifestDigest;
            if (!hostState.projectBinding.contentRootHint.empty())
                binding["contentRootHint"] = hostState.projectBinding.contentRootHint;
            if (!hostState.projectBinding.portableRelativePath.empty())
                binding["portableRelativePath"] = hostState.projectBinding.portableRelativePath;
            root["projectBinding"] = std::move(binding);

            ordered_json authoring;
            authoring["revision"] = hostState.authoringState.revision;
            authoring["savedRevision"] = hostState.authoringState.savedRevision;
            authoring["dirty"] = hostState.authoringState.dirty;
            if (hostState.authoringState.projectSnapshot.has_value())
            {
                auto path = snapshotManifestPath;
                if (path.empty())
                    path = hostState.projectBinding.manifestPath;
                if (path.empty())
                    path = hostState.projectBinding.manifestFileName;
                authoring["projectSnapshot"] = parseSerializedObject(
                    serializeRuntimeProjectManifest(*hostState.authoringState.projectSnapshot, path));
            }
            root["authoringState"] = std::move(authoring);
        }

        ordered_json published = ordered_json::object();
        if (hostState.publishedState.has_value())
        {
            published["revision"] = hostState.publishedState->revision;
            published["projectGeneration"] = hostState.publishedState->projectGeneration;
            published["authoredContentDigest"] = hostState.publishedState->authoredContentDigest;
            published["macroSchemaDigest"] = hostState.publishedState->macroSchemaDigest;
            published["preparedContentDigest"] = hostState.publishedState->preparedContentDigest;
            if (!hostState.publishedState->dspGraphDigest.empty())
                published["dspGraphDigest"] = hostState.publishedState->dspGraphDigest;
        }
        root["publishedState"] = std::move(published);

        auto text = root.dump(2) + "\n";
        const auto validation = parseHostSessionState(text, snapshotManifestPath);
        if (!validation.isValidHostState())
        {
            result.findings = validation.findings;
            return result;
        }

        result.serialized = true;
        result.state = "Host state serialized";
        result.text = std::move(text);
        return result;
    }
    catch (const json::exception& exception)
    {
        addFinding(result,
                   HostSessionStateFindingCode::jsonInvalid,
                   "/",
                   std::string("Host-state serialization failed: ") + exception.what());
        return result;
    }
}

std::string computeHostProjectManifestDigest(const RuntimeProjectModel& project,
                                             const std::string& manifestPath)
{
    const auto canonical = serializeRuntimeProjectManifest(project, manifestPath);
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : canonical)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }

    std::ostringstream stream;
    stream << "fnv1a64:" << std::hex << std::setw(16) << std::setfill('0') << hash;
    return stream.str();
}

HostProjectBindingVerification verifyHostProjectBinding(
    const HostProjectBinding& binding,
    const RuntimeProjectModel& project,
    const std::string& manifestPath)
{
    HostProjectBindingVerification result;
    result.expectedProjectId = binding.projectId;
    result.actualProjectId = project.projectId;
    result.expectedManifestDigest = binding.manifestDigest;
    result.actualManifestDigest = computeHostProjectManifestDigest(project, manifestPath);

    if (result.expectedProjectId != result.actualProjectId)
    {
        result.match = HostProjectBindingMatch::projectIdentityMismatch;
        return result;
    }

    if (result.expectedManifestDigest != result.actualManifestDigest)
    {
        result.match = HostProjectBindingMatch::manifestDigestMismatch;
        return result;
    }

    result.match = HostProjectBindingMatch::match;
    return result;
}

const char* toString(const HostSessionStateParseDisposition disposition) noexcept
{
    switch (disposition)
    {
        case HostSessionStateParseDisposition::absent: return "absent";
        case HostSessionStateParseDisposition::invalid: return "invalid";
        case HostSessionStateParseDisposition::legacyPreset: return "legacyPreset";
        case HostSessionStateParseDisposition::valid: return "valid";
    }

    return "invalid";
}

const char* toString(const HostProjectBindingMatch match) noexcept
{
    switch (match)
    {
        case HostProjectBindingMatch::match: return "match";
        case HostProjectBindingMatch::projectIdentityMismatch: return "projectIdentityMismatch";
        case HostProjectBindingMatch::manifestDigestMismatch: return "manifestDigestMismatch";
    }

    return "manifestDigestMismatch";
}

const char* toString(const HostSessionStateFindingSeverity severity) noexcept
{
    switch (severity)
    {
        case HostSessionStateFindingSeverity::info: return "info";
        case HostSessionStateFindingSeverity::warning: return "warning";
        case HostSessionStateFindingSeverity::error: return "error";
    }

    return "error";
}

const char* toString(const HostSessionStateFindingCode code) noexcept
{
    switch (code)
    {
        case HostSessionStateFindingCode::none: return "none";
        case HostSessionStateFindingCode::payloadEmpty: return "host-state-payload-empty";
        case HostSessionStateFindingCode::payloadTooLarge: return "host-state-payload-too-large";
        case HostSessionStateFindingCode::jsonInvalid: return "host-state-json-invalid";
        case HostSessionStateFindingCode::jsonDepthExceeded: return "host-state-json-depth-exceeded";
        case HostSessionStateFindingCode::rootInvalid: return "host-state-root-invalid";
        case HostSessionStateFindingCode::schemaNameMissing: return "host-state-schema-name-missing";
        case HostSessionStateFindingCode::schemaNameInvalid: return "host-state-schema-name-invalid";
        case HostSessionStateFindingCode::schemaVersionMissing: return "host-state-schema-version-missing";
        case HostSessionStateFindingCode::schemaVersionUnsupported: return "host-state-schema-version-unsupported";
        case HostSessionStateFindingCode::unknownField: return "host-state-unknown-field";
        case HostSessionStateFindingCode::requiredFieldMissing: return "host-state-required-field-missing";
        case HostSessionStateFindingCode::fieldTypeInvalid: return "host-state-field-type-invalid";
        case HostSessionStateFindingCode::fieldValueInvalid: return "host-state-field-value-invalid";
        case HostSessionStateFindingCode::stringTooLong: return "host-state-string-too-long";
        case HostSessionStateFindingCode::collectionTooLarge: return "host-state-collection-too-large";
        case HostSessionStateFindingCode::projectSnapshotTooLarge: return "host-state-project-snapshot-too-large";
        case HostSessionStateFindingCode::presetInvalid: return "host-state-preset-invalid";
        case HostSessionStateFindingCode::projectBindingInvalid: return "host-state-project-binding-invalid";
        case HostSessionStateFindingCode::performancePackageBindingInvalid:
            return "host-state-performance-package-binding-invalid";
        case HostSessionStateFindingCode::authoringStateInvalid: return "host-state-authoring-state-invalid";
        case HostSessionStateFindingCode::projectSnapshotInvalid: return "host-state-project-snapshot-invalid";
        case HostSessionStateFindingCode::projectIdentityMismatch: return "host-state-project-identity-mismatch";
        case HostSessionStateFindingCode::projectManifestDigestMismatch: return "host-state-project-manifest-digest-mismatch";
        case HostSessionStateFindingCode::publishedStateInvalid: return "host-state-published-state-invalid";
    }

    return "host-state-unknown-finding";
}
} // namespace drs::engine
