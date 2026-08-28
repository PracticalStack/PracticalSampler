#pragma once

#include "drs/engine/PackageV3.h"

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
inline constexpr std::size_t packageV3SignatureStreamBufferBytes = 64u * 1024u;

// Blocking file I/O and cryptography. Call only from a loader/streaming worker,
// never from the audio callback.
struct PackageV3FileOpenResult
{
    bool opened = false;
    std::string packagePath;
    std::uint64_t packageBytes = 0;
    std::uint64_t indexBytesRead = 0;
    std::uint64_t signatureBytesRead = 0;
    std::uint64_t verificationBytesRead = 0;
    std::uint64_t peakReadBufferBytes = 0;
    PackageV3OpenResult package;
    std::vector<std::string> issues;
};

struct PackageV3FileRecordOpenResult
{
    bool opened = false;
    std::uint64_t ciphertextBytesRead = 0;
    std::vector<std::uint8_t> plaintext;
    std::vector<std::string> issues;
};

PackageV3FileOpenResult openPackageV3File(
    const std::string& packagePath,
    const std::vector<PackageSigningKey>& trustStore);

PackageV3FileOpenResult openPackageV3File(
    const std::string& packagePath,
    const PackagePublisherTrustStore& trustStore);

PackageV3FileRecordOpenResult openPackageV3FileRecord(
    const PackageV3FileOpenResult& file,
    const SecureBuffer& contentKey,
    const PackageV3RecordDescriptor& record);
} // namespace drs::engine
