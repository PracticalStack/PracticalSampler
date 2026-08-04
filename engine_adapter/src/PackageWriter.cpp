#include "drs/engine/PackageWriter.h"

#include "drs/engine/RuntimeLoader.h"

#include <json/json.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cstddef>
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
constexpr std::uint32_t kPackageFlagSealed = 1u << 0u;
constexpr std::uint32_t kPackageFlagEncryptedToc = 1u << 1u;
constexpr std::uint32_t kPackageFlagEncryptedPayloads = 1u << 2u;
constexpr std::uint32_t kPackageFlagAuthoringUnavailable = 1u << 3u;

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

struct SealedPayload
{
    PerformancePackagePayloadSource source;
    std::string plaintextChecksumHex;
    PackageSealedBlob sealed;
    std::uint64_t payloadOffsetBytes = 0;
};

void addIssue(PerformancePackageWriteResult& result, const std::string& issue)
{
    result.issues.push_back(issue);
}

void addIssue(PerformancePackageInspectionResult& result, const std::string& issue)
{
    result.issues.push_back(issue);
}

ordered_json serializeStringArray(const std::vector<std::string>& values)
{
    ordered_json array = ordered_json::array();
    for (const auto& value : values)
        array.push_back(value);
    return array;
}

std::vector<std::uint8_t> toBytes(std::string_view text)
{
    return std::vector<std::uint8_t>(text.begin(), text.end());
}

std::string toStringViewBytes(const std::vector<std::uint8_t>& bytes)
{
    return std::string(bytes.begin(), bytes.end());
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

bool writeBinaryFile(const fs::path& path, const std::vector<std::uint8_t>& bytes, std::string& issue)
{
    std::error_code errorCode;
    fs::create_directories(path.parent_path(), errorCode);

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.good())
    {
        issue = "Could not open package file for writing: " + path.generic_string();
        return false;
    }

    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output.good())
    {
        issue = "Could not finish writing package file: " + path.generic_string();
        return false;
    }

    issue.clear();
    return true;
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

