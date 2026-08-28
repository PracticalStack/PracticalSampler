#include <drs/engine/PackageV3FileReader.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>

#include <sodium/core.h>
#include <sodium/crypto_sign_ed25519.h>

namespace drs::engine
{
namespace
{
constexpr std::array<std::uint8_t, 8> kMagic { 'D', 'R', 'S', 'P', 'K', 'G', '3', 0 };

std::uint32_t readU32(const std::uint8_t* bytes) noexcept
{
    std::uint32_t value = 0;
    for (int index = 0; index < 4; ++index)
        value |= static_cast<std::uint32_t>(bytes[index]) << (index * 8);
    return value;
}

std::uint64_t readU64(const std::uint8_t* bytes) noexcept
{
    std::uint64_t value = 0;
    for (int index = 0; index < 8; ++index)
        value |= static_cast<std::uint64_t>(bytes[index]) << (index * 8);
    return value;
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

bool readExact(std::ifstream& input,
               const std::uint64_t offset,
               std::uint8_t* destination,
               const std::size_t size)
{
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())
        || size > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
        return false;
    input.clear();
    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (! input) return false;
    if (size == 0u) return true;
    input.read(reinterpret_cast<char*>(destination), static_cast<std::streamsize>(size));
    return input.good() || (input.eof() && input.gcount() == static_cast<std::streamsize>(size));
}

bool sameIndexedDescriptor(const PackageV3RecordDescriptor& left,
                           const PackageV3RecordDescriptor& right)
{
    return left.ordinal == right.ordinal && left.recordId == right.recordId
        && left.recordKind == right.recordKind && left.generation == right.generation
        && left.pageIndex == right.pageIndex && left.plaintextSize == right.plaintextSize
        && left.ciphertextOffset == right.ciphertextOffset
        && left.ciphertextSize == right.ciphertextSize
        && left.sealed.nonce == right.sealed.nonce
        && left.sealed.tag == right.sealed.tag
        && left.sealed.ciphertext.empty() && right.sealed.ciphertext.empty();
}

bool ensureSodium(std::string& issue)
{
    static std::once_flag flag;
    static int initialized = -1;
    std::call_once(flag, [] { initialized = sodium_init(); });
    if (initialized < 0)
    {
        issue = "libsodium initialization failed";
        return false;
    }
    return true;
}
} // namespace

PackageV3FileOpenResult openPackageV3File(
    const std::string& packagePath,
    const std::vector<PackageSigningKey>& trustStore)
{
    namespace fs = std::filesystem;
    PackageV3FileOpenResult result;
    result.packagePath = packagePath;

    std::error_code error;
    result.packageBytes = fs::file_size(fs::path(packagePath), error);
    if (error || result.packageBytes < packageV3FixedHeaderBytes
        || result.packageBytes > packageV3MaximumPackageBytes)
    {
        result.issues.push_back("V3 package is missing or outside the bounded file-size limit");
        return result;
    }

    std::ifstream input(fs::path(packagePath), std::ios::binary);
    std::array<std::uint8_t, packageV3FixedHeaderBytes> fixedHeader {};
    if (! input || ! readExact(input, 0u, fixedHeader.data(), fixedHeader.size()))
    {
        result.issues.push_back("V3 fixed header could not be read");
        return result;
    }
    result.indexBytesRead = fixedHeader.size();
    result.peakReadBufferBytes = fixedHeader.size();

    const auto headerSize = readU32(fixedHeader.data() + 12u);
    const auto tocOffset = readU64(fixedHeader.data() + 28u);
    const auto tocSize = readU64(fixedHeader.data() + 36u);
    const auto payloadOffset = readU64(fixedHeader.data() + 44u);
    const auto payloadSize = readU64(fixedHeader.data() + 52u);
    const auto signatureOffset = readU64(fixedHeader.data() + 60u);
    const auto signatureSize = readU32(fixedHeader.data() + 68u);
    std::uint64_t expectedPayloadOffset = 0, expectedSignatureOffset = 0, expectedEnd = 0;
    if (! std::equal(kMagic.begin(), kMagic.end(), fixedHeader.begin())
        || headerSize < packageV3FixedHeaderBytes
        || headerSize > packageV3MaximumHeaderBytes
        || tocSize > packageV3MaximumTocBytes
        || tocOffset != headerSize
        || ! checkedAdd(tocOffset, tocSize, expectedPayloadOffset)
        || payloadOffset != expectedPayloadOffset
        || ! checkedAdd(payloadOffset, payloadSize, expectedSignatureOffset)
        || signatureOffset != expectedSignatureOffset
        || signatureSize != packageEd25519SignatureBytes
        || ! checkedAdd(signatureOffset, signatureSize, expectedEnd)
        || expectedEnd != result.packageBytes
        || payloadOffset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    {
        result.issues.push_back("V3 fixed header section bounds are invalid");
        return result;
    }

    std::vector<std::uint8_t> indexBytes(static_cast<std::size_t>(payloadOffset));
    std::copy(fixedHeader.begin(), fixedHeader.end(), indexBytes.begin());
    const auto remainingIndexBytes = indexBytes.size() - fixedHeader.size();
    if (! readExact(input, fixedHeader.size(), indexBytes.data() + fixedHeader.size(),
                    remainingIndexBytes))
    {
        result.issues.push_back("V3 bounded header/TOC range is truncated");
        return result;
    }
    result.indexBytesRead = indexBytes.size();
    result.peakReadBufferBytes = std::max<std::uint64_t>(
        result.peakReadBufferBytes, indexBytes.size());
    result.package = parsePackageV3Index(indexBytes, result.packageBytes);
    if (! result.package.opened)
    {
        result.issues = result.package.issues;
        return result;
    }

    result.package.signature.resize(signatureSize);
    if (! readExact(input, signatureOffset, result.package.signature.data(),
                    result.package.signature.size()))
    {
        result.package.signature.clear();
        result.issues.push_back("V3 signature is truncated");
        return result;
    }
    result.signatureBytesRead = signatureSize;

    std::string issue;
    std::vector<std::uint8_t> publicKey;
    if (! resolvePackageSigningPublicKey(
            result.package.signingKeyId, trustStore, publicKey, issue)
        || ! ensureSodium(issue))
    {
        result.issues.push_back(issue);
        return result;
    }

    crypto_sign_ed25519ph_state signatureState;
    if (crypto_sign_ed25519ph_init(&signatureState) != 0
        || (! indexBytes.empty()
            && crypto_sign_ed25519ph_update(
                &signatureState, indexBytes.data(),
                static_cast<unsigned long long>(indexBytes.size())) != 0))
    {
        result.issues.push_back("V3 streaming signature verification could not start");
        return result;
    }
    result.verificationBytesRead = indexBytes.size();
    std::vector<std::uint8_t>().swap(indexBytes);

    std::vector<std::uint8_t> buffer(packageV3SignatureStreamBufferBytes);
    result.peakReadBufferBytes = std::max<std::uint64_t>(
        result.peakReadBufferBytes, buffer.size());
    std::uint64_t offset = payloadOffset;
    while (offset < signatureOffset)
    {
        const auto remaining = signatureOffset - offset;
        const auto bytesToRead = static_cast<std::size_t>(
            std::min<std::uint64_t>(remaining, buffer.size()));
        if (! readExact(input, offset, buffer.data(), bytesToRead)
            || crypto_sign_ed25519ph_update(
                &signatureState, buffer.data(),
                static_cast<unsigned long long>(bytesToRead)) != 0)
        {
            result.issues.push_back("V3 signed payload is truncated or unreadable");
            return result;
        }
        offset += bytesToRead;
        result.verificationBytesRead += bytesToRead;
    }
    if (crypto_sign_ed25519ph_final_verify(
            &signatureState, result.package.signature.data(), publicKey.data()) != 0)
    {
        result.issues.push_back("V3 Ed25519ph publisher signature verification failed");
        return result;
    }

    result.package.signatureVerified = true;
    result.opened = true;
    return result;
}

PackageV3FileRecordOpenResult openPackageV3FileRecord(
    const PackageV3FileOpenResult& file,
    const std::vector<std::uint8_t>& contentKey,
    const PackageV3RecordDescriptor& record)
{
    namespace fs = std::filesystem;
    PackageV3FileRecordOpenResult result;
    if (! file.opened || ! file.package.opened || ! file.package.signatureVerified
        || contentKey.size() != securePackageKeySizeBytes
        || record.ordinal >= file.package.records.size())
    {
        result.issues.push_back("V3 file, signature, content key, or record identity is invalid");
        return result;
    }
    const auto& trusted = file.package.records[record.ordinal];
    if (! sameIndexedDescriptor(record, trusted))
    {
        result.issues.push_back("V3 record descriptor is not part of the verified file index");
        return result;
    }
    if (trusted.ciphertextSize > packageV3MaximumRecordBytes
        || trusted.ciphertextSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    {
        result.issues.push_back("V3 requested record exceeds the allocation ceiling");
        return result;
    }

    std::error_code error;
    if (fs::file_size(fs::path(file.packagePath), error) != file.packageBytes || error)
    {
        result.issues.push_back("V3 package changed after signature verification");
        return result;
    }
    std::ifstream input(fs::path(file.packagePath), std::ios::binary);
    auto openedPackage = file.package;
    auto& openedRecord = openedPackage.records[record.ordinal];
    openedRecord.sealed.ciphertext.resize(static_cast<std::size_t>(openedRecord.ciphertextSize));
    if (! input || ! readExact(input, openedRecord.ciphertextOffset,
                               openedRecord.sealed.ciphertext.data(),
                               openedRecord.sealed.ciphertext.size()))
    {
        openedRecord.sealed.ciphertext.clear();
        result.issues.push_back("V3 requested ciphertext range is truncated");
        return result;
    }
    result.ciphertextBytesRead = openedRecord.ciphertextSize;
    std::string issue;
    if (! openPackageV3Record(
            contentKey, openedPackage, openedRecord, result.plaintext, issue))
    {
        result.plaintext.clear();
        result.issues.push_back(issue);
        return result;
    }
    result.opened = true;
    return result;
}
} // namespace drs::engine
