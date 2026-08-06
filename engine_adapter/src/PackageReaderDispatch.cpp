#include "drs/engine/PackageReaderDispatch.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace drs::engine
{
PerformancePackageDispatchResult dispatchPerformancePackageReader(
    const std::string& packagePath,
    const PackageCryptoProvider& crypto,
    const int supportedV1ReaderSchemaVersion)
{
    namespace fs = std::filesystem;
    PerformancePackageDispatchResult result;
    result.state = "Performance package reader dispatch failed";
    std::error_code error;
    result.packageBytes = fs::file_size(fs::path(packagePath), error);
    if (error)
    {
        result.issues.push_back("Performance package is missing or unreadable.");
        return result;
    }
    std::ifstream input(fs::path(packagePath), std::ios::binary);
    std::array<char, 8> magic {};
    if (!input.read(magic.data(), magic.size()))
    {
        result.issues.push_back("Performance package is truncated before its format signature.");
        return result;
    }
    const std::array<char, 8> v1 { 'D', 'R', 'S', 'P', 'K', 'G', '1', '\0' };
    const std::array<char, 8> v2 { 'D', 'R', 'S', 'P', 'K', 'G', '2', '\0' };
    if (magic == v2)
    {
        result.format = PerformancePackageDiskFormat::version2;
        result.version2 = openPackageV2(packagePath);
        result.opened = result.version2.opened;
        result.state = result.version2.state;
        result.issues = result.version2.issues;
        return result;
    }
    if (magic != v1)
    {
        result.issues.push_back("Performance package signature is unsupported.");
        return result;
    }
    result.format = PerformancePackageDiskFormat::version1;
    if (result.packageBytes > maximumResidentV1PackageBytes)
    {
        result.migrationRequired = true;
        result.state = "Performance package v1 exceeds the resident compatibility ceiling";
        result.issues.push_back(
            "This v1 package is larger than the 64 MiB resident compatibility ceiling; re-export it as package v2.");
        return result;
    }
    result.version1 = readPerformancePackage(packagePath, crypto,
                                             supportedV1ReaderSchemaVersion);
    result.opened = result.version1.valid;
    result.state = result.version1.state;
    result.issues = result.version1.issues;
    return result;
}

PerformancePackageV2MetadataLoadResult loadPerformancePackageV2Metadata(
    const std::string& packagePath,
    const PackageCryptoProvider& crypto)
{
    PerformancePackageV2MetadataLoadResult result;
    result.state = "Performance package v2 metadata load failed";
    auto opened = std::make_shared<PackageV2OpenResult>(openPackageV2(packagePath));
    if (!opened->opened)
    {
        result.issues = opened->issues;
        return result;
    }
    result.package = opened;
    const auto openChunks = [&](const std::string& sourceId,
                                const PackageV2RecordKind kind,
                                std::vector<std::uint8_t>& output)
    {
        std::vector<PackageV2RecordDescriptor> chunks;
        for (const auto& record : opened->records)
            if (record.identity.sourceId == sourceId && record.identity.kind == kind)
                chunks.push_back(record);
        std::sort(chunks.begin(), chunks.end(), [](const auto& left, const auto& right)
        {
            return left.identity.pageIndex < right.identity.pageIndex;
        });
        if (chunks.empty())
        {
            result.issues.push_back("Package v2 metadata record is missing: " + sourceId + ".");
            return false;
        }
        for (std::size_t index = 0; index < chunks.size(); ++index)
        {
            if (chunks[index].identity.pageIndex != index)
            {
                result.issues.push_back("Package v2 metadata chunks are not contiguous: "
                                        + sourceId + ".");
                return false;
            }
            const auto openedChunk = openPackageV2Record(*opened, chunks[index].identity, crypto);
            if (!openedChunk.opened)
            {
                result.issues.insert(result.issues.end(), openedChunk.issues.begin(),
                                     openedChunk.issues.end());
                return false;
            }
            output.insert(output.end(), openedChunk.plaintextBytes.begin(),
                          openedChunk.plaintextBytes.end());
        }
        return true;
    };

    std::vector<std::uint8_t> manifestBytes;
    std::vector<std::uint8_t> instrumentBytes;
    std::vector<std::uint8_t> streamBytes;
    if (!openChunks("package-manifest", PackageV2RecordKind::manifest, manifestBytes)
        || !openChunks("runtime-instrument", PackageV2RecordKind::runtimeInstrument, instrumentBytes)
        || !openChunks("runtime-stream-index", PackageV2RecordKind::streamIndex, streamBytes))
        return result;

    const auto manifest = parsePerformancePackageManifestJson(
        std::string(manifestBytes.begin(), manifestBytes.end()));
    if (!manifest.parsed)
    {
        result.issues = manifest.issues;
        return result;
    }
    result.metadata.packageFound = true;
    result.metadata.packagePath = packagePath;
    result.metadata.manifest = manifest.manifest;
    result.metadata.instrument = parseRuntimeInstrumentManifest(
        std::string(instrumentBytes.begin(), instrumentBytes.end()),
        packagePath + "#runtime-instrument", false);
    result.metadata.stream = parseRuntimeStreamContainer(
        std::string(streamBytes.begin(), streamBytes.end()),
        packagePath + "#runtime-stream-index", false, nullptr);
    result.issues.insert(result.issues.end(), result.metadata.instrument.issues.begin(),
                         result.metadata.instrument.issues.end());
    result.issues.insert(result.issues.end(), result.metadata.stream.issues.begin(),
                         result.metadata.stream.issues.end());
    if (!result.metadata.instrument.loaded || !result.metadata.stream.loaded)
        return result;
    if (result.metadata.manifest.packageId != opened->packageId
        || result.metadata.manifest.instrumentId
            != result.metadata.instrument.instrument.instrumentId
        || result.metadata.stream.container.containerId
            != result.metadata.instrument.instrument.instrumentId)
    {
        result.issues.push_back("Package v2 manifest, instrument, stream, or package identities differ.");
        return result;
    }

    for (const auto& sample : result.metadata.stream.container.samples)
    {
        const auto head = std::find_if(opened->records.begin(), opened->records.end(),
            [&](const auto& record)
            {
                return record.identity.sourceId == sample.sampleId
                    && record.identity.kind == PackageV2RecordKind::sampleHead;
            });
        if (head == opened->records.end())
        {
            result.issues.push_back("Package v2 sample head is missing: " + sample.sampleId + ".");
            return result;
        }
        SampleDataSourceDescriptor descriptor;
        descriptor.kind = SampleDataSourceKind::packageRecord;
        descriptor.sourceId = sample.sampleId;
        descriptor.canonicalSourceIdentity = packagePath + "#" + sample.sampleId;
        descriptor.provenanceIdentity = opened->packageId + ":" + sample.sampleId + ":g"
            + std::to_string(head->identity.sourceGeneration);
        descriptor.formatName = "package-float32";
        descriptor.channelLayout = sample.channelLayout;
        descriptor.generation = head->identity.sourceGeneration;
        descriptor.sampleRate = sample.sampleRate;
        descriptor.frameCount = sample.frameCount;
        descriptor.channelCount = sample.channelCount;
        descriptor.bytesPerFrame = static_cast<std::uint64_t>(sample.channelCount) * sizeof(float);
        descriptor.dataSizeBytes = sample.frameCount * descriptor.bytesPerFrame;
        descriptor.headSizeBytes = head->plaintextSizeBytes;
        descriptor.pageSizeBytes = result.metadata.stream.container.pageSizeBytes;
        const auto validation = validateSampleDataSourceDescriptor(descriptor);
        if (!validation.valid)
        {
            result.issues.insert(result.issues.end(), validation.findings.begin(),
                                 validation.findings.end());
            return result;
        }
        result.sampleDescriptors.push_back(std::move(descriptor));
    }
    result.metadata.loaded = true;
    result.metadata.failureCategory = PerformancePackageFailureCategory::none;
    result.metadata.state = "Performance package v2 metadata loaded";
    result.loaded = true;
    result.state = result.metadata.state;
    return result;
}
} // namespace drs::engine
