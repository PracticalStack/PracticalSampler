#pragma once

#include "drs/engine/PackageWriter.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"

#include <string>
#include <vector>

namespace drs::engine
{
using PerformancePackageReaderResult = PerformancePackageInspectionResult;

struct PerformancePackagePayloadLoadResult
{
    bool found = false;
    bool loaded = false;
    std::string state;
    std::vector<std::string> issues;
    PerformancePackagePayloadView payload;
};

struct PerformancePackageLoadResult
{
    bool packageFound = false;
    bool loaded = false;
    std::string packagePath;
    std::string state;
    std::vector<std::string> issues;
    PerformancePackageReaderResult package;
    PerformancePackageManifest manifest;
    RuntimeManifestLoadResult instrument;
    RuntimeStreamLoadResult stream;
};

PerformancePackageReaderResult readPerformancePackage(
    const std::string& packagePath,
    const PackageCryptoProvider& cryptoProvider = getDeterministicPackageCryptoProvider(),
    int supportedReaderSchemaVersion = performancePackageSchemaVersion);

PerformancePackagePayloadLoadResult openPerformancePackagePayload(
    const PerformancePackageReaderResult& package,
    const std::string& payloadId,
    const PackageCryptoProvider& cryptoProvider = getDeterministicPackageCryptoProvider());

PerformancePackageLoadResult loadPerformancePackage(
    const std::string& packagePath,
    const PackageCryptoProvider& cryptoProvider = getDeterministicPackageCryptoProvider(),
    int supportedReaderSchemaVersion = performancePackageSchemaVersion);
} // namespace drs::engine
