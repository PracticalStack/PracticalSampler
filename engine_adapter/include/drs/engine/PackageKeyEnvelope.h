#pragma once

#include "drs/engine/PackageCrypto.h"
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
                           const std::vector<std::uint8_t>& contentKey,
                           const std::vector<std::uint8_t>& releaseKey,
                           PackageKeyEnvelope& envelope,
                           std::string& issue);

bool unwrapPackageContentKey(const std::string& packageId,
                             const PackageKeyEnvelope& envelope,
                             const std::vector<std::uint8_t>& releaseKey,
                             std::vector<std::uint8_t>& contentKey,
                             std::string& issue);

struct PackageSigningKey
{
    std::string keyId;
    std::vector<std::uint8_t> publicKey;
    bool revoked = false;
};

bool verifyPackageSignature(const std::vector<std::uint8_t>& packageBytes,
                            const std::vector<std::uint8_t>& signature,
                            const std::string& signingKeyId,
                            const std::vector<PackageSigningKey>& trustStore,
                            std::string& issue);
} // namespace drs::engine
