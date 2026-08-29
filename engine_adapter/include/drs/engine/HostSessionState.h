#pragma once

#include "drs/engine/RuntimeModel.h"
#include "drs/engine/RuntimePresetState.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace drs::engine
{
inline constexpr const char* hostSessionStateSchemaName = "drs.hostState";
inline constexpr int hostSessionStateSchemaVersion = 1;

// Schema-7 piano projects persist an immutable 128-point damper curve per route.
// The 16 MiB envelope admits the qualified 1,700-route schema-7 Salamander
// snapshot, including explicit zone pan fields and immutable 128-point damper
// curves, while retaining a strict pre-parse ceiling and excluding every
// sample/stream byte.
inline constexpr std::size_t hostSessionStateMaxBytes = 16u * 1024u * 1024u;
inline constexpr std::size_t hostSessionStateMaxProjectSnapshotBytes = 15u * 1024u * 1024u;
inline constexpr std::size_t hostSessionStateMaxJsonDepth = 64u;
inline constexpr std::size_t hostSessionStateMaxStringBytes = 64u * 1024u;
inline constexpr std::size_t hostSessionStateMaxPathBytes = 32u * 1024u;
inline constexpr std::size_t hostSessionStateMaxIdentityBytes = 512u;
inline constexpr std::size_t hostSessionStateMaxSampleSources = 8192u;
inline constexpr std::size_t hostSessionStateMaxZones = 65536u;
inline constexpr std::size_t hostSessionStateMaxGroups = 2048u;
inline constexpr std::size_t hostSessionStateMaxMacros = 128u;
inline constexpr std::size_t hostSessionStateMaxFxSlots = 128u;
inline constexpr std::size_t hostSessionStateMaxDspParameters = 1024u;
inline constexpr std::size_t hostSessionStateMaxRoutingBuses = 128u;
inline constexpr std::size_t hostSessionStateMaxPerformanceBanks = 256u;
inline constexpr std::size_t hostSessionStateMaxNotesOrIssues = 4096u;

enum class HostSessionStateParseDisposition
{
    absent,
    invalid,
    legacyPreset,
    valid
};

enum class HostSessionStateFindingSeverity
{
    info,
    warning,
    error
};

enum class HostSessionStateFindingCode
{
    none,
    payloadEmpty,
    payloadTooLarge,
    jsonInvalid,
    jsonDepthExceeded,
    rootInvalid,
    schemaNameMissing,
    schemaNameInvalid,
    schemaVersionMissing,
    schemaVersionUnsupported,
    unknownField,
    requiredFieldMissing,
    fieldTypeInvalid,
    fieldValueInvalid,
    stringTooLong,
    collectionTooLarge,
    projectSnapshotTooLarge,
    presetInvalid,
    projectBindingInvalid,
    performancePackageBindingInvalid,
    authoringStateInvalid,
    projectSnapshotInvalid,
    projectIdentityMismatch,
    projectManifestDigestMismatch,
    publishedStateInvalid
};

struct HostSessionStateFinding
{
    HostSessionStateFindingSeverity severity = HostSessionStateFindingSeverity::error;
    HostSessionStateFindingCode code = HostSessionStateFindingCode::none;
    std::string path;
    std::string message;
};

struct HostProjectBinding
{
    std::string projectId;
    std::string manifestPath;
    std::string manifestFileName;
    std::string manifestDigest;
    std::string contentRootHint;
    std::string portableRelativePath;
};

struct HostPerformancePackageBinding
{
    std::string packageId;
    std::string packagePath;
    std::string packageFileName;
};

struct HostAuthoringCheckpoint
{
    std::size_t revision = 0;
    std::size_t savedRevision = 0;
    bool dirty = false;
    std::optional<RuntimeProjectModel> projectSnapshot;
};

struct HostPublishedCheckpoint
{
    std::size_t revision = 0;
    std::uint64_t projectGeneration = 0;
    std::string authoredContentDigest;
    std::string macroSchemaDigest;
    std::string preparedContentDigest;
    std::string dspGraphDigest;
};

enum class HostProjectBindingMatch
{
    match,
    projectIdentityMismatch,
    manifestDigestMismatch
};

struct HostProjectBindingVerification
{
    HostProjectBindingMatch match = HostProjectBindingMatch::manifestDigestMismatch;
    std::string expectedProjectId;
    std::string actualProjectId;
    std::string expectedManifestDigest;
    std::string actualManifestDigest;

    bool matched() const noexcept { return match == HostProjectBindingMatch::match; }
};

struct HostSessionState
{
    std::string schemaName = hostSessionStateSchemaName;
    int schemaVersion = hostSessionStateSchemaVersion;
    RuntimePresetState presetState;
    std::optional<HostPerformancePackageBinding> performancePackageBinding;
    HostProjectBinding projectBinding;
    HostAuthoringCheckpoint authoringState;
    std::optional<HostPublishedCheckpoint> publishedState;
};

struct HostSessionStateParseResult
{
    HostSessionStateParseDisposition disposition = HostSessionStateParseDisposition::absent;
    std::string state;
    std::vector<HostSessionStateFinding> findings;
    std::optional<HostSessionState> hostState;
    std::optional<RuntimePresetState> legacyPreset;

    bool isValidHostState() const noexcept
    {
        return disposition == HostSessionStateParseDisposition::valid && hostState.has_value();
    }

    bool isLegacyPreset() const noexcept
    {
        return disposition == HostSessionStateParseDisposition::legacyPreset
            && legacyPreset.has_value();
    }
};

struct HostSessionStateSerializeResult
{
    bool serialized = false;
    std::string state;
    std::vector<HostSessionStateFinding> findings;
    std::string text;
};

HostSessionStateParseResult parseHostSessionState(const std::string& text,
                                                  const std::string& snapshotManifestPath = {});
HostSessionStateSerializeResult serializeHostSessionState(
    const HostSessionState& hostState,
    const std::string& snapshotManifestPath = {});
std::string computeHostProjectManifestDigest(const RuntimeProjectModel& project,
                                             const std::string& manifestPath);
HostProjectBindingVerification verifyHostProjectBinding(
    const HostProjectBinding& binding,
    const RuntimeProjectModel& project,
    const std::string& manifestPath);

const char* toString(HostSessionStateParseDisposition disposition) noexcept;
const char* toString(HostProjectBindingMatch match) noexcept;
const char* toString(HostSessionStateFindingSeverity severity) noexcept;
const char* toString(HostSessionStateFindingCode code) noexcept;
} // namespace drs::engine
