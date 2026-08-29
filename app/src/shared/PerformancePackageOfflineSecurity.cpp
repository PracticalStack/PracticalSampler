#include "shared/PerformancePackageOfflineSecurity.h"

#include "drs/engine/PackageOfflineRecognitionSigner.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace drs::app
{
namespace
{
struct OfflineSecurityParts
{
    std::shared_ptr<const drs::engine::PackageKeyProvider> keyProvider;
    std::shared_ptr<const drs::engine::PackagePublisherTrustStore> trustStore;
};

OfflineSecurityParts buildOfflineReadParts(const bool requireSigningKey)
{
    if (! drs::engine::offlinePackageProtectionProfileConfigured())
        return {};

    auto source = std::make_shared<drs::engine::OfflinePackageReleaseKeySource>();
    auto provider = std::make_shared<drs::engine::VersionedPackageKeyProvider>(
        drs::engine::offlinePackageReleaseKeyPolicies(), std::move(source));
    if (! provider->valid())
        return {};

    std::string issue;
    std::string activeKeyId;
    if (! provider->selectActiveEncryptionKeyId(activeKeyId, issue)
        || activeKeyId != drs::engine::offlinePackageReleaseKeyId())
        return {};

    auto trustStore = std::make_shared<drs::engine::PackagePublisherTrustStore>(
        drs::engine::builtInPackagePublisherTrustStore().keys());
    if (! trustStore->valid() || trustStore->keys().empty())
        return {};

    if (requireSigningKey)
    {
        std::vector<std::uint8_t> publicKey;
        if (! trustStore->resolvePublicKey(
                drs::engine::offlinePackageRecognitionSigningKeyId(), publicKey, issue))
            return {};
    }

    OfflineSecurityParts parts;
    parts.keyProvider = std::move(provider);
    parts.trustStore = std::move(trustStore);
    return parts;
}
} // namespace

std::shared_ptr<const PerformancePackageExportSecurityContext>
makeOfflinePerformancePackageExportSecurityContext()
{
    auto parts = buildOfflineReadParts(true);
    if (parts.keyProvider == nullptr || parts.trustStore == nullptr
        || ! drs::engine::offlinePackageRecognitionSigningConfigured())
        return {};

    auto signer = std::make_shared<drs::engine::OfflinePackageRecognitionSigner>();
    if (! signer->isConfigured())
        return {};

    auto context = std::make_shared<PerformancePackageExportSecurityContext>();
    context->compatibilityId = performancePackageV3CompatibilityId;
    context->encryptionKeyId = drs::engine::offlinePackageReleaseKeyId();
    context->signingKeyId = drs::engine::offlinePackageRecognitionSigningKeyId();
    context->keyProvider = std::move(parts.keyProvider);
    context->publisherSigner = std::move(signer);
    context->trustStore = std::move(parts.trustStore);
    return context->valid() ? context : nullptr;
}

std::shared_ptr<const drs::engine::PerformancePackageV3ActivationSecurityContext>
makeOfflinePerformancePackageActivationSecurityContext()
{
    auto parts = buildOfflineReadParts(false);
    if (parts.keyProvider == nullptr || parts.trustStore == nullptr)
        return {};

    auto context = std::make_shared<drs::engine::PerformancePackageV3ActivationSecurityContext>();
    context->compatibilityId = drs::engine::performancePackageV3CompatibilityId;
    context->keyProvider = std::move(parts.keyProvider);
    context->trustStore = std::move(parts.trustStore);
    return context->valid() ? context : nullptr;
}
} // namespace drs::app
