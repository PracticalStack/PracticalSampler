#include "drs/engine/PackageV2.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <type_traits>
#include <unordered_set>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;
constexpr std::array<char, 8> magic { 'D', 'R', 'S', 'P', 'K', 'G', '2', '\0' };
constexpr std::uint64_t fnvOffset = 14695981039346656037ull;
constexpr std::uint64_t fnvPrime = 1099511628211ull;

#pragma pack(push, 1)
struct DiskHeader
{
    char magicBytes[8] {};
    std::uint32_t formatVersion = performancePackageV2FormatVersion;
    std::uint32_t headerBytes = sizeof(DiskHeader);
    std::uint32_t recordCount = 0;
    std::uint32_t flags = 1;
    std::uint64_t tocOffsetBytes = sizeof(DiskHeader);
    std::uint64_t tocSizeBytes = 0;
    std::uint64_t recordRegionOffsetBytes = 0;
    std::uint64_t recordRegionSizeBytes = 0;
    char packageId[64] {};
};

struct DiskRecord
{
    char sourceId[64] {};
    std::uint32_t kind = 0;
    std::uint64_t pageIndex = 0;
    std::uint64_t sealedOffsetBytes = 0;
    std::uint64_t sealedSizeBytes = 0;
    std::uint64_t plaintextSizeBytes = 0;
    char checksumHex[17] {};
    std::uint64_t sourceGeneration = 1;
    char reserved[3] {};
};
#pragma pack(pop)

static_assert(std::is_standard_layout_v<DiskHeader> && std::is_standard_layout_v<DiskRecord>);

bool cancelled(const std::function<bool()>& probe)
{
    return probe && probe();
}

std::string boundedString(const char* value, const std::size_t capacity)
{
    const auto* end = static_cast<const char*>(std::memchr(value, '\0', capacity));
    return end == nullptr ? std::string {} : std::string(value, end);
}

bool checkedAdd(const std::uint64_t left, const std::uint64_t right,
                std::uint64_t& output) noexcept
{
    if (left > std::numeric_limits<std::uint64_t>::max() - right)
        return false;
    output = left + right;
    return true;
}

std::string checksum(const std::vector<std::uint8_t>& bytes)
{
    auto hash = fnvOffset;
    for (const auto byte : bytes)
    {
        hash ^= byte;
        hash *= fnvPrime;
    }
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

std::string aad(const std::string& packageId,
                const PackageV2RecordDescriptor& record)
{
    return "format=2|package=" + packageId
        + "|source=" + record.identity.sourceId
        + "|kind=" + std::to_string(static_cast<std::uint32_t>(record.identity.kind))
        + "|page=" + std::to_string(record.identity.pageIndex)
        + "|generation=" + std::to_string(record.identity.sourceGeneration)
        + "|bytes=" + std::to_string(record.plaintextSizeBytes)
        + "|checksum=" + record.plaintextChecksumHex;
}

std::string recordId(const PackageV2RecordIdentity& identity)
{
    return identity.sourceId + ":" + std::to_string(static_cast<std::uint32_t>(identity.kind))
        + ":" + std::to_string(identity.pageIndex);
}

std::string identityKey(const PackageV2RecordIdentity& identity)
{
    return recordId(identity) + ":" + std::to_string(identity.sourceGeneration);
}

std::uint64_t serializedSealedSize(const PackageCryptoProvider& crypto,
                                   const std::uint64_t plaintextBytes)
{
    return 16ull + crypto.nonceSizeBytes() + plaintextBytes + crypto.tagSizeBytes();
}

std::vector<std::uint8_t> serialize(const PackageSealedBlob& sealed)
{
    const auto total = 16ull + sealed.nonce.size() + sealed.ciphertext.size() + sealed.tag.size();
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(total));
    const auto nonceSize = static_cast<std::uint32_t>(sealed.nonce.size());
    const auto ciphertextSize = static_cast<std::uint64_t>(sealed.ciphertext.size());
    const auto tagSize = static_cast<std::uint32_t>(sealed.tag.size());
    std::memcpy(bytes.data(), &nonceSize, sizeof(nonceSize));
    std::memcpy(bytes.data() + 4, &ciphertextSize, sizeof(ciphertextSize));
    std::memcpy(bytes.data() + 12, &tagSize, sizeof(tagSize));
    auto offset = std::size_t {16};
    std::copy(sealed.nonce.begin(), sealed.nonce.end(), bytes.begin() + offset);
    offset += sealed.nonce.size();
    std::copy(sealed.ciphertext.begin(), sealed.ciphertext.end(), bytes.begin() + offset);
    offset += sealed.ciphertext.size();
    std::copy(sealed.tag.begin(), sealed.tag.end(), bytes.begin() + offset);
    return bytes;
}

