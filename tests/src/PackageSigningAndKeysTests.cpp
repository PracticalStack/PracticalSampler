#include <drs/engine/PackageKeyEnvelope.h>

#include <iostream>

namespace
{
bool check(bool condition, const char* message)
{
    if (! condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    using namespace drs::engine;
    std::string issue;
    SecureBuffer contentKey, releaseKey;
    std::vector<std::uint8_t> publicKey, privateKey;
    bool ok = generateSecurePackageKey(contentKey, issue) && generateSecurePackageKey(releaseKey, issue)
              && generatePackageSigningKeyPair(publicKey, privateKey, issue);
    PackageKeyEnvelope envelope;
    ok &= check(wrapPackageContentKey("pkg", "release-1", contentKey, releaseKey, envelope, issue), "wrap content key");
    SecureBuffer unwrapped;
    ok &= check(unwrapPackageContentKey("pkg", envelope, releaseKey, unwrapped, issue), "unwrap content key");
    ok &= check(unwrapped.bytes() == contentKey.bytes(), "content key round trip");
    SecureBuffer wrong;
    ok &= check(generateSecurePackageKey(wrong, issue), "generate wrong release key");
    ok &= check(! unwrapPackageContentKey("pkg", envelope, wrong, unwrapped, issue), "wrong release key rejected");

    const std::vector<std::uint8_t> bytes { 'V', '3', 'p', 'a', 'y', 'l', 'o', 'a', 'd' };
    std::vector<std::uint8_t> signature;
    ok &= check(packageSignEd25519(privateKey, bytes, signature, issue), "sign package");
    std::vector<PackageSigningKey> trust {
        { "signing-1", publicKey, PackageSigningKeyState::active,
          "2026-08-28T00:00:00Z", {}, {} }
    };
    ok &= check(verifyPackageSignature(bytes, signature, "signing-1", trust, issue), "trust-store verify");
    trust[0].state = PackageSigningKeyState::revoked;
    trust[0].revokedUtc = "2026-08-28T00:00:00Z";
    ok &= check(! verifyPackageSignature(bytes, signature, "signing-1", trust, issue), "revoked key rejected");
    if (! ok) return 1;
    std::cout << "Package signing and key tests passed\n";
    return 0;
}
