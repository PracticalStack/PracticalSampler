#include <drs/engine/PackageV3FileReader.h>
#include "PackageProtectionTestSupport.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
bool check(const bool condition, const std::string& message)
{
    if (! condition) std::cerr << "FAIL: " << message << '\n';
    return condition;
}

bool writeFile(const std::filesystem::path& path,
               const std::vector<std::uint8_t>& bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    return output.good();
}

void setU32(std::vector<std::uint8_t>& bytes,
            const std::size_t offset,
            const std::uint32_t value)
{
    for (int index = 0; index < 4; ++index)
        bytes[offset + static_cast<std::size_t>(index)]
            = static_cast<std::uint8_t>(value >> (index * 8));
}

void setU16(std::vector<std::uint8_t>& bytes,
            const std::size_t offset,
            const std::uint16_t value)
{
    bytes[offset] = static_cast<std::uint8_t>(value & 0xffu);
    bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8u);
}

void setU64(std::vector<std::uint8_t>& bytes,
            const std::size_t offset,
            const std::uint64_t value)
{
    for (int index = 0; index < 8; ++index)
        bytes[offset + static_cast<std::size_t>(index)]
            = static_cast<std::uint8_t>(value >> (index * 8));
}

const drs::engine::PackageV3RecordDescriptor* findRecord(
    const drs::engine::PackageV3OpenResult& package,
    const std::string& recordId)
{
    const auto found = std::find_if(package.records.begin(), package.records.end(),
        [&](const auto& record) { return record.recordId == recordId; });
    return found == package.records.end() ? nullptr : &*found;
}
} // namespace

