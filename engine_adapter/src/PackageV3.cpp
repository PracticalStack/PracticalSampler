#include <drs/engine/PackageV3.h>
#include <drs/engine/PackageV3FileReader.h>
#include <drs/engine/PackageV3StreamingExport.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <tuple>

#include <sodium/crypto_hash_sha256.h>

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
constexpr std::array<std::uint8_t, 8> kMagic { 'D', 'R', 'S', 'P', 'K', 'G', '3', 0 };
constexpr std::array<std::uint8_t, 8> kAadMagic { 'D', 'R', 'S', 'A', 'A', 'D', '3', 0 };
constexpr std::array<std::uint8_t, 8> kSemanticMagic { 'D', 'R', 'S', 'S', 'E', 'M', '3', 0 };
constexpr std::uint32_t kRequiredFlags = 0x00000007u; // encrypted, authenticated, signed
constexpr std::uint32_t kSignatureSize = 64u;

void appendU16(std::vector<std::uint8_t>& out, const std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void appendU32(std::vector<std::uint8_t>& out, const std::uint32_t value)
{
    for (int i = 0; i < 4; ++i)
        out.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
}

void appendU64(std::vector<std::uint8_t>& out, const std::uint64_t value)
{
    for (int i = 0; i < 8; ++i)
        out.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
}

bool appendString(std::vector<std::uint8_t>& out, const std::string& value)
{
    if (value.empty() || value.size() > packageV3MaximumIdentityBytes
        || value.size() > std::numeric_limits<std::uint16_t>::max())
        return false;
    appendU16(out, static_cast<std::uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
    return true;
}

bool checkedAdd(const std::uint64_t left,
                const std::uint64_t right,
                std::uint64_t& result) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        return false;
    result = left + right;
    return true;
}

bool recordLess(const PackageV3RecordInput& left, const PackageV3RecordInput& right)
{
    return std::tie(left.recordKind, left.recordId, left.generation, left.pageIndex)
        < std::tie(right.recordKind, right.recordId, right.generation, right.pageIndex);
}

bool sameRecordIdentity(const PackageV3RecordInput& left, const PackageV3RecordInput& right)
{
    return left.recordKind == right.recordKind && left.recordId == right.recordId
        && left.generation == right.generation && left.pageIndex == right.pageIndex;
}

std::string asBinaryString(const std::vector<std::uint8_t>& bytes)
{
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

struct SealedRecord
{
    PackageV3RecordInput input;
    PackageSealedBlob sealed;
    std::uint32_t ordinal = 0;
    std::uint64_t plaintextSize = 0;
    std::uint64_t ciphertextSize = 0;
    std::uint64_t ciphertextOffset = 0;
};

std::vector<std::uint8_t> serializeHeader(
    const PackageV3WriteRequest& request,
    const PackageKeyEnvelope& envelope,
    const std::uint32_t headerSize,
    const std::uint32_t recordCount,
    const std::uint64_t tocOffset,
    const std::uint64_t tocSize,
    const std::uint64_t payloadOffset,
    const std::uint64_t payloadSize,
    const std::uint64_t signatureOffset)
{
    std::vector<std::uint8_t> bytes;
    bytes.insert(bytes.end(), kMagic.begin(), kMagic.end());
    appendU32(bytes, packageV3FormatVersion);
    appendU32(bytes, headerSize);
    appendU32(bytes, kRequiredFlags);
    appendU16(bytes, packageV3CryptoSuiteXChaCha20Poly1305);
    appendU16(bytes, packageV3SignatureSuiteEd25519ph);
    appendU32(bytes, recordCount);
    appendU64(bytes, tocOffset);
    appendU64(bytes, tocSize);
    appendU64(bytes, payloadOffset);
    appendU64(bytes, payloadSize);
    appendU64(bytes, signatureOffset);
    appendU32(bytes, kSignatureSize);
    appendU32(bytes, 0u);
    if (! appendString(bytes, request.packageId)
        || ! appendString(bytes, request.compatibilityId)
        || ! appendString(bytes, request.encryptionKeyId)
        || ! appendString(bytes, request.signingKeyId))
        return {};
    bytes.insert(bytes.end(), envelope.sealedContentKey.nonce.begin(),
                 envelope.sealedContentKey.nonce.end());
    appendU16(bytes, static_cast<std::uint16_t>(envelope.sealedContentKey.ciphertext.size()));
    bytes.insert(bytes.end(), envelope.sealedContentKey.ciphertext.begin(),
                 envelope.sealedContentKey.ciphertext.end());
    bytes.insert(bytes.end(), envelope.sealedContentKey.tag.begin(),
                 envelope.sealedContentKey.tag.end());
    return bytes;
}

std::vector<std::uint8_t> serializeToc(const std::vector<SealedRecord>& records)
{
    std::vector<std::uint8_t> bytes;
    for (const auto& record : records)
    {
        appendU32(bytes, record.ordinal);
        if (! appendString(bytes, record.input.recordId)
            || ! appendString(bytes, record.input.recordKind))
            return {};
        appendU32(bytes, record.input.generation);
        appendU32(bytes, record.input.pageIndex);
        appendU64(bytes, record.plaintextSize);
        appendU64(bytes, record.ciphertextOffset);
        appendU64(bytes, record.ciphertextSize);
        bytes.insert(bytes.end(), record.sealed.nonce.begin(), record.sealed.nonce.end());
        bytes.insert(bytes.end(), record.sealed.tag.begin(), record.sealed.tag.end());
    }
    return bytes;
}

bool buildSemanticDigest(const PackageV3WriteRequest& request,
                         const std::vector<PackageV3RecordInput>& records,
                         std::vector<std::uint8_t>& digest)
{
    crypto_hash_sha256_state state;
    if (crypto_hash_sha256_init(&state) != 0)
        return false;
    crypto_hash_sha256_update(&state, kSemanticMagic.data(), kSemanticMagic.size());
    std::vector<std::uint8_t> metadata;
    if (! appendString(metadata, request.packageId)
        || ! appendString(metadata, request.compatibilityId))
        return false;
    appendU32(metadata, static_cast<std::uint32_t>(records.size()));
    crypto_hash_sha256_update(&state, metadata.data(), metadata.size());
    for (std::size_t index = 0; index < records.size(); ++index)
    {
        metadata.clear();
        appendU32(metadata, static_cast<std::uint32_t>(index));
        if (! appendString(metadata, records[index].recordId)
            || ! appendString(metadata, records[index].recordKind))
            return false;
        appendU32(metadata, records[index].generation);
        appendU32(metadata, records[index].pageIndex);
        appendU64(metadata, static_cast<std::uint64_t>(records[index].plaintext.size()));
        crypto_hash_sha256_update(&state, metadata.data(), metadata.size());
        if (! records[index].plaintext.empty())
            crypto_hash_sha256_update(&state, records[index].plaintext.data(),
                                      records[index].plaintext.size());
    }
    digest.resize(crypto_hash_sha256_BYTES);
    return crypto_hash_sha256_final(&state, digest.data()) == 0;
}

class Reader
{
public:
    Reader(const std::vector<std::uint8_t>& source,
           const std::size_t begin,
           const std::size_t end) : bytes(source), position(begin), limit(end) {}

    bool readU16(std::uint16_t& value)
    {
        if (remaining() < 2u) return false;
        value = static_cast<std::uint16_t>(bytes[position])
            | static_cast<std::uint16_t>(bytes[position + 1u] << 8u);
        position += 2u;
        return true;
    }

    bool readU32(std::uint32_t& value)
    {
        if (remaining() < 4u) return false;
        value = 0;
        for (int i = 0; i < 4; ++i)
            value |= static_cast<std::uint32_t>(bytes[position++]) << (i * 8);
        return true;
    }

    bool readU64(std::uint64_t& value)
    {
        if (remaining() < 8u) return false;
        value = 0;
        for (int i = 0; i < 8; ++i)
            value |= static_cast<std::uint64_t>(bytes[position++]) << (i * 8);
        return true;
    }

    bool readString(std::string& value)
    {
        std::uint16_t length = 0;
        if (! readU16(length) || length == 0u || length > packageV3MaximumIdentityBytes
            || remaining() < length)
            return false;
        value.assign(reinterpret_cast<const char*>(bytes.data() + position), length);
        position += length;
        return true;
    }

    bool readBytes(const std::size_t size, std::vector<std::uint8_t>& value)
    {
        if (remaining() < size) return false;
        value.assign(bytes.begin() + static_cast<std::ptrdiff_t>(position),
                     bytes.begin() + static_cast<std::ptrdiff_t>(position + size));
        position += size;
        return true;
    }

    std::size_t remaining() const noexcept { return limit - position; }
    std::size_t position = 0;
    std::size_t limit = 0;

private:
    const std::vector<std::uint8_t>& bytes;
};

bool descriptorMatches(const PackageV3RecordDescriptor& left,
                       const PackageV3RecordDescriptor& right)
{
    return left.ordinal == right.ordinal && left.recordId == right.recordId
        && left.recordKind == right.recordKind && left.generation == right.generation
        && left.pageIndex == right.pageIndex && left.plaintextSize == right.plaintextSize
        && left.ciphertextOffset == right.ciphertextOffset
        && left.ciphertextSize == right.ciphertextSize && left.sealed.nonce == right.sealed.nonce
        && left.sealed.ciphertext == right.sealed.ciphertext && left.sealed.tag == right.sealed.tag;
}
} // namespace

std::vector<std::uint8_t> buildPackageV3RecordAad(
    const std::string& packageId,
    const std::string& compatibilityId,
    const std::uint32_t ordinal,
    const std::string& recordId,
    const std::string& recordKind,
    const std::uint32_t generation,
    const std::uint32_t pageIndex,
    const std::uint64_t plaintextSize)
{
    std::vector<std::uint8_t> bytes(kAadMagic.begin(), kAadMagic.end());
    appendU32(bytes, packageV3FormatVersion);
    appendU16(bytes, packageV3CryptoSuiteXChaCha20Poly1305);
    appendU32(bytes, ordinal);
    if (! appendString(bytes, packageId) || ! appendString(bytes, compatibilityId)
        || ! appendString(bytes, recordId) || ! appendString(bytes, recordKind))
        return {};
    appendU32(bytes, generation);
    appendU32(bytes, pageIndex);
    appendU64(bytes, plaintextSize);
    return bytes;
}

PackageV3WriteResult writePackageV3(const PackageV3WriteRequest& request)
{
    PackageV3WriteResult result;
    if (request.packageId.empty() || request.compatibilityId.empty()
        || request.encryptionKeyId.empty() || request.signingKeyId.empty()
        || request.packageId.size() > packageV3MaximumIdentityBytes
        || request.compatibilityId.size() > packageV3MaximumIdentityBytes
        || request.encryptionKeyId.size() > packageV3MaximumIdentityBytes
        || request.signingKeyId.size() > packageV3MaximumIdentityBytes
        || request.releaseKey == nullptr
        || request.releaseKey->size() != securePackageKeySizeBytes
        || request.publisherSigner == nullptr
        || request.records.empty() || request.records.size() > packageV3MaximumRecords)
    {
        result.issues.push_back("V3 package identity, key material, or record count is invalid");
        return result;
    }

    auto records = request.records;
    for (const auto& record : records)
    {
        if (record.recordId.empty() || record.recordKind.empty()
            || record.recordId.size() > packageV3MaximumIdentityBytes
            || record.recordKind.size() > packageV3MaximumIdentityBytes
            || record.plaintext.size() > packageV3MaximumRecordBytes)
        {
            result.issues.push_back("V3 record identity or size is invalid");
            return result;
        }
    }
    std::sort(records.begin(), records.end(), recordLess);
    for (std::size_t index = 1; index < records.size(); ++index)
    {
        if (sameRecordIdentity(records[index - 1u], records[index]))
        {
            result.issues.push_back("V3 record identities must be unique");
            return result;
        }
    }

    std::string issue;
    SecureBuffer contentKey;
    if (! generateSecurePackageKey(contentKey, issue))
    { result.issues.push_back(issue); return result; }

    PackageKeyEnvelope envelope;
    if (! wrapPackageContentKey(request.packageId, request.encryptionKeyId,
                                contentKey, *request.releaseKey, envelope, issue))
    { result.issues.push_back(issue); return result; }

    std::vector<SealedRecord> sealedRecords;
    sealedRecords.reserve(records.size());
    const auto& crypto = getSecurePackageCryptoProvider();
    std::uint64_t payloadSize = 0;
    for (std::size_t index = 0; index < records.size(); ++index)
    {
        const auto aad = buildPackageV3RecordAad(
            request.packageId, request.compatibilityId, static_cast<std::uint32_t>(index),
            records[index].recordId, records[index].recordKind, records[index].generation,
            records[index].pageIndex, static_cast<std::uint64_t>(records[index].plaintext.size()));
        if (aad.empty())
        { result.issues.push_back("V3 record AAD could not be encoded"); return result; }
        PackageSealRequest seal;
        seal.packageId = request.packageId;
        seal.recordId = records[index].recordId;
        seal.encryptionKeyId = request.encryptionKeyId;
        seal.secureEncryptionKey = &contentKey;
        seal.additionalAuthenticatedData = asBinaryString(aad);
        seal.plaintext = records[index].plaintext;
        SealedRecord sealed;
        sealed.input = std::move(records[index]);
        sealed.ordinal = static_cast<std::uint32_t>(index);
        sealed.plaintextSize = sealed.input.plaintext.size();
        if (! crypto.seal(seal, sealed.sealed, issue))
        { result.issues.push_back(issue); return result; }
        sealed.ciphertextSize = sealed.sealed.ciphertext.size();
        if (! checkedAdd(payloadSize, sealed.sealed.ciphertext.size(), payloadSize))
        { result.issues.push_back("V3 payload size overflow"); return result; }
        sealedRecords.push_back(std::move(sealed));
    }

    auto provisionalHeader = serializeHeader(
        request, envelope, 0u, static_cast<std::uint32_t>(sealedRecords.size()),
        0u, 0u, 0u, payloadSize, 0u);
    if (provisionalHeader.empty() || provisionalHeader.size() > std::numeric_limits<std::uint32_t>::max())
    { result.issues.push_back("V3 header could not be encoded"); return result; }
    const auto headerSize = static_cast<std::uint32_t>(provisionalHeader.size());

    auto provisionalToc = serializeToc(sealedRecords);
    if (provisionalToc.empty())
    { result.issues.push_back("V3 TOC could not be encoded"); return result; }
    const std::uint64_t tocOffset = headerSize;
    const std::uint64_t tocSize = provisionalToc.size();
    std::uint64_t payloadOffset = 0;
    std::uint64_t signatureOffset = 0;
    std::uint64_t totalSize = 0;
    if (! checkedAdd(tocOffset, tocSize, payloadOffset)
        || ! checkedAdd(payloadOffset, payloadSize, signatureOffset)
        || ! checkedAdd(signatureOffset, kSignatureSize, totalSize)
        || totalSize > packageV3MaximumPackageBytes
        || totalSize > std::numeric_limits<std::size_t>::max())
    { result.issues.push_back("V3 package size exceeds its bounded format limit"); return result; }

    std::uint64_t ciphertextOffset = payloadOffset;
    for (auto& record : sealedRecords)
    {
        record.ciphertextOffset = ciphertextOffset;
        if (! checkedAdd(ciphertextOffset, record.sealed.ciphertext.size(), ciphertextOffset))
        { result.issues.push_back("V3 ciphertext offset overflow"); return result; }
    }
    if (ciphertextOffset != signatureOffset)
    { result.issues.push_back("V3 payload layout is inconsistent"); return result; }

    auto header = serializeHeader(
        request, envelope, headerSize, static_cast<std::uint32_t>(sealedRecords.size()),
        tocOffset, tocSize, payloadOffset, payloadSize, signatureOffset);
    auto toc = serializeToc(sealedRecords);
    if (header.size() != headerSize || toc.size() != tocSize)
    { result.issues.push_back("V3 canonical layout changed during serialization"); return result; }

    result.packageBytes.reserve(static_cast<std::size_t>(totalSize));
    result.packageBytes.insert(result.packageBytes.end(), header.begin(), header.end());
    result.packageBytes.insert(result.packageBytes.end(), toc.begin(), toc.end());
    for (const auto& record : sealedRecords)
        result.packageBytes.insert(result.packageBytes.end(), record.sealed.ciphertext.begin(),
                                   record.sealed.ciphertext.end());
    PackagePublisherSigningRequest signingRequest;
    signingRequest.signingKeyId = request.signingKeyId;
    signingRequest.canonicalSignedBytes = &result.packageBytes;
    PackagePublisherSigningResponse signingResponse;
    if (! request.publisherSigner->signCanonicalPackage(
            signingRequest, signingResponse, issue)
        || signingResponse.signature.size() != packageEd25519SignatureBytes
        || signingResponse.auditId.empty())
    {
        if (issue.empty()) issue = "publisher signing response is invalid";
        result.issues.push_back(issue);
        result.packageBytes.clear();
        return result;
    }
    result.packageBytes.insert(result.packageBytes.end(), signingResponse.signature.begin(),
                               signingResponse.signature.end());
    result.signingAuditId = std::move(signingResponse.auditId);
    if (result.packageBytes.size() != totalSize)
    { result.issues.push_back("V3 package size does not match its canonical layout"); result.packageBytes.clear(); return result; }

    std::vector<PackageV3RecordInput> semanticRecords;
    semanticRecords.reserve(sealedRecords.size());
    for (const auto& record : sealedRecords)
        semanticRecords.push_back(record.input);
    if (! buildSemanticDigest(request, semanticRecords, result.semanticDigest))
    { result.issues.push_back("V3 semantic SHA-256 could not be computed"); result.packageBytes.clear(); return result; }
    result.written = true;
    return result;
}

PackageV3StreamingWriteResult writePackageV3Streaming(
    const PackageV3StreamingWritePlan& plan,
    const PackageV3StreamingWriteOptions& options)
{
    namespace fs = std::filesystem;
    PackageV3StreamingWriteResult result;
    static std::atomic<std::uint64_t> nextStagingId { 1u };
    const auto started = std::chrono::steady_clock::now();
    const auto elapsedMicros = [&]
    {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started).count());
    };
    result.packagePath = plan.outputPath;
    result.stagingPath = plan.outputPath + ".v3-stage-"
        + std::to_string(nextStagingId.fetch_add(1u, std::memory_order_relaxed));
    result.state = "Package V3 streaming write not attempted";
    const auto cancelled = [&]
    {
        return options.cancellationProbe && options.cancellationProbe();
    };
    const auto publish = [&](const PackageV3StreamingWriteStage stage,
                             const std::size_t index,
                             const std::uint64_t completed,
                             const PackageV3StreamingRecordSource* record,
                             const std::string& status,
                             const std::uint64_t total)
    {
        if (options.progressSink)
        {
            options.progressSink({ stage, index, plan.records.size(), completed, total,
                                   record == nullptr ? std::string {} : record->recordId,
                                   record == nullptr ? std::string {} : record->recordKind,
                                   status });
        }
    };
    const auto fail = [&](const PackageV3StreamingFailure failure,
                          std::string issue)
    {
        result.failure = failure;
        result.state = std::move(issue);
        result.issues = { result.state };
        result.totalDurationMicros = elapsedMicros();
        std::error_code error;
        fs::remove(fs::path(result.stagingPath), error);
        publish(failure == PackageV3StreamingFailure::cancelled
                    ? PackageV3StreamingWriteStage::cancelled
                    : PackageV3StreamingWriteStage::failed,
                result.completedRecordCount, result.processedPlaintextBytes, nullptr,
                result.state, result.processedPlaintextBytes);
        return result;
    };

    if (plan.packageId.empty() || plan.compatibilityId.empty() || plan.outputPath.empty()
        || plan.encryptionKeyId.empty() || plan.signingKeyId.empty()
        || plan.packageId.size() > packageV3MaximumIdentityBytes
        || plan.compatibilityId.size() > packageV3MaximumIdentityBytes
        || plan.encryptionKeyId.size() > packageV3MaximumIdentityBytes
        || plan.signingKeyId.size() > packageV3MaximumIdentityBytes
        || plan.keyProvider == nullptr || plan.publisherSigner == nullptr
        || plan.trustStore == nullptr || ! plan.trustStore->valid()
        || plan.records.empty() || plan.records.size() > packageV3MaximumRecords)
        return fail(PackageV3StreamingFailure::configuration,
                    "V3 export security configuration or record plan is invalid");

    auto records = plan.records;
    const auto less = [](const auto& left, const auto& right)
    {
        return std::tie(left.recordKind, left.recordId, left.generation, left.pageIndex)
            < std::tie(right.recordKind, right.recordId, right.generation, right.pageIndex);
    };
    std::sort(records.begin(), records.end(), less);
    std::uint64_t totalPlaintextBytes = 0;
    for (std::size_t index = 0; index < records.size(); ++index)
    {
        const auto& record = records[index];
        if (record.recordId.empty() || record.recordKind.empty()
            || record.recordId.size() > packageV3MaximumIdentityBytes
            || record.recordKind.size() > packageV3MaximumIdentityBytes
            || record.expectedPlaintextBytes > packageV3MaximumRecordBytes
            || ! record.loadPlaintext
            || ! checkedAdd(totalPlaintextBytes, record.expectedPlaintextBytes,
                            totalPlaintextBytes))
            return fail(PackageV3StreamingFailure::bounds,
                        "V3 streaming record identity, size, or byte accounting is invalid");
        if (index != 0 && ! less(records[index - 1u], record))
            return fail(PackageV3StreamingFailure::format,
                        "V3 streaming record identities must be unique");
    }
    if (cancelled())
        return fail(PackageV3StreamingFailure::cancelled,
                    "Package V3 export was cancelled before key resolution");

    std::string issue;
    SecureBuffer releaseKey;
    if (! plan.keyProvider->resolvePackageKey(
            plan.packageId, plan.encryptionKeyId, PackageKeyUse::encryptNewPackage,
            releaseKey, issue))
        return fail(PackageV3StreamingFailure::keyUnavailable,
                    issue.empty() ? "Package release key is unavailable" : issue);
    SecureBuffer contentKey;
    if (! generateSecurePackageKey(contentKey, issue))
        return fail(PackageV3StreamingFailure::keyUnavailable, issue);
    PackageKeyEnvelope envelope;
    if (! wrapPackageContentKey(plan.packageId, plan.encryptionKeyId,
                                contentKey, releaseKey, envelope, issue))
        return fail(PackageV3StreamingFailure::authentication, issue);

    PackageV3WriteRequest layout;
    layout.packageId = plan.packageId;
    layout.compatibilityId = plan.compatibilityId;
    layout.encryptionKeyId = plan.encryptionKeyId;
    layout.releaseKey = &releaseKey;
    layout.signingKeyId = plan.signingKeyId;
    layout.publisherSigner = plan.publisherSigner;
    std::vector<SealedRecord> descriptors;
    descriptors.reserve(records.size());
    std::uint64_t payloadSize = 0;
    for (std::size_t index = 0; index < records.size(); ++index)
    {
        SealedRecord descriptor;
        descriptor.input.recordId = records[index].recordId;
        descriptor.input.recordKind = records[index].recordKind;
        descriptor.input.generation = records[index].generation;
        descriptor.input.pageIndex = records[index].pageIndex;
        descriptor.ordinal = static_cast<std::uint32_t>(index);
        descriptor.plaintextSize = records[index].expectedPlaintextBytes;
        descriptor.ciphertextSize = records[index].expectedPlaintextBytes;
        if (! checkedAdd(payloadSize, descriptor.ciphertextSize, payloadSize))
            return fail(PackageV3StreamingFailure::bounds, "V3 payload size overflow");
        descriptor.sealed.nonce.resize(getSecurePackageCryptoProvider().nonceSizeBytes());
        descriptor.sealed.tag.resize(getSecurePackageCryptoProvider().tagSizeBytes());
        descriptors.push_back(std::move(descriptor));
    }
    auto provisionalHeader = serializeHeader(
        layout, envelope, 0u, static_cast<std::uint32_t>(descriptors.size()),
        0u, 0u, 0u, payloadSize, 0u);
    if (provisionalHeader.empty()
        || provisionalHeader.size() > packageV3MaximumHeaderBytes
        || provisionalHeader.size() > std::numeric_limits<std::uint32_t>::max())
        return fail(PackageV3StreamingFailure::bounds, "V3 header could not be encoded");
    const auto headerSize = static_cast<std::uint32_t>(provisionalHeader.size());
    auto provisionalToc = serializeToc(descriptors);
    if (provisionalToc.empty() || provisionalToc.size() > packageV3MaximumTocBytes)
        return fail(PackageV3StreamingFailure::bounds, "V3 TOC could not be encoded");
    const std::uint64_t tocOffset = headerSize;
    const std::uint64_t tocSize = provisionalToc.size();
    std::uint64_t payloadOffset = 0, signatureOffset = 0, totalPackageBytes = 0;
    if (! checkedAdd(tocOffset, tocSize, payloadOffset)
        || ! checkedAdd(payloadOffset, payloadSize, signatureOffset)
        || ! checkedAdd(signatureOffset, packageEd25519SignatureBytes, totalPackageBytes)
        || totalPackageBytes > packageV3MaximumPackageBytes)
        return fail(PackageV3StreamingFailure::bounds,
                    "V3 package size exceeds its bounded format limit");
    auto ciphertextOffset = payloadOffset;
    for (auto& descriptor : descriptors)
    {
        descriptor.ciphertextOffset = ciphertextOffset;
        if (! checkedAdd(ciphertextOffset, descriptor.ciphertextSize, ciphertextOffset))
            return fail(PackageV3StreamingFailure::bounds, "V3 ciphertext offset overflow");
    }

    std::error_code error;
    const auto parent = fs::path(plan.outputPath).parent_path();
    if (! parent.empty()) fs::create_directories(parent, error);
    fs::remove(fs::path(result.stagingPath), error);
    std::ofstream output(fs::path(result.stagingPath), std::ios::binary | std::ios::trunc);
    if (! output)
        return fail(PackageV3StreamingFailure::io,
                    "Could not create the V3 package staging file");
    output.seekp(static_cast<std::streamoff>(payloadOffset), std::ios::beg);
    if (! output)
    {
        output.close();
        return fail(PackageV3StreamingFailure::io,
                    "Could not reserve the V3 package index region");
    }

    crypto_hash_sha256_state semanticState;
    if (crypto_hash_sha256_init(&semanticState) != 0)
    {
        output.close();
        return fail(PackageV3StreamingFailure::authentication,
                    "V3 semantic SHA-256 could not be initialized");
    }
    crypto_hash_sha256_update(&semanticState, kSemanticMagic.data(), kSemanticMagic.size());
    std::vector<std::uint8_t> semanticMetadata;
    if (! appendString(semanticMetadata, plan.packageId)
        || ! appendString(semanticMetadata, plan.compatibilityId))
    {
        output.close();
        return fail(PackageV3StreamingFailure::format,
                    "V3 semantic identities could not be encoded");
    }
    appendU32(semanticMetadata, static_cast<std::uint32_t>(records.size()));
    crypto_hash_sha256_update(&semanticState, semanticMetadata.data(), semanticMetadata.size());

    const auto& crypto = getSecurePackageCryptoProvider();
    for (std::size_t index = 0; index < records.size(); ++index)
    {
        const auto& source = records[index];
        if (cancelled())
        {
            output.close();
            return fail(PackageV3StreamingFailure::cancelled,
                        "Package V3 export was cancelled between records");
        }
        publish(PackageV3StreamingWriteStage::loadingRecord, index,
                result.processedPlaintextBytes, &source, source.sourceLabel,
                totalPlaintextBytes);
        if (cancelled())
        {
            output.close();
            return fail(PackageV3StreamingFailure::cancelled,
                        "Package V3 export was cancelled before loading a record");
        }
        std::vector<std::uint8_t> plaintext;
        if (! source.loadPlaintext(plaintext, issue)
            || plaintext.size() != source.expectedPlaintextBytes)
        {
            output.close();
            return fail(PackageV3StreamingFailure::io,
                        issue.empty() ? "Could not load a bounded V3 record" : issue);
        }
        result.peakPlaintextBufferBytes = std::max<std::uint64_t>(
            result.peakPlaintextBufferBytes, plaintext.size());
        semanticMetadata.clear();
        appendU32(semanticMetadata, static_cast<std::uint32_t>(index));
        if (! appendString(semanticMetadata, source.recordId)
            || ! appendString(semanticMetadata, source.recordKind))
        {
            output.close();
            return fail(PackageV3StreamingFailure::format,
                        "V3 semantic record identity could not be encoded");
        }
        appendU32(semanticMetadata, source.generation);
        appendU32(semanticMetadata, source.pageIndex);
        appendU64(semanticMetadata, source.expectedPlaintextBytes);
        crypto_hash_sha256_update(&semanticState, semanticMetadata.data(),
                                  semanticMetadata.size());
        if (! plaintext.empty())
            crypto_hash_sha256_update(&semanticState, plaintext.data(), plaintext.size());

        const auto aad = buildPackageV3RecordAad(
            plan.packageId, plan.compatibilityId, static_cast<std::uint32_t>(index),
            source.recordId, source.recordKind, source.generation, source.pageIndex,
            source.expectedPlaintextBytes);
        SecureBuffer securePlaintext(std::move(plaintext));
        PackageSealRequest seal;
        seal.packageId = plan.packageId;
        seal.recordId = source.recordId;
        seal.encryptionKeyId = plan.encryptionKeyId;
        seal.secureEncryptionKey = &contentKey;
        seal.additionalAuthenticatedData = asBinaryString(aad);
        seal.securePlaintext = &securePlaintext;
        publish(PackageV3StreamingWriteStage::sealingRecord, index,
                result.processedPlaintextBytes, &source, "Sealing protected record",
                totalPlaintextBytes);
        if (cancelled())
        {
            output.close();
            return fail(PackageV3StreamingFailure::cancelled,
                        "Package V3 export was cancelled before sealing a record");
        }
        PackageSealedBlob sealed;
        if (aad.empty() || ! crypto.seal(seal, sealed, issue)
            || sealed.ciphertext.size() != source.expectedPlaintextBytes
            || sealed.nonce.size() != crypto.nonceSizeBytes()
            || sealed.tag.size() != crypto.tagSizeBytes())
        {
            output.close();
            return fail(PackageV3StreamingFailure::authentication,
                        issue.empty() ? "Could not seal a bounded V3 record" : issue);
        }
        result.peakSealedBufferBytes = std::max<std::uint64_t>(
            result.peakSealedBufferBytes, sealed.ciphertext.size());
        descriptors[index].sealed.nonce = std::move(sealed.nonce);
        descriptors[index].sealed.tag = std::move(sealed.tag);
        publish(PackageV3StreamingWriteStage::writingRecord, index,
                result.processedPlaintextBytes, &source, "Writing protected record",
                totalPlaintextBytes);
        if (cancelled())
        {
            output.close();
            return fail(PackageV3StreamingFailure::cancelled,
                        "Package V3 export was cancelled before writing a record");
        }
        output.write(reinterpret_cast<const char*>(sealed.ciphertext.data()),
                     static_cast<std::streamsize>(sealed.ciphertext.size()));
        if (! output)
        {
            output.close();
            return fail(PackageV3StreamingFailure::io,
                        "Could not append a protected V3 record");
        }
        result.processedPlaintextBytes += source.expectedPlaintextBytes;
        ++result.completedRecordCount;
    }
    result.semanticDigest.resize(crypto_hash_sha256_BYTES);
    if (crypto_hash_sha256_final(&semanticState, result.semanticDigest.data()) != 0)
    {
        output.close();
        return fail(PackageV3StreamingFailure::authentication,
                    "V3 semantic SHA-256 could not be finalized");
    }

    publish(PackageV3StreamingWriteStage::finalizingIndex, records.size(),
            result.processedPlaintextBytes, nullptr, "Finalizing canonical V3 index",
            totalPlaintextBytes);
    if (cancelled())
    {
        output.close();
        return fail(PackageV3StreamingFailure::cancelled,
                    "Package V3 export was cancelled before finalizing the index");
    }
    const auto header = serializeHeader(
        layout, envelope, headerSize, static_cast<std::uint32_t>(descriptors.size()),
        tocOffset, tocSize, payloadOffset, payloadSize, signatureOffset);
    const auto toc = serializeToc(descriptors);
    if (header.size() != headerSize || toc.size() != tocSize)
    {
        output.close();
        return fail(PackageV3StreamingFailure::format,
                    "V3 canonical layout changed during serialization");
    }
    output.seekp(0, std::ios::beg);
    output.write(reinterpret_cast<const char*>(header.data()),
                 static_cast<std::streamsize>(header.size()));
    output.write(reinterpret_cast<const char*>(toc.data()),
                 static_cast<std::streamsize>(toc.size()));
    output.flush();
    output.close();
    if (! output)
        return fail(PackageV3StreamingFailure::io,
                    "Could not finalize the V3 package staging file");

    if (cancelled())
        return fail(PackageV3StreamingFailure::cancelled,
                    "Package V3 export was cancelled before publisher signing");
    publish(PackageV3StreamingWriteStage::signing, records.size(),
            result.processedPlaintextBytes, nullptr, "Requesting publisher signature",
            totalPlaintextBytes);
    if (cancelled())
        return fail(PackageV3StreamingFailure::cancelled,
                    "Package V3 export was cancelled before publisher signing");
    PackagePublisherSigningRequest signingRequest;
    signingRequest.signingKeyId = plan.signingKeyId;
    signingRequest.canonicalSignedFilePath = result.stagingPath;
    signingRequest.canonicalSignedBytesLength = signatureOffset;
    PackagePublisherSigningResponse signingResponse;
    if (! plan.publisherSigner->signCanonicalPackage(
            signingRequest, signingResponse, issue)
        || signingResponse.signature.size() != packageEd25519SignatureBytes
        || signingResponse.auditId.empty())
        return fail(PackageV3StreamingFailure::signing,
                    issue.empty() ? "Publisher signing response is invalid" : issue);
    if (cancelled())
        return fail(PackageV3StreamingFailure::cancelled,
                    "Package V3 export was cancelled after publisher signing");
    result.signingAuditId = std::move(signingResponse.auditId);
    std::ofstream append(fs::path(result.stagingPath), std::ios::binary | std::ios::app);
    append.write(reinterpret_cast<const char*>(signingResponse.signature.data()),
                 static_cast<std::streamsize>(signingResponse.signature.size()));
    append.flush();
    append.close();
    if (! append)
        return fail(PackageV3StreamingFailure::io,
                    "Could not append the publisher signature to the V3 package");

    publish(PackageV3StreamingWriteStage::verifying, records.size(),
            result.processedPlaintextBytes, nullptr, "Verifying staged signed V3 package",
            totalPlaintextBytes);
    if (cancelled())
        return fail(PackageV3StreamingFailure::cancelled,
                    "Package V3 export was cancelled before staged verification");
    const auto verified = openPackageV3File(result.stagingPath, *plan.trustStore);
    if (! verified.opened || verified.package.packageId != plan.packageId
        || verified.package.compatibilityId != plan.compatibilityId
        || verified.package.encryptionKeyId != plan.encryptionKeyId
        || verified.package.signingKeyId != plan.signingKeyId
        || verified.package.records.size() != records.size())
        return fail(PackageV3StreamingFailure::signing,
                    verified.issues.empty() ? "Staged V3 publisher verification failed"
                                            : verified.issues.front());
    result.verificationBytesRead = verified.verificationBytesRead
        + verified.indexBytesRead + verified.signatureBytesRead;
    const std::array<std::size_t, 2> selected { 0u, records.size() - 1u };
    for (std::size_t index = 0; index < selected.size(); ++index)
    {
        if (index == 1u && selected[1] == selected[0]) continue;
        const auto opened = openPackageV3FileRecord(
            verified, contentKey, verified.package.records[selected[index]]);
        result.verificationBytesRead += opened.ciphertextBytesRead;
        if (! opened.opened)
            return fail(PackageV3StreamingFailure::authentication,
                        opened.issues.empty() ? "Staged V3 record verification failed"
                                              : opened.issues.front());
    }
    result.verified = true;
    if (cancelled())
        return fail(PackageV3StreamingFailure::cancelled,
                    "Package V3 export was cancelled before atomic publication");

    publish(PackageV3StreamingWriteStage::publishing, records.size(),
            result.processedPlaintextBytes, nullptr, "Publishing verified V3 package atomically",
            totalPlaintextBytes);
    if (cancelled())
        return fail(PackageV3StreamingFailure::cancelled,
                    "Package V3 export was cancelled before atomic publication");
