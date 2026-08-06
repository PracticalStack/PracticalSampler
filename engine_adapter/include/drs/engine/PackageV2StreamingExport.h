#pragma once

#include "drs/engine/PackageV2.h"
#include "drs/engine/SampleDataSource.h"
#include "drs/engine/PackageWriter.h"

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct PackageV2CompiledSampleInput
{
    std::string sourceId;
    std::uint64_t sourceGeneration = 1;
    std::string compiledFloatPayloadPath;
    std::uint64_t dataOffsetBytes = 0;
    std::uint64_t dataSizeBytes = 0;
    std::uint64_t headBytes = 16ull * 1024ull;
    std::uint64_t pageBytes = 64ull * 1024ull;
};

struct PackageV2WavSampleInput
{
    std::string sourceId;
    std::uint64_t sourceGeneration = 0;
    std::string wavPath;
    std::uint64_t headBytes = defaultSampleHeadBytes;
    std::uint64_t pageBytes = defaultSamplePageBytes;
};

struct PackageV2StreamingExportBuildResult
{
    bool built = false;
    std::string state;
    std::vector<std::string> issues;
    std::uint64_t totalPlaintextBytes = 0;
    std::vector<SampleDataSourceDescriptor> sampleDescriptors;
    PerformancePackageManifest manifest;
    PackageV2StreamingWritePlan plan;
};

PackageV2StreamingExportBuildResult buildPackageV2StreamingExportPlan(
    const std::string& packageId,
    const std::string& outputPath,
    const std::vector<PackageV2RecordSource>& metadataRecords,
    const std::vector<PackageV2CompiledSampleInput>& samples);

PackageV2StreamingExportBuildResult buildPackageV2WavStreamingExportPlan(
    const std::string& packageId,
    const std::string& outputPath,
    const std::vector<PackageV2RecordSource>& metadataRecords,
    const std::vector<PackageV2WavSampleInput>& samples);

void appendPackageV2MetadataChunks(
    std::vector<PackageV2RecordSource>& records,
    const std::string& sourceId,
    PackageV2RecordKind kind,
    const std::vector<std::uint8_t>& bytes,
    std::uint64_t sourceGeneration = 1);

PackageV2StreamingExportBuildResult buildPerformancePackageV2StreamingExportPlan(
    PerformancePackageManifest manifest,
    const RuntimeCompileResult& compiledRuntime,
    const std::string& outputPath,
    const std::vector<PerformancePackagePayloadSource>& additionalPayloads = {});
} // namespace drs::engine
