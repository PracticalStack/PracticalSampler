#include <drs/engine/PackageKeyEnvelope.h>

#include <algorithm>

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
                           const std::vector<std::uint8_t>& contentKey,
                           const std::vector<std::uint8_t>& releaseKey,
                           PackageKeyEnvelope& envelope,
                           std::string& issue)
{
    envelope = {};
    if (packageId.empty() || keyId.empty() || contentKey.size() != securePackageKeySizeBytes
        || releaseKey.size() != securePackageKeySizeBytes)
    { issue = "key envelope identity or key length is invalid"; return false; }
    PackageSealRequest request;
    request.packageId = packageId;
    request.recordId = "key-envelope";
    request.encryptionKey = releaseKey;
    request.additionalAuthenticatedData = envelopeAad(packageId, keyId);
    if (request.additionalAuthenticatedData.empty())
    { issue = "key envelope identity is too long"; return false; }
    request.plaintext = contentKey;
    if (! getSecurePackageCryptoProvider().seal(request, envelope.sealedContentKey, issue)) return false;
    envelope.keyId = keyId;
    issue.clear();
    return true;
}

bool unwrapPackageContentKey(const std::string& packageId,
                             const PackageKeyEnvelope& envelope,
                             const std::vector<std::uint8_t>& releaseKey,
                             std::vector<std::uint8_t>& contentKey,
                             std::string& issue)
{
    contentKey.clear();
    if (packageId.empty() || envelope.keyId.empty() || releaseKey.size() != securePackageKeySizeBytes)
    { issue = "key envelope identity or release key length is invalid"; return false; }
    PackageOpenRequest request;
    request.packageId = packageId;
    request.recordId = "key-envelope";
    request.encryptionKey = releaseKey;
    request.additionalAuthenticatedData = envelopeAad(packageId, envelope.keyId);
    if (request.additionalAuthenticatedData.empty())
    { issue = "key envelope identity is too long"; return false; }
    request.sealed = envelope.sealedContentKey;
    if (! getSecurePackageCryptoProvider().open(request, contentKey, issue)) return false;
    if (contentKey.size() != securePackageKeySizeBytes)
    { contentKey.clear(); issue = "unwrapped content key has invalid length"; return false; }
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
    publicKey.clear();
    const auto found = std::find_if(trustStore.begin(), trustStore.end(), [&](const auto& key)
    { return key.keyId == signingKeyId; });
    if (found == trustStore.end()) { issue = "package signing key id is unknown"; return false; }
    if (found->revoked) { issue = "package signing key is revoked"; return false; }
    if (found->publicKey.size() != packageEd25519PublicKeyBytes)
    { issue = "package signing public key length is invalid"; return false; }
    publicKey = found->publicKey;
    issue.clear();
    return true;
}
} // namespace drs::engine
