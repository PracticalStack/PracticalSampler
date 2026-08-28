#include <drs/engine/PackageV3.h>
#include "PackageProtectionTestSupport.h"

#include <algorithm>
#include <cstdint>
#include <iostream>

namespace
{
bool check(const bool condition, const char* message)
{
    if (! condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}

struct FixtureKeys
{
    drs::engine::SecureBuffer releaseKey;
    std::vector<std::uint8_t> signingPublicKey;
    std::vector<std::uint8_t> signingPrivateKey;
};

bool makeKeys(FixtureKeys& keys, std::string& issue)
{
    return drs::engine::generateSecurePackageKey(keys.releaseKey, issue)
        && drs::engine::generatePackageSigningKeyPair(
            keys.signingPublicKey, keys.signingPrivateKey, issue);
}

drs::engine::PackageV3WriteRequest makeRequest(
    const FixtureKeys& keys,
    const drs::engine::PackagePublisherSigningClient& signer)
{
    drs::engine::PackageV3WriteRequest request;
    request.packageId = "v3-fixture";
    request.compatibilityId = "drs.runtime.v1";
    request.encryptionKeyId = "release-key-test";
    request.releaseKey = &keys.releaseKey;
    request.signingKeyId = "signing-key-test";
    request.publisherSigner = &signer;
    // Deliberately non-canonical input order: the writer must canonicalize it.
    request.records = {
        { "sample-page-0", "sample-page", 4, 0, { 0x10, 0x20, 0x30, 0x40 } },
        { "runtime-settings", "settings", 4, 0,
          { '{', '"', 'g', 'a', 'i', 'n', '"', ':', '1', '}' } },
        { "empty-head", "sample-head", 4, 0, {} }
    };
    return request;
}

const drs::engine::PackageV3RecordDescriptor* findRecord(
    const drs::engine::PackageV3OpenResult& package,
    const std::string& recordId)
{
    const auto found = std::find_if(package.records.begin(), package.records.end(),
        [&](const auto& record) { return record.recordId == recordId; });
    return found == package.records.end() ? nullptr : &*found;
}

bool mutationIsRejected(const std::vector<std::uint8_t>& original,
                        const std::size_t offset,
                        const std::vector<drs::engine::PackageSigningKey>& trustStore)
{
    if (offset >= original.size()) return false;
    auto mutated = original;
    mutated[offset] ^= 0x01u;
    auto parsed = drs::engine::parsePackageV3(mutated);
    if (! parsed.opened) return true;
    std::string issue;
    return ! drs::engine::verifyPackageV3Signature(mutated, trustStore, parsed, issue);
}

std::uint32_t readU32At(const std::vector<std::uint8_t>& bytes, const std::size_t offset)
{
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i)
        value |= static_cast<std::uint32_t>(bytes[offset + static_cast<std::size_t>(i)]) << (i * 8);
    return value;
}
} // namespace

