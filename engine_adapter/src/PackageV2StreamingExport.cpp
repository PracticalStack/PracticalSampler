#include "drs/engine/PackageV2StreamingExport.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>

namespace drs::engine
{
void appendPackageV2MetadataChunks(
    std::vector<PackageV2RecordSource>& records,
    const std::string& sourceId,
    const PackageV2RecordKind kind,
    const std::vector<std::uint8_t>& bytes,
    const std::uint64_t sourceGeneration)
{
    const auto chunkCount = std::max<std::size_t>(
        1, (bytes.size() + static_cast<std::size_t>(performancePackageV2MaximumRecordBytes) - 1)
            / static_cast<std::size_t>(performancePackageV2MaximumRecordBytes));
    for (std::size_t chunk = 0; chunk < chunkCount; ++chunk)
    {
        const auto offset = chunk * static_cast<std::size_t>(performancePackageV2MaximumRecordBytes);
        const auto size = offset < bytes.size()
            ? std::min<std::size_t>(static_cast<std::size_t>(performancePackageV2MaximumRecordBytes),
                                    bytes.size() - offset)
            : 0;
        PackageV2RecordSource record;
        record.identity = { sourceId, kind, chunk, sourceGeneration };
        record.plaintextBytes.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                                     bytes.begin() + static_cast<std::ptrdiff_t>(offset + size));
        records.push_back(std::move(record));
    }
}

namespace
{
bool appendMetadataRecords(PackageV2StreamingExportBuildResult& result,
                           const std::vector<PackageV2RecordSource>& metadataRecords)
{
    for (const auto& metadata : metadataRecords)
    {
        if (metadata.plaintextBytes.size() > performancePackageV2MaximumRecordBytes)
        {
            result.issues.push_back("A metadata record exceeds the 64 KiB record policy.");
            return false;
        }
        PackageV2StreamingRecordSource record;
        record.identity = metadata.identity;
        record.expectedPlaintextBytes = metadata.plaintextBytes.size();
        record.sourceLabel = "metadata:" + metadata.identity.sourceId;
        auto retainedBytes = metadata.plaintextBytes;
        record.loadPlaintext = [retainedBytes = std::move(retainedBytes)](
            std::vector<std::uint8_t>& output, std::string& issue)
        {
            output = retainedBytes;
            issue.clear();
            return true;
        };
        if (result.totalPlaintextBytes
            > std::numeric_limits<std::uint64_t>::max() - record.expectedPlaintextBytes)
        {
            result.issues.push_back("Metadata byte accounting overflowed.");
            return false;
        }
        result.totalPlaintextBytes += record.expectedPlaintextBytes;
        result.plan.records.push_back(std::move(record));
    }
    return true;
}
} // namespace

