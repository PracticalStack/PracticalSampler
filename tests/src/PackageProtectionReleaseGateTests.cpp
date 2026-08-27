#include <drs/engine/PackageProtectionDiagnostics.h>
#include <drs/engine/PackageV3.h>

#include <chrono>
#include <iostream>
#include <string>

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
    std::vector<std::uint8_t> releaseKey, signingPublicKey, signingPrivateKey;
    bool ok = check(generateSecurePackageKey(releaseKey, issue), "release key generation")
        && check(generatePackageSigningKeyPair(signingPublicKey, signingPrivateKey, issue),
                 "signing key generation");
    PackageV3WriteRequest request;
    request.packageId = "release-gate";
    request.compatibilityId = "drs.runtime.v1";
    request.encryptionKeyId = "release-key-1";
    request.releaseKey = releaseKey;
    request.signingKeyId = "signing-key-1";
    request.signingPrivateKey = signingPrivateKey;
    request.records = {
        { "settings", "runtime-settings", 1, 0, { 'g', 'a', 'i', 'n', '=', '1' } },
        { "sample", "sample-page", 1, 0, { 0x52, 0x49, 0x46, 0x46, 0x00, 0x01 } }
    };
    const auto start = std::chrono::steady_clock::now();
    const auto first = writePackageV3(request);
    const auto second = writePackageV3(request);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    ok &= check(first.written && second.written, "release gate writes");
    ok &= check(first.packageBytes != second.packageBytes, "encrypted outputs are nondeterministic");
    ok &= check(first.semanticDigest == second.semanticDigest, "semantic digest is stable");
    const std::string bytes(first.packageBytes.begin(), first.packageBytes.end());
    ok &= check(bytes.find("gain") == std::string::npos, "settings are not package plaintext");
    ok &= check(bytes.find("RIFF") == std::string::npos, "WAV marker is not package plaintext");
    ok &= check(elapsed < 2000, "small package crypto budget");
    auto opened = parsePackageV3(first.packageBytes);
    const std::vector<PackageSigningKey> trustStore {
        { request.signingKeyId, signingPublicKey, false }
    };
    ok &= check(opened.opened
                    && verifyPackageV3Signature(first.packageBytes, trustStore, opened, issue),
                "release gate signature verification");
    ok &= check(redactedPackageProtectionMessage(PackageProtectionFailure::aeadFailure).find("key") == std::string::npos,
                "diagnostics do not expose key material");
    if (! ok) return 1;
    std::cout << "Package protection release gate tests passed\n";
    return 0;
}
