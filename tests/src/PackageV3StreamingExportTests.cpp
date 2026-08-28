#include "PackageProtectionTestSupport.h"

#include <drs/engine/PackageCrypto.h>
#include <drs/engine/PackageSignature.h>
#include <drs/engine/PackageV3FileReader.h>
#include <drs/engine/PackageV3StreamingExport.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (! condition) throw std::runtime_error(message);
}

std::vector<std::uint8_t> readBytes(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

bool contains(const std::vector<std::uint8_t>& bytes, const std::string& needle)
{
    return std::search(bytes.begin(), bytes.end(), needle.begin(), needle.end()) != bytes.end();
}

class StaticKeyProvider final : public drs::engine::PackageKeyProvider
{
public:
    StaticKeyProvider(std::string keyId, std::vector<std::uint8_t> key)
        : keyId_(std::move(keyId)), key_(std::move(key)) {}

    bool resolvePackageKey(const std::string&,
                           const std::string& keyId,
                           drs::engine::PackageKeyUse,
                           drs::engine::SecureBuffer& key,
                           std::string& issue) const override
    {
        key.clear();
        if (keyId != keyId_)
        {
            issue = "unknown release key";
            return false;
        }
        key = drs::engine::SecureBuffer(key_);
        issue.clear();
        return true;
    }

private:
    std::string keyId_;
    std::vector<std::uint8_t> key_;
};

class FailingSigner final : public drs::engine::PackagePublisherSigningClient
{
public:
    bool signCanonicalPackage(const drs::engine::PackagePublisherSigningRequest&,
                              drs::engine::PackagePublisherSigningResponse& response,
                              std::string& issue) const override
    {
        response = {};
        issue = "publisher signing service unavailable";
        return false;
    }
};

drs::engine::PackageV3StreamingWritePlan makePlan(
    const fs::path& output,
    const drs::engine::PackageKeyProvider& provider,
    const drs::engine::PackagePublisherSigningClient& signer,
    const drs::engine::PackagePublisherTrustStore& trust,
    const std::map<std::string, std::vector<std::uint8_t>>& plaintext)
{
    drs::engine::PackageV3StreamingWritePlan plan;
    plan.packageId = "streaming-export-package";
    plan.compatibilityId = "practical-sampler.performance-package.v3";
    plan.outputPath = output.generic_string();
    plan.encryptionKeyId = "release-2026-08";
    plan.signingKeyId = "publisher-2026-08";
    plan.keyProvider = &provider;
    plan.publisherSigner = &signer;
    plan.trustStore = &trust;
    const auto append = [&](const std::string& id,
                            const std::string& kind,
                            const std::uint32_t page)
    {
        const auto bytes = plaintext.at(kind + ":" + id + ":" + std::to_string(page));
        drs::engine::PackageV3StreamingRecordSource source;
        source.recordId = id;
        source.recordKind = kind;
        source.generation = 1;
        source.pageIndex = page;
        source.expectedPlaintextBytes = bytes.size();
        source.sourceLabel = kind + ":" + id;
        source.loadPlaintext = [bytes](std::vector<std::uint8_t>& outputBytes,
                                      std::string& issue)
        {
            outputBytes = bytes;
            issue.clear();
            return true;
        };
        plan.records.push_back(std::move(source));
    };
    append("sample-a", "sample-page", 1);
    append("package-manifest", "manifest", 0);
    append("background-image", "background-image", 0);
    append("sample-a", "sample-head", 0);
    append("sample-a", "sample-page", 0);
    return plan;
}
} // namespace