#if defined(_WIN32)
    const auto moved = MoveFileExW(fs::path(result.stagingPath).c_str(),
                                   fs::path(plan.outputPath).c_str(),
                                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    fs::rename(fs::path(result.stagingPath), fs::path(plan.outputPath), error);
    const auto moved = ! error;
#endif
    if (! moved)
        return fail(PackageV3StreamingFailure::io,
                    "Verified V3 package could not be atomically published");
    result.atomicallyPublished = true;
    result.written = true;
    result.failure = PackageV3StreamingFailure::none;
    result.packageBytes = totalPackageBytes;
    result.state = "Package V3 streaming export completed";
    result.totalDurationMicros = elapsedMicros();
    if (result.totalDurationMicros != 0)
        result.plaintextThroughputBytesPerSecond
            = static_cast<double>(result.processedPlaintextBytes) * 1000000.0
            / static_cast<double>(result.totalDurationMicros);
    publish(PackageV3StreamingWriteStage::completed, records.size(),
            result.processedPlaintextBytes, nullptr, result.state, totalPlaintextBytes);
    return result;
}

PackageV3OpenResult parsePackageV3Index(const std::vector<std::uint8_t>& bytes,
                                        const std::uint64_t totalPackageBytes)
{
    PackageV3OpenResult result;
    if (totalPackageBytes > packageV3MaximumPackageBytes)
    { result.issues.push_back("V3 package exceeds the format size limit"); return result; }
    if (bytes.size() < packageV3FixedHeaderBytes)
    { result.issues.push_back("V3 fixed header is truncated"); return result; }
    Reader header(bytes, 0u, bytes.size());
    std::vector<std::uint8_t> magic;
    std::uint32_t version = 0, headerSize = 0, flags = 0, recordCount = 0;
    std::uint16_t cryptoSuite = 0, signatureSuite = 0;
    std::uint64_t tocOffset = 0, tocSize = 0, payloadOffset = 0, payloadSize = 0, signatureOffset = 0;
    std::uint32_t signatureSize = 0, reserved = 0;
    if (! header.readBytes(kMagic.size(), magic) || ! std::equal(magic.begin(), magic.end(), kMagic.begin())
        || ! header.readU32(version) || ! header.readU32(headerSize) || ! header.readU32(flags)
        || ! header.readU16(cryptoSuite) || ! header.readU16(signatureSuite)
        || ! header.readU32(recordCount) || ! header.readU64(tocOffset) || ! header.readU64(tocSize)
        || ! header.readU64(payloadOffset) || ! header.readU64(payloadSize)
        || ! header.readU64(signatureOffset) || ! header.readU32(signatureSize)
        || ! header.readU32(reserved))
    { result.issues.push_back("V3 fixed header is truncated"); return result; }
    if (version != packageV3FormatVersion || flags != kRequiredFlags
        || cryptoSuite != packageV3CryptoSuiteXChaCha20Poly1305
        || signatureSuite != packageV3SignatureSuiteEd25519ph || reserved != 0u
        || signatureSize != kSignatureSize || recordCount == 0u
        || recordCount > packageV3MaximumRecords
        || headerSize < packageV3FixedHeaderBytes
        || headerSize > packageV3MaximumHeaderBytes
        || tocSize > packageV3MaximumTocBytes
        || headerSize > bytes.size())
    { result.issues.push_back("V3 fixed header contains unsupported or invalid values"); return result; }

    if (! header.readString(result.packageId) || ! header.readString(result.compatibilityId)
        || ! header.readString(result.encryptionKeyId) || ! header.readString(result.signingKeyId))
    { result.issues.push_back("V3 package identity fields are invalid"); return result; }
    result.keyEnvelope.keyId = result.encryptionKeyId;
    std::uint16_t wrappedKeySize = 0;
    if (! header.readBytes(24u, result.keyEnvelope.sealedContentKey.nonce)
        || ! header.readU16(wrappedKeySize) || wrappedKeySize != securePackageKeySizeBytes
        || ! header.readBytes(wrappedKeySize, result.keyEnvelope.sealedContentKey.ciphertext)
        || ! header.readBytes(16u, result.keyEnvelope.sealedContentKey.tag)
        || header.position != headerSize)
    { result.issues.push_back("V3 key envelope or header size is invalid"); return result; }

    std::uint64_t expectedPayloadOffset = 0, expectedSignatureOffset = 0, expectedEnd = 0;
    if (tocOffset != headerSize || ! checkedAdd(tocOffset, tocSize, expectedPayloadOffset)
        || payloadOffset != expectedPayloadOffset
        || ! checkedAdd(payloadOffset, payloadSize, expectedSignatureOffset)
        || signatureOffset != expectedSignatureOffset
        || ! checkedAdd(signatureOffset, signatureSize, expectedEnd)
        || expectedEnd != totalPackageBytes
        || payloadOffset > bytes.size()
        || payloadOffset != bytes.size())
    { result.issues.push_back("V3 section offsets are non-canonical or out of bounds"); return result; }

    Reader toc(bytes, static_cast<std::size_t>(tocOffset),
               static_cast<std::size_t>(payloadOffset));
    result.records.reserve(recordCount);
    std::uint64_t expectedCiphertextOffset = payloadOffset;
    PackageV3RecordInput previousIdentity;
    bool hasPrevious = false;
    for (std::uint32_t index = 0; index < recordCount; ++index)
    {
        PackageV3RecordDescriptor record;
        if (! toc.readU32(record.ordinal) || ! toc.readString(record.recordId)
            || ! toc.readString(record.recordKind) || ! toc.readU32(record.generation)
            || ! toc.readU32(record.pageIndex) || ! toc.readU64(record.plaintextSize)
            || ! toc.readU64(record.ciphertextOffset) || ! toc.readU64(record.ciphertextSize)
            || ! toc.readBytes(24u, record.sealed.nonce)
            || ! toc.readBytes(16u, record.sealed.tag))
        { result.issues.push_back("V3 TOC record is truncated or malformed"); return result; }
        if (record.ordinal != index || record.plaintextSize > packageV3MaximumRecordBytes
            || record.ciphertextSize != record.plaintextSize
            || record.ciphertextOffset != expectedCiphertextOffset)
        { result.issues.push_back("V3 TOC record order, size, or offset is non-canonical"); return result; }
        std::uint64_t ciphertextEnd = 0;
        if (! checkedAdd(record.ciphertextOffset, record.ciphertextSize, ciphertextEnd)
            || ciphertextEnd > signatureOffset)
        { result.issues.push_back("V3 ciphertext range is out of bounds"); return result; }
        PackageV3RecordInput identity;
        identity.recordId = record.recordId;
        identity.recordKind = record.recordKind;
        identity.generation = record.generation;
        identity.pageIndex = record.pageIndex;
        if (hasPrevious && ! recordLess(previousIdentity, identity))
        { result.issues.push_back("V3 TOC record identities are duplicate or not canonical"); return result; }
        previousIdentity = std::move(identity);
        hasPrevious = true;
        expectedCiphertextOffset = ciphertextEnd;
        result.records.push_back(std::move(record));
    }
    if (toc.position != toc.limit || expectedCiphertextOffset != signatureOffset)
    { result.issues.push_back("V3 TOC or payload coverage is incomplete"); return result; }
    result.tocOffset = tocOffset;
    result.tocSize = tocSize;
    result.payloadOffset = payloadOffset;
    result.payloadSize = payloadSize;
    result.signatureOffset = signatureOffset;
    result.opened = true;
    return result;
}

