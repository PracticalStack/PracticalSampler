#include <drs/engine/PackageCrypto.h>
#include <drs/engine/PackageKeys.h>
#include <drs/engine/PackageOfflineRecognitionSigner.h>
#include <drs/engine/PackagePublisherTrustStore.h>
#include <drs/engine/PackageV3.h>
#include <drs/signing/ControlledPackageSigner.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
bool check(const bool condition, const std::string& message)
{
    if (! condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}

std::string readText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

class RecordingAuditSink final : public drs::signing::PackageSigningAuditSink
{
public:
    bool available = true;
    mutable std::vector<drs::signing::PackageSigningAuditEvent> events;

    bool recordPackageSigningEvent(
        const drs::signing::PackageSigningAuditEvent& event) const override
    {
        if (! available) return false;
        events.push_back(event);
        return true;
    }
};

drs::engine::PackageV3WriteRequest makeWriteRequest(
    const drs::engine::SecureBuffer& releaseKey,
    const std::string& signingKeyId,
    const drs::engine::PackagePublisherSigningClient& signer)
{
    drs::engine::PackageV3WriteRequest request;
    request.packageId = "phase3-key-lifecycle";
    request.compatibilityId = "drs.runtime.v1";
    request.encryptionKeyId = "release-2026-08";
    request.releaseKey = &releaseKey;
    request.signingKeyId = signingKeyId;
    request.publisherSigner = &signer;
    request.records = {
        { "settings", "metadata", 1u, 0u, { 'g', 'a', 'i', 'n', '=', '1' } },
        { "page", "sample-page", 1u, 0u, { 0x10u, 0x20u, 0x30u, 0x40u } }
    };
    return request;
}

drs::engine::PackageSigningKey signingKey(
    const std::string& keyId,
    const std::vector<std::uint8_t>& publicKey,
    const drs::engine::PackageSigningKeyState state)
{
    drs::engine::PackageSigningKey key;
    key.keyId = keyId;
    key.publicKey = publicKey;
    key.state = state;
    key.activatedUtc = "2026-08-01T00:00:00Z";
    if (state == drs::engine::PackageSigningKeyState::retired)
        key.retiredUtc = "2026-08-28T00:00:00Z";
    if (state == drs::engine::PackageSigningKeyState::revoked)
        key.revokedUtc = "2026-08-28T00:00:00Z";
    return key;
}
} // namespace

