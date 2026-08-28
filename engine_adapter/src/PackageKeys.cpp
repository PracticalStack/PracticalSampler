#include <drs/engine/PackageKeys.h>

#include <drs/engine/PackageCrypto.h>

#include <algorithm>
#include <set>
#include <utility>

namespace drs::engine
{
CallbackPackageReleaseKeySource::CallbackPackageReleaseKeySource(Resolver resolver)
    : resolver_(std::move(resolver))
{
}

bool CallbackPackageReleaseKeySource::loadReleaseKey(
    const std::string& packageId,
    const std::string& keyId,
    SecureBuffer& key) const
{
    key.clear();
    return resolver_ && resolver_(packageId, keyId, key);
}

VersionedPackageKeyProvider::VersionedPackageKeyProvider(
    std::vector<PackageReleaseKeyPolicy> policies,
    const PackageReleaseKeySource& source)
    : policies_(std::move(policies)), source_(source)
{
    std::set<std::string> keyIds;
    for (const auto& policy : policies_)
    {
        if (policy.keyId.empty() || ! keyIds.insert(policy.keyId).second)
        {
            configurationIssue_ = "release key policy contains an empty or duplicate key id";
            return;
        }
        if (policy.activatedUtc.empty())
        {
            configurationIssue_ = "release key policy is missing activation metadata";
            return;
        }
        if (policy.state == PackageReleaseKeyState::retired && policy.retiredUtc.empty())
        {
            configurationIssue_ = "retired release key policy is missing retirement metadata";
            return;
        }
        if (policy.state == PackageReleaseKeyState::revoked && policy.revokedUtc.empty())
        {
            configurationIssue_ = "revoked release key policy is missing revocation metadata";
            return;
        }
    }
    valid_ = true;
}

bool VersionedPackageKeyProvider::selectActiveEncryptionKeyId(
    std::string& keyId,
    std::string& issue) const
{
    keyId.clear();
    if (! valid_)
    {
        issue = "release key policy is invalid";
        return false;
    }
    for (const auto& policy : policies_)
    {
        if (policy.state != PackageReleaseKeyState::active)
            continue;
        if (! keyId.empty())
        {
            keyId.clear();
            issue = "release key policy has multiple active encryption keys";
            return false;
        }
        keyId = policy.keyId;
    }
    if (keyId.empty())
    {
        issue = "release key policy has no active encryption key";
        return false;
    }
    issue.clear();
    return true;
}

bool VersionedPackageKeyProvider::resolvePackageKey(
    const std::string& packageId,
    const std::string& keyId,
    const PackageKeyUse use,
    SecureBuffer& key,
    std::string& issue) const
{
    key.clear();
    if (! valid_ || packageId.empty() || keyId.empty())
    {
        issue = "release key request or policy is invalid";
        return false;
    }
    const auto found = std::find_if(policies_.begin(), policies_.end(),
        [&](const auto& policy) { return policy.keyId == keyId; });
    if (found == policies_.end())
    {
        issue = "release key id is unknown";
        return false;
    }
    if (found->state == PackageReleaseKeyState::revoked)
    {
        issue = "release key is revoked";
        return false;
    }
    if (use == PackageKeyUse::encryptNewPackage
        && found->state != PackageReleaseKeyState::active)
    {
        issue = "release key is retired for new package encryption";
        return false;
    }
    if (! source_.loadReleaseKey(packageId, keyId, key))
    {
        key.clear();
        issue = "release key source is unavailable";
        return false;
    }
    if (key.size() != securePackageKeySizeBytes)
    {
        key.clear();
        issue = "release key source returned an invalid key length";
        return false;
    }
    issue.clear();
    return true;
}
} // namespace drs::engine
