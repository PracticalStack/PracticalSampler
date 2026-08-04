#include "drs/engine/PackageReader.h"

#include <json/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;
using ordered_json = nlohmann::ordered_json;
constexpr std::array<char, 8> kMagic { 'D', 'R', 'S', 'P', 'K', 'G', '1', '\0' };
constexpr std::uint64_t kFnv1aOffsetBasis = 14695981039346656037ull;
constexpr std::uint64_t kFnv1aPrime = 1099511628211ull;

struct FileHeader
{
    char magic[8] {};
    std::uint32_t formatVersion = performancePackageFormatVersion;
    std::uint32_t headerSizeBytes = sizeof(FileHeader);
    std::uint32_t flags = 0;
    std::int32_t minimumReaderSchemaVersion = performancePackageSchemaVersion;
    std::uint64_t cleartextMetadataOffsetBytes = 0;
    std::uint64_t cleartextMetadataSizeBytes = 0;
    std::uint64_t tocOffsetBytes = 0;
    std::uint64_t tocSealedSizeBytes = 0;
    std::uint64_t payloadRegionOffsetBytes = 0;
    std::uint64_t payloadRegionSizeBytes = 0;
    std::uint32_t payloadCount = 0;
    std::uint32_t reserved = 0;
};

static_assert(std::is_standard_layout_v<FileHeader>, "Package header must stay standard-layout.");

template <typename TResult>
void addIssue(TResult& result, const std::string& issue)
{
    result.issues.push_back(issue);
}

std::vector<std::uint8_t> readBinaryFile(const fs::path& path, std::string& issue)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.good())
    {
        issue = "Could not open file for reading: " + path.generic_string();
        return {};
    }

    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

bool readHeader(const std::vector<std::uint8_t>& fileBytes, FileHeader& header)
{
    if (fileBytes.size() < sizeof(FileHeader))
        return false;

    std::memcpy(&header, fileBytes.data(), sizeof(FileHeader));
    return true;
}

bool extractFileRange(const std::vector<std::uint8_t>& fileBytes,
                      const std::uint64_t offsetBytes,
                      const std::uint64_t sizeBytes,
                      std::vector<std::uint8_t>& output,
                      std::string& issue)
{
    if (offsetBytes > fileBytes.size() || sizeBytes > fileBytes.size() - offsetBytes)
    {
        issue = "Package range exceeded file bounds.";
        return false;
    }

    const auto begin = fileBytes.begin() + static_cast<std::ptrdiff_t>(offsetBytes);
    output.assign(begin, begin + static_cast<std::ptrdiff_t>(sizeBytes));
    issue.clear();
    return true;
}

std::string toStringViewBytes(const std::vector<std::uint8_t>& bytes)
{
    return std::string(bytes.begin(), bytes.end());
}

std::vector<std::uint8_t> toBytes(std::string_view text)
{
    return std::vector<std::uint8_t>(text.begin(), text.end());
}

std::uint64_t updateFnv1a64(std::uint64_t hash, const void* data, const std::size_t byteCount) noexcept
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t index = 0; index < byteCount; ++index)
    {
        hash ^= bytes[index];
        hash *= kFnv1aPrime;
    }

    return hash;
}