PackageV2StreamingExportBuildResult buildPackageV2StreamingExportPlan(
    const std::string& packageId,
    const std::string& outputPath,
    const std::vector<PackageV2RecordSource>& metadataRecords,
    const std::vector<PackageV2CompiledSampleInput>& samples)
{
    namespace fs = std::filesystem;
    PackageV2StreamingExportBuildResult result;
    result.state = "Package v2 streaming export plan rejected";
    result.plan.packageId = packageId;
    result.plan.outputPath = outputPath;
    if (packageId.empty() || outputPath.empty() || metadataRecords.empty() || samples.empty())
    {
        result.issues.push_back("Package id, output path, metadata records, and samples are required.");
        return result;
    }
    const auto addBytes = [&](const std::uint64_t bytes)
    {
        if (result.totalPlaintextBytes > std::numeric_limits<std::uint64_t>::max() - bytes)
            return false;
        result.totalPlaintextBytes += bytes;
        return true;
    };
    if (!appendMetadataRecords(result, metadataRecords))
        return result;
    for (const auto& sample : samples)
    {
        std::error_code error;
        const auto fileBytes = fs::file_size(fs::path(sample.compiledFloatPayloadPath), error);
        if (sample.sourceId.empty() || sample.sourceGeneration == 0 || error
            || sample.dataSizeBytes == 0 || sample.headBytes == 0 || sample.pageBytes == 0
            || sample.headBytes > performancePackageV2MaximumRecordBytes
            || sample.pageBytes > performancePackageV2MaximumRecordBytes
            || sample.dataOffsetBytes > fileBytes
            || sample.dataSizeBytes > fileBytes - sample.dataOffsetBytes)
        {
            result.issues.push_back("Compiled float sample range or record sizing is invalid.");
            return result;
        }
        const auto addRange = [&](const PackageV2RecordKind kind,
                                  const std::uint64_t pageIndex,
                                  const std::uint64_t offset,
                                  const std::uint64_t size)
        {
            PackageV2StreamingRecordSource record;
            record.identity = { sample.sourceId, kind, pageIndex, sample.sourceGeneration };
            record.expectedPlaintextBytes = size;
            record.sourceLabel = sample.compiledFloatPayloadPath + "@" + std::to_string(offset);
            const auto path = sample.compiledFloatPayloadPath;
            record.loadPlaintext = [path, offset, size](std::vector<std::uint8_t>& output,
                                                        std::string& issue)
            {
                std::ifstream input(fs::path(path), std::ios::binary);
                input.seekg(static_cast<std::streamoff>(offset));
                output.resize(static_cast<std::size_t>(size));
                if (!input.read(reinterpret_cast<char*>(output.data()),
                                static_cast<std::streamsize>(output.size())))
                {
                    issue = "Compiled float range read failed.";
                    output.clear();
                    return false;
                }
                issue.clear();
                return true;
            };
            result.plan.records.push_back(std::move(record));
            return addBytes(size);
        };
        const auto headSize = std::min(sample.headBytes, sample.dataSizeBytes);
        if (!addRange(PackageV2RecordKind::sampleHead, 0,
                      sample.dataOffsetBytes, headSize))
        {
            result.issues.push_back("Sample head byte accounting overflowed.");
            return result;
        }
        std::uint64_t consumed = headSize;
        std::uint64_t pageIndex = 0;
        while (consumed < sample.dataSizeBytes)
        {
            const auto size = std::min(sample.pageBytes, sample.dataSizeBytes - consumed);
            if (!addRange(PackageV2RecordKind::samplePage, pageIndex,
                          sample.dataOffsetBytes + consumed, size))
            {
                result.issues.push_back("Sample page byte accounting overflowed.");
                return result;
            }
            consumed += size;
            ++pageIndex;
        }
    }
    result.built = true;
    result.state = "Package v2 streaming export plan built";
    return result;
}

