#include "drs/engine/PackageCrypto.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string_view>

#include <sodium/core.h>
#include <sodium/crypto_aead_xchacha20poly1305.h>
#include <sodium/randombytes.h>

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

constexpr std::string_view kSecureAlgorithmId = "drs.xchacha20poly1305.ietf.v1";

bool ensureSodium(std::string& issue) noexcept
{
    static std::once_flag initializationFlag;
    static int initializationResult = -1;
    std::call_once(initializationFlag, [] { initializationResult = sodium_init(); });
    if (initializationResult < 0)
    {
        issue = "The package crypto library could not initialize its secure random source.";
        return false;
    }
    return true;
}

struct SecureBytesView
{
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
};

SecureBytesView encryptionKeyView(const PackageSealRequest& request) noexcept
{
    if (request.secureEncryptionKey != nullptr)
        return { request.secureEncryptionKey->data(), request.secureEncryptionKey->size() };
    return {};
}

SecureBytesView encryptionKeyView(const PackageOpenRequest& request) noexcept
{
    if (request.secureEncryptionKey != nullptr)
        return { request.secureEncryptionKey->data(), request.secureEncryptionKey->size() };
    return {};
}

SecureBytesView plaintextView(const PackageSealRequest& request) noexcept
{
    if (request.securePlaintext != nullptr)
        return { request.securePlaintext->data(), request.securePlaintext->size() };
    return { request.plaintext.data(), request.plaintext.size() };
}

bool validateSecureKey(const SecureBytesView key,
                       std::string& issue) noexcept
{
    if (key.size != crypto_aead_xchacha20poly1305_ietf_KEYBYTES)
    {
        issue = "Secure package crypto requires a 32-byte encryption key.";
        return false;
    }
    return true;
}

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

class SecurePackageCryptoProvider final : public PackageCryptoProvider
{
public:
    const char* algorithmId() const noexcept override
    {
        return kSecureAlgorithmId.data();
    }

    std::size_t nonceSizeBytes() const noexcept override
    {
        return crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    }

    std::size_t tagSizeBytes() const noexcept override
    {
        return crypto_aead_xchacha20poly1305_ietf_ABYTES;
    }

    bool seal(const PackageSealRequest& request,
              PackageSealedBlob& output,
              std::string& issue) const override
    {
        output = {};
        if (request.packageId.empty())
        {
            issue = "Secure package crypto seal requires a non-empty packageId.";
            return false;
        }
        if (request.recordId.empty())
        {
            issue = "Secure package crypto seal requires a non-empty recordId.";
            return false;
        }
        const auto encryptionKey = encryptionKeyView(request);
        const auto plaintext = plaintextView(request);
        if (!validateSecureKey(encryptionKey, issue)
            || !ensureSodium(issue))
            return false;

        if (! generateSecurePackageNonce(output.nonce, issue))
            return false;

        std::vector<std::uint8_t> ciphertextWithTag(
            plaintext.size + tagSizeBytes());
        unsigned long long ciphertextSize = 0;
        const auto* aad = reinterpret_cast<const unsigned char*>(
            request.additionalAuthenticatedData.data());
        const auto aadSize = static_cast<unsigned long long>(
            request.additionalAuthenticatedData.size());
        const auto result = crypto_aead_xchacha20poly1305_ietf_encrypt(
            ciphertextWithTag.data(), &ciphertextSize,
            plaintext.data,
            static_cast<unsigned long long>(plaintext.size),
            request.additionalAuthenticatedData.empty() ? nullptr : aad,
            aadSize,
            nullptr,
            output.nonce.data(),
            encryptionKey.data);
        if (result != 0 || ciphertextSize < tagSizeBytes())
        {
            issue = "Secure package crypto encryption failed.";
            output = {};
            return false;
        }

        ciphertextWithTag.resize(static_cast<std::size_t>(ciphertextSize));
        const auto tagOffset = ciphertextWithTag.size() - tagSizeBytes();
        output.ciphertext.assign(ciphertextWithTag.begin(),
                                 ciphertextWithTag.begin()
                                     + static_cast<std::ptrdiff_t>(tagOffset));
        output.tag.assign(ciphertextWithTag.begin()
                              + static_cast<std::ptrdiff_t>(tagOffset),
                          ciphertextWithTag.end());
        issue.clear();
        return true;
    }

    bool open(const PackageOpenRequest& request,
              std::vector<std::uint8_t>& plaintext,
              std::string& issue) const override
    {
        plaintext.clear();
        if (request.packageId.empty())
        {
            issue = "Secure package crypto open requires a non-empty packageId.";
            return false;
        }
        if (request.recordId.empty())
        {
            issue = "Secure package crypto open requires a non-empty recordId.";
            return false;
        }
        if (request.sealed.nonce.size() != nonceSizeBytes())
        {
            issue = "Secure package crypto open received a nonce with an unexpected size.";
            return false;
        }
        if (request.sealed.tag.size() != tagSizeBytes())
        {
            issue = "Secure package crypto open received an authentication tag with an unexpected size.";
            return false;
        }
        const auto encryptionKey = encryptionKeyView(request);
        if (!validateSecureKey(encryptionKey, issue)
            || !ensureSodium(issue))
            return false;

        std::vector<std::uint8_t> ciphertextWithTag;
        ciphertextWithTag.reserve(request.sealed.ciphertext.size() + request.sealed.tag.size());
        ciphertextWithTag.insert(ciphertextWithTag.end(),
                                 request.sealed.ciphertext.begin(),
                                 request.sealed.ciphertext.end());
        ciphertextWithTag.insert(ciphertextWithTag.end(),
                                 request.sealed.tag.begin(),
                                 request.sealed.tag.end());
        plaintext.resize(request.sealed.ciphertext.size());
        unsigned long long plaintextSize = 0;
        const auto* aad = reinterpret_cast<const unsigned char*>(
            request.additionalAuthenticatedData.data());
        const auto aadSize = static_cast<unsigned long long>(
            request.additionalAuthenticatedData.size());
        const auto result = crypto_aead_xchacha20poly1305_ietf_decrypt(
            plaintext.data(), &plaintextSize,
            nullptr,
            ciphertextWithTag.data(),
            static_cast<unsigned long long>(ciphertextWithTag.size()),
            request.additionalAuthenticatedData.empty() ? nullptr : aad,
            aadSize,
            request.sealed.nonce.data(),
            encryptionKey.data);
        if (result != 0)
        {
            plaintext.clear();
            issue = "Secure package crypto authentication failed.";
            return false;
        }

        plaintext.resize(static_cast<std::size_t>(plaintextSize));
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

const PackageCryptoProvider& getSecurePackageCryptoProvider()
{
    static const SecurePackageCryptoProvider provider;
    return provider;
}

bool generateSecurePackageKey(SecureBuffer& key,
                              std::string& issue)
{
    key.clear();
    if (! ensureSodium(issue))
        return false;
    std::vector<std::uint8_t> bytes;
    bytes.resize(securePackageKeySizeBytes);
    randombytes_buf(bytes.data(), bytes.size());
    key = SecureBuffer(std::move(bytes));
    issue.clear();
    return true;
}

bool generateSecurePackageNonce(std::vector<std::uint8_t>& nonce,
                                std::string& issue)
{
    nonce.clear();
    if (! ensureSodium(issue))
        return false;
    nonce.resize(securePackageNonceSizeBytes);
    randombytes_buf(nonce.data(), nonce.size());
    issue.clear();
    return true;
}
} // namespace drs::engine
