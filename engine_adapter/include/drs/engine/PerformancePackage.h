#pragma once

#include <string>
#include <vector>

namespace drs::engine
{
inline constexpr const char* performancePackageSchemaName = "drs.performancePackage";
inline constexpr int performancePackageSchemaVersion = 1;
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

struct PerformancePackageManifest
{
    std::string schemaName = performancePackageSchemaName;
    int schemaVersion = performancePackageSchemaVersion;
    std::string packageId;
    std::string displayName;
    std::string instrumentId;
    std::string defaultLoadProfile;
    int minimumReaderSchemaVersion = performancePackageSchemaVersion;
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
