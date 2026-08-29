#pragma once

#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageKeys.h"
#include "drs/engine/PackagePublisherTrustStore.h"
#include "drs/engine/PackageV2.h"
#include "drs/engine/PackageV3.h"
#include "drs/engine/PackageV3FileReader.h"
#include "drs/engine/SampleDataSource.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace drs::engine
{
enum class PerformancePackageDiskFormat
{
    unknown,
    version1,
    version2,
    version3
};

struct PerformancePackageDispatchResult
{
    bool opened = false;
    bool migrationRequired = false;
    PerformancePackageDiskFormat format = PerformancePackageDiskFormat::unknown;
    std::uint64_t packageBytes = 0;
    std::string state;
    std::vector<std::string> issues;
    PerformancePackageReaderResult version1;
    PackageV2OpenResult version2;
    PackageV3OpenResult version3;
};

PerformancePackageDispatchResult dispatchPerformancePackageReader(
    const std::string& packagePath,
    const PackageCryptoProvider& crypto = getDeterministicPackageCryptoProvider(),
    int supportedV1ReaderSchemaVersion = performancePackageSchemaVersion);

struct PerformancePackageV2MetadataLoadResult
{
    bool loaded = false;
    std::string state;
    std::vector<std::string> issues;
    std::shared_ptr<const PackageV2OpenResult> package;
    PerformancePackageLoadResult metadata;
    std::vector<SampleDataSourceDescriptor> sampleDescriptors;
};

PerformancePackageV2MetadataLoadResult loadPerformancePackageV2Metadata(
    const std::string& packagePath,
    const PackageCryptoProvider& crypto = getDeterministicPackageCryptoProvider(),
    int supportedReaderSchemaVersion = performancePackageFxRoutingMinimumReaderSchemaVersion);

// Read-only V1 compatibility entry points. The legacy deterministic provider
// is intentionally selected inside the reader boundary and is never exposed
// to production export code.
PerformancePackageLoadResult loadLegacyPerformancePackageV1(
    const std::string& packagePath,
    int supportedReaderSchemaVersion = performancePackageSchemaVersion);

PerformancePackageLoadResult loadLegacyPerformancePackageV1MetadataOnly(
    const std::string& packagePath,
    int supportedReaderSchemaVersion = performancePackageSchemaVersion);

inline constexpr const char* performancePackageV3CompatibilityId
    = "practical-sampler.performance-package.v3";

enum class PerformancePackageV3ActivationFailure
{
    none,
    configuration,
    format,
    signature,
    keyUnavailable,
    compatibility,
    authentication,
    corruption,
    io
};

const char* toString(PerformancePackageV3ActivationFailure failure) noexcept;

struct PerformancePackageV3ActivationSecurityContext
{
    std::string compatibilityId = performancePackageV3CompatibilityId;
    std::shared_ptr<const PackageKeyProvider> keyProvider;
    std::shared_ptr<const PackagePublisherTrustStore> trustStore;

    bool valid() const noexcept
    {
        return ! compatibilityId.empty() && keyProvider != nullptr
            && trustStore != nullptr && trustStore->valid();
    }
};

struct PerformancePackageV3MetadataLoadResult
{
    bool loaded = false;
    PerformancePackageV3ActivationFailure failure
        = PerformancePackageV3ActivationFailure::none;
    std::string state;
    std::vector<std::string> issues;
    std::shared_ptr<const PackageV3FileOpenResult> package;
    std::shared_ptr<const SecureBuffer> contentKey;
    PerformancePackageLoadResult metadata;
    std::vector<SampleDataSourceDescriptor> sampleDescriptors;
};

PerformancePackageV3MetadataLoadResult loadPerformancePackageV3Metadata(
    const std::string& packagePath,
    const PerformancePackageV3ActivationSecurityContext& securityContext,
    int supportedReaderSchemaVersion = performancePackageFxRoutingMinimumReaderSchemaVersion);
} // namespace drs::engine
