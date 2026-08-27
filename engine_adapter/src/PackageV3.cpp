#include <drs/engine/PackageV3.h>

#include <algorithm>
#include <array>
#include <limits>
#include <tuple>

#include <sodium/crypto_hash_sha256.h>
#include <sodium/utils.h>

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
    appendU16(bytes, packageV3SignatureSuiteEd25519);
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
        appendU64(bytes, static_cast<std::uint64_t>(record.input.plaintext.size()));
        appendU64(bytes, record.ciphertextOffset);
        appendU64(bytes, static_cast<std::uint64_t>(record.sealed.ciphertext.size()));
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
        || request.releaseKey.size() != securePackageKeySizeBytes
        || request.signingPrivateKey.size() != packageEd25519PrivateKeyBytes
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
    std::vector<std::uint8_t> contentKey;
    if (! generateSecurePackageKey(contentKey, issue))
    { result.issues.push_back(issue); return result; }
    struct KeyWiper
    {
        std::vector<std::uint8_t>& bytes;
        ~KeyWiper() { if (! bytes.empty()) sodium_memzero(bytes.data(), bytes.size()); }
    } keyWiper { contentKey };

    PackageKeyEnvelope envelope;
    if (! wrapPackageContentKey(request.packageId, request.encryptionKeyId,
                                contentKey, request.releaseKey, envelope, issue))
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
        seal.encryptionKey = contentKey;
        seal.additionalAuthenticatedData = asBinaryString(aad);
        seal.plaintext = records[index].plaintext;
        SealedRecord sealed;
        sealed.input = std::move(records[index]);
        sealed.ordinal = static_cast<std::uint32_t>(index);
        if (! crypto.seal(seal, sealed.sealed, issue))
        { result.issues.push_back(issue); return result; }
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
    std::vector<std::uint8_t> signature;
    if (! packageSignEd25519(request.signingPrivateKey, result.packageBytes, signature, issue))
    { result.issues.push_back(issue); result.packageBytes.clear(); return result; }
    result.packageBytes.insert(result.packageBytes.end(), signature.begin(), signature.end());
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

PackageV3OpenResult parsePackageV3(const std::vector<std::uint8_t>& bytes)
{
    PackageV3OpenResult result;
    if (bytes.size() > packageV3MaximumPackageBytes)
    { result.issues.push_back("V3 package exceeds the format size limit"); return result; }
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
        || signatureSuite != packageV3SignatureSuiteEd25519 || reserved != 0u
        || signatureSize != kSignatureSize || recordCount == 0u
        || recordCount > packageV3MaximumRecords || headerSize > bytes.size())
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
        || expectedEnd != bytes.size())
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
        record.sealed.ciphertext.assign(
            bytes.begin() + static_cast<std::ptrdiff_t>(record.ciphertextOffset),
            bytes.begin() + static_cast<std::ptrdiff_t>(ciphertextEnd));
        expectedCiphertextOffset = ciphertextEnd;
        result.records.push_back(std::move(record));
    }
    if (toc.position != toc.limit || expectedCiphertextOffset != signatureOffset)
    { result.issues.push_back("V3 TOC or payload coverage is incomplete"); return result; }
    result.signature.assign(bytes.begin() + static_cast<std::ptrdiff_t>(signatureOffset), bytes.end());
    result.tocOffset = tocOffset;
    result.tocSize = tocSize;
    result.payloadOffset = payloadOffset;
    result.payloadSize = payloadSize;
    result.signatureOffset = signatureOffset;
    result.opened = true;
    return result;
}

bool verifyPackageV3Signature(const std::vector<std::uint8_t>& packageBytes,
                              const std::vector<PackageSigningKey>& trustStore,
                              PackageV3OpenResult& package,
                              std::string& issue)
{
    package.signatureVerified = false;
    if (! package.opened || package.signatureOffset > packageBytes.size()
        || packageBytes.size() - static_cast<std::size_t>(package.signatureOffset) != package.signature.size())
    { issue = "V3 package is not structurally open for signature verification"; return false; }
    const std::vector<std::uint8_t> signedBytes(
        packageBytes.begin(), packageBytes.begin() + static_cast<std::ptrdiff_t>(package.signatureOffset));
    if (! verifyPackageSignature(signedBytes, package.signature, package.signingKeyId,
                                 trustStore, issue))
        return false;
    package.signatureVerified = true;
    issue.clear();
    return true;
}

bool unwrapPackageV3ContentKey(const PackageV3OpenResult& package,
                               const std::vector<std::uint8_t>& releaseKey,
                               std::vector<std::uint8_t>& contentKey,
                               std::string& issue)
{
    contentKey.clear();
    if (! package.opened || ! package.signatureVerified)
    { issue = "V3 content key cannot be unwrapped before signature verification"; return false; }
    return unwrapPackageContentKey(package.packageId, package.keyEnvelope,
                                   releaseKey, contentKey, issue);
}

bool openPackageV3Record(const std::vector<std::uint8_t>& contentKey,
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
    open.encryptionKey = contentKey;
    open.sealed = trustedRecord.sealed;
    return getSecurePackageCryptoProvider().open(open, plaintext, issue);
}
} // namespace drs::engine
