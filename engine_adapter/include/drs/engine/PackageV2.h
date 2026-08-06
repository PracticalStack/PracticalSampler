#pragma once

#include "drs/engine/PackageCrypto.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace drs::engine
{
inline constexpr std::uint32_t performancePackageV2FormatVersion = 2;
inline constexpr std::uint64_t performancePackageV2MaximumRecordBytes = 64ull * 1024ull;

enum class PackageV2RecordKind : std::uint32_t
{
    manifest = 1,
    runtimeInstrument = 2,
    streamIndex = 3,
    sampleHead = 4,
    samplePage = 5,
    backgroundImage = 6
};

enum class PackageV2Failure
{
    none,
    missing,
    format,
    unsupportedVersion,
    bounds,
    duplicateRecord,
    recordTooLarge,
    authentication,
    checksum,
    cancelled,
    io
};

struct PackageV2RecordIdentity
{
    std::string sourceId;
    PackageV2RecordKind kind = PackageV2RecordKind::manifest;
    std::uint64_t pageIndex = 0;
    std::uint64_t sourceGeneration = 1;
};

struct PackageV2RecordSource
{
    PackageV2RecordIdentity identity;
    std::vector<std::uint8_t> plaintextBytes;
};

struct PackageV2WritePlan
{
    std::string packageId;
    std::string outputPath;
    std::vector<PackageV2RecordSource> records;
};

struct PackageV2RecordDescriptor
{
    PackageV2RecordIdentity identity;
    std::uint64_t sealedOffsetBytes = 0;
    std::uint64_t sealedSizeBytes = 0;
    std::uint64_t plaintextSizeBytes = 0;
    std::string plaintextChecksumHex;
};

struct PackageV2Metrics
{
    std::uint64_t bytesRead = 0;
    std::uint64_t recordsOpened = 0;
    std::uint64_t largestPlaintextRecordBytes = 0;
    std::uint64_t authenticationFailures = 0;
    std::uint64_t checksumFailures = 0;
    std::uint64_t cancellationCount = 0;
};

struct PackageV2WriteResult
{
    bool written = false;
    PackageV2Failure failure = PackageV2Failure::none;
    std::string state;
    std::vector<std::string> issues;
    std::uint64_t packageBytes = 0;
};

enum class PackageV2StreamingWriteStage
{
    preparing,
    loadingRecord,
    sealingRecord,
    writingRecord,
    finalizingToc,
    verifying,
    publishing,
    completed,
    cancelled,
    failed
};

struct PackageV2StreamingRecordSource
{
    PackageV2RecordIdentity identity;
    std::uint64_t expectedPlaintextBytes = 0;
    std::string sourceLabel;
    std::function<bool(std::vector<std::uint8_t>&, std::string&)> loadPlaintext;
};

struct PackageV2StreamingWriteProgress
{
    PackageV2StreamingWriteStage stage = PackageV2StreamingWriteStage::preparing;
    std::size_t recordIndex = 0;
    std::size_t recordCount = 0;
    std::uint64_t completedPlaintextBytes = 0;
    std::uint64_t totalPlaintextBytes = 0;
    PackageV2RecordIdentity identity;
    std::string status;
};

struct PackageV2StreamingWriteOptions
{
    std::function<void(const PackageV2StreamingWriteProgress&)> progressSink;
    std::function<bool()> cancellationProbe;
};

struct PackageV2StreamingWritePlan
{
    std::string packageId;
    std::string outputPath;
    std::vector<PackageV2StreamingRecordSource> records;
};

struct PackageV2StreamingWriteResult : PackageV2WriteResult
{
    bool verified = false;
    bool atomicallyPublished = false;
    std::string stagingPath;
    std::uint64_t processedPlaintextBytes = 0;
    std::uint64_t peakPlaintextBufferBytes = 0;
    std::uint64_t peakSealedBufferBytes = 0;
    std::uint64_t verificationBytesRead = 0;
    std::uint64_t completedRecordCount = 0;
    std::uint64_t loadDurationMicros = 0;
    std::uint64_t sealDurationMicros = 0;
    std::uint64_t writeDurationMicros = 0;
    std::uint64_t verificationDurationMicros = 0;
    std::uint64_t totalDurationMicros = 0;
    std::uint64_t cancellationResponseMicros = 0;
    double plaintextThroughputBytesPerSecond = 0.0;
};

struct PackageV2OpenResult
{
    bool opened = false;
    PackageV2Failure failure = PackageV2Failure::none;
    std::string state;
    std::vector<std::string> issues;
    std::string packageId;
    std::string packagePath;
    std::uint64_t packageBytes = 0;
    std::uint64_t tocBytes = 0;
    std::vector<PackageV2RecordDescriptor> records;
    std::unordered_map<std::string, std::size_t> recordIndexByIdentity;
};

struct PackageV2RecordOpenResult
{
    bool opened = false;
    PackageV2Failure failure = PackageV2Failure::none;
    std::string state;
    std::vector<std::string> issues;
    PackageV2RecordDescriptor descriptor;
    std::vector<std::uint8_t> plaintextBytes;
    PackageV2Metrics metrics;
};

const char* toString(PackageV2RecordKind kind) noexcept;
const char* toString(PackageV2Failure failure) noexcept;

PackageV2WriteResult writePackageV2(
    const PackageV2WritePlan& plan,
    const PackageCryptoProvider& crypto = getDeterministicPackageCryptoProvider(),
    const std::function<bool()>& cancellationProbe = {});

PackageV2StreamingWriteResult writePackageV2Streaming(
    const PackageV2StreamingWritePlan& plan,
    const PackageCryptoProvider& crypto = getDeterministicPackageCryptoProvider(),
    const PackageV2StreamingWriteOptions& options = {});

PackageV2OpenResult openPackageV2(const std::string& packagePath);

PackageV2RecordOpenResult openPackageV2Record(
    const PackageV2OpenResult& package,
    const PackageV2RecordIdentity& identity,
    const PackageCryptoProvider& crypto = getDeterministicPackageCryptoProvider(),
    const std::function<bool()>& cancellationProbe = {});
} // namespace drs::engine
