#include "drs/engine/PackageReaderDispatch.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <limits>

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
    const std::array<char, 8> v3 { 'D', 'R', 'S', 'P', 'K', 'G', '3', '\0' };
    if (magic == v3)
    {
        result.format = PerformancePackageDiskFormat::version3;
        result.state = "Performance package v3 requires a key-aware activation loader";
        result.issues.push_back(
            "Use the bounded, signature-first V3 file reader with an explicit trust store; "
            "unkeyed dispatch reads only the format signature and never publishes plaintext.");
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
            "This v1 package is larger than the 64 MiB resident compatibility ceiling; re-export the original authoring project as package V3.");
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
    const PackageCryptoProvider& crypto,
    const int supportedReaderSchemaVersion)
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
                                std::vector<std::uint8_t>& output,
                                const std::uint64_t maximumBytes
                                    = std::numeric_limits<std::uint64_t>::max())
    {
        std::vector<PackageV2RecordDescriptor> chunks;
        std::uint64_t totalBytes = 0;
        for (const auto& record : opened->records)
        {
            if (record.identity.sourceId == sourceId && record.identity.kind == kind)
            {
                if (record.plaintextSizeBytes > maximumBytes - totalBytes)
                {
                    result.issues.push_back("Package v2 metadata record exceeds its size limit: "
                                            + sourceId + ".");
                    return false;
                }
                totalBytes += record.plaintextSizeBytes;
                chunks.push_back(record);
            }
        }
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
    if (manifest.manifest.minimumReaderSchemaVersion > supportedReaderSchemaVersion)
    {
        result.metadata.failureCategory
            = PerformancePackageFailureCategory::playbackCompatibilityFailure;
        result.metadata.state = "Performance package requires a newer reader schema";
        result.metadata.issues.push_back(
            "Performance package requires reader schema "
            + std::to_string(manifest.manifest.minimumReaderSchemaVersion)
            + ", but this reader supports schema "
            + std::to_string(supportedReaderSchemaVersion) + ".");
        result.issues = result.metadata.issues;
        result.state = result.metadata.state;
        return result;
    }
    if (!manifest.manifest.backgroundImage.payloadId.empty())
    {
        constexpr std::uint64_t maximumBackgroundImageBytes = 16ull * 1024ull * 1024ull;
        std::vector<std::uint8_t> backgroundImageBytes;
        if (!openChunks(manifest.manifest.backgroundImage.payloadId,
                        PackageV2RecordKind::backgroundImage,
                        backgroundImageBytes,
                        maximumBackgroundImageBytes))
        {
            result.issues.push_back("Required package v2 background image could not be opened.");
            return result;
        }

        result.metadata.backgroundImage.found = true;
        result.metadata.backgroundImage.loaded = true;
        result.metadata.backgroundImage.failureCategory
            = PerformancePackageFailureCategory::none;
        result.metadata.backgroundImage.state = "Performance package v2 background image loaded";
        result.metadata.backgroundImage.payload.payloadId
            = manifest.manifest.backgroundImage.payloadId;
        result.metadata.backgroundImage.payload.payloadKind = "backgroundImage";
        result.metadata.backgroundImage.payload.logicalPath = "images/background.jpg";
        result.metadata.backgroundImage.payload.mediaType = "image/jpeg";
        result.metadata.backgroundImage.payload.plaintextSizeBytes
            = backgroundImageBytes.size();
        result.metadata.backgroundImage.payload.plaintextBytes
            = std::move(backgroundImageBytes);
    }
    if (!manifest.manifest.license.payloadId.empty())
    {
        std::vector<std::uint8_t> licenseBytes;
        if (!openChunks(manifest.manifest.license.payloadId,
                        PackageV2RecordKind::licenseText,
                        licenseBytes,
                        maximumPlayableInstrumentLicenseBytes))
        {
            result.metadata.failureCategory
                = PerformancePackageFailureCategory::payloadCorruption;
            result.metadata.state = "Performance package v2 license text could not be opened";
            result.state = result.metadata.state;
            result.issues.push_back("Required package v2 license text could not be opened.");
            return result;
        }

        const auto validation = validatePlayableInstrumentLicenseBytes(licenseBytes);
        if (!validation.valid)
        {
            result.metadata.failureCategory
                = PerformancePackageFailureCategory::payloadCorruption;
            result.metadata.state = "Performance package v2 license text validation failed";
            result.state = result.metadata.state;
            result.issues.push_back("Package v2 license text is invalid: " + validation.issue);
            return result;
        }

        result.metadata.licenseText.found = true;
        result.metadata.licenseText.loaded = true;
        result.metadata.licenseText.failureCategory
            = PerformancePackageFailureCategory::none;
        result.metadata.licenseText.state = "Performance package v2 license text loaded";
        result.metadata.licenseText.payload.payloadId
            = manifest.manifest.license.payloadId;
        result.metadata.licenseText.payload.payloadKind = "licenseText";
        result.metadata.licenseText.payload.logicalPath
            = playableInstrumentLicenseLogicalPath;
        result.metadata.licenseText.payload.mediaType
            = playableInstrumentLicenseMediaType;
        result.metadata.licenseText.payload.plaintextSizeBytes = licenseBytes.size();
        result.metadata.licenseText.payload.plaintextBytes = std::move(licenseBytes);
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

PerformancePackageLoadResult loadLegacyPerformancePackageV1(
    const std::string& packagePath,
    const int supportedReaderSchemaVersion)
{
    return loadPerformancePackage(packagePath,
                                  getDeterministicPackageCryptoProvider(),
                                  supportedReaderSchemaVersion);
}

PerformancePackageLoadResult loadLegacyPerformancePackageV1MetadataOnly(
    const std::string& packagePath,
    const int supportedReaderSchemaVersion)
{
    return loadPerformancePackageMetadataOnly(packagePath,
                                              getDeterministicPackageCryptoProvider(),
                                              supportedReaderSchemaVersion);
}

const char* toString(const PerformancePackageV3ActivationFailure failure) noexcept
{
    switch (failure)
    {
        case PerformancePackageV3ActivationFailure::none: return "none";
        case PerformancePackageV3ActivationFailure::configuration: return "configuration";
        case PerformancePackageV3ActivationFailure::format: return "format";
        case PerformancePackageV3ActivationFailure::signature: return "signature";
        case PerformancePackageV3ActivationFailure::keyUnavailable: return "key-unavailable";
        case PerformancePackageV3ActivationFailure::compatibility: return "compatibility";
        case PerformancePackageV3ActivationFailure::authentication: return "authentication";
        case PerformancePackageV3ActivationFailure::corruption: return "corruption";
        case PerformancePackageV3ActivationFailure::io: return "io";
    }
    return "format";
}

PerformancePackageV3MetadataLoadResult loadPerformancePackageV3Metadata(
    const std::string& packagePath,
    const PerformancePackageV3ActivationSecurityContext& securityContext,
    const int supportedReaderSchemaVersion)
{
    namespace fs = std::filesystem;
    constexpr std::uint64_t maximumMetadataBytes = 16ull * 1024ull * 1024ull;
    constexpr std::uint64_t maximumBackgroundImageBytes = 16ull * 1024ull * 1024ull;
    PerformancePackageV3MetadataLoadResult result;
    result.state = "Performance package V3 activation rejected";
    const auto reject = [&](const PerformancePackageV3ActivationFailure failure,
                            const PerformancePackageFailureCategory metadataFailure
                                = PerformancePackageFailureCategory::payloadCorruption)
    {
        result.failure = failure;
        result.metadata.failureCategory = metadataFailure;
        result.metadata.state = result.state;
        result.issues = { std::string("V3 activation rejected [") + toString(failure) + "]." };
        result.metadata.issues = result.issues;
        return false;
    };

    if (! securityContext.valid())
    {
        reject(PerformancePackageV3ActivationFailure::configuration,
               PerformancePackageFailureCategory::playbackCompatibilityFailure);
        return result;
    }
    std::error_code fileError;
    if (! fs::is_regular_file(fs::path(packagePath), fileError) || fileError)
    {
        reject(PerformancePackageV3ActivationFailure::io);
        return result;
    }

    auto opened = std::make_shared<PackageV3FileOpenResult>(
        openPackageV3File(packagePath, *securityContext.trustStore));
    if (! opened->opened)
    {
        reject(opened->package.opened
                   ? PerformancePackageV3ActivationFailure::signature
                   : PerformancePackageV3ActivationFailure::format);
        return result;
    }
    if (opened->package.compatibilityId != securityContext.compatibilityId)
    {
        reject(PerformancePackageV3ActivationFailure::compatibility,
               PerformancePackageFailureCategory::playbackCompatibilityFailure);
        return result;
    }

    SecureBuffer releaseKey;
    std::string privateIssue;
    if (! securityContext.keyProvider->resolvePackageKey(
            opened->package.packageId, opened->package.encryptionKeyId,
            PackageKeyUse::decryptExistingPackage, releaseKey, privateIssue))
    {
        reject(PerformancePackageV3ActivationFailure::keyUnavailable,
               PerformancePackageFailureCategory::playbackCompatibilityFailure);
        return result;
    }
    SecureBuffer unwrappedContentKey;
    if (! unwrapPackageV3ContentKey(opened->package, releaseKey,
                                    unwrappedContentKey, privateIssue))
    {
        reject(PerformancePackageV3ActivationFailure::authentication);
        return result;
    }
    auto contentKey = std::make_shared<SecureBuffer>(std::move(unwrappedContentKey));
    result.package = opened;
    result.contentKey = contentKey;

    const auto openChunks = [&](const std::string& recordId,
                                const std::string& recordKind,
                                std::vector<std::uint8_t>& output,
                                const std::uint64_t maximumBytes)
    {
        std::vector<const PackageV3RecordDescriptor*> chunks;
        std::uint64_t totalBytes = 0;
        for (const auto& record : opened->package.records)
        {
            if (record.recordId == recordId && record.recordKind == recordKind)
            {
                if (record.plaintextSize > maximumBytes - totalBytes)
                    return reject(PerformancePackageV3ActivationFailure::corruption);
                totalBytes += record.plaintextSize;
                chunks.push_back(&record);
            }
        }
        std::sort(chunks.begin(), chunks.end(), [](const auto* left, const auto* right)
        {
            return left->pageIndex < right->pageIndex;
        });
        if (chunks.empty())
            return reject(PerformancePackageV3ActivationFailure::corruption);
        output.reserve(static_cast<std::size_t>(totalBytes));
        for (std::size_t index = 0; index < chunks.size(); ++index)
        {
            if (chunks[index]->pageIndex != index)
                return reject(PerformancePackageV3ActivationFailure::corruption);
            auto chunk = openPackageV3FileRecord(*opened, *contentKey, *chunks[index]);
            if (! chunk.opened || chunk.plaintext.size() != chunks[index]->plaintextSize)
                return reject(PerformancePackageV3ActivationFailure::authentication);
            output.insert(output.end(), chunk.plaintext.begin(), chunk.plaintext.end());
        }
        return true;
    };

    std::vector<std::uint8_t> manifestBytes;
    std::vector<std::uint8_t> instrumentBytes;
    std::vector<std::uint8_t> streamBytes;
    if (! openChunks("package-manifest", "manifest", manifestBytes, maximumMetadataBytes)
        || ! openChunks("runtime-instrument", "runtime-instrument", instrumentBytes,
                        maximumMetadataBytes)
        || ! openChunks("runtime-stream-index", "stream-index", streamBytes,
                        maximumMetadataBytes))
        return result;

    const auto manifest = parsePerformancePackageManifestJson(
        std::string(manifestBytes.begin(), manifestBytes.end()));
    if (! manifest.parsed)
    {
        reject(PerformancePackageV3ActivationFailure::corruption);
        return result;
    }
    if (manifest.manifest.minimumReaderSchemaVersion > supportedReaderSchemaVersion)
    {
        reject(PerformancePackageV3ActivationFailure::compatibility,
               PerformancePackageFailureCategory::playbackCompatibilityFailure);
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
    if (! result.metadata.instrument.loaded || ! result.metadata.stream.loaded
        || result.metadata.manifest.packageId != opened->package.packageId
        || result.metadata.manifest.instrumentId
            != result.metadata.instrument.instrument.instrumentId
        || result.metadata.stream.container.containerId
            != result.metadata.instrument.instrument.instrumentId)
    {
        reject(PerformancePackageV3ActivationFailure::corruption);
        return result;
    }

    if (! result.metadata.manifest.backgroundImage.payloadId.empty())
    {
        std::vector<std::uint8_t> bytes;
        if (! openChunks(result.metadata.manifest.backgroundImage.payloadId,
                         "background-image", bytes, maximumBackgroundImageBytes))
            return result;
        result.metadata.backgroundImage.found = true;
        result.metadata.backgroundImage.loaded = true;
        result.metadata.backgroundImage.state = "Performance package V3 artwork loaded";
        result.metadata.backgroundImage.payload.payloadId
            = result.metadata.manifest.backgroundImage.payloadId;
        result.metadata.backgroundImage.payload.payloadKind = "backgroundImage";
        result.metadata.backgroundImage.payload.logicalPath = "images/background.jpg";
        result.metadata.backgroundImage.payload.mediaType = "image/jpeg";
        result.metadata.backgroundImage.payload.plaintextSizeBytes = bytes.size();
        result.metadata.backgroundImage.payload.plaintextBytes = std::move(bytes);
    }
    if (! result.metadata.manifest.license.payloadId.empty())
    {
        std::vector<std::uint8_t> bytes;
        if (! openChunks(result.metadata.manifest.license.payloadId,
                         "license-text", bytes, maximumPlayableInstrumentLicenseBytes))
            return result;
        if (! validatePlayableInstrumentLicenseBytes(bytes).valid)
        {
            reject(PerformancePackageV3ActivationFailure::corruption);
            return result;
        }
        result.metadata.licenseText.found = true;
        result.metadata.licenseText.loaded = true;
        result.metadata.licenseText.state = "Performance package V3 license loaded";
        result.metadata.licenseText.payload.payloadId
            = result.metadata.manifest.license.payloadId;
        result.metadata.licenseText.payload.payloadKind = "licenseText";
        result.metadata.licenseText.payload.logicalPath = playableInstrumentLicenseLogicalPath;
        result.metadata.licenseText.payload.mediaType = playableInstrumentLicenseMediaType;
        result.metadata.licenseText.payload.plaintextSizeBytes = bytes.size();
        result.metadata.licenseText.payload.plaintextBytes = std::move(bytes);
    }

    for (const auto& sample : result.metadata.stream.container.samples)
    {
        std::vector<const PackageV3RecordDescriptor*> heads;
        std::vector<const PackageV3RecordDescriptor*> pages;
        for (const auto& record : opened->package.records)
        {
            if (record.recordId != sample.sampleId) continue;
            if (record.recordKind == "sample-head") heads.push_back(&record);
            if (record.recordKind == "sample-page") pages.push_back(&record);
        }
        std::sort(pages.begin(), pages.end(), [](const auto* left, const auto* right)
        {
            return left->pageIndex < right->pageIndex;
        });
        if (heads.size() != 1 || heads.front()->pageIndex != 0
            || heads.front()->generation == 0)
        {
            reject(PerformancePackageV3ActivationFailure::corruption);
            return result;
        }
        const auto* head = heads.front();
        const auto bytesPerFrame = static_cast<std::uint64_t>(sample.channelCount) * sizeof(float);
        if (sample.channelCount == 0 || (sample.frameCount > 0
            && bytesPerFrame > std::numeric_limits<std::uint64_t>::max() / sample.frameCount)
            || sample.frameCount * bytesPerFrame != sample.payloadSizeBytes
            || result.metadata.stream.container.pageSizeBytes == 0)
        {
            reject(PerformancePackageV3ActivationFailure::corruption);
            return result;
        }
        const auto expectedHeadBytes = std::min<std::uint64_t>(
            sample.prefetchBytes == 0 ? defaultSampleHeadBytes : sample.prefetchBytes,
            sample.payloadSizeBytes);
        const auto remainingBytes = sample.payloadSizeBytes - expectedHeadBytes;
        const auto expectedPageCount = remainingBytes == 0 ? 0
            : 1 + (remainingBytes - 1) / result.metadata.stream.container.pageSizeBytes;
        if (head->plaintextSize != expectedHeadBytes || pages.size() != expectedPageCount)
        {
            reject(PerformancePackageV3ActivationFailure::corruption);
            return result;
        }
        for (std::size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex)
        {
            const auto consumed = static_cast<std::uint64_t>(pageIndex)
                * result.metadata.stream.container.pageSizeBytes;
            const auto expectedBytes = std::min<std::uint64_t>(
                result.metadata.stream.container.pageSizeBytes, remainingBytes - consumed);
            if (pages[pageIndex]->pageIndex != pageIndex
                || pages[pageIndex]->generation != head->generation
                || pages[pageIndex]->plaintextSize != expectedBytes)
            {
                reject(PerformancePackageV3ActivationFailure::corruption);
                return result;
            }
        }
        SampleDataSourceDescriptor descriptor;
        descriptor.kind = SampleDataSourceKind::packageRecord;
        descriptor.sourceId = sample.sampleId;
        descriptor.canonicalSourceIdentity = packagePath + "#" + sample.sampleId;
        descriptor.provenanceIdentity = opened->package.packageId + ":" + sample.sampleId
            + ":v3:g" + std::to_string(head->generation);
        descriptor.formatName = "package-v3-float32";
        descriptor.channelLayout = sample.channelLayout;
        descriptor.generation = head->generation;
        descriptor.sampleRate = sample.sampleRate;
        descriptor.frameCount = sample.frameCount;
        descriptor.channelCount = sample.channelCount;
        descriptor.bytesPerFrame = bytesPerFrame;
        descriptor.dataSizeBytes = sample.frameCount * bytesPerFrame;
        descriptor.headSizeBytes = head->plaintextSize;
        descriptor.pageSizeBytes = result.metadata.stream.container.pageSizeBytes;
        if (! validateSampleDataSourceDescriptor(descriptor).valid)
        {
            reject(PerformancePackageV3ActivationFailure::corruption);
            return result;
        }
        result.sampleDescriptors.push_back(std::move(descriptor));
    }

    result.metadata.loaded = true;
    result.metadata.failureCategory = PerformancePackageFailureCategory::none;
    result.metadata.state = "Performance package V3 metadata authenticated";
    result.loaded = true;
    result.failure = PerformancePackageV3ActivationFailure::none;
    result.state = result.metadata.state;
    result.issues.clear();
    return result;
}
} // namespace drs::engine