bool parseSealed(const std::vector<std::uint8_t>& bytes,
                 PackageSealedBlob& sealed)
{
    if (bytes.size() < 16)
        return false;
    std::uint32_t nonceSize = 0;
    std::uint64_t ciphertextSize = 0;
    std::uint32_t tagSize = 0;
    std::memcpy(&nonceSize, bytes.data(), sizeof(nonceSize));
    std::memcpy(&ciphertextSize, bytes.data() + 4, sizeof(ciphertextSize));
    std::memcpy(&tagSize, bytes.data() + 12, sizeof(tagSize));
    std::uint64_t total = 16;
    return checkedAdd(total, nonceSize, total)
        && checkedAdd(total, ciphertextSize, total)
        && checkedAdd(total, tagSize, total)
        && total == bytes.size()
        && ([&]
        {
            auto offset = std::size_t {16};
            sealed.nonce.assign(bytes.begin() + offset, bytes.begin() + offset + nonceSize);
            offset += nonceSize;
            sealed.ciphertext.assign(bytes.begin() + offset,
                                     bytes.begin() + offset + static_cast<std::size_t>(ciphertextSize));
            offset += static_cast<std::size_t>(ciphertextSize);
            sealed.tag.assign(bytes.begin() + offset, bytes.end());
            return true;
        })();
}

void fail(PackageV2WriteResult& result, const PackageV2Failure failure,
          const std::string& issue)
{
    result.failure = failure;
    result.state = "Package v2 write failed";
    result.issues.push_back(issue);
}

void fail(PackageV2OpenResult& result, const PackageV2Failure failure,
          const std::string& issue)
{
    result.failure = failure;
    result.state = "Package v2 open failed";
    result.issues.push_back(issue);
}

void fail(PackageV2RecordOpenResult& result, const PackageV2Failure failure,
          const std::string& issue)
{
    result.failure = failure;
    result.state = "Package v2 record open failed";
    result.issues.push_back(issue);
}
} // namespace

const char* toString(const PackageV2RecordKind kind) noexcept
{
    switch (kind)
    {
        case PackageV2RecordKind::manifest: return "manifest";
        case PackageV2RecordKind::runtimeInstrument: return "runtime-instrument";
        case PackageV2RecordKind::streamIndex: return "stream-index";
        case PackageV2RecordKind::sampleHead: return "sample-head";
        case PackageV2RecordKind::samplePage: return "sample-page";
        case PackageV2RecordKind::backgroundImage: return "background-image";
        case PackageV2RecordKind::licenseText: return "license-text";
    }
    return "unknown";
}

const char* toString(const PackageV2Failure failure) noexcept
{
    switch (failure)
    {
        case PackageV2Failure::none: return "none";
        case PackageV2Failure::missing: return "missing";
        case PackageV2Failure::format: return "format";
        case PackageV2Failure::unsupportedVersion: return "unsupported-version";
        case PackageV2Failure::bounds: return "bounds";
        case PackageV2Failure::duplicateRecord: return "duplicate-record";
        case PackageV2Failure::recordTooLarge: return "record-too-large";
        case PackageV2Failure::authentication: return "authentication";
        case PackageV2Failure::checksum: return "checksum";
        case PackageV2Failure::cancelled: return "cancelled";
        case PackageV2Failure::io: return "io";
    }
    return "unknown";
}

