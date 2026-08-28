#include "drs/engine/PackageV3StreamingExport.h"

#include "drs/engine/PlayableInstrumentLicense.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <utility>

namespace drs::engine
{
namespace
{
constexpr std::uint64_t exportRecordBytes = 64ull * 1024ull;

bool addBytes(std::uint64_t& total, const std::uint64_t bytes)
{
    if (total > std::numeric_limits<std::uint64_t>::max() - bytes)
        return false;
    total += bytes;
    return true;
}

bool appendMemoryRecords(PackageV3StreamingExportBuildResult& result,
                         const std::string& recordId,
                         const std::string& recordKind,
                         const std::vector<std::uint8_t>& bytes)
{
    const auto count = std::max<std::size_t>(
        1u, (bytes.size() + static_cast<std::size_t>(exportRecordBytes) - 1u)
            / static_cast<std::size_t>(exportRecordBytes));
    for (std::size_t chunk = 0; chunk < count; ++chunk)
    {
        const auto offset = chunk * static_cast<std::size_t>(exportRecordBytes);
        const auto size = offset < bytes.size()
            ? std::min<std::size_t>(static_cast<std::size_t>(exportRecordBytes),
                                    bytes.size() - offset)
            : 0u;
        if (chunk > std::numeric_limits<std::uint32_t>::max()
            || ! addBytes(result.totalPlaintextBytes, size))
        {
            result.issues.push_back("V3 metadata record accounting overflowed.");
            return false;
        }
        std::vector<std::uint8_t> retained(
            bytes.begin() + static_cast<std::ptrdiff_t>(offset),
            bytes.begin() + static_cast<std::ptrdiff_t>(offset + size));
        PackageV3StreamingRecordSource record;
        record.recordId = recordId;
        record.recordKind = recordKind;
        record.generation = 1;
        record.pageIndex = static_cast<std::uint32_t>(chunk);
        record.expectedPlaintextBytes = size;
        record.sourceLabel = recordKind + ":" + recordId;
        record.loadPlaintext = [retained = std::move(retained)](
            std::vector<std::uint8_t>& output, std::string& issue)
        {
            output = retained;
            issue.clear();
            return true;
        };
        result.plan.records.push_back(std::move(record));
    }
    return true;
}

PackageV3StreamingExportBuildResult reject(std::string issue)
{
    PackageV3StreamingExportBuildResult result;
    result.state = "Performance package V3 export plan rejected";
    result.issues.push_back(std::move(issue));
    return result;
}
} // namespace

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
    const std::vector<PerformancePackagePayloadSource>& additionalPayloads)
{
    namespace fs = std::filesystem;
    if (! compiledRuntime.compiled)
        return reject("Runtime compilation must succeed before package V3 export.");
    if (manifest.packageId.empty() || outputPath.empty() || compatibilityId.empty()
        || encryptionKeyId.empty() || signingKeyId.empty() || ! trustStore.valid())
        return reject("V3 export identities and publisher trust configuration are required.");

    const auto licensePayloadCount = std::count_if(
        additionalPayloads.begin(), additionalPayloads.end(), [](const auto& payload)
        {
            return payload.kind == PerformancePackagePayloadKind::licenseText;
        });
    if (manifest.license.payloadId.empty())
    {
        if (licensePayloadCount != 0)
            return reject("Performance package licenseText payload requires manifest.license.payloadId.");
    }
    else
    {
        if (manifest.license.payloadId != playableInstrumentLicensePayloadId)
            return reject("Performance package manifest license.payloadId must be 'license-text'.");
        const auto licensePayload = std::find_if(
            additionalPayloads.begin(), additionalPayloads.end(), [&](const auto& payload)
            {
                return payload.payloadId == manifest.license.payloadId;
            });
        if (licensePayload == additionalPayloads.end()
            || licensePayload->kind != PerformancePackagePayloadKind::licenseText
            || licensePayloadCount != 1
            || licensePayload->mediaType != playableInstrumentLicenseMediaType
            || licensePayload->logicalPath != playableInstrumentLicenseLogicalPath)
            return reject("Performance package licenseText payload metadata is not canonical.");
        const auto validation = validatePlayableInstrumentLicenseBytes(
            licensePayload->plaintextBytes);
        if (! validation.valid)
            return reject("Performance package licenseText payload is invalid: " + validation.issue);
    }

    PackageV3StreamingExportBuildResult result;
    result.state = "Performance package V3 export plan rejected";
    result.plan.packageId = manifest.packageId;
    result.plan.compatibilityId = std::move(compatibilityId);
    result.plan.outputPath = outputPath;
    result.plan.encryptionKeyId = std::move(encryptionKeyId);
    result.plan.signingKeyId = std::move(signingKeyId);
    result.plan.keyProvider = &keyProvider;
    result.plan.publisherSigner = &publisherSigner;
    result.plan.trustStore = &trustStore;

    auto packagedRuntime = buildPackageRuntimeMetadata(compiledRuntime);
    manifest.masterGainDb = packagedRuntime.masterGainDb;
    manifest.groupRoutes.clear();
    for (const auto& group : packagedRuntime.instrument.groups)
        manifest.groupRoutes.push_back({ group.id, group.gainDb });
    const auto toBytes = [](const std::string& text)
    {
        return std::vector<std::uint8_t>(text.begin(), text.end());
    };
    if (! appendMemoryRecords(result, "package-manifest", "manifest",
                              toBytes(serializePerformancePackageManifest(manifest)))
        || ! appendMemoryRecords(result, "runtime-instrument", "runtime-instrument",
                                 toBytes(serializeRuntimeInstrumentManifest(
                                     packagedRuntime.instrument,
                                     "package://manifest/runtime-instrument.drinst")))
        || ! appendMemoryRecords(result, "runtime-stream-index", "stream-index",
                                 toBytes(serializeCompiledStreamIndex(
                                     packagedRuntime,
                                     "package://manifest/runtime-stream-index.drstrm"))))
        return result;

    for (const auto& payload : additionalPayloads)
    {
        if (payload.kind == PerformancePackagePayloadKind::backgroundImage)
        {
            if (! appendMemoryRecords(result,
                                     payload.payloadId.empty() ? "background-image"
                                                               : payload.payloadId,
                                     "background-image", payload.plaintextBytes))
                return result;
        }
        else if (payload.kind == PerformancePackagePayloadKind::licenseText)
        {
            if (! appendMemoryRecords(result, payload.payloadId, "license-text",
                                     payload.plaintextBytes))
                return result;
        }
    }

    for (const auto& sample : compiledRuntime.streamSamples)
    {
        std::error_code error;
        const auto fileBytes = fs::file_size(fs::path(compiledRuntime.payloadFilePath), error);
        const auto headBytes = sample.prefetchBytes == 0 ? exportRecordBytes
                                                         : sample.prefetchBytes;
        const auto pageBytes = compiledRuntime.pageSizeBytes == 0 ? exportRecordBytes
                                                                  : compiledRuntime.pageSizeBytes;
        if (sample.sampleId.empty() || error || sample.payloadSizeBytes == 0
            || headBytes == 0 || pageBytes == 0
            || headBytes > exportRecordBytes || pageBytes > exportRecordBytes
            || sample.payloadOffsetBytes > fileBytes
            || sample.payloadSizeBytes > fileBytes - sample.payloadOffsetBytes)
            return reject("Compiled float sample range or V3 record sizing is invalid.");

        const auto appendRange = [&](const std::string& kind,
                                     const std::uint64_t pageIndex,
                                     const std::uint64_t offset,
                                     const std::uint64_t size)
        {
            if (pageIndex > std::numeric_limits<std::uint32_t>::max()
                || size > packageV3MaximumRecordBytes
                || ! addBytes(result.totalPlaintextBytes, size))
                return false;
            PackageV3StreamingRecordSource record;
            record.recordId = sample.sampleId;
            record.recordKind = kind;
            record.generation = 1;
            record.pageIndex = static_cast<std::uint32_t>(pageIndex);
            record.expectedPlaintextBytes = size;
            record.sourceLabel = kind + ":" + sample.sampleId;
            const auto path = compiledRuntime.payloadFilePath;
            record.loadPlaintext = [path, offset, size](
                std::vector<std::uint8_t>& output, std::string& issue)
            {
                std::ifstream input(fs::path(path), std::ios::binary);
                input.seekg(static_cast<std::streamoff>(offset));
                output.resize(static_cast<std::size_t>(size));
                if (! input.read(reinterpret_cast<char*>(output.data()),
                                 static_cast<std::streamsize>(output.size())))
                {
                    output.clear();
                    issue = "Compiled float range read failed.";
                    return false;
                }
                issue.clear();
                return true;
            };
            result.plan.records.push_back(std::move(record));
            return true;
        };

        const auto headSize = std::min<std::uint64_t>(headBytes, sample.payloadSizeBytes);
        if (! appendRange("sample-head", 0, sample.payloadOffsetBytes, headSize))
            return reject("Sample head byte accounting overflowed.");
        std::uint64_t consumed = headSize;
        std::uint64_t pageIndex = 0;
        while (consumed < sample.payloadSizeBytes)
        {
            const auto size = std::min<std::uint64_t>(pageBytes,
                                                      sample.payloadSizeBytes - consumed);
            if (! appendRange("sample-page", pageIndex,
                              sample.payloadOffsetBytes + consumed, size))
                return reject("Sample page byte accounting overflowed.");
            consumed += size;
            ++pageIndex;
        }
    }

    if (result.plan.records.empty() || result.plan.records.size() > packageV3MaximumRecords)
        return reject("V3 export record count is outside the format limit.");
    result.manifest = std::move(manifest);
    result.built = true;
    result.state = "Performance package V3 export plan built";
    return result;
}
} // namespace drs::engine
