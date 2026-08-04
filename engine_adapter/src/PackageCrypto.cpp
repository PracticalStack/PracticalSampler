#include "drs/engine/PackageCrypto.h"

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace drs::engine
{
namespace
{
constexpr std::string_view kAlgorithmId = "drs.sha256.stream-seal.v1";
constexpr std::string_view kDeterministicSeed = "DecentRhapsodyStudio.PackageCrypto.Sprint3.InternalSeed";
constexpr std::size_t kNonceSizeBytes = 24;
constexpr std::size_t kTagSizeBytes = 16;

std::vector<std::uint8_t> toBytes(std::string_view text)
{
    return std::vector<std::uint8_t>(text.begin(), text.end());
}

void appendBytes(std::vector<std::uint8_t>& target, const std::vector<std::uint8_t>& bytes)
{
    target.insert(target.end(), bytes.begin(), bytes.end());
}

void appendBytes(std::vector<std::uint8_t>& target, std::string_view text)
{
    target.insert(target.end(), text.begin(), text.end());
}

void appendSeparator(std::vector<std::uint8_t>& target)
{
    target.push_back(0xffu);
}

void appendUint64LittleEndian(std::vector<std::uint8_t>& target, const std::uint64_t value)
{
    for (std::size_t index = 0; index < sizeof(value); ++index)
        target.push_back(static_cast<std::uint8_t>((value >> (index * 8u)) & 0xffu));
}

std::uint64_t mix64(std::uint64_t value) noexcept
{
    value += 0x9e3779b97f4a7c15ull;
    value = (value ^ (value >> 30u)) * 0xbf58476d1ce4e5b9ull;
    value = (value ^ (value >> 27u)) * 0x94d049bb133111ebull;
    return value ^ (value >> 31u);
}

std::uint64_t computeFnv1a64(const std::vector<std::uint8_t>& bytes) noexcept
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    std::uint64_t hash = offsetBasis;
    for (const auto byte : bytes)
    {
        hash ^= byte;
        hash *= prime;
    }

    return hash;
}

std::vector<std::uint8_t> expandDigestWords(const std::vector<std::uint8_t>& bytes, const std::size_t outputSizeBytes)
{
    std::vector<std::uint8_t> output;
    output.reserve(outputSizeBytes);

    auto state = computeFnv1a64(bytes) ^ mix64(static_cast<std::uint64_t>(bytes.size()));
    while (output.size() < outputSizeBytes)
    {
        state = mix64(state ^ 0xa0761d6478bd642full ^ static_cast<std::uint64_t>(output.size()));
        for (std::size_t index = 0; index < sizeof(state) && output.size() < outputSizeBytes; ++index)
            output.push_back(static_cast<std::uint8_t>((state >> (index * 8u)) & 0xffu));
    }

    return output;
}

std::vector<std::uint8_t> buildDomainBytes(std::string_view packageId,
                                           std::string_view recordId,
                                           std::string_view additionalAuthenticatedData)
{
    std::vector<std::uint8_t> bytes;
    appendBytes(bytes, kDeterministicSeed);
    appendSeparator(bytes);
    appendBytes(bytes, kAlgorithmId);
    appendSeparator(bytes);
    appendBytes(bytes, packageId);
    appendSeparator(bytes);
    appendBytes(bytes, recordId);
    appendSeparator(bytes);
    appendBytes(bytes, additionalAuthenticatedData);
    return bytes;
}

std::vector<std::uint8_t> deriveNonceBytes(std::string_view packageId,
                                           std::string_view recordId,
                                           std::string_view additionalAuthenticatedData)
{
    auto domain = buildDomainBytes(packageId, recordId, additionalAuthenticatedData);
    appendSeparator(domain);
    appendBytes(domain, "nonce");
    return expandDigestWords(domain, kNonceSizeBytes);
}

std::vector<std::uint8_t> computeKeystreamBlock(const std::vector<std::uint8_t>& nonce,
                                                std::string_view packageId,
                                                std::string_view recordId,
                                                std::string_view additionalAuthenticatedData,
                                                const std::uint64_t blockIndex)
{
    auto bytes = buildDomainBytes(packageId, recordId, additionalAuthenticatedData);
    appendSeparator(bytes);
    appendBytes(bytes, "stream");
    appendSeparator(bytes);
    appendBytes(bytes, nonce);
    appendSeparator(bytes);
    appendUint64LittleEndian(bytes, blockIndex);
    return expandDigestWords(bytes, 32);
}

std::vector<std::uint8_t> xorWithDeterministicKeystream(const std::vector<std::uint8_t>& input,
                                                        const std::vector<std::uint8_t>& nonce,
                                                        std::string_view packageId,
                                                        std::string_view recordId,
                                                        std::string_view additionalAuthenticatedData)
{
    std::vector<std::uint8_t> output(input.size(), 0);
    std::size_t inputOffset = 0;
    std::uint64_t blockIndex = 0;

    while (inputOffset < input.size())
    {
        const auto keystreamBlock = computeKeystreamBlock(nonce,
                                                          packageId,
                                                          recordId,
                                                          additionalAuthenticatedData,
                                                          blockIndex++);
        const auto chunkSize = std::min<std::size_t>(keystreamBlock.size(), input.size() - inputOffset);
        for (std::size_t index = 0; index < chunkSize; ++index)
            output[inputOffset + index] = input[inputOffset + index] ^ keystreamBlock[index];

        inputOffset += chunkSize;
    }

    return output;
}

std::vector<std::uint8_t> computeAuthenticationTag(const std::vector<std::uint8_t>& nonce,
                                                   const std::vector<std::uint8_t>& ciphertext,
                                                   std::string_view packageId,
                                                   std::string_view recordId,
                                                   std::string_view additionalAuthenticatedData)
{
    auto bytes = buildDomainBytes(packageId, recordId, additionalAuthenticatedData);
    appendSeparator(bytes);
    appendBytes(bytes, "tag");
    appendSeparator(bytes);
    appendBytes(bytes, nonce);
    appendSeparator(bytes);
    appendBytes(bytes, ciphertext);
    return expandDigestWords(bytes, kTagSizeBytes);
}

bool constantTimeEquals(const std::vector<std::uint8_t>& left, const std::vector<std::uint8_t>& right) noexcept
{
    if (left.size() != right.size())
        return false;

    std::uint8_t delta = 0;
    for (std::size_t index = 0; index < left.size(); ++index)
        delta |= static_cast<std::uint8_t>(left[index] ^ right[index]);

    return delta == 0;
}

class DeterministicPackageCryptoProvider final : public PackageCryptoProvider
{
public:
    const char* algorithmId() const noexcept override
    {
        return kAlgorithmId.data();
    }

    std::size_t nonceSizeBytes() const noexcept override
    {
        return kNonceSizeBytes;
    }

    std::size_t tagSizeBytes() const noexcept override
    {
        return kTagSizeBytes;
    }

    bool seal(const PackageSealRequest& request,
              PackageSealedBlob& output,
              std::string& issue) const override
    {
        if (request.packageId.empty())
        {
            issue = "Package crypto seal requires a non-empty packageId.";
            return false;
        }

        if (request.recordId.empty())
        {
            issue = "Package crypto seal requires a non-empty recordId.";
            return false;
        }

        output.nonce = deriveNonceBytes(request.packageId,
                                        request.recordId,
                                        request.additionalAuthenticatedData);
        output.ciphertext = xorWithDeterministicKeystream(request.plaintext,
                                                          output.nonce,
                                                          request.packageId,
                                                          request.recordId,
                                                          request.additionalAuthenticatedData);
        output.tag = computeAuthenticationTag(output.nonce,
                                              output.ciphertext,
                                              request.packageId,
                                              request.recordId,
                                              request.additionalAuthenticatedData);
        issue.clear();
        return true;
    }

    bool open(const PackageOpenRequest& request,
              std::vector<std::uint8_t>& plaintext,
              std::string& issue) const override
    {
        if (request.packageId.empty())
        {
            issue = "Package crypto open requires a non-empty packageId.";
            return false;
        }

        if (request.recordId.empty())
        {
            issue = "Package crypto open requires a non-empty recordId.";
            return false;
        }

        if (request.sealed.nonce.size() != nonceSizeBytes())
        {
            issue = "Package crypto open received a nonce with an unexpected size.";
            return false;
        }

        if (request.sealed.tag.size() != tagSizeBytes())
        {
            issue = "Package crypto open received an authentication tag with an unexpected size.";
            return false;
        }

        const auto expectedTag = computeAuthenticationTag(request.sealed.nonce,
                                                          request.sealed.ciphertext,
                                                          request.packageId,
                                                          request.recordId,
                                                          request.additionalAuthenticatedData);
        if (!constantTimeEquals(request.sealed.tag, expectedTag))
        {
            issue = "Package crypto authentication tag mismatch.";
            return false;
        }

        plaintext = xorWithDeterministicKeystream(request.sealed.ciphertext,
                                                  request.sealed.nonce,
                                                  request.packageId,
                                                  request.recordId,
                                                  request.additionalAuthenticatedData);
        issue.clear();
        return true;
    }
};
} // namespace

const PackageCryptoProvider& getDeterministicPackageCryptoProvider()
{
    static const DeterministicPackageCryptoProvider provider;
    return provider;
}
} // namespace drs::engine