PackageV2WriteResult writePackageV2(const PackageV2WritePlan& plan,
                                    const PackageCryptoProvider& crypto,
                                    const std::function<bool()>& cancellationProbe)
{
    PackageV2WriteResult result;
    result.state = "Package v2 write not attempted";
    if (plan.packageId.empty() || plan.packageId.size() >= sizeof(DiskHeader {}.packageId)
        || plan.outputPath.empty() || plan.records.empty())
    {
        fail(result, PackageV2Failure::format, "Package id, output path, and records are required.");
        return result;
    }
    std::unordered_set<std::string> identities;
    identities.reserve(plan.records.size());
    for (std::size_t index = 0; index < plan.records.size(); ++index)
    {
        const auto& record = plan.records[index];
        if (record.identity.sourceId.empty()
            || record.identity.sourceId.size() >= sizeof(DiskRecord {}.sourceId))
        {
            fail(result, PackageV2Failure::format, "Record source identity is empty or too long.");
            return result;
        }
        if (record.plaintextBytes.size() > performancePackageV2MaximumRecordBytes)
        {
            fail(result, PackageV2Failure::recordTooLarge,
                 "Record exceeds the 64 KiB bounded plaintext policy.");
            return result;
        }
        if (!identities.insert(identityKey(record.identity)).second)
        {
            fail(result, PackageV2Failure::duplicateRecord,
                 "Package record identities must be unique.");
            return result;
        }
    }
    if (cancelled(cancellationProbe))
    {
        fail(result, PackageV2Failure::cancelled, "Package write was cancelled.");
        return result;
    }

    DiskHeader header;
    std::copy(magic.begin(), magic.end(), header.magicBytes);
    std::copy(plan.packageId.begin(), plan.packageId.end(), header.packageId);
    header.recordCount = static_cast<std::uint32_t>(plan.records.size());
    header.tocSizeBytes = plan.records.size() * sizeof(DiskRecord);
    header.recordRegionOffsetBytes = sizeof(DiskHeader) + header.tocSizeBytes;

    std::vector<DiskRecord> toc(plan.records.size());
    std::uint64_t nextOffset = header.recordRegionOffsetBytes;
    for (std::size_t index = 0; index < plan.records.size(); ++index)
    {
        const auto& source = plan.records[index];
        auto& entry = toc[index];
        std::copy(source.identity.sourceId.begin(), source.identity.sourceId.end(), entry.sourceId);
        entry.kind = static_cast<std::uint32_t>(source.identity.kind);
        entry.pageIndex = source.identity.pageIndex;
        entry.sourceGeneration = source.identity.sourceGeneration;
        entry.sealedOffsetBytes = nextOffset;
        entry.plaintextSizeBytes = source.plaintextBytes.size();
        entry.sealedSizeBytes = serializedSealedSize(crypto, entry.plaintextSizeBytes);
        const auto digest = checksum(source.plaintextBytes);
        std::copy(digest.begin(), digest.end(), entry.checksumHex);
        if (!checkedAdd(nextOffset, entry.sealedSizeBytes, nextOffset))
        {
            fail(result, PackageV2Failure::bounds, "Package record offsets overflow 64-bit range.");
            return result;
        }
    }
    header.recordRegionSizeBytes = nextOffset - header.recordRegionOffsetBytes;

    std::error_code error;
    fs::create_directories(fs::path(plan.outputPath).parent_path(), error);
    std::ofstream output(fs::path(plan.outputPath), std::ios::binary | std::ios::trunc);
    if (!output)
    {
        fail(result, PackageV2Failure::io, "Could not open package v2 output.");
        return result;
    }
    output.write(reinterpret_cast<const char*>(&header), sizeof(header));
    output.write(reinterpret_cast<const char*>(toc.data()),
                 static_cast<std::streamsize>(toc.size() * sizeof(DiskRecord)));

    for (std::size_t index = 0; index < plan.records.size(); ++index)
    {
        if (cancelled(cancellationProbe))
        {
            output.close();
            fs::remove(fs::path(plan.outputPath), error);
            fail(result, PackageV2Failure::cancelled, "Package write was cancelled between records.");
            return result;
        }
        PackageV2RecordDescriptor descriptor;
        descriptor.identity = plan.records[index].identity;
        descriptor.plaintextSizeBytes = toc[index].plaintextSizeBytes;
        descriptor.plaintextChecksumHex = boundedString(toc[index].checksumHex,
                                                        sizeof(toc[index].checksumHex));
        PackageSealRequest request;
        request.packageId = plan.packageId;
        request.recordId = recordId(descriptor.identity);
        request.additionalAuthenticatedData = aad(plan.packageId, descriptor);
        request.plaintext = plan.records[index].plaintextBytes;
        PackageSealedBlob sealed;
        std::string issue;
        if (!crypto.seal(request, sealed, issue))
        {
            output.close();
            fs::remove(fs::path(plan.outputPath), error);
            fail(result, PackageV2Failure::authentication, "Could not seal record: " + issue);
            return result;
        }
        const auto bytes = serialize(sealed);
        if (bytes.size() != toc[index].sealedSizeBytes)
        {
            output.close();
            fs::remove(fs::path(plan.outputPath), error);
            fail(result, PackageV2Failure::format,
                 "Crypto provider violated bounded sealed-size prediction.");
            return result;
        }
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        if (!output)
        {
            fail(result, PackageV2Failure::io, "Could not write a package v2 record.");
            return result;
        }
    }
    output.close();
    result.packageBytes = nextOffset;
    result.written = true;
    result.state = "Package v2 written";
    return result;
}

