#pragma once

#include <functional>
#include <string>
#include <vector>

#include <drs/engine/SecureBuffer.h>

namespace drs::engine
{
enum class PackageKeyUse
{
    encryptNewPackage,
    decryptExistingPackage
};

enum class PackageReleaseKeyState
{
    active,
    retired,
    revoked
};

struct PackageReleaseKeyPolicy
{
    std::string keyId;
    PackageReleaseKeyState state = PackageReleaseKeyState::active;
    std::string activatedUtc;
    std::string retiredUtc;
    std::string revokedUtc;
};

// Runtime key lookup boundary. Implementations may resolve through the
// offline application protection profile, a future entitlement service, an
// OS-protected store, or a test fixture. Callers never derive keys from
// package metadata and never need to know the backing store.
class PackageKeyProvider
{
public:
    virtual ~PackageKeyProvider() = default;
    virtual bool resolvePackageKey(const std::string& packageId,
                                   const std::string& keyId,
                                   PackageKeyUse use,
                                   SecureBuffer& key,
                                   std::string& issue) const = 0;
};

class PackageReleaseKeySource
{
public:
    virtual ~PackageReleaseKeySource() = default;
    virtual bool loadReleaseKey(const std::string& packageId,
                                const std::string& keyId,
                                SecureBuffer& key) const = 0;
};

class CallbackPackageReleaseKeySource final : public PackageReleaseKeySource
{
public:
    using Resolver = std::function<bool(const std::string&,
                                        const std::string&,
                                        SecureBuffer&)>;

    explicit CallbackPackageReleaseKeySource(Resolver resolver);
    bool loadReleaseKey(const std::string& packageId,
                        const std::string& keyId,
                        SecureBuffer& key) const override;

private:
    Resolver resolver_;
};

class VersionedPackageKeyProvider final : public PackageKeyProvider
{
public:
    VersionedPackageKeyProvider(std::vector<PackageReleaseKeyPolicy> policies,
                                const PackageReleaseKeySource& source);

    bool valid() const noexcept { return valid_; }
    const std::string& configurationIssue() const noexcept { return configurationIssue_; }
    const std::vector<PackageReleaseKeyPolicy>& policies() const noexcept { return policies_; }

    bool selectActiveEncryptionKeyId(std::string& keyId,
                                     std::string& issue) const;

    bool resolvePackageKey(const std::string& packageId,
                           const std::string& keyId,
                           PackageKeyUse use,
                           SecureBuffer& key,
                           std::string& issue) const override;

private:
    std::vector<PackageReleaseKeyPolicy> policies_;
    const PackageReleaseKeySource& source_;
    bool valid_ = false;
    std::string configurationIssue_;
};
} // namespace drs::engine
