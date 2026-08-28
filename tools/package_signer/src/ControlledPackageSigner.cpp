#include <drs/signing/ControlledPackageSigner.h>

#include <drs/engine/PackageSignature.h>
#include <drs/engine/PackageV3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_sign_ed25519.h>
#include <sodium/core.h>

namespace drs::signing
{
namespace
{
std::uint64_t readU64(const std::uint8_t* bytes) noexcept
{
    std::uint64_t value = 0;
    for (int index = 0; index < 8; ++index)
        value |= static_cast<std::uint64_t>(bytes[index]) << (index * 8);
    return value;
}

bool validKeyId(const std::string& value)
{
    return ! value.empty() && value.size() <= drs::engine::packageV3MaximumIdentityBytes
        && std::all_of(value.begin(), value.end(), [](const unsigned char character)
        {
            return (character >= 'a' && character <= 'z')
                || (character >= 'A' && character <= 'Z')
                || (character >= '0' && character <= '9')
                || character == '-' || character == '_' || character == '.';
        });
}

std::string digestHex(const std::vector<std::uint8_t>& bytes)
{
    std::array<std::uint8_t, crypto_hash_sha256_BYTES> digest {};
    if (crypto_hash_sha256(digest.data(), bytes.data(),
                           static_cast<unsigned long long>(bytes.size())) != 0)
        return {};
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (const auto value : digest)
        stream << std::setw(2) << static_cast<unsigned int>(value);
    return stream.str();
}

std::string digestHex(const std::array<std::uint8_t, crypto_hash_sha256_BYTES>& digest)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (const auto value : digest)
        stream << std::setw(2) << static_cast<unsigned int>(value);
    return stream.str();
}

bool readExact(std::ifstream& input, std::uint8_t* destination, const std::size_t size)
{
    if (size > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
        return false;
    input.read(reinterpret_cast<char*>(destination), static_cast<std::streamsize>(size));
    return input.good() || (input.eof() && input.gcount() == static_cast<std::streamsize>(size));
}
} // namespace

ControlledPackageSigner::ControlledPackageSigner(
    std::string signingKeyId,
    const drs::engine::PackageSigningKeyState keyState,
    drs::engine::SecureBuffer privateKey,
    const PackageSigningAuditSink& auditSink)
    : signingKeyId_(std::move(signingKeyId)),
      keyState_(keyState),
      privateKey_(std::move(privateKey)),
      auditSink_(auditSink)
{
}

bool ControlledPackageSigner::signCanonicalPackage(
    const drs::engine::PackagePublisherSigningRequest& request,
    drs::engine::PackagePublisherSigningResponse& response,
    std::string& issue) const
{
    using namespace drs::engine;
    response = {};
    const auto hasBytes = request.canonicalSignedBytes != nullptr;
    const auto hasFile = ! request.canonicalSignedFilePath.empty();
    if (! validKeyId(signingKeyId_)
        || keyState_ != PackageSigningKeyState::active
        || request.signingKeyId != signingKeyId_
        || hasBytes == hasFile
        || privateKey_.size() != packageEd25519PrivateKeyBytes)
    {
        issue = "controlled publisher signing request is invalid";
        return false;
    }
    std::string digest;
    std::uint64_t canonicalBytes = 0;
    if (hasBytes)
    {
        const auto& bytes = *request.canonicalSignedBytes;
        if (bytes.size() < packageV3FixedHeaderBytes)
        {
            issue = "controlled publisher signer rejected a non-canonical package region";
            return false;
        }
        const auto payloadOffset = readU64(bytes.data() + 44u);
        if (payloadOffset > bytes.size()
            || payloadOffset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        {
            issue = "controlled publisher signer rejected a non-canonical package region";
            return false;
        }
        std::vector<std::uint8_t> index(
            bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset));
        const auto parsed = parsePackageV3Index(
            index, static_cast<std::uint64_t>(bytes.size()) + packageEd25519SignatureBytes);
        if (! parsed.opened || parsed.signatureOffset != bytes.size()
            || parsed.signingKeyId != signingKeyId_)
        {
            issue = "controlled publisher signer rejected a non-canonical package region";
            return false;
        }
        if (! packageSignEd25519ph(
                privateKey_.bytes(), bytes, response.signature, issue))
            return false;
        digest = digestHex(bytes);
        canonicalBytes = bytes.size();
    }
    else
    {
        namespace fs = std::filesystem;
        std::error_code error;
        const auto fileBytes = fs::file_size(fs::path(request.canonicalSignedFilePath), error);
        if (error || fileBytes != request.canonicalSignedBytesLength
            || fileBytes < packageV3FixedHeaderBytes
            || fileBytes > packageV3MaximumPackageBytes - packageEd25519SignatureBytes
            || sodium_init() < 0)
        {
            issue = "controlled publisher signer rejected a non-canonical package file";
            return false;
        }
        std::ifstream input(fs::path(request.canonicalSignedFilePath), std::ios::binary);
        std::array<std::uint8_t, packageV3FixedHeaderBytes> fixedHeader {};
        if (! input || ! readExact(input, fixedHeader.data(), fixedHeader.size()))
        {
            issue = "controlled publisher signer could not read the canonical package file";
            return false;
        }
        const auto payloadOffset = readU64(fixedHeader.data() + 44u);
        if (payloadOffset < fixedHeader.size() || payloadOffset > fileBytes
            || payloadOffset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        {
            issue = "controlled publisher signer rejected a non-canonical package file";
            return false;
        }
        std::vector<std::uint8_t> index(static_cast<std::size_t>(payloadOffset));
        std::copy(fixedHeader.begin(), fixedHeader.end(), index.begin());
        if (! readExact(input, index.data() + fixedHeader.size(),
                        index.size() - fixedHeader.size()))
        {
            issue = "controlled publisher signer could not read the canonical package index";
            return false;
        }
        const auto parsed = parsePackageV3Index(
            index, fileBytes + packageEd25519SignatureBytes);
        if (! parsed.opened || parsed.signatureOffset != fileBytes
            || parsed.signingKeyId != signingKeyId_)
        {
            issue = "controlled publisher signer rejected a non-canonical package file";
            return false;
        }
        crypto_sign_ed25519ph_state signatureState;
        crypto_hash_sha256_state digestState;
        if (crypto_sign_ed25519ph_init(&signatureState) != 0
            || crypto_hash_sha256_init(&digestState) != 0)
        {
            issue = "controlled publisher signer could not initialize streaming signing";
            return false;
        }
        input.clear();
        input.seekg(0, std::ios::beg);
        std::vector<std::uint8_t> buffer(64u * 1024u);
        std::uint64_t remaining = fileBytes;
        while (remaining != 0)
        {
            const auto count = static_cast<std::size_t>(
                std::min<std::uint64_t>(remaining, buffer.size()));
            if (! readExact(input, buffer.data(), count)
                || crypto_sign_ed25519ph_update(&signatureState, buffer.data(), count) != 0
                || crypto_hash_sha256_update(&digestState, buffer.data(), count) != 0)
            {
                issue = "controlled publisher signer could not stream the canonical package file";
                return false;
            }
            remaining -= count;
        }
        response.signature.resize(packageEd25519SignatureBytes);
        unsigned long long signatureBytes = 0;
        if (crypto_sign_ed25519ph_final_create(
                &signatureState, response.signature.data(), &signatureBytes,
                privateKey_.data()) != 0
            || signatureBytes != packageEd25519SignatureBytes)
        {
            response = {};
            issue = "controlled publisher signing failed";
            return false;
        }
        std::array<std::uint8_t, crypto_hash_sha256_BYTES> digestBytes {};
        if (crypto_hash_sha256_final(&digestState, digestBytes.data()) != 0)
        {
            response = {};
            issue = "controlled publisher signing audit digest failed";
            return false;
        }
        digest = digestHex(digestBytes);
        canonicalBytes = fileBytes;
    }
    if (digest.empty())
    {
        response = {};
        issue = "controlled publisher signing audit digest failed";
        return false;
    }
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    PackageSigningAuditEvent event;
    event.signingKeyId = signingKeyId_;
    event.canonicalDigestHex = digest;
    event.canonicalBytes = canonicalBytes;
    event.eventUtc = std::to_string(milliseconds);
    event.auditId = signingKeyId_ + ":" + digest.substr(0u, 24u);
    if (! auditSink_.recordPackageSigningEvent(event))
    {
        response = {};
        issue = "controlled publisher signing audit sink is unavailable";
        return false;
    }
    response.auditId = event.auditId;
    issue.clear();
    return true;
}
} // namespace drs::signing