PackageV3OpenResult parsePackageV3(const std::vector<std::uint8_t>& bytes)
{
    PackageV3OpenResult result;
    if (bytes.size() < packageV3FixedHeaderBytes
        || bytes.size() > packageV3MaximumPackageBytes)
    { result.issues.push_back("V3 package size is outside the format limits"); return result; }

    Reader fixedHeader(bytes, 0u, bytes.size());
    std::vector<std::uint8_t> magic;
    std::uint32_t version = 0, headerSize = 0, flags = 0, recordCount = 0;
    std::uint16_t cryptoSuite = 0, signatureSuite = 0;
    std::uint64_t tocOffset = 0, tocSize = 0, payloadOffset = 0, payloadSize = 0;
    std::uint64_t signatureOffset = 0;
    std::uint32_t signatureSize = 0, reserved = 0;
    if (! fixedHeader.readBytes(kMagic.size(), magic)
        || ! fixedHeader.readU32(version) || ! fixedHeader.readU32(headerSize)
        || ! fixedHeader.readU32(flags) || ! fixedHeader.readU16(cryptoSuite)
        || ! fixedHeader.readU16(signatureSuite) || ! fixedHeader.readU32(recordCount)
        || ! fixedHeader.readU64(tocOffset) || ! fixedHeader.readU64(tocSize)
        || ! fixedHeader.readU64(payloadOffset) || ! fixedHeader.readU64(payloadSize)
        || ! fixedHeader.readU64(signatureOffset) || ! fixedHeader.readU32(signatureSize)
        || ! fixedHeader.readU32(reserved)
        || payloadOffset > bytes.size())
    { result.issues.push_back("V3 fixed header is truncated or out of bounds"); return result; }

    std::vector<std::uint8_t> indexBytes(
        bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset));
    result = parsePackageV3Index(indexBytes, bytes.size());
    if (! result.opened)
        return result;
    for (auto& record : result.records)
    {
        const auto begin = static_cast<std::size_t>(record.ciphertextOffset);
        const auto end = begin + static_cast<std::size_t>(record.ciphertextSize);
        record.sealed.ciphertext.assign(bytes.begin() + static_cast<std::ptrdiff_t>(begin),
                                        bytes.begin() + static_cast<std::ptrdiff_t>(end));
    }
    result.signature.assign(
        bytes.begin() + static_cast<std::ptrdiff_t>(result.signatureOffset), bytes.end());
    return result;
}

