#pragma once

#include "PackageProtectionTestSupport.h"
#include "shared/PerformancePackageExportService.h"

#include <drs/engine/PackageCrypto.h>
#include <drs/engine/PackageSignature.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace drs::tests
{
class StaticPerformancePackageTestKeyProvider final : public engine::PackageKeyProvider
{
public:
    StaticPerformancePackageTestKeyProvider(std::string keyId,
                                            std::vector<std::uint8_t> key)
        : keyId_(std::move(keyId)), key_(std::move(key)) {}

    bool resolvePackageKey(const std::string&,
                           const std::string& keyId,
                           engine::PackageKeyUse,
                           engine::SecureBuffer& key,
                           std::string& issue) const override
    {
        key.clear();
        if (keyId != keyId_)
        {
            issue = "unknown test release key";
            return false;
        }
        key = engine::SecureBuffer(key_);
        issue.clear();
        return true;
    }

private:
    std::string keyId_;
    std::vector<std::uint8_t> key_;
};

inline std::shared_ptr<const app::PerformancePackageExportSecurityContext>
makePerformancePackageExportTestSecurityContext()
{
    constexpr const char* releaseKeyId = "test-release-2026-08";
    constexpr const char* signingKeyId = "test-publisher-2026-08";
    std::string issue;
    engine::SecureBuffer releaseKey;
    if (! engine::generateSecurePackageKey(releaseKey, issue))
        throw std::runtime_error(issue);
    auto provider = std::make_shared<StaticPerformancePackageTestKeyProvider>(
        releaseKeyId,
        std::vector<std::uint8_t>(releaseKey.bytes().begin(), releaseKey.bytes().end()));
    std::vector<std::uint8_t> publicKey;
    std::vector<std::uint8_t> privateKey;
    if (! engine::generatePackageSigningKeyPair(publicKey, privateKey, issue))
        throw std::runtime_error(issue);
    auto signer = std::make_shared<PackageProtectionTestSigner>(signingKeyId, privateKey);
    auto trust = std::make_shared<engine::PackagePublisherTrustStore>(
        std::vector<engine::PackageSigningKey> {
            activeTestSigningKey(signingKeyId, publicKey)
        });
    if (! trust->valid())
        throw std::runtime_error(trust->configurationIssue());
    auto context = std::make_shared<app::PerformancePackageExportSecurityContext>();
    context->encryptionKeyId = releaseKeyId;
    context->signingKeyId = signingKeyId;
    context->keyProvider = std::move(provider);
    context->publisherSigner = std::move(signer);
    context->trustStore = std::move(trust);
    return context;
}
} // namespace drs::tests
