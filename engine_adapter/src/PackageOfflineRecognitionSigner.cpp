#include <drs/engine/PackageOfflineRecognitionSigner.h>

#include <drs/engine/PackageCrypto.h>
#include <drs/engine/PackageOfflineProtection.generated.h>
#include <drs/engine/PackageSignature.h>
#include <drs/engine/PackageV3.h>

#include <array>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

#include <sodium/utils.h>

namespace drs::engine
{
namespace
{
bool hasReservedIdentifierToken(const std::string_view value) noexcept
{
    constexpr std::array<std::string_view, 7> reserved {
        "test", "tests", "dev", "debug", "fixture", "example", "development" };
    std::size_t tokenStart = 0;
    while (tokenStart < value.size())
    {
        while (tokenStart < value.size()
               && (value[tokenStart] == '.' || value[tokenStart] == '_'
                   || value[tokenStart] == '-'))
            ++tokenStart;
        const auto tokenEnd = value.find_first_of("._-", tokenStart);
        const auto token = value.substr(tokenStart,
                                         tokenEnd == std::string_view::npos
                                             ? value.size() - tokenStart
                                             : tokenEnd - tokenStart);
        for (const auto reservedToken : reserved)
        {
            if (token.size() != reservedToken.size()) continue;
            bool equal = true;
            for (std::size_t index = 0; index < token.size(); ++index)
            {
                auto character = token[index];
                if (character >= 'A' && character <= 'Z')
                    character = static_cast<char>(character - 'A' + 'a');
                if (character != reservedToken[index])
                {
                    equal = false;
                    break;
                }
            }
            if (equal) return true;
        }
        if (tokenEnd == std::string_view::npos) break;
        tokenStart = tokenEnd;
    }
    return false;
}

bool hasUsableSigningFragments() noexcept
{
    bool hasNonZeroByte = false;
    for (std::size_t index = 0; index < packageEd25519PrivateKeyBytes; ++index)
        hasNonZeroByte = hasNonZeroByte
            || ((offline_generated::recognitionSigningKey.mask[index]
                 ^ offline_generated::recognitionSigningKey.xorFragment[index]) != 0);
    return hasNonZeroByte;
}

bool configured() noexcept
{
    const auto& signingKey = offline_generated::recognitionSigningKey;
    return offline_generated::profileId != nullptr
        && ! std::string_view(offline_generated::profileId).empty()
        && ! hasReservedIdentifierToken(offline_generated::profileId)
        && signingKey.keyId != nullptr && ! std::string_view(signingKey.keyId).empty()
        && ! hasReservedIdentifierToken(signingKey.keyId)
        && signingKey.state != nullptr && std::string_view(signingKey.state) == "active"
        && signingKey.activatedUtc != nullptr
        && ! std::string_view(signingKey.activatedUtc).empty()
        && signingKey.retiredUtc == nullptr && signingKey.revokedUtc == nullptr
        && hasUsableSigningFragments();
}

bool loadPrivateKey(SecureBuffer& key)
{
    key.clear();
    if (! configured()) return false;
    std::array<std::uint8_t, packageEd25519PrivateKeyBytes> reconstructed {};
    for (std::size_t index = 0; index < reconstructed.size(); ++index)
    {
        reconstructed[index] = offline_generated::recognitionSigningKey.mask[index]
            ^ offline_generated::recognitionSigningKey.xorFragment[index];
    }
    try
    {
        key = SecureBuffer(std::vector<std::uint8_t>(
            reconstructed.begin(), reconstructed.end()));
    }
    catch (...)
    {
        sodium_memzero(reconstructed.data(), reconstructed.size());
        throw;
    }
    sodium_memzero(reconstructed.data(), reconstructed.size());
    return key.size() == packageEd25519PrivateKeyBytes;
}

std::uint64_t readU64(const std::uint8_t* bytes) noexcept
{
    std::uint64_t value = 0;
    for (int index = 0; index < 8; ++index)
        value |= static_cast<std::uint64_t>(bytes[index]) << (index * 8);
    return value;
}

bool readExact(std::ifstream& input, std::uint8_t* destination, const std::size_t size)
{
    if (size > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
        return false;
    input.read(reinterpret_cast<char*>(destination), static_cast<std::streamsize>(size));
    return input.good() || (input.eof() && input.gcount() == static_cast<std::streamsize>(size));
}

bool validateCanonicalBytes(const std::vector<std::uint8_t>& bytes,
                            const std::string& keyId,
                            std::string& issue)
{
    if (bytes.size() < packageV3FixedHeaderBytes)
    {
        issue = "offline package-recognition signer rejected a non-canonical package region";
        return false;
    }
    const auto payloadOffset = readU64(bytes.data() + 44u);
    if (payloadOffset > bytes.size())
    {
        issue = "offline package-recognition signer rejected a non-canonical package region";
        return false;
    }
    std::vector<std::uint8_t> index(
        bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset));
    const auto parsed = parsePackageV3Index(
        index, static_cast<std::uint64_t>(bytes.size()) + packageEd25519SignatureBytes);
    if (! parsed.opened || parsed.signatureOffset != bytes.size()
        || parsed.signingKeyId != keyId)
    {
        issue = "offline package-recognition signer rejected a non-canonical package region";
        return false;
    }
    return true;
}

bool validateCanonicalFile(const std::string& path,
                           const std::uint64_t expectedBytes,
                           const std::string& keyId,
                           std::string& issue)
{
    namespace fs = std::filesystem;
    std::error_code error;
    const auto fileBytes = fs::file_size(fs::path(path), error);
    if (error || fileBytes != expectedBytes
        || fileBytes < packageV3FixedHeaderBytes
        || fileBytes > packageV3MaximumPackageBytes - packageEd25519SignatureBytes)
    {
        issue = "offline package-recognition signer rejected a non-canonical package file";
        return false;
    }
    std::ifstream input(fs::path(path), std::ios::binary);
    std::array<std::uint8_t, packageV3FixedHeaderBytes> fixedHeader {};
    if (! input || ! readExact(input, fixedHeader.data(), fixedHeader.size()))
    {
        issue = "offline package-recognition signer could not read the canonical package file";
        return false;
    }
    const auto payloadOffset = readU64(fixedHeader.data() + 44u);
    if (payloadOffset < fixedHeader.size() || payloadOffset > fileBytes
        || payloadOffset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    {
        issue = "offline package-recognition signer rejected a non-canonical package file";
        return false;
    }
    std::vector<std::uint8_t> index(static_cast<std::size_t>(payloadOffset));
    std::copy(fixedHeader.begin(), fixedHeader.end(), index.begin());
    if (! readExact(input, index.data() + fixedHeader.size(),
                    index.size() - fixedHeader.size()))
    {
        issue = "offline package-recognition signer could not read the canonical package index";
        return false;
    }
    const auto parsed = parsePackageV3Index(
        index, fileBytes + packageEd25519SignatureBytes);
    if (! parsed.opened || parsed.signatureOffset != fileBytes
        || parsed.signingKeyId != keyId)
    {
        issue = "offline package-recognition signer rejected a non-canonical package file";
        return false;
    }
    return true;
}
} // namespace

bool OfflinePackageRecognitionSigner::isConfigured() const noexcept
{
    return configured();
}

bool OfflinePackageRecognitionSigner::signCanonicalPackage(
    const PackagePublisherSigningRequest& request,
    PackagePublisherSigningResponse& response,
    std::string& issue) const
{
    response = {};
    issue.clear();
    if (! configured())
    {
        issue = "offline package-recognition signer is not configured";
        return false;
    }
    if (request.signingKeyId != offline_generated::recognitionSigningKey.keyId)
    {
        issue = "offline package-recognition signing key id is unknown";
        return false;
    }
    const bool hasMemoryInput = request.canonicalSignedBytes != nullptr;
    const bool hasFileInput = ! request.canonicalSignedFilePath.empty();
    if (hasMemoryInput == hasFileInput)
    {
        issue = "offline package-recognition signing requires exactly one canonical input";
        return false;
    }
    if ((hasMemoryInput && ! validateCanonicalBytes(
            *request.canonicalSignedBytes, request.signingKeyId, issue))
        || (hasFileInput && ! validateCanonicalFile(
            request.canonicalSignedFilePath, request.canonicalSignedBytesLength,
            request.signingKeyId, issue)))
        return false;

    SecureBuffer privateKey;
    if (! loadPrivateKey(privateKey))
    {
        issue = "offline package-recognition signing key is unavailable";
        return false;
    }
    const bool signedSuccessfully = hasMemoryInput
        ? packageSignEd25519ph(privateKey.bytes(), *request.canonicalSignedBytes,
                               response.signature, issue)
        : packageSignEd25519phFile(privateKey.bytes(), request.canonicalSignedFilePath,
                                   request.canonicalSignedBytesLength,
                                   response.signature, issue);
    if (! signedSuccessfully)
    {
        response = {};
        if (issue.empty()) issue = "offline package-recognition signing failed";
        return false;
    }
    response.auditId = "offline-recognition:" +
        std::string(offline_generated::recognitionSigningKey.keyId);
    return true;
}

const char* offlinePackageRecognitionSigningKeyId() noexcept
{
    return offline_generated::recognitionSigningKey.keyId == nullptr
        ? "" : offline_generated::recognitionSigningKey.keyId;
}

bool offlinePackageRecognitionSigningConfigured() noexcept
{
    return configured();
}
} // namespace drs::engine