PackageV2StreamingExportBuildResult buildPackageV2WavStreamingExportPlan(
    const std::string& packageId,
    const std::string& outputPath,
    const std::vector<PackageV2RecordSource>& metadataRecords,
    const std::vector<PackageV2WavSampleInput>& samples)
{
    PackageV2StreamingExportBuildResult result;
    result.state = "Package v2 WAV streaming export plan rejected";
    result.plan.packageId = packageId;
    result.plan.outputPath = outputPath;
    if (packageId.empty() || outputPath.empty() || metadataRecords.empty() || samples.empty())
    {
        result.issues.push_back("Package id, output path, metadata records, and WAV samples are required.");
        return result;
    }
    if (!appendMetadataRecords(result, metadataRecords))
        return result;

    const auto addBytes = [&](const std::uint64_t bytes)
    {
        if (result.totalPlaintextBytes > std::numeric_limits<std::uint64_t>::max() - bytes)
            return false;
        result.totalPlaintextBytes += bytes;
        return true;
    };
    for (const auto& input : samples)
    {
        if (input.sourceId.empty() || input.wavPath.empty()
            || input.headBytes == 0 || input.pageBytes == 0
            || input.headBytes > performancePackageV2MaximumRecordBytes
            || input.pageBytes > performancePackageV2MaximumRecordBytes)
        {
            result.issues.push_back("WAV sample identity or record sizing is invalid.");
            return result;
        }
        auto wavDescriptor = buildWavSampleDataSourceDescriptor(
            input.sourceId, input.wavPath, input.sourceGeneration,
            input.headBytes, input.pageBytes);
        if (!wavDescriptor.built)
        {
            result.issues.push_back("WAV descriptor failed for source '" + input.sourceId + "': "
                                    + (wavDescriptor.findings.empty() ? wavDescriptor.state
                                                                     : wavDescriptor.findings.front()));
            return result;
        }
        const auto descriptor = wavDescriptor.descriptor;
        const auto cacheBudget = std::max<std::uint64_t>(input.pageBytes * 2,
                                                         performancePackageV2MaximumRecordBytes);
        auto wavSource = std::make_shared<WavPagedSampleDataSource>(
            std::move(wavDescriptor), cacheBudget);
        const auto decodedBytesPerFrame = static_cast<std::uint64_t>(descriptor.channelCount)
            * sizeof(float);
        const auto headFrames = std::min<std::uint64_t>(
            descriptor.frameCount, std::max<std::uint64_t>(1, input.headBytes / decodedBytesPerFrame));
        const auto pageFrames = std::max<std::uint64_t>(1, input.pageBytes / decodedBytesPerFrame);
        const auto makeLoader = [wavSource, descriptor](const bool head,
                                                       const std::uint64_t pageIndex,
                                                       const std::uint64_t firstFrame,
                                                       const std::uint64_t frameCount)
        {
            return [wavSource, descriptor, head, pageIndex, firstFrame, frameCount](
                std::vector<std::uint8_t>& output, std::string& issue)
            {
                if (!(head ? wavSource->prepareHead() : wavSource->preparePage(pageIndex)))
                {
                    issue = wavSource->lastFailure();
                    output.clear();
                    return false;
                }
                const auto view = wavSource->acquireFrameView(
                    firstFrame, static_cast<std::uint32_t>(frameCount));
                if (view.status != SampleFrameViewStatus::ready || view.frameCount != frameCount
                    || view.channelCount != descriptor.channelCount)
                {
                    issue = "Prepared WAV range did not expose the expected frame view.";
                    output.clear();
                    return false;
                }
                output.resize(static_cast<std::size_t>(frameCount * descriptor.channelCount
                                                       * sizeof(float)));
                for (std::uint64_t frame = 0; frame < frameCount; ++frame)
                for (std::uint32_t channel = 0; channel < descriptor.channelCount; ++channel)
                {
                    const auto offset = static_cast<std::size_t>(
                        (frame * descriptor.channelCount + channel) * sizeof(float));
                    std::memcpy(output.data() + offset, view.channels[channel] + frame, sizeof(float));
                }
                issue.clear();
                return true;
            };
        };
        const auto appendRange = [&](const PackageV2RecordKind kind,
                                     const std::uint64_t pageIndex,
                                     const std::uint64_t firstFrame,
                                     const std::uint64_t frameCount)
        {
            const auto byteCount = frameCount * decodedBytesPerFrame;
            if (!addBytes(byteCount))
                return false;
            PackageV2StreamingRecordSource record;
            record.identity = { input.sourceId, kind, pageIndex, descriptor.generation };
            record.expectedPlaintextBytes = byteCount;
            record.sourceLabel = input.wavPath + "#frames=" + std::to_string(firstFrame)
                + "+" + std::to_string(frameCount);
            record.loadPlaintext = makeLoader(kind == PackageV2RecordKind::sampleHead,
                                              pageIndex, firstFrame, frameCount);
            result.plan.records.push_back(std::move(record));
            return true;
        };
        if (!appendRange(PackageV2RecordKind::sampleHead, 0, 0, headFrames))
        {
            result.issues.push_back("WAV head byte accounting overflowed.");
            return result;
        }
        std::uint64_t consumedFrames = headFrames;
        std::uint64_t pageIndex = 0;
        while (consumedFrames < descriptor.frameCount)
        {
            const auto frameCount = std::min(pageFrames, descriptor.frameCount - consumedFrames);
            if (!appendRange(PackageV2RecordKind::samplePage, pageIndex,
                             consumedFrames, frameCount))
            {
                result.issues.push_back("WAV page byte accounting overflowed.");
                return result;
            }
            consumedFrames += frameCount;
            ++pageIndex;
        }

        auto packageDescriptor = descriptor;
        packageDescriptor.kind = SampleDataSourceKind::packageRecord;
        packageDescriptor.canonicalSourceIdentity = outputPath + "#" + input.sourceId;
        packageDescriptor.provenanceIdentity = packageId + ":" + input.sourceId + ":g"
            + std::to_string(descriptor.generation);
        packageDescriptor.formatName = "package-float32";
        packageDescriptor.bytesPerFrame = decodedBytesPerFrame;
        packageDescriptor.dataOffsetBytes = 0;
        packageDescriptor.dataSizeBytes = descriptor.frameCount * decodedBytesPerFrame;
        packageDescriptor.headSizeBytes = input.headBytes;
        packageDescriptor.pageSizeBytes = input.pageBytes;
        result.sampleDescriptors.push_back(std::move(packageDescriptor));
    }

    result.built = true;
    result.state = "Package v2 WAV streaming export plan built";
    return result;
}

