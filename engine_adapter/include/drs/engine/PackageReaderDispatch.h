#pragma once

#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageV2.h"
#include "drs/engine/PackageV3.h"
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
} // namespace drs::engine
