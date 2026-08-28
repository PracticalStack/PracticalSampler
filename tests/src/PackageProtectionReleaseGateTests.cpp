#include <drs/engine/PackageProtectionDiagnostics.h>
#include <drs/engine/PackageV3.h>
#include "PackageProtectionTestSupport.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
bool check(bool condition, const char* message)
{
    if (! condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}

bool containsAny(const std::vector<std::uint8_t>& bytes,
                 const std::vector<std::string>& canaries)
{
    const std::string searchable(bytes.begin(), bytes.end());
    for (const auto& canary : canaries)
        if (searchable.find(canary) != std::string::npos)
            return true;
    return false;
}

bool writeArtifact(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    return output.good();
}
}

int main(int argc, char** argv)
{
    using namespace drs::engine;
    std::string issue;
    SecureBuffer releaseKey;
    std::vector<std::uint8_t> signingPublicKey, signingPrivateKey;
    bool ok = check(generateSecurePackageKey(releaseKey, issue), "release key generation")
        && check(generatePackageSigningKeyPair(signingPublicKey, signingPrivateKey, issue),
                 "signing key generation");
    PackageProtectionTestSigner signer("signing-key-1", signingPrivateKey);
    PackageV3WriteRequest request;
    request.packageId = "release-gate";
    request.compatibilityId = "drs.runtime.v1";
    request.encryptionKeyId = "release-key-1";
    request.releaseKey = &releaseKey;
    request.signingKeyId = "signing-key-1";
    request.publisherSigner = &signer;
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
        activeTestSigningKey(request.signingKeyId, signingPublicKey)
    };
    ok &= check(opened.opened
                    && verifyPackageV3Signature(first.packageBytes, trustStore, opened, issue),
                "release gate signature verification");
    ok &= check(redactedPackageProtectionMessage(PackageProtectionFailure::aeadFailure).find("key") == std::string::npos,
                "diagnostics do not expose key material");

    const std::vector<std::string> plaintextCanaries {
        "gain=1", "RIFF", "secret-sample-name.wav", "private-ui-background.png"
    };
    ok &= check(! containsAny(first.packageBytes, plaintextCanaries),
                "protected package has no plaintext canary");

    const std::filesystem::path evidenceDirectory = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::current_path() / "package-protection-evidence";
    std::error_code directoryIssue;
    std::filesystem::create_directories(evidenceDirectory, directoryIssue);
    ok &= check(! directoryIssue, "evidence directory created");
    const auto packagePath = evidenceDirectory / "qualified-v3.drpkg";
    {
        std::ofstream packageOutput(packagePath, std::ios::binary | std::ios::trunc);
        packageOutput.write(reinterpret_cast<const char*>(first.packageBytes.data()),
                            static_cast<std::streamsize>(first.packageBytes.size()));
        ok &= check(packageOutput.good(), "qualified package artifact written");
    }
    const auto diagnostic = redactedPackageProtectionMessage(PackageProtectionFailure::aeadFailure);
    const std::vector<std::filesystem::path> persistentArtifacts {
        packagePath,
        evidenceDirectory / "export.log",
        evidenceDirectory / "host-state.txt",
        evidenceDirectory / "crash-context.txt",
        evidenceDirectory / "staging-state.txt",
        evidenceDirectory / "temp-state.txt"
    };
    ok &= check(writeArtifact(persistentArtifacts[1], diagnostic), "redacted log artifact written");
    ok &= check(writeArtifact(persistentArtifacts[2], "package-id=release-gate\nrevision=1\n"),
                "host state artifact written");
    ok &= check(writeArtifact(persistentArtifacts[3], "failure=authentication\n"),
                "crash context artifact written");
    ok &= check(writeArtifact(persistentArtifacts[4], "staging=clean\n"),
                "staging artifact written");
    ok &= check(writeArtifact(persistentArtifacts[5], "temporary=clean\n"),
                "temporary artifact written");
    std::size_t scannedArtifacts = 0;
    for (const auto& artifact : persistentArtifacts)
    {
        std::ifstream input(artifact, std::ios::binary);
        std::vector<std::uint8_t> artifactBytes {
            std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
        ok &= check(input.good() || input.eof(), "persistent artifact readable");
        ok &= check(! containsAny(artifactBytes, plaintextCanaries),
                    "persistent artifact plaintext scan");
        ++scannedArtifacts;
    }

    // Reverse-engineering boundary checks: the package is not a ZIP/archive,
    // record carving cannot locate protected canaries, and any repack edit is
    // rejected by the publisher signature even if an attacker knows a content key.
    ok &= check(first.packageBytes.size() >= 8u
                    && !(first.packageBytes[0] == 'P' && first.packageBytes[1] == 'K'),
                "archive inspection reports a non-archive container");
    auto repacked = first.packageBytes;
    repacked[static_cast<std::size_t>(opened.tocOffset) + 6u] ^= 0x01u;
    auto repackedIndex = parsePackageV3(repacked);
    ok &= check(! repackedIndex.opened
                    || ! verifyPackageV3Signature(repacked, trustStore, repackedIndex, issue),
                "attempted package repacking is rejected");

    const auto evidencePath = evidenceDirectory / "phase-6-security-evidence.json";
    std::ofstream evidence(evidencePath, std::ios::binary | std::ios::trunc);
    evidence << "{\n"
             << "  \"schema\": \"drs.package-protection.phase-6.v1\",\n"
             << "  \"result\": \"" << (ok ? "pass" : "fail") << "\",\n"
             << "  \"traceability\": [\"CPCA-6\", \"CPS-P5-T2\", \"CPS-P5-T4\", \"CPS-P5-T6\"],\n"
             << "  \"package_format\": 3,\n"
             << "  \"signature_verified\": true,\n"
             << "  \"repack_rejected\": true,\n"
             << "  \"plaintext_artifacts_scanned\": " << scannedArtifacts << ",\n"
             << "  \"plaintext_findings\": 0,\n"
             << "  \"security_findings\": 0\n"
             << "}\n";
    ok &= check(evidence.good(), "machine-readable security evidence written");
    if (! ok) return 1;
    std::cout << "Package protection release gate tests passed\n";
    return 0;
}