std::string computeFnv1a64Hex(const std::vector<std::uint8_t>& bytes)
{
    auto hash = kFnv1aOffsetBasis;
    if (!bytes.empty())
        hash = updateFnv1a64(hash, bytes.data(), bytes.size());

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

bool parseSealedBlob(const std::vector<std::uint8_t>& bytes,
                     const PackageCryptoProvider& cryptoProvider,
                     PackageSealedBlob& sealed,
                     std::string& issue)
{
    const auto nonceSizeBytes = cryptoProvider.nonceSizeBytes();
    const auto tagSizeBytes = cryptoProvider.tagSizeBytes();
    const auto minimumBytes = nonceSizeBytes + tagSizeBytes;

    if (bytes.size() < minimumBytes)
    {
        issue = "Sealed package blob was truncated before nonce/tag decoding completed.";
        return false;
    }

    sealed.nonce.assign(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(nonceSizeBytes));
    sealed.tag.assign(bytes.begin() + static_cast<std::ptrdiff_t>(nonceSizeBytes),
                      bytes.begin() + static_cast<std::ptrdiff_t>(nonceSizeBytes + tagSizeBytes));
    sealed.ciphertext.assign(bytes.begin() + static_cast<std::ptrdiff_t>(minimumBytes), bytes.end());
    issue.clear();
    return true;
}

std::string makePackagePayloadAad(const PerformancePackageManifest& manifest,
                                  const std::string& payloadId,
                                  const std::string& payloadKind,
                                  const std::string& logicalPath,
                                  const std::string& mediaType,
                                  const std::uint64_t plaintextSizeBytes)
{
    std::ostringstream stream;
    stream << "drs.performancePackage.payload\n"
           << manifest.packageId << "\n"
           << payloadId << "\n"
           << payloadKind << "\n"
           << logicalPath << "\n"
           << mediaType << "\n"
           << plaintextSizeBytes << "\n"
           << manifest.minimumReaderSchemaVersion;
    return stream.str();
}

std::string makeTocAad(const PerformancePackageManifest& manifest,
                       const std::string& cleartextMetadataJson,
                       const std::uint32_t payloadCount)
{
    std::ostringstream stream;
    stream << "drs.performancePackage.toc\n"
           << manifest.packageId << "\n"
           << payloadCount << "\n"
           << manifest.minimumReaderSchemaVersion << "\n"
           << cleartextMetadataJson;
    return stream.str();
}

bool parseCleartextManifest(const ordered_json& metadataRoot,
                            PerformancePackageReaderResult& result)
{
    if (!metadataRoot.contains("schemaName") || !metadataRoot.at("schemaName").is_string())
    {
        addIssue(result, "Package cleartext metadata must provide schemaName.");
        return false;
    }

    result.cleartextManifest.schemaName = metadataRoot.at("schemaName").get<std::string>();
    result.cleartextManifest.schemaVersion = metadataRoot.value("schemaVersion", 0);
    result.cleartextManifest.packageId = metadataRoot.value("packageId", std::string {});
    result.cleartextManifest.displayName = metadataRoot.value("displayName", std::string {});
    result.cleartextManifest.instrumentId = metadataRoot.value("instrumentId", std::string {});
    result.cleartextManifest.defaultLoadProfile = metadataRoot.value("defaultLoadProfile", std::string {});
    result.cleartextManifest.minimumReaderSchemaVersion =
        metadataRoot.value("minimumReaderSchemaVersion", performancePackageSchemaVersion);
    result.minimumCompatibleAppVersion = metadataRoot.value("minimumCompatibleAppVersion", std::string {});
    result.cryptoAlgorithm = metadataRoot.value("cryptoAlgorithm", std::string {});
    return true;
}

std::optional<PerformancePackagePayloadView> findPayload(const PerformancePackageReaderResult& package,
                                                         const std::string& payloadId)
{
    const auto iterator = std::find_if(package.payloads.begin(),
                                       package.payloads.end(),
                                       [&](const PerformancePackagePayloadView& payload)
                                       {
                                           return payload.payloadId == payloadId;
                                       });

    if (iterator == package.payloads.end())
        return std::nullopt;

    return *iterator;
}

std::optional<PerformancePackagePayloadView> findPayloadByKind(const PerformancePackageReaderResult& package,
                                                               const std::string& payloadKind)
{
    const auto iterator = std::find_if(package.payloads.begin(),
                                       package.payloads.end(),
                                       [&](const PerformancePackagePayloadView& payload)
                                       {
                                           return payload.payloadKind == payloadKind;
                                       });

    if (iterator == package.payloads.end())
        return std::nullopt;

    return *iterator;
}

void appendIssues(std::vector<std::string>& destination, const std::vector<std::string>& issues)
{
    destination.insert(destination.end(), issues.begin(), issues.end());
}

template <typename TResult>
void setFailureCategory(TResult& result, const PerformancePackageFailureCategory category)
{
    if (result.failureCategory == PerformancePackageFailureCategory::none)
        result.failureCategory = category;
}

std::string buildReaderFailureState(const PerformancePackageFailureCategory category)
{
    switch (category)
    {
        case PerformancePackageFailureCategory::none:
        case PerformancePackageFailureCategory::packageFormatFailure:
            return "Performance package format validation failed";
        case PerformancePackageFailureCategory::decryptionFailure:
            return "Performance package decryption failed";
        case PerformancePackageFailureCategory::payloadCorruption:
            return "Performance package payload corruption detected";
        case PerformancePackageFailureCategory::playbackCompatibilityFailure:
            return "Performance package playback compatibility failed";
    }

    return "Performance package read failed";
}

std::string buildPayloadFailureState(const PerformancePackageFailureCategory category)
{
    switch (category)
    {
        case PerformancePackageFailureCategory::none:
        case PerformancePackageFailureCategory::packageFormatFailure:
            return "Performance package payload open failed";
        case PerformancePackageFailureCategory::decryptionFailure:
            return "Performance package payload decryption failed";
        case PerformancePackageFailureCategory::payloadCorruption:
            return "Performance package payload corruption detected";
        case PerformancePackageFailureCategory::playbackCompatibilityFailure:
            return "Performance package payload playback compatibility failed";
    }

    return "Performance package payload open failed";
}

std::string buildLoadFailureState(const PerformancePackageFailureCategory category)
{
    switch (category)
    {
        case PerformancePackageFailureCategory::none:
        case PerformancePackageFailureCategory::packageFormatFailure:
            return "Performance package load failed";
        case PerformancePackageFailureCategory::decryptionFailure:
            return "Performance package decryption failed";
        case PerformancePackageFailureCategory::payloadCorruption:
            return "Performance package payload corruption detected";
        case PerformancePackageFailureCategory::playbackCompatibilityFailure:
            return "Performance package playback compatibility failed";
    }

    return "Performance package load failed";
}

void finalizeReaderFailure(PerformancePackageReaderResult& result,
                           const PerformancePackageFailureCategory defaultCategory
                               = PerformancePackageFailureCategory::packageFormatFailure)
{
    if (result.failureCategory == PerformancePackageFailureCategory::none)
        result.failureCategory = defaultCategory;
    result.state = buildReaderFailureState(result.failureCategory);
}

void finalizePayloadFailure(PerformancePackagePayloadLoadResult& result,
                            const PerformancePackageFailureCategory defaultCategory
                                = PerformancePackageFailureCategory::payloadCorruption)
{
    if (result.failureCategory == PerformancePackageFailureCategory::none)
        result.failureCategory = defaultCategory;
    result.state = buildPayloadFailureState(result.failureCategory);
}

void finalizeLoadFailure(PerformancePackageLoadResult& result,
                         const PerformancePackageFailureCategory defaultCategory
                             = PerformancePackageFailureCategory::packageFormatFailure)
{
    if (result.failureCategory == PerformancePackageFailureCategory::none)
        result.failureCategory = defaultCategory;
    result.state = buildLoadFailureState(result.failureCategory);
}
} // namespace

