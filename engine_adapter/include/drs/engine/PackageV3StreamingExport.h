#pragma once

#include "drs/engine/PackageKeys.h"
#include "drs/engine/PackagePublisherTrustStore.h"
#include "drs/engine/PackageV3.h"
#include "drs/engine/PackageWriter.h"
#include "drs/engine/RuntimeCompiler.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace drs::engine
{
enum class PackageV3StreamingFailure
{
    none,
    configuration,
    format,
    bounds,
    keyUnavailable,
    signing,
    authentication,
    cancelled,
    io
};

enum class PackageV3StreamingWriteStage
{
    preparing,
    loadingRecord,
    sealingRecord,
    writingRecord,
    finalizingIndex,
    signing,
    verifying,
    publishing,
    completed,
    cancelled,
    failed
};

struct PackageV3StreamingRecordSource
{
    std::string recordId;
    std::string recordKind;
    std::uint32_t generation = 1;
    std::uint32_t pageIndex = 0;
    std::uint64_t expectedPlaintextBytes = 0;
    std::string sourceLabel;
    std::function<bool(std::vector<std::uint8_t>&, std::string&)> loadPlaintext;
};

struct PackageV3StreamingWritePlan
{
    std::string packageId;
    std::string compatibilityId;
    std::string outputPath;
    std::string encryptionKeyId;
    std::string signingKeyId;
    const PackageKeyProvider* keyProvider = nullptr;
    const PackagePublisherSigningClient* publisherSigner = nullptr;
    const PackagePublisherTrustStore* trustStore = nullptr;
    std::vector<PackageV3StreamingRecordSource> records;
};

struct PackageV3StreamingWriteProgress
{
    PackageV3StreamingWriteStage stage = PackageV3StreamingWriteStage::preparing;
    std::size_t recordIndex = 0;
    std::size_t recordCount = 0;
    std::uint64_t completedPlaintextBytes = 0;
    std::uint64_t totalPlaintextBytes = 0;
    std::string recordId;
    std::string recordKind;
    std::string status;
};

struct PackageV3StreamingWriteOptions
{
    std::function<void(const PackageV3StreamingWriteProgress&)> progressSink;
    std::function<bool()> cancellationProbe;
};

struct PackageV3StreamingWriteResult
{
    bool written = false;
    bool verified = false;
    bool atomicallyPublished = false;
    PackageV3StreamingFailure failure = PackageV3StreamingFailure::none;
    std::string state;
    std::vector<std::string> issues;
    std::string packagePath;
    std::string stagingPath;
    std::string signingAuditId;
    std::vector<std::uint8_t> semanticDigest;
    std::uint64_t packageBytes = 0;
    std::uint64_t processedPlaintextBytes = 0;
    std::uint64_t peakPlaintextBufferBytes = 0;
    std::uint64_t peakSealedBufferBytes = 0;
    std::uint64_t verificationBytesRead = 0;
    std::uint64_t completedRecordCount = 0;
    std::uint64_t totalDurationMicros = 0;
    double plaintextThroughputBytesPerSecond = 0.0;
};

struct PackageV3StreamingExportBuildResult
{
    bool built = false;
    std::string state;
    std::vector<std::string> issues;
    std::uint64_t totalPlaintextBytes = 0;
    PerformancePackageManifest manifest;
    PackageV3StreamingWritePlan plan;
};

PackageV3StreamingExportBuildResult buildPerformancePackageV3StreamingExportPlan(
    PerformancePackageManifest manifest,
    const RuntimeCompileResult& compiledRuntime,
    const std::string& outputPath,
    std::string compatibilityId,
    std::string encryptionKeyId,
    std::string signingKeyId,
    const PackageKeyProvider& keyProvider,
    const PackagePublisherSigningClient& publisherSigner,
    const PackagePublisherTrustStore& trustStore,
    const std::vector<PerformancePackagePayloadSource>& additionalPayloads = {});

PackageV3StreamingWriteResult writePackageV3Streaming(
    const PackageV3StreamingWritePlan& plan,
    const PackageV3StreamingWriteOptions& options = {});
} // namespace drs::engine