std::vector<std::uint8_t> serializeSealedBlob(const PackageSealedBlob& sealed)
{
    std::vector<std::uint8_t> bytes;
    bytes.reserve(sealed.nonce.size() + sealed.tag.size() + sealed.ciphertext.size());
    bytes.insert(bytes.end(), sealed.nonce.begin(), sealed.nonce.end());
    bytes.insert(bytes.end(), sealed.tag.begin(), sealed.tag.end());
    bytes.insert(bytes.end(), sealed.ciphertext.begin(), sealed.ciphertext.end());
    return bytes;
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

std::string makePayloadUri(const std::string& payloadId)
{
    return std::string(performancePackageInternalUriPrefix) + payloadId;
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

std::vector<std::string> buildCleartextFlags()
{
    return {
        "sealed",
        "encryptedToc",
        "encryptedPayloads",
        "performanceOnly",
        "authoringUnavailable"
    };
}

std::string serializePackageManifestJson(const PerformancePackageManifest& manifest)
{
    ordered_json root;
    root["schemaName"] = manifest.schemaName;
    root["schemaVersion"] = manifest.schemaVersion;
    root["packageId"] = manifest.packageId;
    root["displayName"] = manifest.displayName;
    root["instrumentId"] = manifest.instrumentId;
    root["defaultLoadProfile"] = manifest.defaultLoadProfile;
    root["minimumReaderSchemaVersion"] = manifest.minimumReaderSchemaVersion;
    root["notes"] = serializeStringArray(manifest.notes);
    return root.dump(2) + "\n";
}

std::string serializeCleartextMetadataJson(const PerformancePackageManifest& manifest,
                                           const std::string& minimumCompatibleAppVersion,
                                           const PackageCryptoProvider& cryptoProvider,
                                           const std::uint32_t payloadCount)
{
    ordered_json root;
    root["schemaName"] = performancePackageSchemaName;
    root["schemaVersion"] = performancePackageSchemaVersion;
    root["formatVersion"] = performancePackageFormatVersion;
    root["packageId"] = manifest.packageId;
    root["displayName"] = manifest.displayName;
    root["instrumentId"] = manifest.instrumentId;
    root["defaultLoadProfile"] = manifest.defaultLoadProfile;
    root["minimumReaderSchemaVersion"] = manifest.minimumReaderSchemaVersion;
    root["minimumCompatibleAppVersion"] = minimumCompatibleAppVersion;
    root["documentKind"] = toString(WorkspaceDocumentKind::performancePackage);
    root["workspaceMode"] = toString(WorkspaceMode::performanceOnly);
    root["cryptoAlgorithm"] = cryptoProvider.algorithmId();
    root["nonceSizeBytes"] = cryptoProvider.nonceSizeBytes();
    root["tagSizeBytes"] = cryptoProvider.tagSizeBytes();
    root["payloadCount"] = payloadCount;
    root["flags"] = serializeStringArray(buildCleartextFlags());
    return root.dump(2) + "\n";
}

RuntimeCompileResult buildPackageCompileResult(const RuntimeCompileResult& compiledRuntime)
{
    RuntimeCompileResult packagedRuntime = compiledRuntime;
    packagedRuntime.instrument.sourceProjectPath = "package://authoring/unavailable";
    packagedRuntime.instrument.compiledStreamAssetPath = makePayloadUri("runtime-stream-index");

    std::unordered_map<std::string, std::string> payloadUriBySourcePath;
    payloadUriBySourcePath.reserve(packagedRuntime.streamSamples.size());

    for (auto& sample : packagedRuntime.streamSamples)
    {
        const auto sampleUri = "package://sample/" + sample.sampleId;
        payloadUriBySourcePath[sample.sourcePath] = sampleUri;
        sample.sourcePath = sampleUri;
    }

    packagedRuntime.payloadFilePath = makePayloadUri("runtime-stream-payload");

    for (auto& zone : packagedRuntime.instrument.zones)
    {
        const auto iterator = payloadUriBySourcePath.find(zone.samplePath);
        if (iterator != payloadUriBySourcePath.end())
            zone.samplePath = iterator->second;

        zone.streamAssetPath = makePayloadUri("runtime-stream-index");
    }

    return packagedRuntime;
}

PerformancePackagePayloadSource buildPackageManifestPayload(const PerformancePackageManifest& manifest)
{
    PerformancePackagePayloadSource payload;
    payload.payloadId = "package-manifest";
    payload.kind = PerformancePackagePayloadKind::packageManifest;
    payload.logicalPath = "manifest/package-manifest.json";
    payload.mediaType = "application/json";
    payload.plaintextBytes = toBytes(serializePackageManifestJson(manifest));
    return payload;
}

PerformancePackagePayloadSource buildRuntimeInstrumentPayload(const RuntimeCompileResult& compiledRuntime)
{
    PerformancePackagePayloadSource payload;
    payload.payloadId = "runtime-instrument";
    payload.kind = PerformancePackagePayloadKind::runtimeInstrument;
    payload.logicalPath = "manifest/runtime-instrument.drinst";
    payload.mediaType = "application/json";
    payload.plaintextBytes = toBytes(serializeRuntimeInstrumentManifest(compiledRuntime.instrument,
                                                                        "package://manifest/runtime-instrument.drinst"));
    return payload;
}

PerformancePackagePayloadSource buildRuntimeStreamIndexPayload(const RuntimeCompileResult& compiledRuntime)
{
    PerformancePackagePayloadSource payload;
    payload.payloadId = "runtime-stream-index";
    payload.kind = PerformancePackagePayloadKind::runtimeStreamIndex;
    payload.logicalPath = "manifest/runtime-stream-index.drstrm";
    payload.mediaType = "application/json";
    payload.plaintextBytes = toBytes(serializeCompiledStreamIndex(compiledRuntime,
                                                                  "package://manifest/runtime-stream-index.drstrm"));
    return payload;
}

PerformancePackagePayloadSource buildRuntimeStreamPayloadPayload(const RuntimeCompileResult& compiledRuntime)
{
    PerformancePackagePayloadSource payload;
    payload.payloadId = "runtime-stream-payload";
    payload.kind = PerformancePackagePayloadKind::runtimeStreamPayload;
    payload.logicalPath = "stream/runtime-stream-payload.drstrm.bin";
    payload.mediaType = "application/octet-stream";

    std::string issue;
    payload.plaintextBytes = readBinaryFile(fs::path(compiledRuntime.payloadFilePath), issue);
    return payload;
}

bool validateWritePlan(const PerformancePackageWritePlan& plan, PerformancePackageWriteResult& result)
{
    if (plan.outputPackagePath.empty())
        addIssue(result, "Performance package write plan requires a non-empty outputPackagePath.");

    if (plan.manifest.packageId.empty())
        addIssue(result, "Performance package write plan requires manifest.packageId.");

    if (plan.manifest.displayName.empty())
        addIssue(result, "Performance package write plan requires manifest.displayName.");

    if (plan.manifest.instrumentId.empty())
        addIssue(result, "Performance package write plan requires manifest.instrumentId.");

    if (plan.payloads.empty())
        addIssue(result, "Performance package write plan requires at least one payload.");

    std::unordered_map<std::string, bool> payloadIds;
    for (const auto& payload : plan.payloads)
    {
        if (payload.payloadId.empty())
        {
            addIssue(result, "Performance package payload ids must be non-empty.");
            continue;
        }

        const auto [iterator, inserted] = payloadIds.emplace(payload.payloadId, true);
        if (!inserted)
        {
            static_cast<void>(iterator);
            addIssue(result, "Performance package payload ids must be unique; duplicate id '" + payload.payloadId + "' was provided.");
        }

        if (payload.logicalPath.empty())
            addIssue(result, "Performance package payload '" + payload.payloadId + "' requires a logicalPath.");

        if (payload.mediaType.empty())
            addIssue(result, "Performance package payload '" + payload.payloadId + "' requires a mediaType.");
    }

    return result.issues.empty();
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

bool parseCleartextManifest(const ordered_json& metadataRoot,
                            PerformancePackageInspectionResult& result)
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

} // namespace

const char* toString(const PerformancePackagePayloadKind kind) noexcept
{
    switch (kind)
    {
        case PerformancePackagePayloadKind::packageManifest:
            return "packageManifest";
        case PerformancePackagePayloadKind::runtimeInstrument:
            return "runtimeInstrument";
        case PerformancePackagePayloadKind::runtimeStreamIndex:
            return "runtimeStreamIndex";
        case PerformancePackagePayloadKind::runtimeStreamPayload:
            return "runtimeStreamPayload";
    }

    return "unknown";
}

PerformancePackageWritePlan buildPerformancePackageWritePlan(const PerformancePackageCompileWritePlan& plan)
{
    PerformancePackageWritePlan writePlan;
    writePlan.manifest = plan.manifest;
    writePlan.outputPackagePath = plan.outputPackagePath;
    writePlan.minimumCompatibleAppVersion = plan.minimumCompatibleAppVersion;

    const auto packagedRuntime = buildPackageCompileResult(plan.compiledRuntime);
    writePlan.payloads.push_back(buildPackageManifestPayload(writePlan.manifest));
    writePlan.payloads.push_back(buildRuntimeInstrumentPayload(packagedRuntime));
    writePlan.payloads.push_back(buildRuntimeStreamIndexPayload(packagedRuntime));
    writePlan.payloads.push_back(buildRuntimeStreamPayloadPayload(plan.compiledRuntime));
    return writePlan;
}

PerformancePackageWriteResult writePerformancePackage(const PerformancePackageWritePlan& plan,
                                                      const PackageCryptoProvider& cryptoProvider)
{
    PerformancePackageWriteResult result;
    result.packagePath = plan.outputPackagePath;
    result.cryptoAlgorithm = cryptoProvider.algorithmId();
    result.payloadCount = static_cast<std::uint32_t>(plan.payloads.size());
    result.state = "Performance package write not attempted";

    if (!validateWritePlan(plan, result))
    {
        result.state = "Performance package write rejected";
        return result;
    }

    const auto cleartextMetadataJson = serializeCleartextMetadataJson(plan.manifest,
                                                                      plan.minimumCompatibleAppVersion,
                                                                      cryptoProvider,
                                                                      result.payloadCount);

    std::vector<SealedPayload> sealedPayloads;
    sealedPayloads.reserve(plan.payloads.size());

    for (const auto& payload : plan.payloads)
    {
        SealedPayload sealedPayload;
        sealedPayload.source = payload;
        sealedPayload.plaintextChecksumHex = computeFnv1a64Hex(payload.plaintextBytes);

        PackageSealRequest sealRequest;
        sealRequest.packageId = plan.manifest.packageId;
        sealRequest.recordId = payload.payloadId;
        sealRequest.additionalAuthenticatedData = makePackagePayloadAad(plan.manifest,
                                                                       payload.payloadId,
                                                                       toString(payload.kind),
                                                                       payload.logicalPath,
                                                                       payload.mediaType,
                                                                       payload.plaintextBytes.size());
        sealRequest.plaintext = payload.plaintextBytes;

        std::string issue;
        if (!cryptoProvider.seal(sealRequest, sealedPayload.sealed, issue))
        {
            addIssue(result, "Could not seal payload '" + payload.payloadId + "': " + issue);
            result.state = "Performance package write failed";
            return result;
        }

        sealedPayloads.push_back(std::move(sealedPayload));
    }

    std::uint64_t nextPayloadOffsetBytes = 0;
    ordered_json tocRoot;
    tocRoot["schemaName"] = "drs.performancePackage.toc";
    tocRoot["schemaVersion"] = performancePackageSchemaVersion;
    tocRoot["packageId"] = plan.manifest.packageId;
    tocRoot["cryptoAlgorithm"] = cryptoProvider.algorithmId();
    tocRoot["payloads"] = ordered_json::array();

    for (auto& sealedPayload : sealedPayloads)
    {
        sealedPayload.payloadOffsetBytes = nextPayloadOffsetBytes;
        const auto sealedBytes = serializeSealedBlob(sealedPayload.sealed);

        ordered_json payloadObject;
        payloadObject["payloadId"] = sealedPayload.source.payloadId;
        payloadObject["payloadKind"] = toString(sealedPayload.source.kind);
        payloadObject["logicalPath"] = sealedPayload.source.logicalPath;
        payloadObject["mediaType"] = sealedPayload.source.mediaType;
        payloadObject["payloadOffsetBytes"] = sealedPayload.payloadOffsetBytes;
        payloadObject["sealedSizeBytes"] = sealedBytes.size();
        payloadObject["plaintextSizeBytes"] = sealedPayload.source.plaintextBytes.size();
        payloadObject["plaintextChecksumHex"] = sealedPayload.plaintextChecksumHex;
        tocRoot["payloads"].push_back(std::move(payloadObject));

        nextPayloadOffsetBytes += sealedBytes.size();
    }

    tocRoot["payloadRegionSizeBytes"] = nextPayloadOffsetBytes;
    const auto tocJson = tocRoot.dump(2) + "\n";

    PackageSealRequest tocSealRequest;
    tocSealRequest.packageId = plan.manifest.packageId;
    tocSealRequest.recordId = "encrypted-toc";
    tocSealRequest.additionalAuthenticatedData = makeTocAad(plan.manifest,
                                                            cleartextMetadataJson,
                                                            result.payloadCount);
    tocSealRequest.plaintext = toBytes(tocJson);

    PackageSealedBlob sealedToc;
    std::string issue;
    if (!cryptoProvider.seal(tocSealRequest, sealedToc, issue))
    {
        addIssue(result, "Could not seal package TOC: " + issue);
        result.state = "Performance package write failed";
        return result;
    }

    const auto sealedTocBytes = serializeSealedBlob(sealedToc);

    FileHeader header;
    std::copy(kMagic.begin(), kMagic.end(), header.magic);
    header.flags = kPackageFlagSealed | kPackageFlagEncryptedToc | kPackageFlagEncryptedPayloads
        | kPackageFlagAuthoringUnavailable;
    header.minimumReaderSchemaVersion = plan.manifest.minimumReaderSchemaVersion;
    header.cleartextMetadataOffsetBytes = sizeof(FileHeader);
    header.cleartextMetadataSizeBytes = cleartextMetadataJson.size();
    header.tocOffsetBytes = header.cleartextMetadataOffsetBytes + header.cleartextMetadataSizeBytes;
    header.tocSealedSizeBytes = sealedTocBytes.size();
    header.payloadRegionOffsetBytes = header.tocOffsetBytes + header.tocSealedSizeBytes;
    header.payloadRegionSizeBytes = nextPayloadOffsetBytes;
    header.payloadCount = result.payloadCount;

    std::vector<std::uint8_t> fileBytes(sizeof(FileHeader), 0);
    std::memcpy(fileBytes.data(), &header, sizeof(FileHeader));
    {
        const auto metadataBytes = toBytes(cleartextMetadataJson);
        fileBytes.insert(fileBytes.end(), metadataBytes.begin(), metadataBytes.end());
    }
    fileBytes.insert(fileBytes.end(), sealedTocBytes.begin(), sealedTocBytes.end());
    for (const auto& sealedPayload : sealedPayloads)
    {
        const auto sealedBytes = serializeSealedBlob(sealedPayload.sealed);
        fileBytes.insert(fileBytes.end(), sealedBytes.begin(), sealedBytes.end());
    }

    if (!writeBinaryFile(fs::path(plan.outputPackagePath), fileBytes, issue))
    {
        addIssue(result, issue);
        result.state = "Performance package write failed";
        return result;
    }

    result.packageBytes = fileBytes.size();
    result.written = true;
    result.state = "Performance package written";
    return result;
}

PerformancePackageWriteResult writePerformancePackage(const PerformancePackageCompileWritePlan& plan,
                                                      const PackageCryptoProvider& cryptoProvider)
{
    if (!plan.compiledRuntime.compiled)
    {
        PerformancePackageWriteResult result;
        result.packagePath = plan.outputPackagePath;
        result.cryptoAlgorithm = cryptoProvider.algorithmId();
        result.state = "Performance package write rejected";
        addIssue(result, "Cannot write a performance package from a compile result that did not succeed.");
        return result;
    }

    if (plan.compiledRuntime.payloadFilePath.empty())
    {
        PerformancePackageWriteResult result;
        result.packagePath = plan.outputPackagePath;
        result.cryptoAlgorithm = cryptoProvider.algorithmId();
        result.state = "Performance package write rejected";
        addIssue(result, "Cannot write a performance package before compiled stream assets exist.");
        return result;
    }

    auto writePlan = buildPerformancePackageWritePlan(plan);
    const auto payloadIterator = std::find_if(writePlan.payloads.begin(),
                                              writePlan.payloads.end(),
                                              [](const PerformancePackagePayloadSource& payload)
                                              {
                                                  return payload.kind == PerformancePackagePayloadKind::runtimeStreamPayload;
                                              });

    if (payloadIterator == writePlan.payloads.end()
        || payloadIterator->plaintextBytes.size() != plan.compiledRuntime.alignedPayloadBytes)
    {
        PerformancePackageWriteResult result;
        result.packagePath = plan.outputPackagePath;
        result.cryptoAlgorithm = cryptoProvider.algorithmId();
        result.state = "Performance package write rejected";
        addIssue(result,
                 "Cannot write a performance package because the compiled stream payload could not be loaded from '"
                     + plan.compiledRuntime.payloadFilePath + "'.");
        return result;
    }

    return writePerformancePackage(writePlan, cryptoProvider);
}

std::size_t getPerformancePackageHeaderSizeBytes() noexcept
{
    return sizeof(FileHeader);
}

std::size_t getPerformancePackageHeaderPayloadCountOffsetBytes() noexcept
{
    return offsetof(FileHeader, payloadCount);
}

PerformancePackageInspectionResult inspectPerformancePackage(const std::string& packagePath,
                                                             const PackageCryptoProvider& cryptoProvider,
                                                             const int supportedReaderSchemaVersion)
{
    PerformancePackageInspectionResult result;
    result.packagePath = packagePath;
    result.state = "Performance package inspection not attempted";

    std::string issue;
    const auto fileBytes = readBinaryFile(fs::path(packagePath), issue);
    if (!issue.empty())
    {
        addIssue(result, issue);
        result.state = "Performance package inspection failed";
        return result;
    }

    result.packageFound = true;

    FileHeader header;
    if (!readHeader(fileBytes, header))
    {
        addIssue(result, "Performance package was truncated before its fixed header could be read.");
        result.state = "Performance package inspection failed";
        return result;
    }

    if (!std::equal(kMagic.begin(), kMagic.end(), header.magic))
    {
        addIssue(result, "Performance package magic did not match the expected DRS package signature.");
        result.state = "Performance package inspection failed";
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
        result.state = "Performance package inspection failed";
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
        result.state = "Performance package inspection failed";
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
        result.state = "Performance package inspection failed";
        return result;
    }

    if (!parseCleartextManifest(metadataRoot, result))
    {
        result.state = "Performance package inspection failed";
        return result;
    }

    if (result.cleartextManifest.minimumReaderSchemaVersion > supportedReaderSchemaVersion)
    {
        addIssue(result,
                 "Performance package requires reader schema version "
                     + std::to_string(result.cleartextManifest.minimumReaderSchemaVersion)
                     + " but the current reader only supports "
                     + std::to_string(supportedReaderSchemaVersion) + ".");
        result.state = "Performance package inspection failed";
        return result;
    }

    if (result.cleartextManifest.packageId.empty())
    {
        addIssue(result, "Package cleartext metadata did not provide packageId.");
        result.state = "Performance package inspection failed";
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
        result.state = "Performance package inspection failed";
        return result;
    }

    PackageSealedBlob sealedToc;
    if (!parseSealedBlob(sealedTocBytes, cryptoProvider, sealedToc, issue))
    {
        addIssue(result, "Encrypted TOC blob could not be parsed: " + issue);
        result.state = "Performance package inspection failed";
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
        result.state = "Performance package inspection failed";
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
        result.state = "Performance package inspection failed";
        return result;
    }

    if (tocRoot.value("packageId", std::string {}) != result.cleartextManifest.packageId)
    {
        addIssue(result, "Decrypted TOC packageId did not match the cleartext packageId.");
    }

    const auto payloadRegionSizeBytes = tocRoot.value("payloadRegionSizeBytes", std::uint64_t { 0 });
    if (payloadRegionSizeBytes != header.payloadRegionSizeBytes)
    {
        addIssue(result, "Decrypted TOC payloadRegionSizeBytes did not match the cleartext header.");
    }

    if (!tocRoot.contains("payloads") || !tocRoot.at("payloads").is_array())
    {
        addIssue(result, "Decrypted TOC must provide a payloads array.");
        result.state = "Performance package inspection failed";
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

        const auto absoluteOffsetBytes = header.payloadRegionOffsetBytes + payloadView.payloadOffsetBytes;
        std::vector<std::uint8_t> sealedPayloadBytes;
        if (!extractFileRange(fileBytes,
                              absoluteOffsetBytes,
                              payloadView.sealedSizeBytes,
                              sealedPayloadBytes,
                              issue))
        {
            addIssue(result, "Payload '" + payloadView.payloadId + "' exceeded the package file bounds.");
            continue;
        }

        PackageSealedBlob sealedPayload;
        if (!parseSealedBlob(sealedPayloadBytes, cryptoProvider, sealedPayload, issue))
        {
            addIssue(result, "Payload '" + payloadView.payloadId + "' sealed blob could not be parsed: " + issue);
            continue;
        }

        PerformancePackagePayloadSource payloadSource;
        PackageOpenRequest payloadOpenRequest;
        payloadOpenRequest.packageId = result.cleartextManifest.packageId;
        payloadOpenRequest.recordId = payloadView.payloadId;
        payloadOpenRequest.additionalAuthenticatedData = makePackagePayloadAad(result.cleartextManifest,
                                                                              payloadView.payloadId,
                                                                              payloadView.payloadKind,
                                                                              payloadView.logicalPath,
                                                                              payloadView.mediaType,
                                                                              payloadView.plaintextSizeBytes);
        payloadOpenRequest.sealed = std::move(sealedPayload);

        if (!cryptoProvider.open(payloadOpenRequest, payloadView.plaintextBytes, issue))
        {
            addIssue(result, "Payload '" + payloadView.payloadId + "' authentication failed: " + issue);
            continue;
        }

        if (payloadView.plaintextBytes.size() != payloadView.plaintextSizeBytes)
        {
            addIssue(result,
                     "Payload '" + payloadView.payloadId + "' plaintext size did not match the decrypted TOC.");
        }

        if (computeFnv1a64Hex(payloadView.plaintextBytes) != payloadView.plaintextChecksumHex)
        {
            addIssue(result,
                     "Payload '" + payloadView.payloadId + "' plaintext checksum mismatch.");
        }

        result.payloads.push_back(std::move(payloadView));
    }

    if (!result.issues.empty())
    {
        result.state = "Performance package validation failed";
        return result;
    }

    result.valid = true;
    result.state = "Performance package validated";
    return result;
}
} // namespace drs::engine
