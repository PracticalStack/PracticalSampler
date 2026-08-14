#pragma once

#include <string>
#include <vector>

namespace drs::engine
{
inline constexpr const char* performancePackageSchemaName = "drs.performancePackage";
inline constexpr int performancePackageLegacySchemaVersion = 1;
inline constexpr int performancePackageFxRoutingSchemaVersion = 2;
inline constexpr int performancePackageFxRoutingMinimumReaderSchemaVersion = 2;

// PX-01 freezes the target versions without changing production behavior. The
// writer and reader move off the legacy alias only when graph support ships.
inline constexpr int performancePackageSchemaVersion = performancePackageLegacySchemaVersion;
inline constexpr int performancePackageSchemaMajorVersion = 1;
inline constexpr int performancePackageSchemaMinorVersion = 0;
inline constexpr const char* performancePackageFileExtension = ".drpkg";
inline constexpr const char* performancePackageCompatibilityPolicyId = "drs.performancePackage.policy.v1.0";
inline constexpr const char* performancePackageFutureMinorPolicy
    = "Future minor changes must remain additive, keep schemaName and formatVersion stable, and require no higher minimumReaderSchemaVersion than the current reader.";
inline constexpr const char* performancePackageFutureMajorPolicy
    = "Future major changes require a new incompatible reader contract; current readers reject packages that require a higher minimumReaderSchemaVersion or unsupported formatVersion.";

enum class PerformancePackageFailureCategory
{
    none,
    packageFormatFailure,
    decryptionFailure,
    payloadCorruption,
    playbackCompatibilityFailure
};

enum class WorkspaceDocumentKind
{
    authoringProject,
    performancePackage
};

enum class WorkspaceMode
{
    authoring,
    performanceOnly
};

enum class PackageSessionReadiness
{
    metadataLoaded,
    openingSources,
    preparingHeads,
    buildingModel,
    playbackDeferred,
    playable,
    pendingActivation,
    active,
    degraded,
    failed,
    cancelled,
    streamingRequired
};

struct PerformancePackageManifest
{
    struct GroupRoute
    {
        std::string groupId;
        double gainDb = 0.0;
    };

    struct BackgroundImage
    {
        std::string payloadId;
    };

    std::string schemaName = performancePackageSchemaName;
    int schemaVersion = performancePackageSchemaVersion;
    std::string packageId;
    std::string displayName;
    std::string instrumentId;
    std::string defaultLoadProfile;
    int minimumReaderSchemaVersion = performancePackageSchemaVersion;
    double masterGainDb = 0.0;
    std::vector<GroupRoute> groupRoutes;
    BackgroundImage backgroundImage;
    std::vector<std::string> notes;
};

struct WorkspaceDocumentState
{
    WorkspaceDocumentKind kind = WorkspaceDocumentKind::authoringProject;
    WorkspaceMode workspaceMode = WorkspaceMode::authoring;
    std::string displayName;
    std::string sourcePath;
    std::string documentId;
    int schemaVersion = 0;
    int minimumReaderSchemaVersion = 0;
    bool authoringAvailable = true;
    bool dirty = false;
    PackageSessionReadiness readiness = PackageSessionReadiness::playbackDeferred;
    bool playable = false;
};

inline const char* toString(const WorkspaceDocumentKind kind) noexcept
{
    switch (kind)
    {
        case WorkspaceDocumentKind::authoringProject:
            return "authoringProject";
        case WorkspaceDocumentKind::performancePackage:
            return "performancePackage";
    }

    return "unknown";
}

inline const char* toString(const PackageSessionReadiness readiness) noexcept
{
    switch (readiness)
    {
        case PackageSessionReadiness::metadataLoaded:
            return "metadata-loaded";
        case PackageSessionReadiness::openingSources:
            return "opening-sources";
        case PackageSessionReadiness::preparingHeads:
            return "preparing-heads";
        case PackageSessionReadiness::buildingModel:
            return "building-model";
        case PackageSessionReadiness::playbackDeferred:
            return "playback-deferred";
        case PackageSessionReadiness::playable:
            return "playable";
        case PackageSessionReadiness::pendingActivation:
            return "pending-activation";
        case PackageSessionReadiness::active:
            return "active";
        case PackageSessionReadiness::degraded:
            return "degraded";
        case PackageSessionReadiness::failed:
            return "failed";
        case PackageSessionReadiness::cancelled:
            return "cancelled";
        case PackageSessionReadiness::streamingRequired:
            return "streaming-required";
    }

    return "unknown";
}

inline std::string packageWorkspaceStatusText(const PackageSessionReadiness readiness)
{
    switch (readiness)
    {
        case PackageSessionReadiness::metadataLoaded:
            return "Package metadata loaded | Playback deferred";
        case PackageSessionReadiness::openingSources:
            return "Opening package sample sources";
        case PackageSessionReadiness::preparingHeads:
            return "Preparing bounded sample heads";
        case PackageSessionReadiness::buildingModel:
            return "Building package render model";
        case PackageSessionReadiness::playbackDeferred:
            return "Playback deferred";
        case PackageSessionReadiness::playable:
            return "Playable package";
        case PackageSessionReadiness::pendingActivation:
            return "Playable package | Audio activation pending";
        case PackageSessionReadiness::active:
            return "Playable package";
        case PackageSessionReadiness::degraded:
            return "Previous package active | Replacement degraded";
        case PackageSessionReadiness::failed:
            return "Package preparation failed";
        case PackageSessionReadiness::cancelled:
            return "Package preparation cancelled";
        case PackageSessionReadiness::streamingRequired:
            return "Streaming required";
    }
    return "Playback deferred";
}

inline std::string packageWorkspaceStatusTooltip(const WorkspaceDocumentState& document)
{
    std::string result;
    switch (document.readiness)
    {
        case PackageSessionReadiness::playable:
        case PackageSessionReadiness::active:
            result = "Read-only playable package session.";
            break;
        case PackageSessionReadiness::degraded:
            result = "The previous package remains active; the replacement requires attention.";
            break;
        case PackageSessionReadiness::failed:
            result = "Package preparation failed before audio activation.";
            break;
        case PackageSessionReadiness::cancelled:
            result = "Package preparation was cancelled.";
            break;
        default:
            result = "Package metadata is loaded; bounded preparation continues before audio activation.";
            break;
    }
    if (!document.sourcePath.empty())
        result += "\nSource: " + document.sourcePath;
    result += "\nCompatible reader schema: v" + std::to_string(document.minimumReaderSchemaVersion);
    return result;
}

inline const char* toString(const PerformancePackageFailureCategory category) noexcept
{
    switch (category)
    {
        case PerformancePackageFailureCategory::none:
            return "none";
        case PerformancePackageFailureCategory::packageFormatFailure:
            return "package-format-failure";
        case PerformancePackageFailureCategory::decryptionFailure:
            return "decryption-failure";
        case PerformancePackageFailureCategory::payloadCorruption:
            return "payload-corruption";
        case PerformancePackageFailureCategory::playbackCompatibilityFailure:
            return "playback-compatibility-failure";
    }

    return "unknown";
}

inline const char* toString(const WorkspaceMode mode) noexcept
{
    switch (mode)
    {
        case WorkspaceMode::authoring:
            return "authoring";
        case WorkspaceMode::performanceOnly:
            return "performanceOnly";
    }

    return "unknown";
}
} // namespace drs::engine