int main()
{
    using namespace drs::engine;
    bool ok = true;
    std::string issue;
    FixtureKeys keys;
    ok &= check(makeKeys(keys, issue), "fixture keys generated");
    PackageProtectionTestSigner signer("signing-key-test", keys.signingPrivateKey);
    const auto request = makeRequest(keys, signer);

    const auto written = writePackageV3(request);
    ok &= check(written.written, "V3 write");
    ok &= check(written.packageBytes.size() > 64u, "V3 bytes emitted");
    ok &= check(written.semanticDigest.size() == 32u, "semantic SHA-256 length");
    const std::vector<std::uint8_t> expectedMagic { 'D','R','S','P','K','G','3',0 };
    ok &= check(std::equal(written.packageBytes.begin(), written.packageBytes.begin() + 8,
                           expectedMagic.begin()),
                "V3 magic golden bytes");
    ok &= check(readU32At(written.packageBytes, 8u) == packageV3FormatVersion,
                "V3 version fixed offset");
    ok &= check(readU32At(written.packageBytes, 68u) == packageEd25519SignatureBytes,
                "V3 signature size fixed offset");
    ok &= check(packageV3MaximumRecords == 131072u,
                "V3 bounded record limit supports the qualified large corpus");

    auto package = parsePackageV3(written.packageBytes);
    ok &= check(package.opened, "V3 structural parse");
    ok &= check(! package.signatureVerified, "parse alone does not establish trust");
    ok &= check(package.records.size() == request.records.size(), "V3 record count");
    ok &= check(package.records[0].recordKind <= package.records[1].recordKind,
                "writer uses canonical record ordering");

    SecureBuffer contentKey;
    ok &= check(! unwrapPackageV3ContentKey(package, keys.releaseKey, contentKey, issue),
                "content key is unavailable before signature verification");
    const std::vector<PackageSigningKey> trustStore {
        activeTestSigningKey(request.signingKeyId, keys.signingPublicKey)
    };
    ok &= check(verifyPackageV3Signature(written.packageBytes, trustStore, package, issue),
                "publisher signature verifies");
    ok &= check(unwrapPackageV3ContentKey(package, keys.releaseKey, contentKey, issue),
                "wrapped content key opens after signature verification");
    ok &= check(contentKey.size() == securePackageKeySizeBytes, "unwrapped key length");

    const auto* settings = findRecord(package, "runtime-settings");
    ok &= check(settings != nullptr, "settings record found");
    std::vector<std::uint8_t> plaintext;
    if (settings != nullptr)
    {
        ok &= check(openPackageV3Record(contentKey, package, *settings, plaintext, issue),
                    "verified V3 record opens");
        ok &= check(plaintext == request.records[1].plaintext, "V3 metadata round trip");
        auto forged = *settings;
        forged.recordKind += "-forged";
        ok &= check(! openPackageV3Record(contentKey, package, forged, plaintext, issue),
                    "descriptor outside verified package is rejected");
    }

    const auto* emptyHead = findRecord(package, "empty-head");
    if (emptyHead != nullptr)
    {
        ok &= check(openPackageV3Record(contentKey, package, *emptyHead, plaintext, issue),
                    "empty record opens");
        ok &= check(plaintext.empty(), "empty record remains empty");
    }

    SecureBuffer wrongReleaseKey;
    ok &= check(generateSecurePackageKey(wrongReleaseKey, issue),
                "wrong release key generated");
    ok &= check(! unwrapPackageV3ContentKey(package, wrongReleaseKey, contentKey, issue),
                "wrong release key rejected");

    const auto repeated = writePackageV3(request);
    ok &= check(repeated.written, "repeated V3 write");
    ok &= check(repeated.packageBytes != written.packageBytes,
                "identical semantic exports use different encrypted bytes");
    ok &= check(repeated.semanticDigest == written.semanticDigest,
                "identical semantics use the same SHA-256 digest");

    auto reorderedSemantics = request;
    std::reverse(reorderedSemantics.records.begin(), reorderedSemantics.records.end());
    const auto reorderedWrite = writePackageV3(reorderedSemantics);
    ok &= check(reorderedWrite.written && reorderedWrite.semanticDigest == written.semanticDigest,
                "input order canonicalizes to identical semantics");
    auto changedIdentity = request;
    changedIdentity.records[0].recordId += "-changed";
    const auto changedIdentityWrite = writePackageV3(changedIdentity);
    ok &= check(changedIdentityWrite.written
                    && changedIdentityWrite.semanticDigest != written.semanticDigest,
                "semantic digest binds record identity");

    auto duplicate = request;
    duplicate.records.push_back(duplicate.records.front());
    ok &= check(! writePackageV3(duplicate).written, "duplicate identity rejected by writer");

    const auto ambiguousLeft = buildPackageV3RecordAad(
        "pkg", "compat", 0, "a|b", "c", 1, 2, 3);
    const auto ambiguousRight = buildPackageV3RecordAad(
        "pkg", "compat", 0, "a", "b|c", 1, 2, 3);
    ok &= check(! ambiguousLeft.empty() && ambiguousLeft != ambiguousRight,
                "length-prefixed AAD eliminates delimiter ambiguity");
    const std::vector<std::uint8_t> goldenAad {
        'D','R','S','A','A','D','3',0,
        3,0,0,0, 1,0, 1,0,0,0,
        1,0,'p', 1,0,'c', 1,0,'r', 1,0,'k',
        2,0,0,0, 3,0,0,0, 4,0,0,0,0,0,0,0
    };
    ok &= check(buildPackageV3RecordAad("p", "c", 1, "r", "k", 2, 3, 4)
                    == goldenAad,
                "canonical AAD golden vector");

    auto badMagic = written.packageBytes;
    badMagic[0] ^= 0x01u;
    ok &= check(! parsePackageV3(badMagic).opened, "bad magic rejected");
    auto badVersion = written.packageBytes;
    badVersion[8] = 4u;
    ok &= check(! parsePackageV3(badVersion).opened, "future version rejected");
    auto badFlags = written.packageBytes;
    badFlags[16] ^= 0x08u;
    ok &= check(! parsePackageV3(badFlags).opened, "unknown flags rejected");
    auto badSuite = written.packageBytes;
    badSuite[20] = 2u;
    ok &= check(! parsePackageV3(badSuite).opened, "unknown crypto suite rejected");
    auto badOffset = written.packageBytes;
    badOffset[28] ^= 0x01u;
    ok &= check(! parsePackageV3(badOffset).opened, "non-canonical TOC offset rejected");
    auto truncated = written.packageBytes;
    truncated.pop_back();
    ok &= check(! parsePackageV3(truncated).opened, "signature truncation rejected");
    auto trailing = written.packageBytes;
    trailing.push_back(0u);
    ok &= check(! parsePackageV3(trailing).opened, "trailing byte rejected");

    constexpr std::size_t fixedHeaderBytes = 76u;
    const auto packageIdStart = fixedHeaderBytes + 2u;
    const auto compatibilityIdStart = packageIdStart + request.packageId.size() + 2u;
    const auto encryptionKeyIdStart = compatibilityIdStart + request.compatibilityId.size() + 2u;
    const auto signingKeyIdStart = encryptionKeyIdStart + request.encryptionKeyId.size() + 2u;
    const auto envelopeNonceStart = signingKeyIdStart + request.signingKeyId.size();
    const auto envelopeCiphertextStart = envelopeNonceStart + 24u + 2u;
    const auto envelopeTagStart = envelopeCiphertextStart + securePackageKeySizeBytes;
    const auto firstToc = static_cast<std::size_t>(package.tocOffset);
    const auto firstRecordIdStart = firstToc + 4u + 2u;
    const auto firstRecordKindLength = firstRecordIdStart + package.records[0].recordId.size();
    const auto firstRecordKindStart = firstRecordKindLength + 2u;
    const auto firstGeneration = firstRecordKindStart + package.records[0].recordKind.size();
    const auto firstPage = firstGeneration + 4u;
    const auto firstPlaintextSize = firstPage + 4u;
    const auto firstCiphertextOffset = firstPlaintextSize + 8u;
    const auto firstCiphertextSize = firstCiphertextOffset + 8u;
    const auto firstNonce = firstCiphertextSize + 8u;
    const auto firstTag = firstNonce + 24u;
    const std::vector<std::size_t> signedMutationOffsets {
        packageIdStart, compatibilityIdStart, encryptionKeyIdStart, signingKeyIdStart,
        envelopeNonceStart, envelopeCiphertextStart, envelopeTagStart,
        firstToc, firstRecordIdStart, firstRecordKindStart, firstGeneration, firstPage,
        firstPlaintextSize, firstCiphertextOffset, firstCiphertextSize, firstNonce, firstTag,
        static_cast<std::size_t>(package.payloadOffset)
    };
    for (const auto offset : signedMutationOffsets)
        ok &= check(mutationIsRejected(written.packageBytes, offset, trustStore),
                    "signed header/TOC/payload mutation rejected");
    bool completeMutationMatrixPassed = true;
    for (std::size_t offset = 0; offset < written.packageBytes.size(); ++offset)
        completeMutationMatrixPassed = completeMutationMatrixPassed
            && mutationIsRejected(written.packageBytes, offset, trustStore);
    ok &= check(completeMutationMatrixPassed,
                "every single-byte package mutation is rejected");

    auto badSignature = written.packageBytes;
    badSignature.back() ^= 0x01u;
    auto badSignaturePackage = parsePackageV3(badSignature);
    ok &= check(badSignaturePackage.opened, "structurally valid signature mutation parses");
    ok &= check(! verifyPackageV3Signature(
                    badSignature, trustStore, badSignaturePackage, issue),
                "signature mutation rejected");

    auto tocMutation = written.packageBytes;
    const auto firstRecordIdOffset = static_cast<std::size_t>(package.tocOffset) + 6u;
    tocMutation[firstRecordIdOffset] ^= 0x01u;
    auto tocMutationPackage = parsePackageV3(tocMutation);
    if (tocMutationPackage.opened)
        ok &= check(! verifyPackageV3Signature(
                        tocMutation, trustStore, tocMutationPackage, issue),
                    "TOC mutation rejected by signature");

    if (! ok) return 1;
    std::cout << "Package V3 canonical format tests passed\n";
    return 0;
}