bool verifyPackageV3Signature(const std::vector<std::uint8_t>& packageBytes,
                              const std::vector<PackageSigningKey>& trustStore,
                              PackageV3OpenResult& package,
                              std::string& issue)
{
    const PackagePublisherTrustStore immutableStore(trustStore);
    return verifyPackageV3Signature(packageBytes, immutableStore, package, issue);
}

bool verifyPackageV3Signature(const std::vector<std::uint8_t>& packageBytes,
                              const PackagePublisherTrustStore& trustStore,
                              PackageV3OpenResult& package,
                              std::string& issue)
{
    package.signatureVerified = false;
    if (! package.opened || package.signatureOffset > packageBytes.size()
        || packageBytes.size() - static_cast<std::size_t>(package.signatureOffset) != package.signature.size())
    { issue = "V3 package is not structurally open for signature verification"; return false; }
    const std::vector<std::uint8_t> signedBytes(
        packageBytes.begin(), packageBytes.begin() + static_cast<std::ptrdiff_t>(package.signatureOffset));
    std::vector<std::uint8_t> publicKey;
    if (! resolvePackageSigningPublicKey(
            package.signingKeyId, trustStore, publicKey, issue)
        || ! packageVerifyEd25519ph(publicKey, signedBytes, package.signature, issue))
        return false;
    package.signatureVerified = true;
    issue.clear();
    return true;
}

