#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct PackageSealedBlob
{
    std::vector<std::uint8_t> nonce;
    std::vector<std::uint8_t> ciphertext;
    std::vector<std::uint8_t> tag;
};

struct PackageSealRequest
{
    std::string packageId;
    std::string recordId;
    std::string additionalAuthenticatedData;
    std::string encryptionKeyId;
    // A 32-byte key is required by the production AEAD provider.  Legacy
    // deterministic callers intentionally leave this empty.
    std::vector<std::uint8_t> encryptionKey;
    std::vector<std::uint8_t> plaintext;
};

struct PackageOpenRequest
{
    std::string packageId;
    std::string recordId;
    std::string additionalAuthenticatedData;
    std::string encryptionKeyId;
    std::vector<std::uint8_t> encryptionKey;
    PackageSealedBlob sealed;
};

class PackageCryptoProvider
{
public:
    virtual ~PackageCryptoProvider() = default;

    virtual const char* algorithmId() const noexcept = 0;
    virtual std::size_t nonceSizeBytes() const noexcept = 0;
    virtual std::size_t tagSizeBytes() const noexcept = 0;

    virtual bool seal(const PackageSealRequest& request,
                      PackageSealedBlob& output,
                      std::string& issue) const = 0;

    virtual bool open(const PackageOpenRequest& request,
                      std::vector<std::uint8_t>& plaintext,
                      std::string& issue) const = 0;
};

const PackageCryptoProvider& getDeterministicPackageCryptoProvider();

// Production package crypto.  Keys are supplied by the package key
// lifecycle and are never derived from package metadata.  The provider is
// intentionally separate from the deterministic V1/V2 compatibility
// provider so a new writer cannot accidentally select the legacy transform.
const PackageCryptoProvider& getSecurePackageCryptoProvider();

inline constexpr std::size_t securePackageKeySizeBytes = 32;

bool generateSecurePackageKey(std::vector<std::uint8_t>& key,
                              std::string& issue);
} // namespace drs::engine