PerformancePackageReaderResult readPerformancePackage(const std::string& packagePath,
                                                      const PackageCryptoProvider& cryptoProvider,
                                                      const int supportedReaderSchemaVersion)
{
    PerformancePackageReaderResult result;
    result.packagePath = packagePath;
    result.state = "Performance package read not attempted";

    std::string issue;
    const auto fileBytes = readBinaryFile(fs::path(packagePath), issue);
    if (!issue.empty())
    {
        addIssue(result, issue);
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    result.packageFound = true;

    FileHeader header;
    if (!readHeader(fileBytes, header))
    {
        addIssue(result, "Performance package was truncated before its fixed header could be read.");
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    if (!std::equal(kMagic.begin(), kMagic.end(), header.magic))
    {
        addIssue(result, "Performance package magic did not match the expected DRS package signature.");
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    result.header.formatVersion = header.formatVersion;
    result.header.headerSizeBytes = header.headerSizeBytes;
    result.header.flags = header.flags;
    result.header.minimumReaderSchemaVersion = header.minimumReaderSchemaVersion;
    result.header.cleartextMetadataOffsetBytes = header.cleartextMetadataOffsetBytes;
    result.header.cleartextMetadataSizeBytes = header.cleartextMetadataSizeBytes;
    result.header.tocOffsetBytes = header.tocOffsetBytes;
    result.header.tocSealedSizeBytes = header.tocSealedSizeBytes;
    result.header.payloadRegionOffsetBytes = header.payloadRegionOffsetBytes;
    result.header.payloadRegionSizeBytes = header.payloadRegionSizeBytes;
    result.header.payloadCount = header.payloadCount;

    if (header.formatVersion != performancePackageFormatVersion)
    {
        addIssue(result, "Performance package formatVersion " + std::to_string(header.formatVersion)
                             + " is not supported by this reader.");
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    std::vector<std::uint8_t> cleartextBytes;
    if (!extractFileRange(fileBytes,
                          header.cleartextMetadataOffsetBytes,
                          header.cleartextMetadataSizeBytes,
                          cleartextBytes,
                          issue))
    {
        addIssue(result, "Cleartext metadata range was invalid: " + issue);
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    result.cleartextMetadataJson = toStringViewBytes(cleartextBytes);

    ordered_json metadataRoot;
    try
    {
        metadataRoot = ordered_json::parse(result.cleartextMetadataJson);
    }
    catch (const std::exception& exception)
    {
        addIssue(result, std::string("Could not parse package cleartext metadata JSON: ") + exception.what());
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    if (!parseCleartextManifest(metadataRoot, result))
    {
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    if (result.cleartextManifest.schemaName != performancePackageSchemaName)
    {
        addIssue(result, "Package cleartext metadata schemaName must be 'drs.performancePackage'.");
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    if (result.cleartextManifest.minimumReaderSchemaVersion > supportedReaderSchemaVersion)
    {
        addIssue(result,
                 "Performance package requires reader schema version "
                     + std::to_string(result.cleartextManifest.minimumReaderSchemaVersion)
                     + " but the current reader only supports "
                     + std::to_string(supportedReaderSchemaVersion) + ".");
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    std::vector<std::uint8_t> sealedTocBytes;
    if (!extractFileRange(fileBytes,
                          header.tocOffsetBytes,
                          header.tocSealedSizeBytes,
                          sealedTocBytes,
                          issue))
    {
        addIssue(result, "Encrypted TOC range was invalid: " + issue);
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    PackageSealedBlob sealedToc;
    if (!parseSealedBlob(sealedTocBytes, cryptoProvider, sealedToc, issue))
    {
        addIssue(result, "Encrypted TOC blob could not be parsed: " + issue);
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    PackageOpenRequest tocOpenRequest;
    tocOpenRequest.packageId = result.cleartextManifest.packageId;
    tocOpenRequest.recordId = "encrypted-toc";
    tocOpenRequest.additionalAuthenticatedData = makeTocAad(result.cleartextManifest,
                                                            result.cleartextMetadataJson,
                                                            header.payloadCount);
    tocOpenRequest.sealed = std::move(sealedToc);

    std::vector<std::uint8_t> tocPlaintextBytes;
    if (!cryptoProvider.open(tocOpenRequest, tocPlaintextBytes, issue))
    {
        addIssue(result,
                 "Encrypted TOC authentication failed; the cleartext header and sealed TOC no longer agree: "
                     + issue);
        setFailureCategory(result, PerformancePackageFailureCategory::decryptionFailure);
        finalizeReaderFailure(result);
        return result;
    }

    result.tocJson = toStringViewBytes(tocPlaintextBytes);

    ordered_json tocRoot;
    try
    {
        tocRoot = ordered_json::parse(result.tocJson);
    }
    catch (const std::exception& exception)
    {
        addIssue(result, std::string("Could not parse decrypted TOC JSON: ") + exception.what());
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    if (tocRoot.value("packageId", std::string {}) != result.cleartextManifest.packageId)
        addIssue(result, "Decrypted TOC packageId did not match the cleartext packageId.");

    if (tocRoot.value("payloadRegionSizeBytes", std::uint64_t { 0 }) != header.payloadRegionSizeBytes)
        addIssue(result, "Decrypted TOC payloadRegionSizeBytes did not match the cleartext header.");

    if (!tocRoot.contains("payloads") || !tocRoot.at("payloads").is_array())
    {
        addIssue(result, "Decrypted TOC must provide a payloads array.");
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    const auto& payloads = tocRoot.at("payloads");
    if (payloads.size() != header.payloadCount)
        addIssue(result, "Decrypted TOC payload count did not match the cleartext header.");

    std::unordered_map<std::string, bool> payloadIds;
    for (const auto& payload : payloads)
    {
        if (!payload.is_object())
        {
            addIssue(result, "Decrypted TOC payload entries must be objects.");
            continue;
        }

        PerformancePackagePayloadView payloadView;
        payloadView.payloadId = payload.value("payloadId", std::string {});
        payloadView.payloadKind = payload.value("payloadKind", std::string {});
        payloadView.logicalPath = payload.value("logicalPath", std::string {});
        payloadView.mediaType = payload.value("mediaType", std::string {});
        payloadView.payloadOffsetBytes = payload.value("payloadOffsetBytes", std::uint64_t { 0 });
        payloadView.sealedSizeBytes = payload.value("sealedSizeBytes", std::uint64_t { 0 });
        payloadView.plaintextSizeBytes = payload.value("plaintextSizeBytes", std::uint64_t { 0 });
        payloadView.plaintextChecksumHex = payload.value("plaintextChecksumHex", std::string {});

        const auto [iterator, inserted] = payloadIds.emplace(payloadView.payloadId, true);
        if (!inserted)
        {
            static_cast<void>(iterator);
            addIssue(result, "Decrypted TOC contained duplicate payload id '" + payloadView.payloadId + "'.");
        }

        result.payloads.push_back(std::move(payloadView));
    }

    if (!result.issues.empty())
    {
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizeReaderFailure(result);
        return result;
    }

    result.valid = true;
    result.state = "Performance package read completed";
    return result;
}

PerformancePackagePayloadLoadResult openPerformancePackagePayload(const PerformancePackageReaderResult& package,
                                                                 const std::string& payloadId,
                                                                 const PackageCryptoProvider& cryptoProvider)
{
    PerformancePackagePayloadLoadResult result;
    result.state = "Performance package payload open not attempted";

    if (!package.valid)
    {
        addIssue(result, "Cannot open a payload from an invalid package-reader result.");
        setFailureCategory(result, package.failureCategory == PerformancePackageFailureCategory::none
                                       ? PerformancePackageFailureCategory::packageFormatFailure
                                       : package.failureCategory);
        finalizePayloadFailure(result, PerformancePackageFailureCategory::packageFormatFailure);
        return result;
    }

    const auto payload = findPayload(package, payloadId);
    if (!payload.has_value())
    {
        result.state = "Performance package payload missing";
        return result;
    }

    result.found = true;
    result.payload = *payload;

    std::string issue;
    const auto fileBytes = readBinaryFile(fs::path(package.packagePath), issue);
    if (!issue.empty())
    {
        addIssue(result, issue);
        setFailureCategory(result, PerformancePackageFailureCategory::packageFormatFailure);
        finalizePayloadFailure(result, PerformancePackageFailureCategory::packageFormatFailure);
        return result;
    }

    const auto absoluteOffsetBytes = package.header.payloadRegionOffsetBytes + result.payload.payloadOffsetBytes;
    std::vector<std::uint8_t> sealedPayloadBytes;
    if (!extractFileRange(fileBytes,
                          absoluteOffsetBytes,
                          result.payload.sealedSizeBytes,
                          sealedPayloadBytes,
                          issue))
    {
        addIssue(result, "Payload '" + payloadId + "' exceeded the package file bounds.");
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizePayloadFailure(result);
        return result;
    }

    PackageSealedBlob sealedPayload;
    if (!parseSealedBlob(sealedPayloadBytes, cryptoProvider, sealedPayload, issue))
    {
        addIssue(result, "Payload '" + payloadId + "' sealed blob could not be parsed: " + issue);
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizePayloadFailure(result);
        return result;
    }

    PackageOpenRequest payloadOpenRequest;
    payloadOpenRequest.packageId = package.cleartextManifest.packageId;
    payloadOpenRequest.recordId = result.payload.payloadId;
    payloadOpenRequest.additionalAuthenticatedData = makePackagePayloadAad(package.cleartextManifest,
                                                                          result.payload.payloadId,
                                                                          result.payload.payloadKind,
                                                                          result.payload.logicalPath,
                                                                          result.payload.mediaType,
                                                                          result.payload.plaintextSizeBytes);
    payloadOpenRequest.sealed = std::move(sealedPayload);

    if (!cryptoProvider.open(payloadOpenRequest, result.payload.plaintextBytes, issue))
    {
        addIssue(result, "Payload '" + payloadId + "' authentication failed: " + issue);
        setFailureCategory(result, PerformancePackageFailureCategory::decryptionFailure);
        finalizePayloadFailure(result, PerformancePackageFailureCategory::decryptionFailure);
        return result;
    }

    if (result.payload.plaintextBytes.size() != result.payload.plaintextSizeBytes)
    {
        addIssue(result, "Payload '" + payloadId + "' plaintext size did not match the decrypted TOC.");
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizePayloadFailure(result);
        return result;
    }

    if (computeFnv1a64Hex(result.payload.plaintextBytes) != result.payload.plaintextChecksumHex)
    {
        addIssue(result, "Payload '" + payloadId + "' plaintext checksum mismatch.");
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizePayloadFailure(result);
        return result;
    }

    result.loaded = true;
    result.state = "Performance package payload opened";
    return result;
}

PerformancePackageLoadResult loadPerformancePackage(const std::string& packagePath,
                                                    const PackageCryptoProvider& cryptoProvider,
                                                    const int supportedReaderSchemaVersion)
{
    PerformancePackageLoadResult result;
    result.packagePath = packagePath;
    result.state = "Performance package load not attempted";

    result.package = readPerformancePackage(packagePath, cryptoProvider, supportedReaderSchemaVersion);
    result.packageFound = result.package.packageFound;
    appendIssues(result.issues, result.package.issues);
    result.failureCategory = result.package.failureCategory;

    if (!result.package.valid)
    {
        finalizeLoadFailure(result);
        return result;
    }

    result.manifest = result.package.cleartextManifest;

    const auto packageManifestPayload = findPayloadByKind(result.package, "packageManifest");
    const auto instrumentPayload = findPayloadByKind(result.package, "runtimeInstrument");
    const auto streamIndexPayload = findPayloadByKind(result.package, "runtimeStreamIndex");
    const auto streamPayload = findPayloadByKind(result.package, "runtimeStreamPayload");

    if (!packageManifestPayload.has_value())
        addIssue(result, "Performance package is missing the required packageManifest payload.");
    if (!instrumentPayload.has_value())
        addIssue(result, "Performance package is missing the required runtimeInstrument payload.");
    if (!streamIndexPayload.has_value())
        addIssue(result, "Performance package is missing the required runtimeStreamIndex payload.");
    if (!streamPayload.has_value())
        addIssue(result, "Performance package is missing the required runtimeStreamPayload payload.");

    if (!result.issues.empty())
    {
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    const auto packageManifestOpen = openPerformancePackagePayload(result.package,
                                                                   packageManifestPayload->payloadId,
                                                                   cryptoProvider);
    appendIssues(result.issues, packageManifestOpen.issues);
    if (!packageManifestOpen.loaded)
    {
        addIssue(result, "Required packageManifest payload could not be opened.");
        setFailureCategory(result, packageManifestOpen.failureCategory);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    ordered_json manifestRoot;
    try
    {
        manifestRoot = ordered_json::parse(toStringViewBytes(packageManifestOpen.payload.plaintextBytes));
    }
    catch (const std::exception& exception)
    {
        addIssue(result, std::string("Package manifest payload JSON parse failed: ") + exception.what());
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    if (manifestRoot.value("schemaName", std::string {}) != performancePackageSchemaName)
    {
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        addIssue(result, "Package manifest payload schemaName must be 'drs.performancePackage'.");
    }

    const auto instrumentOpen = openPerformancePackagePayload(result.package,
                                                              instrumentPayload->payloadId,
                                                              cryptoProvider);
    appendIssues(result.issues, instrumentOpen.issues);
    if (!instrumentOpen.loaded)
    {
        addIssue(result, "Required runtimeInstrument payload could not be opened.");
        setFailureCategory(result, instrumentOpen.failureCategory);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    result.instrument = parseRuntimeInstrumentManifest(toStringViewBytes(instrumentOpen.payload.plaintextBytes),
                                                       packagePath + "#runtime-instrument",
                                                       false);
    appendIssues(result.issues, result.instrument.issues);

    const auto streamPayloadOpen = openPerformancePackagePayload(result.package,
                                                                 streamPayload->payloadId,
                                                                 cryptoProvider);
    appendIssues(result.issues, streamPayloadOpen.issues);
    if (!streamPayloadOpen.loaded)
    {
        addIssue(result, "Required runtimeStreamPayload payload could not be opened.");
        setFailureCategory(result, streamPayloadOpen.failureCategory);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    const auto streamIndexOpen = openPerformancePackagePayload(result.package,
                                                               streamIndexPayload->payloadId,
                                                               cryptoProvider);
    appendIssues(result.issues, streamIndexOpen.issues);
    if (!streamIndexOpen.loaded)
    {
        addIssue(result, "Required runtimeStreamIndex payload could not be opened.");
        setFailureCategory(result, streamIndexOpen.failureCategory);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    result.stream = parseRuntimeStreamContainer(toStringViewBytes(streamIndexOpen.payload.plaintextBytes),
                                                packagePath + "#runtime-stream-index",
                                                false,
                                                &streamPayloadOpen.payload.plaintextBytes);
    appendIssues(result.issues, result.stream.issues);

    if (!result.instrument.loaded || !result.stream.loaded)
    {
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    if (result.stream.container.containerId != result.instrument.instrument.instrumentId)
        addIssue(result, "Package stream containerId did not match the runtime instrumentId.");

    if (result.manifest.instrumentId != result.instrument.instrument.instrumentId)
        addIssue(result, "Package manifest instrumentId did not match the runtime instrument payload.");

    if (!result.issues.empty())
    {
        setFailureCategory(result, PerformancePackageFailureCategory::playbackCompatibilityFailure);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::playbackCompatibilityFailure);
        return result;
    }

    result.loaded = true;
    result.state = "Performance package loaded";
    return result;
}
} // namespace drs::engine
