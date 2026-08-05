#include "drs/engine/EngineFacade.h"
#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageWriter.h"
#include "Phase1PerformancePackageSupport.h"

#include <json/json.hpp>

#include <algorithm>
#include <cmath>
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

        const auto scopedGainPlan = package_support::buildPackagePlan(scratchDirectory / "scoped-gain",
                                                                      scratchDirectory / "scoped-gain" / "scoped-gain.drpkg");
        const auto scopedGainWrite = drs::engine::writePerformancePackage(scopedGainPlan, cryptoProvider);
        require(scopedGainWrite.written, "Scoped-gain performance package should write successfully.");

        const auto scopedGainLoad = drs::engine::loadPerformancePackage(scopedGainPlan.outputPackagePath,
                                                                        cryptoProvider,
                                                                        drs::engine::performancePackageSchemaVersion);
        require(scopedGainLoad.loaded, "Scoped-gain performance package should load successfully.");
        require(std::abs(scopedGainLoad.manifest.masterGainDb - scopedGainPlan.manifest.masterGainDb) < 1.0e-9,
                "Typed package load should preserve packaged master gain.");
        require(scopedGainLoad.manifest.groupRoutes.size() == scopedGainPlan.manifest.groupRoutes.size(),
                "Typed package load should preserve the packaged group-route count.");
        require(scopedGainLoad.manifest.groupRoutes.at(0).groupId == scopedGainPlan.manifest.groupRoutes.at(0).groupId
                    && std::abs(scopedGainLoad.manifest.groupRoutes.at(0).gainDb
                                    - scopedGainPlan.manifest.groupRoutes.at(0).gainDb)
                        < 1.0e-9,
                "Typed package load should preserve packaged group-route gain.");
        require(scopedGainLoad.instrument.instrument.groups.size() == scopedGainPlan.compiledRuntime.instrument.groups.size(),
                "Typed package load should preserve runtime group gain entries.");
        require(std::abs(scopedGainLoad.instrument.instrument.groups.at(0).gainDb
                             - scopedGainPlan.compiledRuntime.instrument.groups.at(0).gainDb)
                    < 1.0e-9,
                "Typed package load should preserve runtime group gain from the packaged instrument payload.");
        require(std::abs(scopedGainLoad.instrument.instrument.zones.at(0).gainDb
                             - scopedGainPlan.compiledRuntime.instrument.zones.at(0).gainDb)
                    < 1.0e-9,
                "Typed package load should preserve runtime zone gain from the packaged instrument payload.");

        drs::engine::EngineFacade scopedGainFacade;
        const auto scopedGainActivation = scopedGainFacade.activatePerformancePackageSession(scopedGainLoad);
        require(scopedGainActivation.activated,
                "Performance package activation should succeed for a package with scoped gain.");
        const auto scopedGainPayload = scopedGainFacade.getPerformancePackageActivationPayload();
        require(scopedGainPayload != nullptr && scopedGainPayload->snapshot != nullptr
                    && scopedGainPayload->prepared != nullptr,
                "Performance package activation should retain snapshot and prepared payloads.");
        require(std::abs(scopedGainPayload->snapshot->masterGainDb - scopedGainPlan.manifest.masterGainDb) < 1.0e-9,
                "Activated performance-package snapshot should preserve packaged master gain.");
        require(std::abs(scopedGainPayload->snapshot->groupRoutes.at(0).gainDb
                             - scopedGainPlan.manifest.groupRoutes.at(0).gainDb)
                    < 1.0e-9,
                "Activated performance-package snapshot should preserve packaged group-route gain.");
        require(std::abs(scopedGainPayload->snapshot->zones.at(0).gainDb
                             - scopedGainPlan.compiledRuntime.instrument.zones.at(0).gainDb)
                    < 1.0e-9,
                "Activated performance-package snapshot should preserve packaged zone gain.");
        require(std::abs(scopedGainPayload->prepared->masterGainDb - scopedGainPlan.manifest.masterGainDb) < 1.0e-9,
                "Activated prepared playback should preserve packaged master gain.");
        require(std::abs(scopedGainPayload->prepared->groupRoutes.at(0).gainDb
                             - scopedGainPlan.manifest.groupRoutes.at(0).gainDb)
                    < 1.0e-9,
                "Activated prepared playback should preserve packaged group-route gain.");
        require(std::abs(scopedGainPayload->prepared->zones.at(0).gainDb
                             - scopedGainPlan.compiledRuntime.instrument.zones.at(0).gainDb)
                    < 1.0e-9,
                "Activated prepared playback should preserve packaged zone gain.");

        auto malformedManifestPlan = drs::engine::buildPerformancePackageWritePlan(
            package_support::buildPackagePlan(scratchDirectory / "malformed-manifest",
                                              scratchDirectory / "malformed-manifest" / "malformed-manifest.drpkg"));
        auto packageManifestIterator = std::find_if(
            malformedManifestPlan.payloads.begin(),
            malformedManifestPlan.payloads.end(),
            [](const drs::engine::PerformancePackagePayloadSource& payload)
            {
                return payload.kind == drs::engine::PerformancePackagePayloadKind::packageManifest;
            });
        require(packageManifestIterator != malformedManifestPlan.payloads.end(),
                "Malformed-manifest package plan should include a package-manifest payload.");
        auto malformedManifestJson = json::parse(std::string(packageManifestIterator->plaintextBytes.begin(),
                                                             packageManifestIterator->plaintextBytes.end()));
        malformedManifestJson["groupRoutes"][0]["gainDb"] = "not-a-number";
        packageManifestIterator->plaintextBytes = package_support::toBytes(malformedManifestJson.dump(2) + "\n");
        malformedManifestPlan.outputPackagePath = (scratchDirectory / "malformed-manifest" / "malformed-manifest.drpkg").generic_string();

        const auto malformedManifestWrite = drs::engine::writePerformancePackage(malformedManifestPlan, cryptoProvider);
        require(malformedManifestWrite.written, "Malformed-manifest package should still write successfully.");

        const auto malformedManifestLoad = drs::engine::loadPerformancePackage(malformedManifestPlan.outputPackagePath,
                                                                               cryptoProvider,
                                                                               drs::engine::performancePackageSchemaVersion);
        require(!malformedManifestLoad.loaded,
                "Package loader should reject package manifests whose scoped gain fields are malformed.");
        require(malformedManifestLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::payloadCorruption,
                "Malformed package-manifest gain fields should report payload corruption.");
        require(package_support::containsIssue(malformedManifestLoad.issues, "groupRoutes[0]")
                    && package_support::containsIssue(malformedManifestLoad.issues, "gainDb"),
                "Malformed package-manifest gain validation should report the bad packaged group-route gain field.");

        auto malformedInstrumentPlan = drs::engine::buildPerformancePackageWritePlan(
            package_support::buildPackagePlan(scratchDirectory / "malformed-instrument",
                                              scratchDirectory / "malformed-instrument" / "malformed-instrument.drpkg"));
        auto malformedInstrumentIterator = std::find_if(
            malformedInstrumentPlan.payloads.begin(),
            malformedInstrumentPlan.payloads.end(),
            [](const drs::engine::PerformancePackagePayloadSource& payload)
            {
                return payload.kind == drs::engine::PerformancePackagePayloadKind::runtimeInstrument;
            });
        require(malformedInstrumentIterator != malformedInstrumentPlan.payloads.end(),
                "Malformed-instrument package plan should include a runtime-instrument payload.");
        auto malformedInstrumentJson = json::parse(std::string(malformedInstrumentIterator->plaintextBytes.begin(),
                                                               malformedInstrumentIterator->plaintextBytes.end()));
        malformedInstrumentJson["zones"][0]["gainDb"] = "bad-zone-gain";
        malformedInstrumentIterator->plaintextBytes = package_support::toBytes(malformedInstrumentJson.dump(2) + "\n");
        malformedInstrumentPlan.outputPackagePath = (scratchDirectory / "malformed-instrument" / "malformed-instrument.drpkg").generic_string();

        const auto malformedInstrumentWrite = drs::engine::writePerformancePackage(malformedInstrumentPlan, cryptoProvider);
        require(malformedInstrumentWrite.written, "Malformed-instrument package should still write successfully.");

        const auto malformedInstrumentLoad = drs::engine::loadPerformancePackage(malformedInstrumentPlan.outputPackagePath,
                                                                                 cryptoProvider,
                                                                                 drs::engine::performancePackageSchemaVersion);
        require(!malformedInstrumentLoad.loaded,
                "Package loader should reject runtime instruments whose packaged zone gain fields are malformed.");
        require(malformedInstrumentLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::payloadCorruption,
                "Malformed packaged runtime-instrument gain fields should report payload corruption.");
        require(package_support::containsIssue(malformedInstrumentLoad.issues, "Zone[0]")
                    && package_support::containsIssue(malformedInstrumentLoad.issues, "gainDb"),
                "Malformed packaged runtime-instrument gain validation should report the bad zone gain field.");

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
