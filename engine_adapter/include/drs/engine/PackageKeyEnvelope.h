#pragma once

#include "drs/engine/PackageCrypto.h"
#include "drs/engine/PackagePublisherTrustStore.h"
#include "drs/engine/PackageSignature.h"

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct PackageKeyEnvelope
{
    std::string keyId;
    PackageSealedBlob sealedContentKey;
};

bool wrapPackageContentKey(const std::string& packageId,
                           const std::string& keyId,
                           const SecureBuffer& contentKey,
                           const SecureBuffer& releaseKey,
                           PackageKeyEnvelope& envelope,
                           std::string& issue);

bool unwrapPackageContentKey(const std::string& packageId,
                             const PackageKeyEnvelope& envelope,
                             const SecureBuffer& releaseKey,
                             SecureBuffer& contentKey,
                             std::string& issue);

bool verifyPackageSignature(const std::vector<std::uint8_t>& packageBytes,
                            const std::vector<std::uint8_t>& signature,
                            const std::string& signingKeyId,
                            const std::vector<PackageSigningKey>& trustStore,
                            std::string& issue);

bool resolvePackageSigningPublicKey(const std::string& signingKeyId,
                                    const std::vector<PackageSigningKey>& trustStore,
                                    std::vector<std::uint8_t>& publicKey,
                                    std::string& issue);

bool resolvePackageSigningPublicKey(const std::string& signingKeyId,
                                    const PackagePublisherTrustStore& trustStore,
                                    std::vector<std::uint8_t>& publicKey,
                                    std::string& issue);
} // namespace drs::engine
