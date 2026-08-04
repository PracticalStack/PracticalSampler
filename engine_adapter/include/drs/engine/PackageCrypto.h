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
    std::vector<std::uint8_t> plaintext;
};

struct PackageOpenRequest
{
    std::string packageId;
    std::string recordId;
    std::string additionalAuthenticatedData;
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
} // namespace drs::engine