int main()
{
    using namespace drs::engine;
    bool ok = true;
    std::string issue;
    SecureBuffer releaseKey;
    std::vector<std::uint8_t> signingPublicKey;
    std::vector<std::uint8_t> signingPrivateKey;
    ok &= check(generateSecurePackageKey(releaseKey, issue), "generate release key");
    ok &= check(generatePackageSigningKeyPair(
                    signingPublicKey, signingPrivateKey, issue),
                "generate signing key pair");
    PackageProtectionTestSigner signer("publisher-2026-08", signingPrivateKey);

    PackageV3WriteRequest request;
    request.packageId = "reader-package";
    request.compatibilityId = "sampler-runtime-1";
    request.encryptionKeyId = "release-2026-08";
    request.releaseKey = &releaseKey;
    request.signingKeyId = "publisher-2026-08";
    request.publisherSigner = &signer;
    request.records = {
        { "manifest", "metadata", 1u, 0u, { 'h', 'e', 'l', 'l', 'o' } },
        { "page-0", "sample-page", 1u, 0u, std::vector<std::uint8_t>(1024u * 1024u, 0x5au) },
        { "page-1", "sample-page", 1u, 1u, std::vector<std::uint8_t>(32768u, 0xa5u) }
    };
    const auto written = writePackageV3(request);
    ok &= check(written.written, "write signed V3 fixture");
    if (! written.written) return 1;

    const auto unique = std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    const auto directory = std::filesystem::temp_directory_path()
        / ("drs-v3-file-reader-" + unique);
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    const auto packagePath = directory / "instrument.drspkg";
    ok &= check(! error && writeFile(packagePath, written.packageBytes), "write file fixture");

    const std::vector<PackageSigningKey> trustStore {
        activeTestSigningKey(request.signingKeyId, signingPublicKey)
    };
    auto opened = openPackageV3File(packagePath.string(), trustStore);
    ok &= check(opened.opened && opened.package.signatureVerified,
                "signature-first file open succeeds");
    ok &= check(opened.verificationBytesRead == opened.package.signatureOffset,
                "streaming verifier covers every signed byte");
    ok &= check(opened.indexBytesRead == opened.package.payloadOffset,
                "only bounded header and TOC are retained as the index");
    ok &= check(opened.signatureBytesRead == packageEd25519SignatureBytes,
                "only the fixed signature range is read separately");
    ok &= check(opened.peakReadBufferBytes
                    <= std::max<std::uint64_t>(opened.indexBytesRead,
                                               packageV3SignatureStreamBufferBytes),
                "reader buffer ceiling is independent of payload size");
    ok &= check(std::all_of(opened.package.records.begin(), opened.package.records.end(),
                           [](const auto& record) { return record.sealed.ciphertext.empty(); }),
                "file index retains no ciphertext payloads");

    SecureBuffer contentKey;
    ok &= check(unwrapPackageV3ContentKey(
                    opened.package, releaseKey, contentKey, issue),
                "content key unwrap follows signature verification");
    const auto* manifest = findRecord(opened.package, "manifest");
    const auto* page0 = findRecord(opened.package, "page-0");
    ok &= check(manifest != nullptr && page0 != nullptr, "requested records indexed");
    if (manifest != nullptr)
    {
        const auto record = openPackageV3FileRecord(opened, contentKey, *manifest);
        ok &= check(record.opened
                        && record.plaintext == std::vector<std::uint8_t>({ 'h', 'e', 'l', 'l', 'o' }),
                    "requested metadata record decrypts");
        ok &= check(record.ciphertextBytesRead == manifest->ciphertextSize,
                    "metadata open reads exactly its ciphertext range");

        auto forged = *manifest;
        forged.pageIndex += 1u;
        const auto rejected = openPackageV3FileRecord(opened, contentKey, forged);
        ok &= check(! rejected.opened && rejected.plaintext.empty(),
                    "untrusted descriptor cannot select a range");
    }
    if (page0 != nullptr)
    {
        const auto record = openPackageV3FileRecord(opened, contentKey, *page0);
        ok &= check(record.opened && record.plaintext == request.records[1].plaintext,
                    "one requested sample page decrypts independently");
        ok &= check(record.ciphertextBytesRead == page0->ciphertextSize
                        && record.ciphertextBytesRead < opened.package.payloadSize,
                    "one page open does not read unrelated records");
    }

    SecureBuffer wrongContentKey;
    ok &= check(generateSecurePackageKey(wrongContentKey, issue), "generate wrong content key");
    if (manifest != nullptr)
    {
        const auto rejected = openPackageV3FileRecord(opened, wrongContentKey, *manifest);
        ok &= check(! rejected.opened && rejected.plaintext.empty(),
                    "wrong content key returns no plaintext");
    }
    SecureBuffer wrongReleaseKey;
    ok &= check(generateSecurePackageKey(wrongReleaseKey, issue), "generate wrong release key");
    contentKey = SecureBuffer(std::vector<std::uint8_t>(securePackageKeySizeBytes, 0xccu));
    ok &= check(! unwrapPackageV3ContentKey(
                    opened.package, wrongReleaseKey, contentKey, issue)
                    && contentKey.empty(),
                "wrong release key returns no content key");

    const std::vector<PackageSigningKey> unknownStore;
    ok &= check(! openPackageV3File(packagePath.string(), unknownStore).opened,
                "unknown signing key rejected before key unwrap");
    const std::vector<PackageSigningKey> revokedStore {
        { request.signingKeyId, signingPublicKey, PackageSigningKeyState::revoked,
          "2026-08-01T00:00:00Z", {}, "2026-08-28T00:00:00Z" }
    };
    ok &= check(! openPackageV3File(packagePath.string(), revokedStore).opened,
                "revoked signing key rejected");
    std::vector<std::uint8_t> otherPublicKey, otherPrivateKey;
    ok &= check(generatePackageSigningKeyPair(otherPublicKey, otherPrivateKey, issue),
                "generate unrelated signing key");
    ok &= check(! openPackageV3File(packagePath.string(), {
                    activeTestSigningKey(request.signingKeyId, otherPublicKey) }).opened,
                "wrong publisher public key rejected");

    auto mutation = written.packageBytes;
    mutation[static_cast<std::size_t>(opened.package.payloadOffset)] ^= 0x01u;
    ok &= check(writeFile(packagePath, mutation)
                    && ! openPackageV3File(packagePath.string(), trustStore).opened,
                "payload mutation fails publisher signature");

    auto truncated = written.packageBytes;
    truncated.resize(truncated.size() - 1u);
    ok &= check(writeFile(packagePath, truncated)
                    && ! openPackageV3File(packagePath.string(), trustStore).opened,
                "truncated signature rejected");

    auto oversizedHeader = written.packageBytes;
    setU32(oversizedHeader, 12u,
           static_cast<std::uint32_t>(packageV3MaximumHeaderBytes + 1u));
    const auto headerRejected = writeFile(packagePath, oversizedHeader)
        ? openPackageV3File(packagePath.string(), trustStore) : PackageV3FileOpenResult {};
    ok &= check(! headerRejected.opened
                    && headerRejected.indexBytesRead == packageV3FixedHeaderBytes,
                "oversized header rejected after fixed-header read");

    auto oversizedToc = written.packageBytes;
    setU64(oversizedToc, 36u,
           static_cast<std::uint64_t>(packageV3MaximumTocBytes) + 1u);
    const auto tocRejected = writeFile(packagePath, oversizedToc)
        ? openPackageV3File(packagePath.string(), trustStore) : PackageV3FileOpenResult {};
    ok &= check(! tocRejected.opened
                    && tocRejected.indexBytesRead == packageV3FixedHeaderBytes,
                "oversized TOC rejected before allocation");

    auto reorderedToc = written.packageBytes;
    setU32(reorderedToc, static_cast<std::size_t>(opened.package.tocOffset), 1u);
    const auto reorderedRejected = writeFile(packagePath, reorderedToc)
        ? openPackageV3File(packagePath.string(), trustStore) : PackageV3FileOpenResult {};
    ok &= check(! reorderedRejected.opened
                    && reorderedRejected.verificationBytesRead == 0u,
                "non-canonical TOC ordering rejected before signature acceptance");

    auto adversarialLength = written.packageBytes;
    setU16(adversarialLength,
           static_cast<std::size_t>(opened.package.tocOffset) + 4u, 0xffffu);
    const auto lengthRejected = writeFile(packagePath, adversarialLength)
        ? openPackageV3File(packagePath.string(), trustStore) : PackageV3FileOpenResult {};
    ok &= check(! lengthRejected.opened && lengthRejected.verificationBytesRead == 0u,
                "adversarial identity length rejected within bounded index");

    auto duplicateIdentity = written.packageBytes;
    const std::vector<std::uint8_t> page1Name { 'p', 'a', 'g', 'e', '-', '1' };
    const auto page1Position = std::search(
        duplicateIdentity.begin(), duplicateIdentity.end(),
        page1Name.begin(), page1Name.end());
    ok &= check(page1Position != duplicateIdentity.end(), "locate duplicate mutation target");
    if (page1Position != duplicateIdentity.end())
    {
        const auto nameOffset = static_cast<std::size_t>(
            std::distance(duplicateIdentity.begin(), page1Position));
        duplicateIdentity[nameOffset + page1Name.size() - 1u] = '0';
        setU32(duplicateIdentity, nameOffset + page1Name.size() + 2u + 11u + 4u, 0u);
        const auto duplicateRejected = writeFile(packagePath, duplicateIdentity)
            ? openPackageV3File(packagePath.string(), trustStore) : PackageV3FileOpenResult {};
        ok &= check(! duplicateRejected.opened
                        && duplicateRejected.verificationBytesRead == 0u,
                    "duplicate record identity rejected within bounded index");
    }

    auto declaredOversized = written.packageBytes;
    setU64(declaredOversized, 52u, packageV3MaximumPackageBytes);
    const auto declaredOversizedRejected = writeFile(packagePath, declaredOversized)
        ? openPackageV3File(packagePath.string(), trustStore) : PackageV3FileOpenResult {};
    ok &= check(! declaredOversizedRejected.opened
                    && declaredOversizedRejected.indexBytesRead == packageV3FixedHeaderBytes,
                "declared sparse/oversized layout rejected before allocation");

    ok &= check(writeFile(packagePath, written.packageBytes), "restore verified file");
    opened = openPackageV3File(packagePath.string(), trustStore);
    contentKey.clear();
    ok &= check(opened.opened
                    && unwrapPackageV3ContentKey(
                        opened.package, releaseKey, contentKey, issue),
                "reopen fixture before post-verification mutation");
    manifest = findRecord(opened.package, "manifest");
    if (manifest != nullptr)
    {
        mutation = written.packageBytes;
        mutation[static_cast<std::size_t>(manifest->ciphertextOffset)] ^= 0x80u;
        ok &= check(writeFile(packagePath, mutation), "mutate requested record after verification");
        const auto rejected = openPackageV3FileRecord(opened, contentKey, *manifest);
        ok &= check(! rejected.opened && rejected.plaintext.empty(),
                    "post-verification ciphertext mutation fails AEAD with no plaintext");
    }

    std::filesystem::remove_all(directory, error);
    return ok ? 0 : 1;
}
