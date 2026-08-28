#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct PackagePublisherSigningRequest
{
    std::string signingKeyId;
    const std::vector<std::uint8_t>* canonicalSignedBytes = nullptr;
    std::string canonicalSignedFilePath;
    std::uint64_t canonicalSignedBytesLength = 0;
};

struct PackagePublisherSigningResponse
{
    std::vector<std::uint8_t> signature;
    std::string auditId;
};

// Desktop/export code sees only this request boundary. Production private keys
// live behind the controlled implementation and never enter this interface.
class PackagePublisherSigningClient
{
public:
    virtual ~PackagePublisherSigningClient() = default;
    virtual bool signCanonicalPackage(
        const PackagePublisherSigningRequest& request,
        PackagePublisherSigningResponse& response,
        std::string& issue) const = 0;
};
} // namespace drs::engine