PackageV2StreamingWriteResult writePackageV2Streaming(
    const PackageV2StreamingWritePlan& plan,
    const PackageCryptoProvider& crypto,
    const PackageV2StreamingWriteOptions& options)
{
    PackageV2StreamingWriteResult result;
    const auto exportStarted = std::chrono::steady_clock::now();
    const auto elapsedMicros = [](const auto started)
    {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started).count());
    };
    result.state = "Package v2 streaming write not attempted";
    result.stagingPath = plan.outputPath + ".stage";
    std::uint64_t progressTotalPlaintextBytes = 0;
    const auto publish = [&](const PackageV2StreamingWriteStage stage,
                             const std::size_t index,
                             const std::uint64_t completed,
                             const PackageV2RecordIdentity& identity,
                             const std::string& status)
    {
        if (options.progressSink)
            options.progressSink({ stage, index, plan.records.size(), completed,
                                   progressTotalPlaintextBytes, identity, status });
    };
    if (plan.packageId.empty() || plan.packageId.size() >= sizeof(DiskHeader {}.packageId)
        || plan.outputPath.empty() || plan.records.empty())
    {
        fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::format,
             "Streaming package id, output path, and records are required.");
        return result;
    }
    std::unordered_set<std::string> identities;
    identities.reserve(plan.records.size());
    for (std::size_t index = 0; index < plan.records.size(); ++index)
    {
        const auto& record = plan.records[index];
        if (record.identity.sourceId.empty()
            || record.identity.sourceId.size() >= sizeof(DiskRecord {}.sourceId)
            || record.expectedPlaintextBytes > performancePackageV2MaximumRecordBytes
            || !record.loadPlaintext)
        {
            fail(static_cast<PackageV2WriteResult&>(result),
                 record.expectedPlaintextBytes > performancePackageV2MaximumRecordBytes
                    ? PackageV2Failure::recordTooLarge : PackageV2Failure::format,
                 "Streaming record source is invalid or exceeds 64 KiB.");
            return result;
        }
        if (!identities.insert(identityKey(record.identity)).second)
        {
            fail(static_cast<PackageV2WriteResult&>(result),
                 PackageV2Failure::duplicateRecord,
                 "Streaming record identities must be unique.");
            return result;
        }
        if (!checkedAdd(result.processedPlaintextBytes, record.expectedPlaintextBytes,
                        result.processedPlaintextBytes))
        {
            fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::bounds,
                 "Streaming plaintext progress overflows 64-bit range.");
            return result;
        }
    }
    const auto totalPlaintextBytes = result.processedPlaintextBytes;
    progressTotalPlaintextBytes = totalPlaintextBytes;
    result.processedPlaintextBytes = 0;
    if (cancelled(options.cancellationProbe))
    {
        fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::cancelled,
             "Streaming package write was cancelled before staging.");
        result.cancellationResponseMicros = elapsedMicros(exportStarted);
        return result;
    }

    namespace fs = std::filesystem;
    std::error_code error;
    fs::create_directories(fs::path(plan.outputPath).parent_path(), error);
    fs::remove(fs::path(result.stagingPath), error);
    DiskHeader header;
    std::copy(magic.begin(), magic.end(), header.magicBytes);
    std::copy(plan.packageId.begin(), plan.packageId.end(), header.packageId);
    header.recordCount = static_cast<std::uint32_t>(plan.records.size());
    header.tocSizeBytes = plan.records.size() * sizeof(DiskRecord);
    header.recordRegionOffsetBytes = sizeof(DiskHeader) + header.tocSizeBytes;
    std::vector<DiskRecord> toc(plan.records.size());

    std::ofstream output(fs::path(result.stagingPath), std::ios::binary | std::ios::trunc);
    if (!output)
    {
        fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::io,
             "Could not create package v2 staging file.");
        return result;
    }
    output.write(reinterpret_cast<const char*>(&header), sizeof(header));
    std::array<char, 64 * 1024> tocZeros {};
    std::uint64_t remainingTocBytes = header.tocSizeBytes;
    while (remainingTocBytes != 0)
    {
        if (cancelled(options.cancellationProbe))
        {
            output.close();
            fs::remove(fs::path(result.stagingPath), error);
            fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::cancelled,
                 "Streaming package write was cancelled while staging the TOC.");
            result.cancellationResponseMicros = elapsedMicros(exportStarted);
            return result;
        }
        const auto writeNow = std::min<std::uint64_t>(remainingTocBytes, tocZeros.size());
        output.write(tocZeros.data(), static_cast<std::streamsize>(writeNow));
        remainingTocBytes -= writeNow;
    }
    auto nextOffset = header.recordRegionOffsetBytes;

    for (std::size_t index = 0; index < plan.records.size(); ++index)
    {
        const auto& source = plan.records[index];
        if (cancelled(options.cancellationProbe))
        {
            output.close();
            fs::remove(fs::path(result.stagingPath), error);
            fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::cancelled,
                 "Streaming package write was cancelled between records.");
            result.cancellationResponseMicros = elapsedMicros(exportStarted);
            publish(PackageV2StreamingWriteStage::cancelled, index,
                    result.processedPlaintextBytes, source.identity, result.state);
            return result;
        }
        publish(PackageV2StreamingWriteStage::loadingRecord, index,
                result.processedPlaintextBytes, source.identity, source.sourceLabel);
        std::vector<std::uint8_t> plaintext;
        std::string issue;
        const auto loadStarted = std::chrono::steady_clock::now();
        if (!source.loadPlaintext(plaintext, issue)
            || plaintext.size() != source.expectedPlaintextBytes)
        {
            output.close();
            fs::remove(fs::path(result.stagingPath), error);
            fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::io,
                 "Could not load bounded streaming record: " + issue);
            return result;
        }
        result.loadDurationMicros += elapsedMicros(loadStarted);
        result.peakPlaintextBufferBytes = std::max<std::uint64_t>(
            result.peakPlaintextBufferBytes, plaintext.size());
        auto& entry = toc[index];
        std::copy(source.identity.sourceId.begin(), source.identity.sourceId.end(), entry.sourceId);
        entry.kind = static_cast<std::uint32_t>(source.identity.kind);
        entry.pageIndex = source.identity.pageIndex;
        entry.sourceGeneration = source.identity.sourceGeneration;
        entry.sealedOffsetBytes = nextOffset;
        entry.plaintextSizeBytes = plaintext.size();
        const auto digest = checksum(plaintext);
        std::copy(digest.begin(), digest.end(), entry.checksumHex);
        PackageV2RecordDescriptor descriptor;
        descriptor.identity = source.identity;
        descriptor.plaintextSizeBytes = plaintext.size();
        descriptor.plaintextChecksumHex = digest;
        PackageSealRequest request;
        request.packageId = plan.packageId;
        request.recordId = recordId(source.identity);
        request.additionalAuthenticatedData = aad(plan.packageId, descriptor);
        request.plaintext = std::move(plaintext);
        PackageSealedBlob sealed;
        publish(PackageV2StreamingWriteStage::sealingRecord, index,
                result.processedPlaintextBytes, source.identity, source.sourceLabel);
        const auto sealStarted = std::chrono::steady_clock::now();
        if (!crypto.seal(request, sealed, issue))
        {
            output.close();
            fs::remove(fs::path(result.stagingPath), error);
            fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::authentication,
                 "Could not seal bounded streaming record: " + issue);
            return result;
        }
        result.sealDurationMicros += elapsedMicros(sealStarted);
        request.plaintext.clear();
        request.plaintext.shrink_to_fit();
        auto sealedBytes = serialize(sealed);
        result.peakSealedBufferBytes = std::max<std::uint64_t>(
            result.peakSealedBufferBytes, sealedBytes.size());
        entry.sealedSizeBytes = sealedBytes.size();
        if (!checkedAdd(nextOffset, entry.sealedSizeBytes, nextOffset))
        {
            output.close();
            fs::remove(fs::path(result.stagingPath), error);
            fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::bounds,
                 "Streaming record offsets overflow 64-bit range.");
            return result;
        }
        publish(PackageV2StreamingWriteStage::writingRecord, index,
                result.processedPlaintextBytes, source.identity, source.sourceLabel);
        const auto writeStarted = std::chrono::steady_clock::now();
        output.write(reinterpret_cast<const char*>(sealedBytes.data()),
                     static_cast<std::streamsize>(sealedBytes.size()));
        if (!output)
        {
            output.close();
            fs::remove(fs::path(result.stagingPath), error);
            fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::io,
                 "Could not append a bounded streaming record.");
            return result;
        }
        result.writeDurationMicros += elapsedMicros(writeStarted);
        result.processedPlaintextBytes += source.expectedPlaintextBytes;
        ++result.completedRecordCount;
    }

    header.recordRegionSizeBytes = nextOffset - header.recordRegionOffsetBytes;
    publish(PackageV2StreamingWriteStage::finalizingToc, plan.records.size(),
            result.processedPlaintextBytes, {}, "Finalizing header and TOC");
    output.seekp(0);
    output.write(reinterpret_cast<const char*>(&header), sizeof(header));
    output.write(reinterpret_cast<const char*>(toc.data()),
                 static_cast<std::streamsize>(toc.size() * sizeof(DiskRecord)));
    output.flush();
    output.close();
    if (!output)
    {
        fs::remove(fs::path(result.stagingPath), error);
        fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::io,
             "Could not finalize the package v2 staging file.");
        return result;
    }

    publish(PackageV2StreamingWriteStage::verifying, plan.records.size(),
            result.processedPlaintextBytes, {}, "Verifying staged package");
    const auto verificationStarted = std::chrono::steady_clock::now();
    const auto staged = openPackageV2(result.stagingPath);
    if (!staged.opened || staged.records.size() != plan.records.size())
    {
        fs::remove(fs::path(result.stagingPath), error);
        fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::format,
             "Staged package v2 failed header/TOC verification.");
        return result;
    }
    result.verificationBytesRead = sizeof(DiskHeader) + staged.tocBytes;
    const std::array<std::size_t, 2> sampleIndices { 0, plan.records.size() - 1 };
    for (std::size_t sample = 0; sample < sampleIndices.size(); ++sample)
    {
        if (sample == 1 && sampleIndices[1] == sampleIndices[0])
            continue;
        const auto verification = openPackageV2Record(
            staged, plan.records[sampleIndices[sample]].identity, crypto,
            options.cancellationProbe);
        result.verificationBytesRead += verification.metrics.bytesRead;
        if (!verification.opened)
        {
            fs::remove(fs::path(result.stagingPath), error);
            fail(static_cast<PackageV2WriteResult&>(result), verification.failure,
                 "Staged package v2 record verification failed.");
            return result;
        }
    }
    result.verified = true;
    result.verificationDurationMicros = elapsedMicros(verificationStarted);
    publish(PackageV2StreamingWriteStage::publishing, plan.records.size(),
            result.processedPlaintextBytes, {}, "Publishing staged package atomically");
