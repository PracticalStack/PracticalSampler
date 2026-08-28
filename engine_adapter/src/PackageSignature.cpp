#include <drs/engine/PackageSignature.h>

#include <algorithm>
#include <mutex>
#include <fstream>

#include <sodium/core.h>
#include <sodium/crypto_sign_ed25519.h>

namespace drs::engine
{
namespace
{
bool ensureSodium(std::string& issue)
{
    static std::once_flag flag;
    static int result = -1;
    std::call_once(flag, [] { result = sodium_init(); });
    if (result < 0)
    {
        issue = "libsodium initialization failed";
        return false;
    }
    return true;
}
}

bool generatePackageSigningKeyPair(std::vector<std::uint8_t>& publicKey,
                                   std::vector<std::uint8_t>& privateKey,
                                   std::string& issue)
{
    publicKey.clear();
    privateKey.clear();
    if (! ensureSodium(issue)) return false;
    publicKey.resize(packageEd25519PublicKeyBytes);
    privateKey.resize(packageEd25519PrivateKeyBytes);
    if (crypto_sign_ed25519_keypair(publicKey.data(), privateKey.data()) != 0)
    {
        publicKey.clear();
        privateKey.clear();
        issue = "Ed25519 key generation failed";
        return false;
    }
    issue.clear();
    return true;
}

bool packageSignEd25519(const std::vector<std::uint8_t>& privateKey,
                        const std::vector<std::uint8_t>& message,
                        std::vector<std::uint8_t>& signature,
                        std::string& issue)
{
    signature.clear();
    if (! ensureSodium(issue) || privateKey.size() != packageEd25519PrivateKeyBytes)
    {
        if (issue.empty()) issue = "Ed25519 private key must be 64 bytes";
        return false;
    }
    signature.resize(packageEd25519SignatureBytes);
    unsigned long long signatureLength = 0;
    if (crypto_sign_ed25519_detached(signature.data(), &signatureLength,
                                     message.data(), static_cast<unsigned long long>(message.size()),
                                     privateKey.data()) != 0
        || signatureLength != packageEd25519SignatureBytes)
    {
        signature.clear();
        issue = "Ed25519 signing failed";
        return false;
    }
    issue.clear();
    return true;
}

bool packageVerifyEd25519(const std::vector<std::uint8_t>& publicKey,
                          const std::vector<std::uint8_t>& message,
                          const std::vector<std::uint8_t>& signature,
                          std::string& issue)
{
    if (! ensureSodium(issue) || publicKey.size() != packageEd25519PublicKeyBytes
        || signature.size() != packageEd25519SignatureBytes)
    {
        if (issue.empty()) issue = "Ed25519 public key/signature length is invalid";
        return false;
    }
    if (crypto_sign_ed25519_verify_detached(signature.data(), message.data(),
                                            static_cast<unsigned long long>(message.size()),
                                            publicKey.data()) != 0)
    {
        issue = "Ed25519 signature verification failed";
        return false;
    }
    issue.clear();
    return true;
}

bool packageSignEd25519ph(const std::vector<std::uint8_t>& privateKey,
                          const std::vector<std::uint8_t>& message,
                          std::vector<std::uint8_t>& signature,
                          std::string& issue)
{
    signature.clear();
    if (! ensureSodium(issue) || privateKey.size() != packageEd25519PrivateKeyBytes)
    {
        if (issue.empty()) issue = "Ed25519ph private key must be 64 bytes";
        return false;
    }
    crypto_sign_ed25519ph_state state;
    signature.resize(packageEd25519SignatureBytes);
    unsigned long long signatureLength = 0;
    if (crypto_sign_ed25519ph_init(&state) != 0
        || (! message.empty()
            && crypto_sign_ed25519ph_update(
                &state, message.data(), static_cast<unsigned long long>(message.size())) != 0)
        || crypto_sign_ed25519ph_final_create(
            &state, signature.data(), &signatureLength, privateKey.data()) != 0
        || signatureLength != packageEd25519SignatureBytes)
    {
        signature.clear();
        issue = "Ed25519ph signing failed";
        return false;
    }
    issue.clear();
    return true;
}

bool packageSignEd25519phFile(const std::vector<std::uint8_t>& privateKey,
                              const std::string& path,
                              const std::uint64_t expectedBytes,
                              std::vector<std::uint8_t>& signature,
                              std::string& issue)
{
    signature.clear();
    if (! ensureSodium(issue) || privateKey.size() != packageEd25519PrivateKeyBytes)
    {
        if (issue.empty()) issue = "Ed25519ph private key must be 64 bytes";
        return false;
    }
    std::ifstream input(path, std::ios::binary);
    if (! input)
    {
        issue = "Ed25519ph signing input could not be opened";
        return false;
    }
    crypto_sign_ed25519ph_state state;
    if (crypto_sign_ed25519ph_init(&state) != 0)
    {
        issue = "Ed25519ph signing failed";
        return false;
    }
    std::vector<std::uint8_t> buffer(1024u * 1024u);
    std::uint64_t consumed = 0;
    while (input && consumed < expectedBytes)
    {
        const auto request = static_cast<std::streamsize>(
            std::min<std::uint64_t>(buffer.size(), expectedBytes - consumed));
        input.read(reinterpret_cast<char*>(buffer.data()), request);
        const auto count = input.gcount();
        if (count <= 0
            || crypto_sign_ed25519ph_update(
                &state, buffer.data(), static_cast<unsigned long long>(count)) != 0)
        {
            issue = "Ed25519ph signing input is truncated or unreadable";
            return false;
        }
        consumed += static_cast<std::uint64_t>(count);
    }
    if (consumed != expectedBytes || input.peek() != std::ifstream::traits_type::eof())
    {
        issue = "Ed25519ph signing input length does not match the canonical request";
        return false;
    }
    signature.resize(packageEd25519SignatureBytes);
    unsigned long long signatureLength = 0;
    if (crypto_sign_ed25519ph_final_create(
            &state, signature.data(), &signatureLength, privateKey.data()) != 0
        || signatureLength != packageEd25519SignatureBytes)
    {
        signature.clear();
        issue = "Ed25519ph signing failed";
        return false;
    }
    issue.clear();
    return true;
}

bool packageVerifyEd25519ph(const std::vector<std::uint8_t>& publicKey,
                            const std::vector<std::uint8_t>& message,
                            const std::vector<std::uint8_t>& signature,
                            std::string& issue)
{
    if (! ensureSodium(issue) || publicKey.size() != packageEd25519PublicKeyBytes
        || signature.size() != packageEd25519SignatureBytes)
    {
        if (issue.empty()) issue = "Ed25519ph public key/signature length is invalid";
        return false;
    }
    crypto_sign_ed25519ph_state state;
    if (crypto_sign_ed25519ph_init(&state) != 0
        || (! message.empty()
            && crypto_sign_ed25519ph_update(
                &state, message.data(), static_cast<unsigned long long>(message.size())) != 0)
        || crypto_sign_ed25519ph_final_verify(
            &state, signature.data(), publicKey.data()) != 0)
    {
        issue = "Ed25519ph signature verification failed";
        return false;
    }
    issue.clear();
    return true;
}
} // namespace drs::engine
