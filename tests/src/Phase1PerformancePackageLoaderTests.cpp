#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageWriter.h"
#include "Phase1PerformancePackageSupport.h"

#include <json/json.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;
namespace package_support = drs::tests::performance_package;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    try
    {
        const auto& cryptoProvider = drs::engine::getDeterministicPackageCryptoProvider();
        const auto scratchDirectory = fs::temp_directory_path() / "drs-phase1-performance-package-loader-tests";
        std::error_code errorCode;
        fs::remove_all(scratchDirectory, errorCode);
        fs::create_directories(scratchDirectory);

        const auto checkedInCorpus = package_support::getCheckedInCorpusPaths();
        require(fs::exists(checkedInCorpus.valid), "Checked-in valid performance package fixture must exist.");
        require(fs::exists(checkedInCorpus.tampered), "Checked-in tampered performance package fixture must exist.");
        require(fs::exists(checkedInCorpus.wrongVersion), "Checked-in wrong-version performance package fixture must exist.");
        require(fs::exists(checkedInCorpus.missingPayload), "Checked-in missing-payload performance package fixture must exist.");
        require(fs::exists(checkedInCorpus.checksumMismatch), "Checked-in checksum-mismatch performance package fixture must exist.");

        const auto loadedPackage = drs::engine::loadPerformancePackage(checkedInCorpus.valid.generic_string(),
                                                                       cryptoProvider,
                                                                       drs::engine::performancePackageSchemaVersion);
        require(loadedPackage.loaded, "Performance package should load through the dedicated runtime loader path.");
        require(loadedPackage.failureCategory == drs::engine::PerformancePackageFailureCategory::none,
                "Valid performance package should not publish a failure category.");
        require(loadedPackage.manifest.packageId == "drs.phase1.tiny-open-instrument.package",
                "Loaded package manifest id changed unexpectedly.");
        require(loadedPackage.instrument.loaded, "Package runtime instrument payload should parse successfully.");
        require(loadedPackage.stream.loaded, "Package runtime stream payload should parse successfully.");
        require(loadedPackage.instrument.instrument.sourceProjectPath == "package://authoring/unavailable",
                "Package runtime loader should keep the runtime instrument on the package-owned authoring placeholder.");
        require(loadedPackage.stream.container.payloadEmbedded,
                "Package runtime loader should hydrate embedded stream payload bytes.");
        require(loadedPackage.stream.metrics.payloadAssetResolved,
                "Package runtime loader should resolve the package-owned stream payload.");
        require(loadedPackage.stream.metrics.payloadChecksumValidatedCount == 3,
                "Package runtime loader payload checksum validation count changed unexpectedly.");
        require(loadedPackage.stream.container.samples.at(0).sourcePath.rfind("package://sample/", 0) == 0,
                "Package runtime loader should preserve package-owned sample identities instead of raw file paths.");

        const auto checksumMismatchLoad = drs::engine::loadPerformancePackage(
            checkedInCorpus.checksumMismatch.generic_string(),
            cryptoProvider,
            drs::engine::performancePackageSchemaVersion);
        require(!checksumMismatchLoad.loaded,
                "Package loader should reject a package whose embedded stream payload no longer matches the stream index.");
        require(checksumMismatchLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::payloadCorruption,
                "Checksum-mismatch package should report the payload-corruption failure category.");
        require(package_support::containsIssue(checksumMismatchLoad.issues, "payload checksum mismatch"),
                "Checksum-mismatch package load should report a payload checksum mismatch.");

        const auto futureReaderLoad = drs::engine::loadPerformancePackage(
            checkedInCorpus.wrongVersion.generic_string(),
            cryptoProvider,
            drs::engine::performancePackageSchemaVersion);
        require(!futureReaderLoad.loaded, "Package loader should reject unsupported reader schema versions.");
        require(futureReaderLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::packageFormatFailure,
                "Unsupported-reader package should report the package-format failure category.");
        require(package_support::containsIssue(futureReaderLoad.issues, "requires reader schema version"),
                "Unsupported-reader package load should explain the reader schema version mismatch.");

        const auto missingPayloadLoad = drs::engine::loadPerformancePackage(
            checkedInCorpus.missingPayload.generic_string(),
            cryptoProvider,
            drs::engine::performancePackageSchemaVersion);
        require(!missingPayloadLoad.loaded, "Package loader should reject packages missing required runtime payloads.");
        require(missingPayloadLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::payloadCorruption,
                "Missing-payload package should report the payload-corruption failure category.");
        require(package_support::containsIssue(missingPayloadLoad.issues, "runtimeStreamIndex payload"),
                "Missing-payload package load should report the missing runtime stream-index payload.");

        const auto tamperedLoad = drs::engine::loadPerformancePackage(checkedInCorpus.tampered.generic_string(),
                                                                      cryptoProvider,
                                                                      drs::engine::performancePackageSchemaVersion);
        require(!tamperedLoad.loaded, "Package loader should reject a package with a tampered encrypted TOC.");
        require(tamperedLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::decryptionFailure,
                "Tampered package should report the decryption failure category.");
        require(package_support::containsIssue(tamperedLoad.issues, "authentication failed"),
                "Tampered package load should report an authentication failure.");

        const auto compatiblePlan = package_support::buildPackagePlan(scratchDirectory / "playback-incompatible",
                                                                      scratchDirectory / "playback-incompatible" / "playback-incompatible.drpkg");
        auto incompatibleWritePlan = drs::engine::buildPerformancePackageWritePlan(compatiblePlan);
        auto instrumentPayloadIterator = std::find_if(
            incompatibleWritePlan.payloads.begin(),
            incompatibleWritePlan.payloads.end(),
            [](const drs::engine::PerformancePackagePayloadSource& payload)
            {
                return payload.kind == drs::engine::PerformancePackagePayloadKind::runtimeInstrument;
            });
        require(instrumentPayloadIterator != incompatibleWritePlan.payloads.end(),
                "Playback-incompatible package plan should include a runtime-instrument payload.");
        auto instrumentJson = json::parse(std::string(instrumentPayloadIterator->plaintextBytes.begin(),
                                                      instrumentPayloadIterator->plaintextBytes.end()));
        instrumentJson["instrumentId"] = "drs.phase1.incompatible-runtime-instrument";
        instrumentPayloadIterator->plaintextBytes = package_support::toBytes(instrumentJson.dump(2) + "\n");
        incompatibleWritePlan.outputPackagePath = (scratchDirectory / "playback-incompatible" / "playback-incompatible.drpkg").generic_string();

        const auto incompatibleWrite = drs::engine::writePerformancePackage(incompatibleWritePlan, cryptoProvider);
        require(incompatibleWrite.written, "Playback-incompatible package should still write successfully.");

        const auto incompatibleLoad = drs::engine::loadPerformancePackage(incompatibleWritePlan.outputPackagePath,
                                                                          cryptoProvider,
                                                                          drs::engine::performancePackageSchemaVersion);
        require(!incompatibleLoad.loaded,
                "Package loader should reject a package whose runtime instrument no longer matches the package manifest.");
        require(incompatibleLoad.failureCategory
                    == drs::engine::PerformancePackageFailureCategory::playbackCompatibilityFailure,
                "Manifest/instrument mismatches should report the playback-compatibility failure category.");
        require(package_support::containsIssue(incompatibleLoad.issues, "instrumentId"),
                "Playback-incompatible package load should report the manifest/instrument mismatch.");

        std::cout << "Phase 1 performance package loader tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 performance package loader tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