bool unwrapPackageV3ContentKey(const PackageV3OpenResult& package,
                               const SecureBuffer& releaseKey,
                               SecureBuffer& contentKey,
                               std::string& issue)
{
    contentKey.clear();
    if (! package.opened || ! package.signatureVerified)
    { issue = "V3 content key cannot be unwrapped before signature verification"; return false; }
    return unwrapPackageContentKey(package.packageId, package.keyEnvelope,
                                   releaseKey, contentKey, issue);
}

bool openPackageV3Record(const SecureBuffer& contentKey,
                         const PackageV3OpenResult& package,
                         const PackageV3RecordDescriptor& record,
                         std::vector<std::uint8_t>& plaintext,
                         std::string& issue)
{
    plaintext.clear();
    if (! package.opened || ! package.signatureVerified
        || contentKey.size() != securePackageKeySizeBytes
        || record.ordinal >= package.records.size())
    { issue = "V3 package, signature, content key, or record identity is invalid"; return false; }
    const auto& trustedRecord = package.records[record.ordinal];
    if (! descriptorMatches(record, trustedRecord))
    { issue = "V3 record descriptor is not part of the verified package"; return false; }
    const auto aad = buildPackageV3RecordAad(
        package.packageId, package.compatibilityId, trustedRecord.ordinal,
        trustedRecord.recordId, trustedRecord.recordKind, trustedRecord.generation,
        trustedRecord.pageIndex, trustedRecord.plaintextSize);
    if (aad.empty())
    { issue = "V3 record AAD could not be encoded"; return false; }
    PackageOpenRequest open;
    open.packageId = package.packageId;
    open.recordId = trustedRecord.recordId;
    open.encryptionKeyId = package.encryptionKeyId;
    open.additionalAuthenticatedData = asBinaryString(aad);
    open.secureEncryptionKey = &contentKey;
    open.sealed = trustedRecord.sealed;
    return getSecurePackageCryptoProvider().open(open, plaintext, issue);
}
} // namespace drs::engine
