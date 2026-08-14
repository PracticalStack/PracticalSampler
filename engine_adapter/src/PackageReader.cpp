#include "drs/engine/PackageReader.h"

#include <json/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
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

bool readHeaderFromFile(const fs::path& path, FileHeader& header, std::string& issue)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.good())
    {
        issue = "Could not open file for reading: " + path.generic_string();
        return false;
    }

    input.read(reinterpret_cast<char*>(&header), static_cast<std::streamsize>(sizeof(FileHeader)));
    if (input.gcount() != static_cast<std::streamsize>(sizeof(FileHeader)))
    {
        issue = "Performance package was truncated before its fixed header could be read.";
        return false;
    }

    issue.clear();
    return true;
}

bool readBinaryFileRange(const fs::path& path,
                         const std::uint64_t offsetBytes,
                         const std::uint64_t sizeBytes,
                         std::vector<std::uint8_t>& output,
                         std::string& issue)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.good())
    {
        issue = "Could not open file for reading: " + path.generic_string();
        return false;
    }

    input.seekg(0, std::ios::end);
    const auto fileSize = static_cast<std::uint64_t>(input.tellg());
    if (offsetBytes > fileSize || sizeBytes > fileSize - offsetBytes)
    {
        issue = "Package range exceeded file bounds.";
        return false;
    }

    input.seekg(static_cast<std::streamoff>(offsetBytes), std::ios::beg);
    output.resize(static_cast<std::size_t>(sizeBytes));
    if (sizeBytes == 0)
    {
        issue.clear();
        return true;
    }

    input.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(sizeBytes));
    if (input.gcount() != static_cast<std::streamsize>(sizeBytes))
    {
        issue = "Package range could not be read completely.";
        output.clear();
        return false;
    }

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

