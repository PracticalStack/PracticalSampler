#pragma once

#include "drs/engine/PackagePublisherSigning.h"

namespace drs::engine
{
// Local Ed25519ph signer for the portable offline profile. The historical
// PackagePublisherSigningClient name remains on the wire/API boundary, but
// this implementation only attests that the package was produced by a
// compatible Practical Sampler build; it does not identify an author.
class OfflinePackageRecognitionSigner final : public PackagePublisherSigningClient
{
public:
    bool isConfigured() const noexcept;

    bool signCanonicalPackage(
        const PackagePublisherSigningRequest& request,
        PackagePublisherSigningResponse& response,
        std::string& issue) const override;
};

const char* offlinePackageRecognitionSigningKeyId() noexcept;
bool offlinePackageRecognitionSigningConfigured() noexcept;
} // namespace drs::engine
