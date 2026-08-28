#pragma once

#include "drs/engine/PackageCrypto.h"
#include "drs/engine/PackageKeyEnvelope.h"
#include "drs/engine/PackagePublisherSigning.h"

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
inline constexpr std::uint32_t packageV3FormatVersion = 3;
inline constexpr std::uint16_t packageV3CryptoSuiteXChaCha20Poly1305 = 1;
inline constexpr std::uint16_t packageV3SignatureSuiteEd25519ph = 1;
inline constexpr std::size_t packageV3FixedHeaderBytes = 76;
inline constexpr std::size_t packageV3MaximumHeaderBytes = 64u * 1024u;
inline constexpr std::size_t packageV3MaximumTocBytes = 256u * 1024u * 1024u;
// Accurate Salamander requires roughly 41k bounded sample-head/page records.
// Keep the parser finite while leaving room for larger supported instruments.
inline constexpr std::size_t packageV3MaximumRecords = 131072;
inline constexpr std::size_t packageV3MaximumRecordBytes = 64u * 1024u * 1024u;
inline constexpr std::uint64_t packageV3MaximumPackageBytes = 16ull * 1024ull * 1024ull * 1024ull;
inline constexpr std::size_t packageV3MaximumIdentityBytes = 4096;

struct PackageV3RecordInput
{
    std::string recordId;
    std::string recordKind;
    std::uint32_t generation = 0;
    std::uint32_t pageIndex = 0;
    std::vector<std::uint8_t> plaintext;
};

struct PackageV3WriteRequest
{
    std::string packageId;
    std::string compatibilityId;
    std::string encryptionKeyId;
    const SecureBuffer* releaseKey = nullptr;
    std::string signingKeyId;
    const PackagePublisherSigningClient* publisherSigner = nullptr;
    std::vector<PackageV3RecordInput> records;
};

struct PackageV3WriteResult
{
    bool written = false;
    std::vector<std::uint8_t> packageBytes;
    std::vector<std::uint8_t> semanticDigest;
    std::string signingAuditId;
    std::vector<std::string> issues;
};

struct PackageV3RecordDescriptor
{
    std::uint32_t ordinal = 0;
    std::string recordId;
    std::string recordKind;
    std::uint32_t generation = 0;
    std::uint32_t pageIndex = 0;
    std::uint64_t plaintextSize = 0;
    std::uint64_t ciphertextOffset = 0;
    std::uint64_t ciphertextSize = 0;
    PackageSealedBlob sealed;
};

struct PackageV3OpenResult
{
    bool opened = false;
    bool signatureVerified = false;
    std::string packageId;
    std::string compatibilityId;
    std::string encryptionKeyId;
    std::string signingKeyId;
    PackageKeyEnvelope keyEnvelope;
    std::uint64_t tocOffset = 0;
    std::uint64_t tocSize = 0;
    std::uint64_t payloadOffset = 0;
    std::uint64_t payloadSize = 0;
    std::uint64_t signatureOffset = 0;
    std::vector<std::uint8_t> signature;
    std::vector<PackageV3RecordDescriptor> records;
    std::vector<std::string> issues;
};

std::vector<std::uint8_t> buildPackageV3RecordAad(
    const std::string& packageId,
    const std::string& compatibilityId,
    std::uint32_t ordinal,
    const std::string& recordId,
    const std::string& recordKind,
    std::uint32_t generation,
    std::uint32_t pageIndex,
    std::uint64_t plaintextSize);

PackageV3WriteResult writePackageV3(const PackageV3WriteRequest& request);
PackageV3OpenResult parsePackageV3(const std::vector<std::uint8_t>& packageBytes);
PackageV3OpenResult parsePackageV3Index(const std::vector<std::uint8_t>& indexBytes,
                                        std::uint64_t totalPackageBytes);

bool verifyPackageV3Signature(const std::vector<std::uint8_t>& packageBytes,
                              const std::vector<PackageSigningKey>& trustStore,
                              PackageV3OpenResult& package,
                              std::string& issue);

bool verifyPackageV3Signature(const std::vector<std::uint8_t>& packageBytes,
                              const PackagePublisherTrustStore& trustStore,
                              PackageV3OpenResult& package,
                              std::string& issue);

bool unwrapPackageV3ContentKey(const PackageV3OpenResult& package,
                               const SecureBuffer& releaseKey,
                               SecureBuffer& contentKey,
                               std::string& issue);

bool openPackageV3Record(const SecureBuffer& contentKey,
                         const PackageV3OpenResult& package,
                         const PackageV3RecordDescriptor& record,
                         std::vector<std::uint8_t>& plaintext,
                         std::string& issue);
} // namespace drs::engine
