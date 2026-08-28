#include <drs/engine/PackageKeyEnvelope.h>

#include <utility>

#include <sodium/utils.h>

namespace drs::engine
{
namespace
{
void appendU16(std::string& bytes, const std::uint16_t value)
{
    bytes.push_back(static_cast<char>(value & 0xffu));
    bytes.push_back(static_cast<char>((value >> 8u) & 0xffu));
}

bool appendField(std::string& bytes, const std::string& value)
{
    if (value.empty() || value.size() > 65535u)
        return false;
    appendU16(bytes, static_cast<std::uint16_t>(value.size()));
    bytes.append(value);
    return true;
}

std::string envelopeAad(const std::string& packageId, const std::string& keyId)
{
    std::string bytes("DRSKEYE3", 8);
    if (! appendField(bytes, packageId) || ! appendField(bytes, keyId))
        return {};
    return bytes;
}
}

bool wrapPackageContentKey(const std::string& packageId,
                           const std::string& keyId,
                           const SecureBuffer& contentKey,
                           const SecureBuffer& releaseKey,
                           PackageKeyEnvelope& envelope,
                           std::string& issue)
{
    envelope = {};
    if (packageId.empty() || keyId.empty()
        || contentKey.size() != securePackageKeySizeBytes
        || releaseKey.size() != securePackageKeySizeBytes)
    { issue = "key envelope identity or key length is invalid"; return false; }
    PackageSealRequest request;
    request.packageId = packageId;
    request.recordId = "key-envelope";
    request.secureEncryptionKey = &releaseKey;
    request.securePlaintext = &contentKey;
    request.additionalAuthenticatedData = envelopeAad(packageId, keyId);
    if (request.additionalAuthenticatedData.empty())
    { issue = "key envelope identity is too long"; return false; }
    if (! getSecurePackageCryptoProvider().seal(
            request, envelope.sealedContentKey, issue))
        return false;
    envelope.keyId = keyId;
    issue.clear();
    return true;
}

bool unwrapPackageContentKey(const std::string& packageId,
                             const PackageKeyEnvelope& envelope,
                             const SecureBuffer& releaseKey,
                             SecureBuffer& contentKey,
                             std::string& issue)
{
    contentKey.clear();
    if (packageId.empty() || envelope.keyId.empty()
        || releaseKey.size() != securePackageKeySizeBytes)
    { issue = "key envelope identity or release key length is invalid"; return false; }
    PackageOpenRequest request;
    request.packageId = packageId;
    request.recordId = "key-envelope";
    request.secureEncryptionKey = &releaseKey;
    request.additionalAuthenticatedData = envelopeAad(packageId, envelope.keyId);
    if (request.additionalAuthenticatedData.empty())
    { issue = "key envelope identity is too long"; return false; }
    request.sealed = envelope.sealedContentKey;
    std::vector<std::uint8_t> openedKey;
    if (! getSecurePackageCryptoProvider().open(request, openedKey, issue))
        return false;
    if (openedKey.size() != securePackageKeySizeBytes)
    {
        if (! openedKey.empty()) sodium_memzero(openedKey.data(), openedKey.size());
        issue = "unwrapped content key has invalid length";
        return false;
    }
    contentKey = SecureBuffer(std::move(openedKey));
    issue.clear();
    return true;
}

bool verifyPackageSignature(const std::vector<std::uint8_t>& packageBytes,
                            const std::vector<std::uint8_t>& signature,
                            const std::string& signingKeyId,
                            const std::vector<PackageSigningKey>& trustStore,
                            std::string& issue)
{
    std::vector<std::uint8_t> publicKey;
    if (! resolvePackageSigningPublicKey(signingKeyId, trustStore, publicKey, issue))
        return false;
    return packageVerifyEd25519(publicKey, packageBytes, signature, issue);
}

bool resolvePackageSigningPublicKey(const std::string& signingKeyId,
                                    const std::vector<PackageSigningKey>& trustStore,
                                    std::vector<std::uint8_t>& publicKey,
                                    std::string& issue)
{
    const PackagePublisherTrustStore immutableStore(trustStore);
    return immutableStore.resolvePublicKey(signingKeyId, publicKey, issue);
}

bool resolvePackageSigningPublicKey(const std::string& signingKeyId,
                                    const PackagePublisherTrustStore& trustStore,
                                    std::vector<std::uint8_t>& publicKey,
                                    std::string& issue)
{
    return trustStore.resolvePublicKey(signingKeyId, publicKey, issue);
}
} // namespace drs::engine
