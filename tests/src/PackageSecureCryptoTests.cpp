#include <drs/engine/PackageCrypto.h>
#include <drs/engine/PackageSignature.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
bool check(bool condition, const char* message)
{
    if (! condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    using namespace drs::engine;

    auto& crypto = getSecurePackageCryptoProvider();
    bool ok = true;
    ok &= check(std::string(crypto.algorithmId()) == "drs.xchacha20poly1305.ietf.v1", "secure algorithm id");
    ok &= check(crypto.nonceSizeBytes() == 24u, "XChaCha20 nonce size");
    ok &= check(crypto.tagSizeBytes() == 16u, "Poly1305 tag size");

    std::vector<std::uint8_t> key;
    std::string issue;
    ok &= check(generateSecurePackageKey(key, issue), "random package key generation");
    ok &= check(key.size() == securePackageKeySizeBytes, "package key length");
    std::vector<std::uint8_t> secondKey;
    ok &= check(generateSecurePackageKey(secondKey, issue), "second random package key generation");
    ok &= check(key != secondKey, "package keys are not deterministic");

    PackageSealRequest request;
    request.packageId = "secure-test-package";
    request.recordId = "record-0001";
    request.encryptionKey = key;
    request.additionalAuthenticatedData = "drs.package.v3|record|0|1|12";
    request.plaintext.assign({ 's', 'e', 'c', 'r', 'e', 't', '-', 's', 'a', 'm', 'p', 'l', 'e' });

    PackageSealedBlob sealed;
    ok &= check(crypto.seal(request, sealed, issue), "seal succeeds");
    ok &= check(sealed.nonce.size() == crypto.nonceSizeBytes(), "nonce length");
    ok &= check(sealed.tag.size() == crypto.tagSizeBytes(), "tag length");
    ok &= check(sealed.ciphertext.size() == request.plaintext.size(), "ciphertext length");
    const auto firstSealed = sealed;

    PackageOpenRequest open;
    open.packageId = request.packageId;
    open.recordId = request.recordId;
    open.encryptionKey = key;
    open.additionalAuthenticatedData = request.additionalAuthenticatedData;
    open.sealed = sealed;
    std::vector<std::uint8_t> plaintext;
    ok &= check(crypto.open(open, plaintext, issue), "open succeeds");
    ok &= check(plaintext == request.plaintext, "round trip plaintext");

    auto tampered = open;
    tampered.sealed.ciphertext[0] ^= 0x01u;
    ok &= check(! crypto.open(tampered, plaintext, issue), "ciphertext tamper rejected");
    ok &= check(plaintext.empty(), "tamper clears plaintext");
    tampered = open;
    tampered.sealed.tag[0] ^= 0x01u;
    ok &= check(! crypto.open(tampered, plaintext, issue), "tag tamper rejected");
    tampered = open;
    tampered.additionalAuthenticatedData += "-changed";
    ok &= check(! crypto.open(tampered, plaintext, issue), "AAD tamper rejected");
    tampered = open;
    tampered.encryptionKey = secondKey;
    ok &= check(! crypto.open(tampered, plaintext, issue), "wrong key rejected");

    std::vector<std::uint8_t> invalidKey(securePackageKeySizeBytes - 1u);
    request.encryptionKey = invalidKey;
    ok &= check(! crypto.seal(request, sealed, issue), "invalid key rejected");

    std::vector<std::uint8_t> signingPublicKey, signingPrivateKey, signature;
    ok &= check(generatePackageSigningKeyPair(signingPublicKey, signingPrivateKey, issue),
                "Ed25519 key generation");
    const std::vector<std::uint8_t> signedMessage { 'p', 'a', 'c', 'k', 'a', 'g', 'e' };
    ok &= check(packageSignEd25519(signingPrivateKey, signedMessage, signature, issue),
                "Ed25519 signing");
    ok &= check(packageVerifyEd25519(signingPublicKey, signedMessage, signature, issue),
                "Ed25519 verification");
    auto modifiedMessage = signedMessage;
    modifiedMessage[0] ^= 0x01u;
    ok &= check(! packageVerifyEd25519(signingPublicKey, modifiedMessage, signature, issue),
                "modified signed bytes rejected");

    // Repeated seals of the same record must not reuse a nonce or produce the
    // deterministic ciphertext behavior of the legacy provider.
    request.encryptionKey = key;
    PackageSealedBlob repeated;
    ok &= check(crypto.seal(request, repeated, issue), "second seal succeeds");
    ok &= check(repeated.nonce != firstSealed.nonce || repeated.ciphertext != firstSealed.ciphertext,
                "repeated seals are randomized");

    if (! ok)
        return 1;
    std::cout << "Package secure crypto tests passed\n";
    return 0;
}
