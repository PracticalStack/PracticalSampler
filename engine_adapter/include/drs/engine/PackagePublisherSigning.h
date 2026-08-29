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

// The historical type name is retained for V3 wire/API compatibility. In the
// planned offline portable profile this client will be local and its signature
// will mean only that the package is recognized and internally consistent; it
// does not prove author identity or issuance by a controlled publisher. A
// future licensed profile may provide a controlled implementation behind the
// same boundary.
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
