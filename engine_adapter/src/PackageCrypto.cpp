#include "drs/engine/PackageCrypto.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace drs::engine
{
namespace
{
constexpr std::string_view kAlgorithmId = "drs.sha256.stream-seal.v1";
constexpr std::string_view kDeterministicSeed = "DecentRhapsodyStudio.PackageCrypto.Sprint3.InternalSeed";
constexpr std::size_t kNonceSizeBytes = 24;
constexpr std::size_t kTagSizeBytes = 16;
constexpr std::uint64_t kFnv1aOffsetBasis = 14695981039346656037ull;
constexpr std::uint64_t kFnv1aPrime = 1099511628211ull;
constexpr std::uint8_t kSeparatorByte = 0xffu;

std::uint64_t mix64(std::uint64_t value) noexcept
{
    value += 0x9e3779b97f4a7c15ull;
    value = (value ^ (value >> 30u)) * 0xbf58476d1ce4e5b9ull;
    value = (value ^ (value >> 27u)) * 0x94d049bb133111ebull;
    return value ^ (value >> 31u);
}

struct Fnv1a64State
{
    std::uint64_t hash = kFnv1aOffsetBasis;
    std::uint64_t sizeBytes = 0;
};

void appendHashBytes(Fnv1a64State& state, const std::uint8_t* bytes, const std::size_t byteCount) noexcept
{
    for (std::size_t index = 0; index < byteCount; ++index)
    {
        state.hash ^= bytes[index];
        state.hash *= kFnv1aPrime;
    }
    state.sizeBytes += static_cast<std::uint64_t>(byteCount);
}

void appendHashBytes(Fnv1a64State& state, const std::vector<std::uint8_t>& bytes) noexcept
{
    if (!bytes.empty())
        appendHashBytes(state, bytes.data(), bytes.size());
}

void appendHashBytes(Fnv1a64State& state, std::string_view text) noexcept
{
    if (!text.empty())
    {
        appendHashBytes(state,
                        reinterpret_cast<const std::uint8_t*>(text.data()),
                        text.size());
    }
}

void appendSeparator(Fnv1a64State& state) noexcept
{
    appendHashBytes(state, &kSeparatorByte, 1);
}

void appendUint64LittleEndian(Fnv1a64State& state, const std::uint64_t value) noexcept
{
    std::uint8_t bytes[sizeof(value)] {};
    for (std::size_t index = 0; index < sizeof(value); ++index)
        bytes[index] = static_cast<std::uint8_t>((value >> (index * 8u)) & 0xffu);
    appendHashBytes(state, bytes, sizeof(bytes));
}

std::vector<std::uint8_t> expandDigestWords(const Fnv1a64State& inputState,
                                            const std::size_t outputSizeBytes)
{
    std::vector<std::uint8_t> output;
    output.reserve(outputSizeBytes);

    auto state = inputState.hash ^ mix64(inputState.sizeBytes);
    while (output.size() < outputSizeBytes)
    {
        state = mix64(state ^ 0xa0761d6478bd642full ^ static_cast<std::uint64_t>(output.size()));
        for (std::size_t index = 0; index < sizeof(state) && output.size() < outputSizeBytes; ++index)
            output.push_back(static_cast<std::uint8_t>((state >> (index * 8u)) & 0xffu));
    }

    return output;
}

Fnv1a64State buildDomainState(std::string_view packageId,
                              std::string_view recordId,
                              std::string_view additionalAuthenticatedData) noexcept
{
    Fnv1a64State state;
    appendHashBytes(state, kDeterministicSeed);
    appendSeparator(state);
    appendHashBytes(state, kAlgorithmId);
    appendSeparator(state);
    appendHashBytes(state, packageId);
    appendSeparator(state);
    appendHashBytes(state, recordId);
    appendSeparator(state);
    appendHashBytes(state, additionalAuthenticatedData);
    return state;
}

std::vector<std::uint8_t> deriveNonceBytes(std::string_view packageId,
                                           std::string_view recordId,
                                           std::string_view additionalAuthenticatedData)
{
    auto domain = buildDomainState(packageId, recordId, additionalAuthenticatedData);
    appendSeparator(domain);
    appendHashBytes(domain, "nonce");
    return expandDigestWords(domain, kNonceSizeBytes);
}

void xorWithExpandedDigest(std::vector<std::uint8_t>& output,
                           const std::vector<std::uint8_t>& input,
                           const std::size_t outputOffset,
                           const Fnv1a64State& seedState)
{
    auto state = seedState.hash ^ mix64(seedState.sizeBytes);
    auto generatedBytes = std::size_t { 0 };
    const auto chunkSize = std::min<std::size_t>(32, input.size() - outputOffset);
    while (generatedBytes < chunkSize)
    {
        state = mix64(state ^ 0xa0761d6478bd642full ^ static_cast<std::uint64_t>(generatedBytes));
        for (std::size_t index = 0; index < sizeof(state) && generatedBytes < chunkSize; ++index, ++generatedBytes)
        {
            output[outputOffset + generatedBytes]
                = input[outputOffset + generatedBytes]
                ^ static_cast<std::uint8_t>((state >> (index * 8u)) & 0xffu);
        }
    }
}

std::vector<std::uint8_t> xorWithDeterministicKeystream(const std::vector<std::uint8_t>& input,
                                                        const std::vector<std::uint8_t>& nonce,
                                                        std::string_view packageId,
                                                        std::string_view recordId,
                                                        std::string_view additionalAuthenticatedData)
{
    std::vector<std::uint8_t> output(input.size(), 0);
    auto keystreamPrefixState = buildDomainState(packageId, recordId, additionalAuthenticatedData);
    appendSeparator(keystreamPrefixState);
    appendHashBytes(keystreamPrefixState, "stream");
    appendSeparator(keystreamPrefixState);
    appendHashBytes(keystreamPrefixState, nonce);
    appendSeparator(keystreamPrefixState);

    for (std::size_t inputOffset = 0, blockIndex = 0; inputOffset < input.size(); inputOffset += 32, ++blockIndex)
    {
        auto blockState = keystreamPrefixState;
        appendUint64LittleEndian(blockState, static_cast<std::uint64_t>(blockIndex));
        xorWithExpandedDigest(output, input, inputOffset, blockState);
    }

    return output;
}

std::vector<std::uint8_t> computeAuthenticationTag(const std::vector<std::uint8_t>& nonce,
                                                   const std::vector<std::uint8_t>& ciphertext,
                                                    std::string_view packageId,
                                                    std::string_view recordId,
                                                    std::string_view additionalAuthenticatedData)
{
    auto tagState = buildDomainState(packageId, recordId, additionalAuthenticatedData);
    appendSeparator(tagState);
    appendHashBytes(tagState, "tag");
    appendSeparator(tagState);
    appendHashBytes(tagState, nonce);
    appendSeparator(tagState);
    appendHashBytes(tagState, ciphertext);
    return expandDigestWords(tagState, kTagSizeBytes);
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
