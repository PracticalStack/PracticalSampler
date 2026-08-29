#pragma once

#include "drs/engine/PackageSignature.h"

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
enum class PackageSigningKeyState
{
    active,
    retired,
    revoked
};

struct PackageSigningKey
{
    std::string keyId;
    std::vector<std::uint8_t> publicKey;
    PackageSigningKeyState state = PackageSigningKeyState::active;
    std::string activatedUtc;
    std::string retiredUtc;
    std::string revokedUtc;
};

// The historical type name is retained for V3 wire/API compatibility. For
// portable offline packages this is a recognition store, not an author or
// publisher identity registry.
class PackagePublisherTrustStore
{
public:
    explicit PackagePublisherTrustStore(std::vector<PackageSigningKey> keys = {});

    bool valid() const noexcept { return valid_; }
    const std::string& configurationIssue() const noexcept { return configurationIssue_; }
    const std::vector<PackageSigningKey>& keys() const noexcept { return keys_; }

    bool resolvePublicKey(const std::string& keyId,
                          std::vector<std::uint8_t>& publicKey,
                          std::string& issue) const;

private:
    std::vector<PackageSigningKey> keys_;
    bool valid_ = false;
    std::string configurationIssue_;
};

// Compiled from public-only CMake configuration. An empty store is valid for
// development builds but cannot recognize a production package. A future
// licensed profile may use a separately governed trust policy.
const PackagePublisherTrustStore& builtInPackagePublisherTrustStore();
} // namespace drs::engine
