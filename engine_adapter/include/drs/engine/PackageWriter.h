#pragma once

#include "drs/engine/PackageCrypto.h"
#include "drs/engine/PerformancePackage.h"
#include "drs/engine/RuntimeCompiler.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace drs::engine
{
inline constexpr const char* performancePackageInternalUriPrefix = "package://payload/";
inline constexpr std::uint32_t performancePackageFormatVersion = 1;

enum class PerformancePackagePayloadKind
{
    packageManifest,
    runtimeInstrument,
    runtimeStreamIndex,
    runtimeStreamPayload,
    backgroundImage
};

struct PerformancePackagePayloadSource
{
    std::string payloadId;
    PerformancePackagePayloadKind kind = PerformancePackagePayloadKind::packageManifest;
    std::string logicalPath;
    std::string mediaType;
    std::vector<std::uint8_t> plaintextBytes;
};

struct PerformancePackageCompileWritePlan
{
    PerformancePackageManifest manifest;
    RuntimeCompileResult compiledRuntime;
    std::string outputPackagePath;
    std::string minimumCompatibleAppVersion = "0.0.0-internal";
    std::vector<PerformancePackagePayloadSource> additionalPayloads;
};

struct PerformancePackageWritePlan
{
    PerformancePackageManifest manifest;
    std::string outputPackagePath;
    std::string minimumCompatibleAppVersion = "0.0.0-internal";
    std::vector<PerformancePackagePayloadSource> payloads;
};

struct PerformancePackageWriteResult
{
    bool written = false;
    std::string state;
    std::vector<std::string> issues;
    std::string packagePath;
    std::string cryptoAlgorithm;
    std::uint64_t packageBytes = 0;
    std::uint32_t payloadCount = 0;
};

enum class PerformancePackageWriteStage
{
    preparing,
    loadingPayloads,
    sealingPayloads,
    sealingToc,
    writingPackage,
    completed,
    canceled,
    failed
};

struct PerformancePackageWriteProgress
{
    PerformancePackageWriteStage stage = PerformancePackageWriteStage::preparing;
    std::size_t completedPayloadCount = 0;
    std::size_t totalPayloadCount = 0;
    std::uint64_t bytesProcessed = 0;
    std::uint64_t totalBytes = 0;
    std::string payloadId;
    std::string status;
};

struct PerformancePackageWriteOptions
{
    std::function<void(const PerformancePackageWriteProgress&)> progressSink;
    std::function<bool()> cancellationProbe;
};

struct PerformancePackageHeaderView
{
    std::uint32_t formatVersion = 0;
    std::uint32_t headerSizeBytes = 0;
    std::uint32_t flags = 0;
    int minimumReaderSchemaVersion = 0;
    std::uint64_t cleartextMetadataOffsetBytes = 0;
    std::uint64_t cleartextMetadataSizeBytes = 0;
    std::uint64_t tocOffsetBytes = 0;
    std::uint64_t tocSealedSizeBytes = 0;
    std::uint64_t payloadRegionOffsetBytes = 0;
    std::uint64_t payloadRegionSizeBytes = 0;
    std::uint32_t payloadCount = 0;
};

struct PerformancePackagePayloadView
{
    std::string payloadId;
    std::string payloadKind;
    std::string logicalPath;
    std::string mediaType;
    std::uint64_t payloadOffsetBytes = 0;
    std::uint64_t sealedSizeBytes = 0;
    std::uint64_t plaintextSizeBytes = 0;
    std::string plaintextChecksumHex;
    std::vector<std::uint8_t> plaintextBytes;
};

struct PerformancePackageInspectionResult
{
    bool packageFound = false;
    bool valid = false;
    std::string packagePath;
    std::string state;
    std::vector<std::string> issues;
    PerformancePackageHeaderView header;
    PerformancePackageManifest cleartextManifest;
    std::string minimumCompatibleAppVersion;
    std::string cryptoAlgorithm;
    std::string cleartextMetadataJson;
    std::string tocJson;
    std::vector<PerformancePackagePayloadView> payloads;
};

const char* toString(PerformancePackagePayloadKind kind) noexcept;

PerformancePackageWritePlan buildPerformancePackageWritePlan(
    const PerformancePackageCompileWritePlan& plan,
    const PerformancePackageWriteOptions& options = {});

PerformancePackageWriteResult writePerformancePackage(
    const PerformancePackageWritePlan& plan,
    const PackageCryptoProvider& cryptoProvider = getDeterministicPackageCryptoProvider(),
    const PerformancePackageWriteOptions& options = {});

PerformancePackageWriteResult writePerformancePackage(
    const PerformancePackageCompileWritePlan& plan,
    const PackageCryptoProvider& cryptoProvider = getDeterministicPackageCryptoProvider(),
    const PerformancePackageWriteOptions& options = {});

std::size_t getPerformancePackageHeaderSizeBytes() noexcept;
std::size_t getPerformancePackageHeaderPayloadCountOffsetBytes() noexcept;

PerformancePackageInspectionResult inspectPerformancePackage(
    const std::string& packagePath,
    const PackageCryptoProvider& cryptoProvider = getDeterministicPackageCryptoProvider(),
    int supportedReaderSchemaVersion = performancePackageSchemaVersion);
} // namespace drs::engine