int main()
{
    using namespace drs::engine;
    static_assert(! std::is_copy_constructible_v<SecureBuffer>);
    static_assert(! std::is_copy_assignable_v<SecureBuffer>);
    static_assert(std::is_move_constructible_v<SecureBuffer>);

    bool ok = true;
    std::string issue;

    OfflinePackageReleaseKeySource offlineSource;
    ok &= check(offlineSource.isConfigured() == offlinePackageProtectionProfileConfigured(),
                "offline package profile reports its generated configuration state");
    const auto& offlinePolicies = offlinePackageReleaseKeyPolicies();
    ok &= check(offlinePolicies.empty() == ! offlinePackageProtectionProfileConfigured(),
                "offline package profile exposes policy metadata only when configured");
    SecureBuffer offlineKey;
    if (offlinePackageProtectionProfileConfigured())
    {
        ok &= check(std::string(offlinePackageProtectionProfileId()).size() > 0
                        && std::string(offlinePackageReleaseKeyId()).size() > 0,
                    "configured offline package profile exposes stable non-secret IDs");
        const auto activePolicy = std::find_if(offlinePolicies.begin(), offlinePolicies.end(),
            [](const auto& policy) { return policy.state == PackageReleaseKeyState::active; });
        ok &= check(activePolicy != offlinePolicies.end()
                        && activePolicy->keyId == offlinePackageReleaseKeyId(),
                    "configured offline package profile exposes the active key policy");
        ok &= check(offlineSource.loadReleaseKey(
                        "offline-package", offlinePackageReleaseKeyId(), offlineKey)
                        && offlineKey.size() == securePackageKeySizeBytes,
                    "configured offline package profile reconstructs a valid release key");
        for (const auto& policy : offlinePolicies)
        {
            SecureBuffer policyKey;
            const bool loaded = offlineSource.loadReleaseKey(
                "offline-package", policy.keyId, policyKey);
            if (policy.state == PackageReleaseKeyState::revoked)
            {
                ok &= check(! loaded && policyKey.empty(),
                            "configured offline package profile rejects revoked keys");
            }
            else
            {
                ok &= check(loaded && policyKey.size() == securePackageKeySizeBytes,
                            "configured offline package profile loads active and retired keys");
            }
        }
        VersionedPackageKeyProvider offlineProvider(offlinePolicies, offlineSource);
        std::string offlineSelectedKeyId;
        std::string offlineIssue;
        ok &= check(offlineProvider.valid()
                        && offlineProvider.selectActiveEncryptionKeyId(
                            offlineSelectedKeyId, offlineIssue)
                        && offlineSelectedKeyId == offlinePackageReleaseKeyId(),
                    "configured offline package profile selects its active key for export");
        SecureBuffer offlineResolved;
        ok &= check(offlineProvider.resolvePackageKey(
                        "offline-package", offlinePackageReleaseKeyId(),
                        PackageKeyUse::encryptNewPackage, offlineResolved, offlineIssue)
                        && offlineResolved.size() == securePackageKeySizeBytes,
                    "configured offline package profile resolves the active export key");
        ok &= check(! offlineSource.loadReleaseKey(
                        "", offlinePackageReleaseKeyId(), offlineKey)
                        && offlineKey.empty(),
                    "offline package profile rejects an empty package identity and clears output");
        ok &= check(! offlineSource.loadReleaseKey(
                        "offline-package", "offline-unknown-key", offlineKey)
                        && offlineKey.empty(),
                    "offline package profile rejects an unknown key ID and clears output");
    }
    else
    {
        ok &= check(! offlineSource.loadReleaseKey(
                        "offline-package", "offline-unknown-key", offlineKey)
                        && offlineKey.empty(),
                    "unconfigured offline package profile fails closed without a key");
    }

    OfflinePackageRecognitionSigner recognitionSigner;
    ok &= check(recognitionSigner.isConfigured()
                    == offlinePackageRecognitionSigningConfigured(),
                "offline recognition signer reports its generated configuration state");
    if (recognitionSigner.isConfigured())
    {
        SecureBuffer signingReleaseKey;
        const bool releaseKeyLoaded = offlineSource.loadReleaseKey(
            "recognition-package", offlinePackageReleaseKeyId(), signingReleaseKey);
        PackageV3WriteRequest recognitionPackageRequest;
        recognitionPackageRequest.packageId = "recognition-package";
        recognitionPackageRequest.compatibilityId = "drs.runtime.v1";
        recognitionPackageRequest.encryptionKeyId = offlinePackageReleaseKeyId();
        recognitionPackageRequest.releaseKey = &signingReleaseKey;
        recognitionPackageRequest.signingKeyId = offlinePackageRecognitionSigningKeyId();
        recognitionPackageRequest.publisherSigner = &recognitionSigner;
        recognitionPackageRequest.records = {
            { "settings", "metadata", 1u, 0u, { 'r', 'e', 'c', 'o', 'g', 'n', 'i', 'z', 'e', 'd' } }
        };
        const auto recognitionPackage = releaseKeyLoaded
            ? writePackageV3(recognitionPackageRequest) : PackageV3WriteResult {};
        ok &= check(releaseKeyLoaded && recognitionPackage.written
                        && recognitionPackage.packageBytes.size() > packageEd25519SignatureBytes,
                    "configured offline recognition profile creates a canonical V3 package");
        std::vector<std::uint8_t> canonicalBytes;
        if (recognitionPackage.packageBytes.size() > packageEd25519SignatureBytes)
        {
            canonicalBytes.assign(
                recognitionPackage.packageBytes.begin(),
                recognitionPackage.packageBytes.end()
                    - static_cast<std::ptrdiff_t>(packageEd25519SignatureBytes));
        }
        PackagePublisherSigningRequest signingRequest;
        signingRequest.signingKeyId = offlinePackageRecognitionSigningKeyId();
        signingRequest.canonicalSignedBytes = &canonicalBytes;
        PackagePublisherSigningResponse signingResponse;
        ok &= check(recognitionSigner.signCanonicalPackage(
                        signingRequest, signingResponse, issue)
                        && signingResponse.signature.size() == packageEd25519SignatureBytes
                        && signingResponse.auditId.rfind("offline-recognition:", 0) == 0,
                    "configured offline recognition signer signs canonical bytes locally");
        const auto canonicalFile = std::filesystem::temp_directory_path()
            / "drs-offline-recognition-canonical.bin";
        {
            std::ofstream output(canonicalFile, std::ios::binary);
            output.write(reinterpret_cast<const char*>(canonicalBytes.data()),
                         static_cast<std::streamsize>(canonicalBytes.size()));
        }
        PackagePublisherSigningRequest fileSigningRequest;
        fileSigningRequest.signingKeyId = offlinePackageRecognitionSigningKeyId();
        fileSigningRequest.canonicalSignedFilePath = canonicalFile.string();
        fileSigningRequest.canonicalSignedBytesLength = canonicalBytes.size();
        PackagePublisherSigningResponse fileSigningResponse;
        ok &= check(recognitionSigner.signCanonicalPackage(
                        fileSigningRequest, fileSigningResponse, issue)
                        && fileSigningResponse.signature.size() == packageEd25519SignatureBytes,
                    "configured offline recognition signer signs staged canonical files locally");
        std::error_code removeError;
        std::filesystem::remove(canonicalFile, removeError);
        std::vector<std::uint8_t> recognitionPublicKey;
        const auto& builtInRecognitionStore = builtInPackagePublisherTrustStore();
        if (builtInRecognitionStore.resolvePublicKey(
                offlinePackageRecognitionSigningKeyId(), recognitionPublicKey, issue))
        {
            ok &= check(packageVerifyEd25519ph(
                            recognitionPublicKey, canonicalBytes,
                            signingResponse.signature, issue),
                        "built-in recognition store verifies the local signature");
        }
        signingRequest.signingKeyId = "offline-unknown-signing-key";
        signingResponse = {};
        ok &= check(! recognitionSigner.signCanonicalPackage(
                        signingRequest, signingResponse, issue)
                        && signingResponse.signature.empty(),
                    "offline recognition signer rejects an unknown key ID");
        signingRequest.signingKeyId = offlinePackageRecognitionSigningKeyId();
        signingRequest.canonicalSignedBytes = nullptr;
        signingResponse = {};
        ok &= check(! recognitionSigner.signCanonicalPackage(
                        signingRequest, signingResponse, issue)
                        && signingResponse.signature.empty(),
                    "offline recognition signer rejects missing canonical input");
        signingRequest.canonicalSignedBytes = &canonicalBytes;
        signingRequest.canonicalSignedFilePath = "ambiguous-canonical-input";
        signingResponse = {};
        ok &= check(! recognitionSigner.signCanonicalPackage(
                        signingRequest, signingResponse, issue)
                        && signingResponse.signature.empty(),
                    "offline recognition signer rejects ambiguous canonical input");
    }
    else
    {
        PackagePublisherSigningRequest signingRequest;
        signingRequest.signingKeyId = "offline-recognition-signing-key";
        const std::vector<std::uint8_t> canonicalBytes { 1u, 2u, 3u };
        signingRequest.canonicalSignedBytes = &canonicalBytes;
        PackagePublisherSigningResponse signingResponse;
        ok &= check(! recognitionSigner.signCanonicalPackage(
                        signingRequest, signingResponse, issue)
                        && signingResponse.signature.empty(),
                    "unconfigured offline recognition signer fails closed");
    }

    SecureBuffer activeReleaseKey, retiredReleaseKey, revokedReleaseKey;
    ok &= check(generateSecurePackageKey(activeReleaseKey, issue), "generate active release key");
    ok &= check(generateSecurePackageKey(retiredReleaseKey, issue), "generate retired release key");
    ok &= check(generateSecurePackageKey(revokedReleaseKey, issue), "generate revoked release key");

    std::map<std::string, std::vector<std::uint8_t>> externalKeys;
    externalKeys["release-active"] = activeReleaseKey.bytes();
    externalKeys["release-retired"] = retiredReleaseKey.bytes();
    externalKeys["release-revoked"] = revokedReleaseKey.bytes();
    bool sourceAvailable = true;
    CallbackPackageReleaseKeySource source(
        [&](const std::string&, const std::string& keyId, SecureBuffer& key)
        {
            if (! sourceAvailable) return false;
            const auto found = externalKeys.find(keyId);
            if (found == externalKeys.end()) return false;
            key = SecureBuffer(found->second);
            return true;
        });
    VersionedPackageKeyProvider provider({
        { "release-retired", PackageReleaseKeyState::retired,
          "2026-01-01T00:00:00Z", "2026-08-01T00:00:00Z", {} },
        { "release-active", PackageReleaseKeyState::active,
          "2026-08-01T00:00:00Z", {}, {} },
        { "release-revoked", PackageReleaseKeyState::revoked,
          "2025-01-01T00:00:00Z", {}, "2026-07-01T00:00:00Z" }
    }, source);
    ok &= check(provider.valid(), "versioned release-key policy valid");
    std::string selectedKeyId;
    ok &= check(provider.selectActiveEncryptionKeyId(selectedKeyId, issue)
                    && selectedKeyId == "release-active",
                "rotation selects the sole active encryption key");
    SecureBuffer resolved;
    ok &= check(provider.resolvePackageKey(
                    "pkg", "release-active", PackageKeyUse::encryptNewPackage,
                    resolved, issue)
                    && resolved.size() == securePackageKeySizeBytes,
                "active release key resolves for encryption");
    ok &= check(provider.resolvePackageKey(
                    "pkg", "release-retired", PackageKeyUse::decryptExistingPackage,
                    resolved, issue),
                "retired release key remains available for old package decryption");
    ok &= check(! provider.resolvePackageKey(
                    "pkg", "release-retired", PackageKeyUse::encryptNewPackage,
                    resolved, issue) && resolved.empty(),
                "retired release key cannot encrypt a new package");
    ok &= check(! provider.resolvePackageKey(
                    "pkg", "release-revoked", PackageKeyUse::decryptExistingPackage,
                    resolved, issue) && resolved.empty(),
                "revoked release key is rejected");
    ok &= check(! provider.resolvePackageKey(
                    "pkg", "release-unknown", PackageKeyUse::decryptExistingPackage,
                    resolved, issue) && resolved.empty(),
                "unknown release key is rejected");
    sourceAvailable = false;
    ok &= check(! provider.resolvePackageKey(
                    "pkg", "release-active", PackageKeyUse::decryptExistingPackage,
                    resolved, issue) && resolved.empty(),
                "release-key source outage returns no key");
    sourceAvailable = true;

    // The production bootstrap transfers ownership of its offline source to
    // the provider; verify the provider remains usable after the local source
    // variable goes out of scope.
    std::shared_ptr<const PackageKeyProvider> ownedProvider;
    {
        auto ownedSource = std::make_shared<CallbackPackageReleaseKeySource>(
            [&](const std::string&, const std::string& keyId, SecureBuffer& key)
            {
                const auto found = externalKeys.find(keyId);
                if (found == externalKeys.end()) return false;
                key = SecureBuffer(found->second);
                return true;
            });
        auto concreteProvider = std::make_shared<VersionedPackageKeyProvider>(
            std::vector<PackageReleaseKeyPolicy> {
                { "release-active", PackageReleaseKeyState::active,
                  "2026-08-01T00:00:00Z", {}, {} }
            }, std::move(ownedSource));
        ok &= check(concreteProvider->valid(),
                    "owned release-key source provider is valid");
        ownedProvider = std::move(concreteProvider);
    }
    ok &= check(ownedProvider->resolvePackageKey(
                    "pkg", "release-active", PackageKeyUse::decryptExistingPackage,
                    resolved, issue)
                    && resolved.size() == securePackageKeySizeBytes,
                "owned release-key source remains available after bootstrap scope");

    VersionedPackageKeyProvider duplicatePolicy({
        { "duplicate", PackageReleaseKeyState::active, "2026-01-01", {}, {} },
        { "duplicate", PackageReleaseKeyState::retired, "2025-01-01", "2026-01-01", {} }
    }, source);
    ok &= check(! duplicatePolicy.valid(), "duplicate release-key policy rejected");

    std::vector<std::uint8_t> oldPublicKey, oldPrivateKey;
    std::vector<std::uint8_t> newPublicKey, newPrivateKey;
    std::vector<std::uint8_t> attackerPublicKey, attackerPrivateKey;
    ok &= check(generatePackageSigningKeyPair(oldPublicKey, oldPrivateKey, issue),
                "generate retired publisher key pair");
    ok &= check(generatePackageSigningKeyPair(newPublicKey, newPrivateKey, issue),
                "generate active publisher key pair");
    ok &= check(generatePackageSigningKeyPair(attackerPublicKey, attackerPrivateKey, issue),
                "generate attacker key pair");

    PackagePublisherTrustStore rotatingTrust({
        signingKey("publisher-old", oldPublicKey, PackageSigningKeyState::retired),
        signingKey("publisher-new", newPublicKey, PackageSigningKeyState::active)
    });
    ok &= check(rotatingTrust.valid(), "publisher rotation trust store valid");
    std::vector<std::uint8_t> resolvedPublicKey;
    ok &= check(rotatingTrust.resolvePublicKey("publisher-old", resolvedPublicKey, issue),
                "retired publisher key verifies historical packages");
    ok &= check(rotatingTrust.resolvePublicKey("publisher-new", resolvedPublicKey, issue),
                "active publisher key resolves");
    ok &= check(! rotatingTrust.resolvePublicKey("publisher-unknown", resolvedPublicKey, issue),
                "unknown publisher key rejected");
    PackagePublisherTrustStore revokedTrust({
        signingKey("publisher-old", oldPublicKey, PackageSigningKeyState::revoked)
    });
    ok &= check(! revokedTrust.resolvePublicKey("publisher-old", resolvedPublicKey, issue),
                "revoked publisher key rejected");
    PackagePublisherTrustStore duplicateTrust({
        signingKey("publisher-new", newPublicKey, PackageSigningKeyState::active),
        signingKey("publisher-new", oldPublicKey, PackageSigningKeyState::retired)
    });
    ok &= check(! duplicateTrust.valid(), "duplicate publisher key id rejected");
    ok &= check(builtInPackagePublisherTrustStore().valid(),
                "generated public-only desktop trust store is structurally valid");

    RecordingAuditSink auditSink;
    drs::signing::ControlledPackageSigner oldSigner(
        "publisher-old", PackageSigningKeyState::active,
        SecureBuffer(oldPrivateKey), auditSink);
    auto oldRequest = makeWriteRequest(activeReleaseKey, "publisher-old", oldSigner);
    const auto oldPackage = writePackageV3(oldRequest);
    ok &= check(oldPackage.written && ! oldPackage.signingAuditId.empty()
                    && auditSink.events.size() == 1u,
                "controlled signing produces an audited V3 signature");
    ok &= check(auditSink.events.front().canonicalDigestHex.size() == 64u
                    && auditSink.events.front().canonicalBytes
                        == oldPackage.packageBytes.size() - packageEd25519SignatureBytes,
                "audit contains digest and byte count without key material");
    auto parsedOld = parsePackageV3(oldPackage.packageBytes);
    ok &= check(parsedOld.opened
                    && verifyPackageV3Signature(
                        oldPackage.packageBytes, rotatingTrust, parsedOld, issue),
                "retired publisher key verifies package created before rotation");
    parsedOld = parsePackageV3(oldPackage.packageBytes);
    ok &= check(! verifyPackageV3Signature(
                    oldPackage.packageBytes, revokedTrust, parsedOld, issue),
                "publisher revocation blocks historical signature");

    const auto streamingSignedPath = std::filesystem::temp_directory_path()
        / "drs-controlled-signer-streaming-region.bin";
    {
        std::ofstream output(streamingSignedPath, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(oldPackage.packageBytes.data()),
                     static_cast<std::streamsize>(oldPackage.packageBytes.size()
                                                  - packageEd25519SignatureBytes));
    }
    PackagePublisherSigningRequest streamingSigningRequest;
    streamingSigningRequest.signingKeyId = "publisher-old";
    streamingSigningRequest.canonicalSignedFilePath = streamingSignedPath.generic_string();
    streamingSigningRequest.canonicalSignedBytesLength
        = oldPackage.packageBytes.size() - packageEd25519SignatureBytes;
    PackagePublisherSigningResponse streamingSigningResponse;
    ok &= check(oldSigner.signCanonicalPackage(
                    streamingSigningRequest, streamingSigningResponse, issue)
                    && streamingSigningResponse.signature.size()
                        == packageEd25519SignatureBytes,
                "controlled signer streams a canonical staged V3 file");
    std::filesystem::remove(streamingSignedPath);

    RecordingAuditSink unavailableAudit;
    unavailableAudit.available = false;
    drs::signing::ControlledPackageSigner unavailableSigner(
        "publisher-new", PackageSigningKeyState::active,
        SecureBuffer(newPrivateKey), unavailableAudit);
    const auto unavailableRequest = makeWriteRequest(
        activeReleaseKey, "publisher-new", unavailableSigner);
    const auto unavailableWrite = writePackageV3(unavailableRequest);
    ok &= check(! unavailableWrite.written && unavailableWrite.packageBytes.empty(),
                "audit outage fails controlled signing closed");

    drs::signing::ControlledPackageSigner retiredSigner(
        "publisher-old", PackageSigningKeyState::retired,
        SecureBuffer(oldPrivateKey), auditSink);
    const auto retiredSigningRequest = makeWriteRequest(
        activeReleaseKey, "publisher-old", retiredSigner);
    ok &= check(! writePackageV3(retiredSigningRequest).written,
                "retired publisher key cannot sign a new package");

    drs::signing::ControlledPackageSigner attackerSigner(
        "publisher-attacker", PackageSigningKeyState::active,
        SecureBuffer(attackerPrivateKey), auditSink);
    const auto forgedRequest = makeWriteRequest(
        activeReleaseKey, "publisher-attacker", attackerSigner);
    const auto forgedPackage = writePackageV3(forgedRequest);
    auto parsedForged = parsePackageV3(forgedPackage.packageBytes);
    ok &= check(forgedPackage.written && parsedForged.opened
                    && ! verifyPackageV3Signature(
                        forgedPackage.packageBytes, rotatingTrust, parsedForged, issue),
                "release-key disclosure cannot forge a publisher-trusted package");

    PackagePublisherSigningRequest malformedSigningRequest;
    malformedSigningRequest.signingKeyId = "publisher-old";
    const std::vector<std::uint8_t> malformedBytes { 1u, 2u, 3u };
    malformedSigningRequest.canonicalSignedBytes = &malformedBytes;
    PackagePublisherSigningResponse malformedResponse;
    ok &= check(! oldSigner.signCanonicalPackage(
                    malformedSigningRequest, malformedResponse, issue)
                    && malformedResponse.signature.empty(),
                "controlled signer refuses non-canonical package bytes");

    const auto sourceRoot = std::filesystem::path(DRS_PHASE3_SOURCE_ROOT);
    const auto v3Header = readText(sourceRoot / "engine_adapter/include/drs/engine/PackageV3.h");
    const auto appBuild = readText(sourceRoot / "app/CMakeLists.txt");
    const auto topBuild = readText(sourceRoot / "CMakeLists.txt");
    const auto trustTemplate = readText(
        sourceRoot / "engine_adapter/cmake/PackageTrustStore.generated.h.in");
    const auto secureBufferSource = readText(
        sourceRoot / "engine_adapter/src/SecureBuffer.cpp");
    ok &= check(v3Header.find("signingPrivateKey") == std::string::npos,
                "desktop V3 request contains no private signing material");
    ok &= check(appBuild.find("drs_controlled_package_signing") == std::string::npos,
                "desktop targets do not link the controlled signer");
    ok &= check(topBuild.find("DRS_BUILD_CONTROLLED_PACKAGE_SIGNER") == std::string::npos
                    && readText(sourceRoot / "tools/package_signer/CMakeLists.txt")
                        .find("DRS_BUILD_CONTROLLED_PACKAGE_SIGNER") != std::string::npos,
                "controlled signing executable is isolated behind an opt-in build target");
    ok &= check(trustTemplate.find("PUBLIC_KEY") != std::string::npos
                    && trustTemplate.find("PRIVATE_KEY") == std::string::npos
                    && trustTemplate.find("RELEASE_KEY") == std::string::npos,
                "generated desktop trust store accepts public material only");
    ok &= check(secureBufferSource.find("sodium_memzero") != std::string::npos,
                "sensitive buffers use dependable libsodium zeroization");

    return ok ? 0 : 1;
}