template <typename TResult>
bool parsePackageManifestPayload(const ordered_json& manifestRoot,
                                 TResult& result,
                                 PerformancePackageManifest& manifest,
                                 const std::string& context)
{
    if (!manifestRoot.is_object())
    {
        addIssue(result, context + " must be an object.");
        return false;
    }

    auto readRequiredString = [&](const char* fieldName, std::string& target)
    {
        if (!manifestRoot.contains(fieldName) || !manifestRoot.at(fieldName).is_string())
        {
            addIssue(result, context + " must provide string field '" + std::string(fieldName) + "'.");
            return false;
        }

        target = manifestRoot.at(fieldName).get<std::string>();
        return true;
    };

    auto readRequiredInt = [&](const char* fieldName, int& target)
    {
        if (!manifestRoot.contains(fieldName) || !manifestRoot.at(fieldName).is_number_integer())
        {
            addIssue(result, context + " must provide integer field '" + std::string(fieldName) + "'.");
            return false;
        }

        target = manifestRoot.at(fieldName).get<int>();
        return true;
    };

    bool valid = true;
    valid = readRequiredString("schemaName", manifest.schemaName) && valid;
    valid = readRequiredInt("schemaVersion", manifest.schemaVersion) && valid;
    valid = readRequiredString("packageId", manifest.packageId) && valid;
    valid = readRequiredString("displayName", manifest.displayName) && valid;
    valid = readRequiredString("instrumentId", manifest.instrumentId) && valid;
    valid = readRequiredInt("minimumReaderSchemaVersion", manifest.minimumReaderSchemaVersion) && valid;

    if (manifestRoot.contains("defaultLoadProfile"))
    {
        if (!manifestRoot.at("defaultLoadProfile").is_string())
        {
            addIssue(result, context + " field 'defaultLoadProfile' must be a string when present.");
            valid = false;
        }
        else
        {
            manifest.defaultLoadProfile = manifestRoot.at("defaultLoadProfile").get<std::string>();
        }
    }

    if (manifestRoot.contains("notes"))
    {
        if (!manifestRoot.at("notes").is_array())
        {
            addIssue(result, context + " field 'notes' must be an array when present.");
            valid = false;
        }
        else
        {
            for (std::size_t index = 0; index < manifestRoot.at("notes").size(); ++index)
            {
                const auto& note = manifestRoot.at("notes").at(index);
                if (!note.is_string())
                {
                    addIssue(result,
                             context + " field 'notes[" + std::to_string(index) + "]' must be a string.");
                    valid = false;
                    continue;
                }

                manifest.notes.push_back(note.get<std::string>());
            }
        }
    }

    if (manifestRoot.contains("backgroundImage"))
    {
        if (!manifestRoot.at("backgroundImage").is_object())
        {
            addIssue(result, context + " field 'backgroundImage' must be an object when present.");
            valid = false;
        }
        else
        {
            const auto& backgroundImageRoot = manifestRoot.at("backgroundImage");
            if (!backgroundImageRoot.contains("payloadId") || !backgroundImageRoot.at("payloadId").is_string())
            {
                addIssue(result, context + " field 'backgroundImage.payloadId' must be a string when present.");
                valid = false;
            }
            else
            {
                manifest.backgroundImage.payloadId = backgroundImageRoot.at("payloadId").get<std::string>();
                if (manifest.backgroundImage.payloadId.empty())
                {
                    addIssue(result, context + " field 'backgroundImage.payloadId' must not be empty.");
                    valid = false;
                }
            }
        }
    }

    if (manifestRoot.contains("masterGainDb"))
    {
        if (!manifestRoot.at("masterGainDb").is_number())
        {
            addIssue(result, context + " field 'masterGainDb' must be numeric when present.");
            valid = false;
        }
        else
        {
            manifest.masterGainDb = manifestRoot.at("masterGainDb").get<double>();
            if (!std::isfinite(manifest.masterGainDb))
            {
                addIssue(result, context + " field 'masterGainDb' must be finite.");
                valid = false;
            }
        }
    }

    if (manifestRoot.contains("groupRoutes"))
    {
        if (!manifestRoot.at("groupRoutes").is_array())
        {
            addIssue(result, context + " field 'groupRoutes' must be an array when present.");
            valid = false;
        }
        else
        {
            std::unordered_map<std::string, bool> groupIds;
            for (std::size_t index = 0; index < manifestRoot.at("groupRoutes").size(); ++index)
            {
                const auto& routeRoot = manifestRoot.at("groupRoutes").at(index);
                const auto routeContext = context + " field 'groupRoutes[" + std::to_string(index) + "]'";

                if (!routeRoot.is_object())
                {
                    addIssue(result, routeContext + " must be an object.");
                    valid = false;
                    continue;
                }

                PerformancePackageManifest::GroupRoute route;
                if (!routeRoot.contains("groupId") || !routeRoot.at("groupId").is_string())
                {
                    addIssue(result, routeContext + " must provide string field 'groupId'.");
                    valid = false;
                }
                else
                {
                    route.groupId = routeRoot.at("groupId").get<std::string>();
                    if (route.groupId.empty())
                    {
                        addIssue(result, routeContext + " field 'groupId' must not be empty.");
                        valid = false;
                    }
                    else if (!groupIds.emplace(route.groupId, true).second)
                    {
                        addIssue(result,
                                 context + " field 'groupRoutes' must not repeat groupId '" + route.groupId + "'.");
                        valid = false;
                    }
                }

                if (!routeRoot.contains("gainDb") || !routeRoot.at("gainDb").is_number())
                {
                    addIssue(result, routeContext + " must provide numeric field 'gainDb'.");
                    valid = false;
                }
                else
                {
                    route.gainDb = routeRoot.at("gainDb").get<double>();
                    if (!std::isfinite(route.gainDb))
                    {
                        addIssue(result, routeContext + " field 'gainDb' must be finite.");
                        valid = false;
                    }
                }

                manifest.groupRoutes.push_back(std::move(route));
            }
        }
    }

    if (manifest.schemaName != performancePackageSchemaName)
    {
        addIssue(result,
                 context + " field 'schemaName' must be '" + std::string(performancePackageSchemaName) + "'.");
        valid = false;
    }

    return valid;
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
    const auto packageFsPath = fs::path(packagePath);

    result.packageFound = true;

    FileHeader header;
    if (!readHeaderFromFile(packageFsPath, header, issue))
    {
        addIssue(result, issue);
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
    if (!readBinaryFileRange(packageFsPath,
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
    if (!readBinaryFileRange(packageFsPath,
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

    const auto payloadView = findPayload(package, payloadId);
    if (!payloadView.has_value())
    {
        addIssue(result, "Performance package payload id '" + payloadId + "' was not present in the package TOC.");
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizePayloadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    result.found = true;
    result.payload = *payloadView;

    const auto absoluteOffsetBytes = package.header.payloadRegionOffsetBytes + result.payload.payloadOffsetBytes;
    std::string issue;
    std::vector<std::uint8_t> sealedPayloadBytes;
    if (!readBinaryFileRange(fs::path(package.packagePath),
                             absoluteOffsetBytes,
                             result.payload.sealedSizeBytes,
                             sealedPayloadBytes,
                             issue))
    {
        addIssue(result, "Payload '" + payloadId + "' range was invalid: " + issue);
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizePayloadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    PackageSealedBlob sealedPayload;
    if (!parseSealedBlob(sealedPayloadBytes, cryptoProvider, sealedPayload, issue))
    {
        addIssue(result, "Payload '" + payloadId + "' sealed bytes could not be parsed: " + issue);
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizePayloadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
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
        finalizePayloadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    if (computeFnv1a64Hex(result.payload.plaintextBytes) != result.payload.plaintextChecksumHex)
    {
        addIssue(result, "Payload '" + payloadId + "' plaintext checksum mismatch.");
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizePayloadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    result.loaded = true;
    result.state = "Performance package payload opened";
    return result;
}

PerformancePackageLoadResult loadPerformancePackageInternal(const std::string& packagePath,
                                                            const PackageCryptoProvider& cryptoProvider,
                                                            const int supportedReaderSchemaVersion,
                                                            const bool includeStreamPayload)
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
    const auto streamPayload = includeStreamPayload
        ? findPayloadByKind(result.package, "runtimeStreamPayload")
        : std::optional<PerformancePackagePayloadView> {};

    if (!packageManifestPayload.has_value())
        addIssue(result, "Performance package is missing the required packageManifest payload.");
    if (!instrumentPayload.has_value())
        addIssue(result, "Performance package is missing the required runtimeInstrument payload.");
    if (!streamIndexPayload.has_value())
        addIssue(result, "Performance package is missing the required runtimeStreamIndex payload.");
    if (includeStreamPayload && !streamPayload.has_value())
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

    PerformancePackageManifest packageManifest;
    if (!parsePackageManifestPayload(manifestRoot, result, packageManifest, "Package manifest payload"))
    {
        setFailureCategory(result, PerformancePackageFailureCategory::payloadCorruption);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
        return result;
    }

    result.manifest = std::move(packageManifest);

    if (result.manifest.packageId != result.package.cleartextManifest.packageId)
        addIssue(result, "Package manifest payload packageId did not match the cleartext packageId.");

    if (result.manifest.instrumentId != result.package.cleartextManifest.instrumentId)
        addIssue(result, "Package manifest payload instrumentId did not match the cleartext instrumentId.");

    if (result.manifest.minimumReaderSchemaVersion
        != result.package.cleartextManifest.minimumReaderSchemaVersion)
    {
        addIssue(result,
                 "Package manifest payload minimumReaderSchemaVersion did not match the cleartext metadata.");
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
    if (result.instrument.loaded
        && result.instrument.instrument.schemaVersion == runtimeInstrumentFxRoutingSchemaVersion)
    {
        addIssue(result,
                 "Runtime instrument v4 FX/routing activation is not enabled in the current package reader.");
        setFailureCategory(result, PerformancePackageFailureCategory::playbackCompatibilityFailure);
        finalizeLoadFailure(result, PerformancePackageFailureCategory::playbackCompatibilityFailure);
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

    std::vector<std::uint8_t> embeddedStreamPayloadBytes;
    std::vector<std::uint8_t>* embeddedStreamPayloadBytesPtr = nullptr;
    if (includeStreamPayload)
    {
        auto streamPayloadOpen = openPerformancePackagePayload(result.package,
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

        embeddedStreamPayloadBytes = std::move(streamPayloadOpen.payload.plaintextBytes);
        embeddedStreamPayloadBytesPtr = &embeddedStreamPayloadBytes;
    }

    result.stream = parseRuntimeStreamContainer(toStringViewBytes(streamIndexOpen.payload.plaintextBytes),
                                                packagePath + "#runtime-stream-index",
                                                false,
                                                embeddedStreamPayloadBytesPtr);
    appendIssues(result.issues, result.stream.issues);

    if (!result.manifest.backgroundImage.payloadId.empty())
    {
        result.backgroundImage = openPerformancePackagePayload(result.package,
                                                               result.manifest.backgroundImage.payloadId,
                                                               cryptoProvider);
        appendIssues(result.issues, result.backgroundImage.issues);
        if (!result.backgroundImage.loaded)
        {
            addIssue(result, "Required backgroundImage payload could not be opened.");
            setFailureCategory(result, result.backgroundImage.failureCategory);
            finalizeLoadFailure(result, PerformancePackageFailureCategory::payloadCorruption);
            return result;
        }

        if (result.backgroundImage.payload.mediaType != "image/jpeg")
        {
            addIssue(result, "Performance package backgroundImage payload must use mediaType 'image/jpeg'.");
        }

        if (result.backgroundImage.payload.payloadKind != "backgroundImage")
        {
            addIssue(result, "Performance package backgroundImage payload kind must be 'backgroundImage'.");
        }
    }

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

PerformancePackageLoadResult loadPerformancePackage(const std::string& packagePath,
                                                    const PackageCryptoProvider& cryptoProvider,
                                                    const int supportedReaderSchemaVersion)
{
    return loadPerformancePackageInternal(packagePath,
                                          cryptoProvider,
                                          supportedReaderSchemaVersion,
                                          true);
}

PerformancePackageManifestParseResult parsePerformancePackageManifestJson(
    const std::string& text)
{
    PerformancePackageManifestParseResult result;
    ordered_json root;
    try
    {
        root = ordered_json::parse(text);
    }
    catch (const std::exception& exception)
    {
        result.issues.push_back(std::string("Package manifest JSON parse failed: ")
                                + exception.what());
        return result;
    }
    result.parsed = parsePackageManifestPayload(root, result, result.manifest,
                                                "Package manifest payload");
    return result;
}

PerformancePackageLoadResult loadPerformancePackageMetadataOnly(
    const std::string& packagePath,
    const PackageCryptoProvider& cryptoProvider,
    const int supportedReaderSchemaVersion)
{
    return loadPerformancePackageInternal(packagePath,
                                          cryptoProvider,
                                          supportedReaderSchemaVersion,
                                          false);
}
} // namespace drs::engine