#if defined(_WIN32)
    const auto moved = MoveFileExW(fs::path(result.stagingPath).c_str(),
                                   fs::path(plan.outputPath).c_str(),
                                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    fs::rename(fs::path(result.stagingPath), fs::path(plan.outputPath), error);
    const auto moved = !error;
#endif
    if (!moved)
    {
        fail(static_cast<PackageV2WriteResult&>(result), PackageV2Failure::io,
             "Verified package v2 staging file could not be atomically published.");
        return result;
    }
    result.atomicallyPublished = true;
    result.packageBytes = nextOffset;
    result.written = true;
    result.state = "Package v2 streaming export completed";
    result.totalDurationMicros = elapsedMicros(exportStarted);
    if (result.totalDurationMicros != 0)
        result.plaintextThroughputBytesPerSecond
            = static_cast<double>(result.processedPlaintextBytes) * 1000000.0
            / static_cast<double>(result.totalDurationMicros);
    publish(PackageV2StreamingWriteStage::completed, plan.records.size(),
            result.processedPlaintextBytes, {}, result.state);
    return result;
}

PackageV2OpenResult openPackageV2(const std::string& packagePath)
{
    PackageV2OpenResult result;
    result.packagePath = packagePath;
    result.state = "Package v2 open not attempted";
    std::error_code error;
    result.packageBytes = fs::file_size(fs::path(packagePath), error);
    if (error)
    {
        fail(result, PackageV2Failure::missing, "Package v2 file is missing or unreadable.");
        return result;
    }
    std::ifstream input(fs::path(packagePath), std::ios::binary);
    DiskHeader header;
    if (!input.read(reinterpret_cast<char*>(&header), sizeof(header)))
    {
        fail(result, PackageV2Failure::format, "Package v2 header is truncated.");
        return result;
    }
    if (!std::equal(magic.begin(), magic.end(), header.magicBytes))
    {
        fail(result, PackageV2Failure::format, "Package v2 magic is invalid.");
        return result;
    }
    if (header.formatVersion != performancePackageV2FormatVersion)
    {
        fail(result, PackageV2Failure::unsupportedVersion, "Package v2 version is unsupported.");
        return result;
    }
    result.packageId = boundedString(header.packageId, sizeof(header.packageId));
    result.tocBytes = header.tocSizeBytes;
    std::uint64_t tocEnd = 0;
    std::uint64_t recordEnd = 0;
    if (result.packageId.empty() || header.headerBytes != sizeof(DiskHeader)
        || header.tocOffsetBytes != sizeof(DiskHeader)
        || header.tocSizeBytes != static_cast<std::uint64_t>(header.recordCount) * sizeof(DiskRecord)
        || !checkedAdd(header.tocOffsetBytes, header.tocSizeBytes, tocEnd)
        || tocEnd != header.recordRegionOffsetBytes
        || !checkedAdd(header.recordRegionOffsetBytes, header.recordRegionSizeBytes, recordEnd)
        || recordEnd != result.packageBytes)
    {
        fail(result, PackageV2Failure::bounds, "Package v2 header ranges are inconsistent.");
        return result;
    }
    std::vector<DiskRecord> toc(header.recordCount);
    if (!input.read(reinterpret_cast<char*>(toc.data()),
                    static_cast<std::streamsize>(header.tocSizeBytes)))
    {
        fail(result, PackageV2Failure::format, "Package v2 TOC is truncated.");
        return result;
    }
    result.records.reserve(toc.size());
    result.recordIndexByIdentity.reserve(toc.size());
    std::uint64_t expectedOffset = header.recordRegionOffsetBytes;
    for (const auto& entry : toc)
    {
        PackageV2RecordDescriptor record;
        record.identity.sourceId = boundedString(entry.sourceId, sizeof(entry.sourceId));
        record.identity.kind = static_cast<PackageV2RecordKind>(entry.kind);
        record.identity.pageIndex = entry.pageIndex;
        record.identity.sourceGeneration = entry.sourceGeneration;
        record.sealedOffsetBytes = entry.sealedOffsetBytes;
        record.sealedSizeBytes = entry.sealedSizeBytes;
        record.plaintextSizeBytes = entry.plaintextSizeBytes;
        record.plaintextChecksumHex = boundedString(entry.checksumHex, sizeof(entry.checksumHex));
        std::uint64_t rangeEnd = 0;
        if (record.identity.sourceId.empty() || record.plaintextChecksumHex.size() != 16
            || record.plaintextSizeBytes > performancePackageV2MaximumRecordBytes
            || record.sealedOffsetBytes < expectedOffset
            || !checkedAdd(record.sealedOffsetBytes, record.sealedSizeBytes, rangeEnd)
            || rangeEnd > result.packageBytes)
        {
            fail(result, record.plaintextSizeBytes > performancePackageV2MaximumRecordBytes
                    ? PackageV2Failure::recordTooLarge : PackageV2Failure::bounds,
                 "Package v2 record range or bounded size is invalid.");
            return result;
        }
        const auto key = identityKey(record.identity);
        if (!result.recordIndexByIdentity.emplace(key, result.records.size()).second)
        {
            fail(result, PackageV2Failure::duplicateRecord,
                 "Package v2 TOC contains a duplicate record identity.");
            return result;
        }
        expectedOffset = rangeEnd;
        result.records.push_back(std::move(record));
    }
    if (expectedOffset > result.packageBytes)
    {
        fail(result, PackageV2Failure::bounds, "Package v2 records do not cover their declared region.");
        return result;
    }
    result.opened = true;
    result.state = "Package v2 metadata and TOC opened";
    return result;
}

