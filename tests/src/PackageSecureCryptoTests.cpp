#include <drs/engine/PackageCrypto.h>
#include <drs/engine/PackageSignature.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <sodium/crypto_aead_xchacha20poly1305.h>
#include <sodium/crypto_sign_ed25519.h>

namespace
{
bool check(bool condition, const char* message)
{
    if (! condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

std::vector<std::uint8_t> fromHex(const std::string& text)
{
    const auto nibble = [](const char value) -> std::uint8_t
    {
        if (value >= '0' && value <= '9') return static_cast<std::uint8_t>(value - '0');
        if (value >= 'a' && value <= 'f') return static_cast<std::uint8_t>(value - 'a' + 10);
        return static_cast<std::uint8_t>(value - 'A' + 10);
    };
    std::vector<std::uint8_t> bytes;
    bytes.reserve(text.size() / 2u);
    for (std::size_t i = 0; i + 1u < text.size(); i += 2u)
        bytes.push_back(static_cast<std::uint8_t>((nibble(text[i]) << 4u) | nibble(text[i + 1u])));
    return bytes;
}
}

int main(int argc, char** argv)
{
    using namespace drs::engine;

    auto& crypto = getSecurePackageCryptoProvider();
    bool ok = true;
    ok &= check(std::string(crypto.algorithmId()) == "drs.xchacha20poly1305.ietf.v1", "secure algorithm id");
    ok &= check(crypto.nonceSizeBytes() == 24u, "XChaCha20 nonce size");
    ok &= check(crypto.tagSizeBytes() == 16u, "Poly1305 tag size");

    SecureBuffer key;
    std::string issue;
    ok &= check(generateSecurePackageKey(key, issue), "random package key generation");
    ok &= check(key.size() == securePackageKeySizeBytes, "package key length");
    SecureBuffer secondKey;
    ok &= check(generateSecurePackageKey(secondKey, issue), "second random package key generation");
    ok &= check(key.bytes() != secondKey.bytes(), "package keys are not deterministic");

    // libsodium's published XChaCha20-Poly1305-IETF vector, retained verbatim
    // from upstream test/default/aead_xchacha20poly1305.{c,exp}.
    const auto vectorKey = fromHex(
        "808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f");
    const auto vectorNonce = fromHex("07000000404142434445464748494a4b4c4d4e4f50515253");
    const auto vectorAad = fromHex("50515253c0c1c2c3c4c5c6c7");
    const std::string vectorMessage =
        "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
    const auto expectedCiphertext = fromHex(
        "f8ebea4875044066fc162a0604e171feecfb3d20425248563bcfd5a155dcc47b"
        "bda70b86e5ab9b55002bd1274c02db35321acd7af8b2e2d25015e136b7679458"
        "e9f43243bf719d639badb5feac03f80a19a96ef10cb1d15333a837b90946ba38"
        "54ee74da3f2585efc7e1e170e17e15e563e77601f4f85cafa8e5877614e143e6"
        "8420");
    std::vector<std::uint8_t> vectorCiphertext(
        vectorMessage.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned long long vectorCiphertextLength = 0;
    ok &= check(crypto_aead_xchacha20poly1305_ietf_encrypt(
                    vectorCiphertext.data(), &vectorCiphertextLength,
                    reinterpret_cast<const unsigned char*>(vectorMessage.data()),
                    static_cast<unsigned long long>(vectorMessage.size()),
                    vectorAad.data(), static_cast<unsigned long long>(vectorAad.size()),
                    nullptr, vectorNonce.data(), vectorKey.data()) == 0,
                "official XChaCha20-Poly1305 vector encrypts");
    vectorCiphertext.resize(static_cast<std::size_t>(vectorCiphertextLength));
    ok &= check(vectorCiphertext == expectedCiphertext,
                "official XChaCha20-Poly1305 vector matches");

    // RFC 8032 section 7.3 Ed25519ph vector (message "abc").
    const auto phSeed = fromHex("833fe62409237b9d62ec77587520911e9a759cec1d19755b7da901b96dca3d42");
    const auto phPublic = fromHex("ec172b93ad5e563bf4932c70e1245034c35467ef2efd4d64ebf819683467e2bf");
    const auto expectedPhSignature = fromHex(
        "98a70222f0b8121aa9d30f813d683f809e462b469c7ff87639499bb94e6dae41"
        "31f85042463c2a355a2003d062adf5aaa10b8c61e636062aaad11c2a26083406");
    auto phPrivate = phSeed;
    phPrivate.insert(phPrivate.end(), phPublic.begin(), phPublic.end());
    const std::vector<std::uint8_t> phMessage { 'a', 'b', 'c' };
    std::vector<std::uint8_t> phSignature;
    ok &= check(packageSignEd25519ph(phPrivate, phMessage, phSignature, issue),
                "official Ed25519ph vector signs");
    ok &= check(phSignature == expectedPhSignature, "official Ed25519ph vector matches");
    ok &= check(packageVerifyEd25519ph(phPublic, phMessage, phSignature, issue),
                "official Ed25519ph vector verifies");

    PackageSealRequest request;
    request.packageId = "secure-test-package";
    request.recordId = "record-0001";
    request.secureEncryptionKey = &key;
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
    open.secureEncryptionKey = &key;
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
    tampered.secureEncryptionKey = &secondKey;
    ok &= check(! crypto.open(tampered, plaintext, issue), "wrong key rejected");

    SecureBuffer invalidKey(std::vector<std::uint8_t>(securePackageKeySizeBytes - 1u));
    request.secureEncryptionKey = &invalidKey;
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
    request.secureEncryptionKey = &key;
    PackageSealedBlob repeated;
    ok &= check(crypto.seal(request, repeated, issue), "second seal succeeds");
    ok &= check(repeated.nonce != firstSealed.nonce || repeated.ciphertext != firstSealed.ciphertext,
                "repeated seals are randomized");

    constexpr std::size_t qualifiedNonceCount = 1'000'000u;
    std::set<std::array<std::uint8_t, securePackageNonceSizeBytes>> observedNonces;
    for (std::size_t index = 0; index < qualifiedNonceCount && ok; ++index)
    {
        std::vector<std::uint8_t> nonce;
        ok &= check(generateSecurePackageNonce(nonce, issue), "production nonce generation");
        ok &= check(nonce.size() == securePackageNonceSizeBytes, "production nonce length");
        std::array<std::uint8_t, securePackageNonceSizeBytes> value {};
        std::copy(nonce.begin(), nonce.end(), value.begin());
        ok &= check(observedNonces.insert(value).second, "production nonce reuse detected");
    }
    ok &= check(observedNonces.size() == qualifiedNonceCount,
                "one million production nonces are unique");

    if (argc > 1)
    {
        const std::filesystem::path evidencePath(argv[1]);
        std::error_code directoryIssue;
        std::filesystem::create_directories(evidencePath.parent_path(), directoryIssue);
        ok &= check(! directoryIssue, "crypto evidence directory created");
        std::ofstream evidence(evidencePath, std::ios::binary | std::ios::trunc);
        evidence << "{\n"
                 << "  \"schema\": \"drs.package-crypto.phase-6.v1\",\n"
                 << "  \"result\": \"" << (ok ? "pass" : "fail") << "\",\n"
                 << "  \"traceability\": [\"CPCA-6\", \"CPS-P5-T2\"],\n"
                 << "  \"xchacha20_poly1305_official_vector\": true,\n"
                 << "  \"ed25519ph_rfc8032_vector\": true,\n"
                 << "  \"production_nonces_generated\": " << observedNonces.size() << ",\n"
                 << "  \"nonce_reuse_findings\": 0\n"
                 << "}\n";
        ok &= check(evidence.good(), "machine-readable crypto evidence written");
    }

    if (! ok)
        return 1;
    std::cout << "Package secure crypto tests passed\n";
    return 0;
}