int main()
{
    try
    {
        using namespace drs::engine;
        const auto root = fs::temp_directory_path() / "drs-package-v3-streaming-tests";
        std::error_code error;
        fs::remove_all(root, error);
        fs::create_directories(root);

        SecureBuffer releaseKey;
        std::string issue;
        require(generateSecurePackageKey(releaseKey, issue), issue);
        const std::vector<std::uint8_t> releaseKeyBytes(
            releaseKey.bytes().begin(), releaseKey.bytes().end());
        StaticKeyProvider provider("release-2026-08", releaseKeyBytes);
        std::vector<std::uint8_t> signingPublicKey;
        std::vector<std::uint8_t> signingPrivateKey;
        require(generatePackageSigningKeyPair(signingPublicKey, signingPrivateKey, issue), issue);
        PackageProtectionTestSigner signer("publisher-2026-08", signingPrivateKey);
        PackagePublisherTrustStore trust({ activeTestSigningKey(
            "publisher-2026-08", signingPublicKey) });
        require(trust.valid(), trust.configurationIssue());

        const std::map<std::string, std::vector<std::uint8_t>> plaintext {
            { "manifest:package-manifest:0",
              std::vector<std::uint8_t> { '{','\"','u','i','L','a','b','e','l','\"',':','\"','C','a','n','a','r','y','\"','}' } },
            { "background-image:background-image:0",
              std::vector<std::uint8_t> { 0xff, 0xd8, 0xff, 'J', 'P', 'E', 'G', '-', 'C', 'A', 'N', 'A', 'R', 'Y' } },
            { "sample-head:sample-a:0",
              std::vector<std::uint8_t> { 'R','I','F','F','W','A','V','E','P','C','M','-','C','A','N','A','R','Y' } },
            { "sample-page:sample-a:0", std::vector<std::uint8_t>(65536u, 0x5au) },
            { "sample-page:sample-a:1", std::vector<std::uint8_t>(4097u, 0xa5u) }
        };

        auto firstPlan = makePlan(root / "first.drpkg", provider, signer, trust, plaintext);
        auto secondPlan = makePlan(root / "second.drpkg", provider, signer, trust, plaintext);
        const auto first = writePackageV3Streaming(firstPlan);
        const auto second = writePackageV3Streaming(secondPlan);
        require(first.written && first.verified && first.atomicallyPublished,
                first.issues.empty() ? "first V3 streaming export failed" : first.issues.front());
        require(second.written && second.verified && second.atomicallyPublished,
                second.issues.empty() ? "second V3 streaming export failed" : second.issues.front());
        require(first.semanticDigest == second.semanticDigest,
                "Equivalent exports must have the same semantic digest.");
        const auto firstBytes = readBytes(firstPlan.outputPath);
        const auto secondBytes = readBytes(secondPlan.outputPath);
        require(firstBytes != secondBytes,
                "Equivalent protected exports must use nondeterministic keys and nonces.");
        require(! contains(firstBytes, "Canary") && ! contains(firstBytes, "RIFF")
                    && ! contains(firstBytes, "WAVE") && ! contains(firstBytes, "PCM-CANARY")
                    && ! contains(firstBytes, "JPEG-CANARY"),
                "Protected package bytes must not expose settings, image, or WAV/PCM canaries.");
        require(first.peakPlaintextBufferBytes <= 65536u
                    && first.peakSealedBufferBytes <= 65536u,
                "Streaming export must retain at most one bounded plaintext/ciphertext record.");
        require(! fs::exists(first.stagingPath) && ! fs::exists(second.stagingPath),
                "Successful export must retire staging files.");

        const auto opened = openPackageV3File(firstPlan.outputPath, trust);
        require(opened.opened && opened.package.records.size() == plaintext.size(),
                "Published V3 package must pass signature-first file open.");
        SecureBuffer resolvedReleaseKey;
        require(provider.resolvePackageKey(opened.package.packageId,
                                           opened.package.encryptionKeyId,
                                           PackageKeyUse::decryptExistingPackage,
                                           resolvedReleaseKey, issue), issue);
        SecureBuffer contentKey;
        require(unwrapPackageV3ContentKey(opened.package, resolvedReleaseKey,
                                         contentKey, issue), issue);
        for (const auto& descriptor : opened.package.records)
        {
            const auto record = openPackageV3FileRecord(opened, contentKey, descriptor);
            const auto key = descriptor.recordKind + ":" + descriptor.recordId + ":"
                + std::to_string(descriptor.pageIndex);
            require(record.opened && record.plaintext == plaintext.at(key),
                    "Every staged record must reopen to its original semantic bytes.");
        }
        for (std::size_t left = 0; left < opened.package.records.size(); ++left)
        for (std::size_t right = left + 1; right < opened.package.records.size(); ++right)
            require(opened.package.records[left].sealed.nonce
                        != opened.package.records[right].sealed.nonce,
                    "Every record in an export must use a unique random nonce.");

        const auto stableBytes = readBytes(firstPlan.outputPath);
        std::size_t probes = 0;
        const auto canceled = writePackageV3Streaming(
            firstPlan, { {}, [&] { return ++probes >= 3u; } });
        require(! canceled.written && canceled.failure == PackageV3StreamingFailure::cancelled,
                "Cancellation must fail closed.");
        require(readBytes(firstPlan.outputPath) == stableBytes
                    && ! fs::exists(canceled.stagingPath),
                "Cancellation must preserve the published file and remove staging.");

        const std::vector<PackageV3StreamingWriteStage> cancellableStages {
            PackageV3StreamingWriteStage::loadingRecord,
            PackageV3StreamingWriteStage::sealingRecord,
            PackageV3StreamingWriteStage::writingRecord,
            PackageV3StreamingWriteStage::finalizingIndex,
            PackageV3StreamingWriteStage::signing,
            PackageV3StreamingWriteStage::verifying,
            PackageV3StreamingWriteStage::publishing
        };
        for (std::size_t index = 0; index < cancellableStages.size(); ++index)
        {
            auto stagePlan = makePlan(root / ("cancel-stage-" + std::to_string(index) + ".drpkg"),
                                      provider, signer, trust, plaintext);
            bool cancelStage = false;
            const auto stageResult = writePackageV3Streaming(
                stagePlan,
                { [&](const PackageV3StreamingWriteProgress& progress)
                  {
                      if (progress.stage == cancellableStages[index]) cancelStage = true;
                  },
                  [&] { return cancelStage; } });
            require(! stageResult.written
                        && stageResult.failure == PackageV3StreamingFailure::cancelled
                        && ! fs::exists(stagePlan.outputPath)
                        && ! fs::exists(stageResult.stagingPath),
                    "Cancellation at every V3 export stage must publish nothing and clean staging.");
        }

        FailingSigner failingSigner;
        auto signingFailurePlan = makePlan(root / "signing-failure.drpkg",
                                           provider, failingSigner, trust, plaintext);
        const auto signingFailure = writePackageV3Streaming(signingFailurePlan);
        require(! signingFailure.written
                    && signingFailure.failure == PackageV3StreamingFailure::signing
                    && ! fs::exists(signingFailurePlan.outputPath)
                    && ! fs::exists(signingFailure.stagingPath),
                "Signing outage must publish nothing and remove staging.");

        auto unavailablePlan = makePlan(root / "missing-key.drpkg",
                                        provider, signer, trust, plaintext);
        unavailablePlan.encryptionKeyId = "unknown-release-key";
        const auto unavailable = writePackageV3Streaming(unavailablePlan);
        require(! unavailable.written
                    && unavailable.failure == PackageV3StreamingFailure::keyUnavailable
                    && ! fs::exists(unavailablePlan.outputPath),
                "Unknown release key must fail before publication.");

        auto mutatedBytes = firstBytes;
        mutatedBytes[opened.package.payloadOffset] ^= 0x01u;
        std::ofstream mutated(root / "mutated.drpkg", std::ios::binary | std::ios::trunc);
        mutated.write(reinterpret_cast<const char*>(mutatedBytes.data()),
                      static_cast<std::streamsize>(mutatedBytes.size()));
        mutated.close();
        require(! openPackageV3File((root / "mutated.drpkg").generic_string(), trust).opened,
                "Mutation of a published V3 payload must fail publisher verification.");

        fs::remove_all(root, error);
        std::cout << "Package V3 streaming export tests passed\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Package V3 streaming export tests failed: " << exception.what() << '\n';
        return 1;
    }
}