PackageV2RecordOpenResult openPackageV2Record(
    const PackageV2OpenResult& package,
    const PackageV2RecordIdentity& identity,
    const PackageCryptoProvider& crypto,
    const std::function<bool()>& cancellationProbe)
{
    PackageV2RecordOpenResult result;
    result.state = "Package v2 record open not attempted";
    if (!package.opened)
    {
        fail(result, PackageV2Failure::format, "Package v2 TOC is not open.");
        return result;
    }
    const auto indexed = package.recordIndexByIdentity.find(identityKey(identity));
    if (indexed == package.recordIndexByIdentity.end()
        || indexed->second >= package.records.size())
    {
        fail(result, PackageV2Failure::missing, "Requested package v2 record is absent.");
        return result;
    }
    const auto& descriptor = package.records[indexed->second];
    result.descriptor = descriptor;
    if (cancelled(cancellationProbe))
    {
        result.metrics.cancellationCount = 1;
        fail(result, PackageV2Failure::cancelled, "Package v2 record open was cancelled.");
        return result;
    }
    std::ifstream input(fs::path(package.packagePath), std::ios::binary);
    input.seekg(static_cast<std::streamoff>(descriptor.sealedOffsetBytes));
    std::vector<std::uint8_t> sealedBytes(static_cast<std::size_t>(descriptor.sealedSizeBytes));
    if (!input.read(reinterpret_cast<char*>(sealedBytes.data()),
                    static_cast<std::streamsize>(sealedBytes.size())))
    {
        fail(result, PackageV2Failure::io, "Could not read the requested package v2 record range.");
        return result;
    }
    result.metrics.bytesRead = sealedBytes.size();
    if (cancelled(cancellationProbe))
    {
        result.metrics.cancellationCount = 1;
        fail(result, PackageV2Failure::cancelled, "Package v2 record open was cancelled after I/O.");
        return result;
    }
    PackageSealedBlob sealed;
    if (!parseSealed(sealedBytes, sealed))
    {
        fail(result, PackageV2Failure::format, "Package v2 sealed record framing is invalid.");
        return result;
    }
    PackageOpenRequest request;
    request.packageId = package.packageId;
    request.recordId = recordId(descriptor.identity);
    request.additionalAuthenticatedData = aad(package.packageId, descriptor);
    request.sealed = std::move(sealed);
    std::string issue;
    if (!crypto.open(request, result.plaintextBytes, issue))
    {
        result.metrics.authenticationFailures = 1;
        fail(result, PackageV2Failure::authentication,
             "Package v2 record authentication failed: " + issue);
        return result;
    }
    if (result.plaintextBytes.size() != descriptor.plaintextSizeBytes
        || checksum(result.plaintextBytes) != descriptor.plaintextChecksumHex)
    {
        result.metrics.checksumFailures = 1;
        result.plaintextBytes.clear();
        fail(result, PackageV2Failure::checksum, "Package v2 record checksum failed.");
        return result;
    }
    result.metrics.recordsOpened = 1;
    result.metrics.largestPlaintextRecordBytes = result.plaintextBytes.size();
    result.opened = true;
    result.state = "Package v2 record opened";
    return result;
}
} // namespace drs::engine