PackageV2StreamingExportBuildResult buildPerformancePackageV2StreamingExportPlan(
    PerformancePackageManifest manifest,
    const RuntimeCompileResult& compiledRuntime,
    const std::string& outputPath,
    const std::vector<PerformancePackagePayloadSource>& additionalPayloads)
{
    if (!compiledRuntime.compiled)
    {
        PackageV2StreamingExportBuildResult result;
        result.state = "Performance package v2 export plan rejected";
        result.issues.push_back("Runtime compilation must succeed before package v2 export.");
        return result;
    }
    auto packagedRuntime = buildPackageRuntimeMetadata(compiledRuntime);
    manifest.masterGainDb = packagedRuntime.masterGainDb;
    manifest.groupRoutes.clear();
    for (const auto& group : packagedRuntime.instrument.groups)
        manifest.groupRoutes.push_back({ group.id, group.gainDb });
    const auto toBytes = [](const std::string& text)
    {
        return std::vector<std::uint8_t>(text.begin(), text.end());
    };
    std::vector<PackageV2RecordSource> metadata;
    appendPackageV2MetadataChunks(metadata, "package-manifest", PackageV2RecordKind::manifest,
                                  toBytes(serializePerformancePackageManifest(manifest)));
    appendPackageV2MetadataChunks(metadata, "runtime-instrument",
                                  PackageV2RecordKind::runtimeInstrument,
                                  toBytes(serializeRuntimeInstrumentManifest(
                                      packagedRuntime.instrument,
                                      "package://manifest/runtime-instrument.drinst")));
    appendPackageV2MetadataChunks(metadata, "runtime-stream-index",
                                  PackageV2RecordKind::streamIndex,
                                  toBytes(serializeCompiledStreamIndex(
                                      packagedRuntime,
                                      "package://manifest/runtime-stream-index.drstrm")));
    for (const auto& payload : additionalPayloads)
    {
        if (payload.kind == PerformancePackagePayloadKind::backgroundImage)
        {
            appendPackageV2MetadataChunks(metadata,
                                          payload.payloadId.empty() ? "background-image"
                                                                    : payload.payloadId,
                                          PackageV2RecordKind::backgroundImage,
                                          payload.plaintextBytes);
        }
    }

    std::vector<PackageV2CompiledSampleInput> samples;
    samples.reserve(compiledRuntime.streamSamples.size());
    for (const auto& sample : compiledRuntime.streamSamples)
    {
        samples.push_back({ sample.sampleId, 1, compiledRuntime.payloadFilePath,
                            sample.payloadOffsetBytes, sample.payloadSizeBytes,
                            sample.prefetchBytes == 0 ? defaultSampleHeadBytes
                                                      : sample.prefetchBytes,
                            compiledRuntime.pageSizeBytes == 0 ? defaultSamplePageBytes
                                                                : compiledRuntime.pageSizeBytes });
    }
    auto result = buildPackageV2StreamingExportPlan(
        manifest.packageId, outputPath, metadata, samples);
    result.manifest = std::move(manifest);
    return result;
}
} // namespace drs::engine
