#include <drs/signing/ControlledPackageSigner.h>

#include <drs/engine/PackageSignature.h>
#include <drs/engine/PackageV3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

#include <sodium/crypto_hash_sha256.h>

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
    if (! validKeyId(signingKeyId_)
        || keyState_ != PackageSigningKeyState::active
        || request.signingKeyId != signingKeyId_
        || request.canonicalSignedBytes == nullptr
        || request.canonicalSignedBytes->size() < packageV3FixedHeaderBytes
        || privateKey_.size() != packageEd25519PrivateKeyBytes)
    {
        issue = "controlled publisher signing request is invalid";
        return false;
    }
    const auto& bytes = *request.canonicalSignedBytes;
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
    const auto digest = digestHex(bytes);
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
    event.canonicalBytes = bytes.size();
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
