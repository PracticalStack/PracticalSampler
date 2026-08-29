#include "drs/engine/EngineFacade.h"
#include "drs/engine/PackageCrypto.h"
#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageV2StreamingExport.h"
#include "drs/engine/PackageWriter.h"
#include "Phase1PerformancePackageSupport.h"

#include <json/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

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

        drs::engine::PackageSealRequest sealRequest;
        sealRequest.packageId = "pkg.test";
        sealRequest.recordId = "payload.alpha";
        sealRequest.additionalAuthenticatedData = "aad";
        sealRequest.plaintext = { 'h', 'e', 'l', 'l', 'o' };

        drs::engine::PackageSealedBlob firstSeal;
        std::string issue;
        require(cryptoProvider.seal(sealRequest, firstSeal, issue), "Deterministic package crypto seal should succeed.");

        drs::engine::PackageSealedBlob secondSeal;
        require(cryptoProvider.seal(sealRequest, secondSeal, issue), "Deterministic package crypto second seal should succeed.");
        require(firstSeal.nonce == secondSeal.nonce, "Deterministic package crypto nonce changed unexpectedly.");
        require(firstSeal.ciphertext == secondSeal.ciphertext,
                "Deterministic package crypto ciphertext changed unexpectedly.");
        require(firstSeal.tag == secondSeal.tag, "Deterministic package crypto tag changed unexpectedly.");

        drs::engine::PackageOpenRequest openRequest;
        openRequest.packageId = sealRequest.packageId;
        openRequest.recordId = sealRequest.recordId;
        openRequest.additionalAuthenticatedData = sealRequest.additionalAuthenticatedData;
        openRequest.sealed = firstSeal;

        std::vector<std::uint8_t> openedPlaintext;
        require(cryptoProvider.open(openRequest, openedPlaintext, issue),
                "Deterministic package crypto open should succeed.");
        require(openedPlaintext == sealRequest.plaintext,
                "Deterministic package crypto should recover the original plaintext.");

        const auto scratchDirectory = fs::temp_directory_path() / "drs-phase1-performance-package-tests";
        std::error_code errorCode;
        fs::remove_all(scratchDirectory, errorCode);
        fs::create_directories(scratchDirectory);

        const auto checkedInCorpus = package_support::getCheckedInCorpusPaths();
        require(fs::exists(checkedInCorpus.index), "Checked-in performance package corpus index must exist.");
        require(fs::exists(checkedInCorpus.valid), "Checked-in valid performance package fixture must exist.");
        require(fs::exists(checkedInCorpus.truncated), "Checked-in truncated performance package fixture must exist.");
        require(fs::exists(checkedInCorpus.tampered), "Checked-in tampered performance package fixture must exist.");
        require(fs::exists(checkedInCorpus.wrongVersion), "Checked-in wrong-version performance package fixture must exist.");

        const auto corpusIndexText = package_support::readTextFile(checkedInCorpus.index);
        require(corpusIndexText.find("\"valid\"") != std::string::npos,
                "Performance package corpus index must name the valid fixture.");
        require(corpusIndexText.find("\"truncated\"") != std::string::npos,
                "Performance package corpus index must name the truncated fixture.");
        require(corpusIndexText.find("\"tampered\"") != std::string::npos,
                "Performance package corpus index must name the tampered fixture.");
        require(corpusIndexText.find("\"wrong-version\"") != std::string::npos,
                "Performance package corpus index must name the wrong-version fixture.");

        const auto inspection = drs::engine::inspectPerformancePackage(checkedInCorpus.valid.generic_string(),
                                                                       cryptoProvider,
                                                                       drs::engine::performancePackageSchemaVersion);
        require(inspection.valid, "Reference performance package should inspect successfully.");
        require(inspection.cleartextManifest.displayName == "DRS Tiny Open Instrument Package",
                "Cleartext package display name changed unexpectedly.");
        require(inspection.header.payloadCount == 6, "Reference performance package payload count changed unexpectedly.");
        require(inspection.payloads.size() == 6,
                "Reference performance package should expose six decrypted payloads, including artwork and license text.");
        require(inspection.cryptoAlgorithm == cryptoProvider.algorithmId(),
                "Reference performance package crypto algorithm changed unexpectedly.");
        require(inspection.minimumCompatibleAppVersion == "0.5.0-internal",
                "Checked-in performance package minimumCompatibleAppVersion changed unexpectedly.");

        const auto instrumentPayloadIterator = std::find_if(
            inspection.payloads.begin(),
            inspection.payloads.end(),
            [](const drs::engine::PerformancePackagePayloadView& payload)
            {
                return payload.payloadId == "runtime-instrument";
            });
        require(instrumentPayloadIterator != inspection.payloads.end(),
                "Performance package should contain the encrypted runtime instrument payload.");

        const auto instrumentJson = json::parse(std::string(instrumentPayloadIterator->plaintextBytes.begin(),
                                                            instrumentPayloadIterator->plaintextBytes.end()));
        require(instrumentJson.at("sourceProject").get<std::string>() == "package://authoring/unavailable",
                "Packaged runtime instrument should not point back to an authored .drsproj.");
        require(instrumentJson.at("compiledStreamAsset").get<std::string>()
                    == "package://payload/runtime-stream-index",
                "Packaged runtime instrument should point at the internal stream-index payload URI.");

        const auto streamIndexPayloadIterator = std::find_if(
            inspection.payloads.begin(),
            inspection.payloads.end(),
            [](const drs::engine::PerformancePackagePayloadView& payload)
            {
                return payload.payloadId == "runtime-stream-index";
            });
        require(streamIndexPayloadIterator != inspection.payloads.end(),
                "Performance package should contain the encrypted runtime stream-index payload.");

        const auto streamIndexJson = json::parse(std::string(streamIndexPayloadIterator->plaintextBytes.begin(),
                                                             streamIndexPayloadIterator->plaintextBytes.end()));
        require(streamIndexJson.at("payloadAssetPath").get<std::string>()
                    == "package://payload/runtime-stream-payload",
                "Packaged runtime stream index should point at the internal stream-payload URI.");
        require(streamIndexJson.at("samples").at(0).at("sourcePath").get<std::string>().rfind("package://sample/", 0) == 0,
                "Packaged runtime stream index should not expose authored source sample paths.");

        const auto futureReaderInspection = drs::engine::inspectPerformancePackage(
            checkedInCorpus.wrongVersion.generic_string(),
            cryptoProvider,
            drs::engine::performancePackageSchemaVersion);
        require(!futureReaderInspection.valid, "Future-reader package should fail version-skew inspection.");
        require(package_support::containsIssue(futureReaderInspection.issues, "requires reader schema version"),
                "Version-skew inspection should explain the minimum reader schema version.");

        const auto truncatedInspection = drs::engine::inspectPerformancePackage(checkedInCorpus.truncated.generic_string(),
                                                                                cryptoProvider,
                                                                                drs::engine::performancePackageSchemaVersion);
        require(!truncatedInspection.valid, "Truncated performance package should fail inspection.");
        require(!truncatedInspection.issues.empty(), "Truncated performance package should report at least one issue.");

        const auto badTagInspection = drs::engine::inspectPerformancePackage(checkedInCorpus.tampered.generic_string(),
                                                                             cryptoProvider,
                                                                             drs::engine::performancePackageSchemaVersion);
        require(!badTagInspection.valid, "Bad-tag performance package should fail inspection.");
        require(package_support::containsIssue(badTagInspection.issues, "authentication failed"),
                "Bad-tag performance package should report TOC authentication failure.");

        const auto packagePlan = package_support::buildPackagePlan(scratchDirectory / "generated",
                                                                   scratchDirectory / "generated" / "tiny-open-instrument.drpkg");
        require(std::abs(packagePlan.compiledRuntime.masterGainDb - packagePlan.manifest.masterGainDb) < 1.0e-9,
                "Compile result should preserve package master gain before package serialization.");
        require(packagePlan.compiledRuntime.instrument.groups.size() == packagePlan.manifest.groupRoutes.size(),
                "Compile result should preserve packaged group-route gain before package serialization.");
        require(packagePlan.compiledRuntime.instrument.groups.at(0).id.empty()
                    || std::abs(packagePlan.compiledRuntime.instrument.groups.at(0).gainDb
                                    - packagePlan.manifest.groupRoutes.at(0).gainDb)
                        < 1.0e-9,
                "Compile result should preserve group gain before package serialization.");
        require(std::abs(packagePlan.compiledRuntime.instrument.zones.at(0).gainDb - (-0.75)) < 1.0e-9,
                "Compile result should preserve zone gain before package serialization.");

        const auto packageWritePlan = drs::engine::buildPerformancePackageWritePlan(packagePlan);
        require(packageWritePlan.payloads.size() == 6,
                "Package write planning should include optional background-image and license payloads.");
        require(std::abs(packageWritePlan.manifest.masterGainDb - packagePlan.compiledRuntime.masterGainDb) < 1.0e-9,
                "Package write planning should source package master gain from the compile result.");
        require(packageWritePlan.manifest.groupRoutes.size() == packagePlan.compiledRuntime.instrument.groups.size(),
                "Package write planning should carry all compile-time group gains without recomputing them later.");
        require(packageWritePlan.manifest.groupRoutes.at(0).groupId
                    == packagePlan.compiledRuntime.instrument.groups.at(0).id
                    && std::abs(packageWritePlan.manifest.groupRoutes.at(0).gainDb
                                    - packagePlan.compiledRuntime.instrument.groups.at(0).gainDb)
                        < 1.0e-9,
                "Package write planning should preserve compile-time group-route gain deterministically.");

        const auto packageWrite = drs::engine::writePerformancePackage(packagePlan, cryptoProvider);
        require(packageWrite.written, "Performance package writer should succeed for the generated reference compile result.");

        const auto generatedInspection = drs::engine::inspectPerformancePackage(packagePlan.outputPackagePath,
                                                                                cryptoProvider,
                                                                                drs::engine::performancePackageSchemaVersion);
        require(generatedInspection.valid, "Generated performance package should inspect successfully.");
        const auto packageManifestPayloadIterator = std::find_if(
            generatedInspection.payloads.begin(),
            generatedInspection.payloads.end(),
            [](const drs::engine::PerformancePackagePayloadView& payload)
            {
                return payload.payloadId == "package-manifest";
            });
        require(packageManifestPayloadIterator != generatedInspection.payloads.end(),
                "Generated performance package should contain the encrypted package manifest payload.");

        const auto packageManifestJson = json::parse(std::string(packageManifestPayloadIterator->plaintextBytes.begin(),
                                                                 packageManifestPayloadIterator->plaintextBytes.end()));
        require(std::abs(packageManifestJson.at("masterGainDb").get<double>() - packagePlan.manifest.masterGainDb) < 1.0e-9,
                "Generated package manifest should preserve authored package master gain.");
        require(packageManifestJson.at("groupRoutes").is_array()
                    && packageManifestJson.at("groupRoutes").size() == packagePlan.manifest.groupRoutes.size(),
                "Generated package manifest should serialize each packaged group route explicitly.");
        require(packageManifestJson.at("groupRoutes").at(0).at("groupId").get<std::string>()
                    == packagePlan.manifest.groupRoutes.at(0).groupId
                    && std::abs(packageManifestJson.at("groupRoutes").at(0).at("gainDb").get<double>()
                                    - packagePlan.manifest.groupRoutes.at(0).gainDb)
                        < 1.0e-9,
                "Generated package manifest should preserve packaged group-route gain deterministically.");
        require(packageManifestJson.at("backgroundImage").at("payloadId").get<std::string>()
                    == packagePlan.manifest.backgroundImage.payloadId,
                "Generated package manifest should advertise the packaged background-image payload id.");
        require(packageManifestJson.at("license").at("payloadId").get<std::string>()
                    == drs::engine::playableInstrumentLicensePayloadId,
                "Generated package manifest should advertise the canonical license payload id.");

        auto invalidLicenseManifestJson = packageManifestJson;
        invalidLicenseManifestJson["license"]["payloadId"] = "non-canonical-license";
        const auto invalidLicenseManifest = drs::engine::parsePerformancePackageManifestJson(
            invalidLicenseManifestJson.dump());
        require(!invalidLicenseManifest.parsed
                    && package_support::containsIssue(invalidLicenseManifest.issues,
                                                      "must be 'license-text'"),
                "Manifest parsing must reject a non-canonical license payload id.");

        const auto generatedInstrumentPayloadIterator = std::find_if(
            generatedInspection.payloads.begin(),
            generatedInspection.payloads.end(),
            [](const drs::engine::PerformancePackagePayloadView& payload)
            {
                return payload.payloadId == "runtime-instrument";
            });
        require(generatedInstrumentPayloadIterator != generatedInspection.payloads.end(),
                "Generated performance package should expose the runtime instrument payload for inspection.");
        const auto generatedInstrumentJson = json::parse(std::string(generatedInstrumentPayloadIterator->plaintextBytes.begin(),
                                                                     generatedInstrumentPayloadIterator->plaintextBytes.end()));
        require(std::abs(generatedInstrumentJson.at("groups").at(0).at("gainDb").get<double>()
                             - packagePlan.compiledRuntime.instrument.groups.at(0).gainDb)
                    < 1.0e-9,
                "Generated runtime-instrument payload should expose packaged group gain for inspection.");
        require(std::abs(generatedInstrumentJson.at("zones").at(0).at("gainDb").get<double>()
                             - packagePlan.compiledRuntime.instrument.zones.at(0).gainDb)
                    < 1.0e-9,
                "Generated runtime-instrument payload should expose packaged zone gain for inspection.");

        auto guardedV4WritePlan = packageWritePlan;
        guardedV4WritePlan.outputPackagePath
            = (scratchDirectory / "v4-package-hydration.drpkg").generic_string();
        guardedV4WritePlan.manifest.schemaVersion
            = drs::engine::performancePackageFxRoutingSchemaVersion;
        guardedV4WritePlan.manifest.minimumReaderSchemaVersion
            = drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion;
        const auto guardedManifestPayload = std::find_if(
            guardedV4WritePlan.payloads.begin(), guardedV4WritePlan.payloads.end(), [](const auto& payload)
            {
                return payload.kind == drs::engine::PerformancePackagePayloadKind::packageManifest;
            });
        require(guardedManifestPayload != guardedV4WritePlan.payloads.end(),
                "The package hydration fixture must contain a package manifest payload.");
        guardedManifestPayload->plaintextBytes = package_support::toBytes(
            drs::engine::serializePerformancePackageManifest(guardedV4WritePlan.manifest));
        const auto guardedRuntimePayload = std::find_if(
            guardedV4WritePlan.payloads.begin(), guardedV4WritePlan.payloads.end(), [](const auto& payload)
            {
                return payload.kind == drs::engine::PerformancePackagePayloadKind::runtimeInstrument;
            });
        require(guardedRuntimePayload != guardedV4WritePlan.payloads.end(),
                "The fail-closed package fixture must contain a runtime instrument payload.");
        auto guardedRuntimeJson = json::parse(std::string(guardedRuntimePayload->plaintextBytes.begin(),
                                                          guardedRuntimePayload->plaintextBytes.end()));
        guardedRuntimeJson["schemaVersion"] = drs::engine::runtimeInstrumentFxRoutingSchemaVersion;
        for (auto& group : guardedRuntimeJson.at("groups"))
            group["routingBusId"] = "master";
        guardedRuntimeJson["fxSlots"] = json::array({ {
            { "id", "room" },
            { "displayName", "Room" },
            { "effectType", "drs.algorithmicReverb" },
            { "effectVersion", 1 },
            { "bypassed", false },
            { "parameters", json::array({
                { { "id", "decaySeconds" }, { "value", 2.5 } },
                { { "id", "mix" }, { "value", 0.18 } }
            }) }
        } });
        guardedRuntimeJson["routingBuses"] = json::array({ {
            { "id", "bus-master" },
            { "displayName", "Master Insert" },
            { "inputSourceId", "master" },
            { "fxSlotIds", json::array({ "room" }) },
            { "chainBypassed", false }
        } });
        guardedRuntimePayload->plaintextBytes = package_support::toBytes(
            guardedRuntimeJson.dump(2) + "\n");
        const auto guardedV4Write = drs::engine::writePerformancePackage(
            guardedV4WritePlan, cryptoProvider);
        require(guardedV4Write.written,
                "The graph-bearing package fixture must write before reader behavior is checked.");
        const auto guardedV4Load = drs::engine::loadPerformancePackage(
            guardedV4WritePlan.outputPackagePath, cryptoProvider,
            drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion);
        require(guardedV4Load.loaded
                    && guardedV4Load.manifest.schemaVersion
                        == drs::engine::performancePackageFxRoutingSchemaVersion
                    && guardedV4Load.instrument.instrument.fxSlots.size() == 1
                    && guardedV4Load.instrument.instrument.fxSlots.front().id == "room"
                    && guardedV4Load.instrument.instrument.routingBuses.size() == 1
                    && guardedV4Load.instrument.instrument.routingBuses.front().inputSourceId
                        == "master",
                "PX-04 package loading must accept and retain a valid schema-2/runtime-v4 graph.");

        const auto backgroundImagePayloadIterator = std::find_if(
            generatedInspection.payloads.begin(),
            generatedInspection.payloads.end(),
            [](const drs::engine::PerformancePackagePayloadView& payload)
            {
                return payload.payloadId == "background-image";
            });
        require(backgroundImagePayloadIterator != generatedInspection.payloads.end(),
                "Generated performance package should expose the background-image payload for inspection.");
        require(backgroundImagePayloadIterator->payloadKind == "backgroundImage"
                    && backgroundImagePayloadIterator->mediaType == "image/jpeg"
                    && !backgroundImagePayloadIterator->plaintextBytes.empty(),
                "Generated performance package should preserve packaged background-image payload metadata and bytes.");

        const auto generatedLoad = drs::engine::loadPerformancePackage(
            packagePlan.outputPackagePath, cryptoProvider,
            drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion);
        require(generatedLoad.loaded
                    && generatedLoad.licenseText.loaded
                    && generatedLoad.licenseText.payload.payloadId
                        == drs::engine::playableInstrumentLicensePayloadId
                    && generatedLoad.licenseText.payload.payloadKind == "licenseText"
                    && generatedLoad.licenseText.payload.logicalPath
                        == drs::engine::playableInstrumentLicenseLogicalPath
                    && generatedLoad.licenseText.payload.mediaType
                        == drs::engine::playableInstrumentLicenseMediaType
                    && generatedLoad.licenseText.payload.plaintextBytes
                        == package_support::buildLicenseTextFixture(),
                "The conventional package reader must return byte-identical authenticated license text.");

        drs::engine::EngineFacade conventionalFacade;
        const auto conventionalActivation
            = conventionalFacade.activatePerformancePackageSession(generatedLoad);
        const auto conventionalLicenseText
            = conventionalFacade.getPerformancePackageLicenseText();
        const auto expectedLicenseBytes = package_support::buildLicenseTextFixture();
        const std::string expectedLicenseText(expectedLicenseBytes.begin() + 3,
                                              expectedLicenseBytes.end());
        require(conventionalActivation.activated
                    && conventionalLicenseText != nullptr
                    && *conventionalLicenseText == expectedLicenseText,
                "Conventional package activation must retain immutable display text without a UTF-8 BOM.");
        conventionalFacade.restoreBundledReferenceRuntimeSession();
        require(conventionalFacade.getPerformancePackageLicenseText() == nullptr,
                "Closing a conventional package must clear its retained license text.");

        const auto generatedLicensePayloadIterator = std::find_if(
            generatedInspection.payloads.begin(), generatedInspection.payloads.end(), [](const auto& payload)
            {
                return payload.payloadId == drs::engine::playableInstrumentLicensePayloadId;
            });
        require(generatedLicensePayloadIterator != generatedInspection.payloads.end()
                    && generatedLicensePayloadIterator->payloadKind == "licenseText"
                    && generatedLicensePayloadIterator->logicalPath
                        == drs::engine::playableInstrumentLicenseLogicalPath
                    && generatedLicensePayloadIterator->mediaType
                        == drs::engine::playableInstrumentLicenseMediaType
                    && generatedLicensePayloadIterator->plaintextBytes
                        == package_support::buildLicenseTextFixture(),
                "Package inspection must preserve canonical license metadata and bytes.");

        auto missingLicensePlan = drs::engine::buildPerformancePackageWritePlan(packagePlan);
        missingLicensePlan.outputPackagePath
            = (scratchDirectory / "missing-license-payload.drpkg").generic_string();
        missingLicensePlan.payloads.erase(
            std::remove_if(missingLicensePlan.payloads.begin(), missingLicensePlan.payloads.end(),
                           [](const auto& payload)
                           {
                               return payload.kind
                                   == drs::engine::PerformancePackagePayloadKind::licenseText;
                           }),
            missingLicensePlan.payloads.end());
        const auto missingLicenseWrite = drs::engine::writePerformancePackage(
            missingLicensePlan, cryptoProvider);
        require(!missingLicenseWrite.written
                    && package_support::containsIssue(missingLicenseWrite.issues,
                                                      "must reference a packaged payload"),
                "The conventional writer must reject a declared missing license payload.");

        auto wrongLicenseMetadataPlan = drs::engine::buildPerformancePackageWritePlan(packagePlan);
        wrongLicenseMetadataPlan.outputPackagePath
            = (scratchDirectory / "wrong-license-metadata.drpkg").generic_string();
        const auto wrongLicensePayload = std::find_if(
            wrongLicenseMetadataPlan.payloads.begin(), wrongLicenseMetadataPlan.payloads.end(),
            [](const auto& payload)
            {
                return payload.kind == drs::engine::PerformancePackagePayloadKind::licenseText;
            });
        require(wrongLicensePayload != wrongLicenseMetadataPlan.payloads.end(),
                "The wrong-license-metadata fixture requires a license payload.");
        wrongLicensePayload->mediaType = "application/octet-stream";
        wrongLicensePayload->logicalPath = "license.bin";
        const auto wrongLicenseMetadataWrite = drs::engine::writePerformancePackage(
            wrongLicenseMetadataPlan, cryptoProvider);
        require(!wrongLicenseMetadataWrite.written
                    && package_support::containsIssue(wrongLicenseMetadataWrite.issues,
                                                      "mediaType 'text/plain; charset=utf-8'")
                    && package_support::containsIssue(wrongLicenseMetadataWrite.issues,
                                                      "logicalPath 'LICENSE.txt'"),
                "The conventional writer must reject non-canonical license media and path metadata.");

        auto wrongV2LicensePayloads = packagePlan.additionalPayloads;
        const auto wrongV2LicensePayload = std::find_if(
            wrongV2LicensePayloads.begin(), wrongV2LicensePayloads.end(), [](const auto& payload)
            {
                return payload.kind == drs::engine::PerformancePackagePayloadKind::licenseText;
            });
        require(wrongV2LicensePayload != wrongV2LicensePayloads.end(),
                "The package-v2 metadata fixture requires a license payload.");
        wrongV2LicensePayload->mediaType = "application/octet-stream";
        const auto wrongV2LicensePlan = drs::engine::buildPerformancePackageV2StreamingExportPlan(
            packagePlan.manifest,
            packagePlan.compiledRuntime,
            (scratchDirectory / "wrong-v2-license-metadata.drpkg").generic_string(),
            wrongV2LicensePayloads);
        require(!wrongV2LicensePlan.built
                    && package_support::containsIssue(wrongV2LicensePlan.issues,
                                                      "metadata is not canonical"),
                "The package-v2 planner must reject non-canonical license payload metadata.");

        auto invalidLicenseBytesPlan = drs::engine::buildPerformancePackageWritePlan(packagePlan);
        invalidLicenseBytesPlan.outputPackagePath
            = (scratchDirectory / "invalid-license-bytes.drpkg").generic_string();
        const auto invalidLicensePayload = std::find_if(
            invalidLicenseBytesPlan.payloads.begin(), invalidLicenseBytesPlan.payloads.end(),
            [](const auto& payload)
            {
                return payload.kind == drs::engine::PerformancePackagePayloadKind::licenseText;
            });
        require(invalidLicensePayload != invalidLicenseBytesPlan.payloads.end(),
                "The invalid-license fixture requires a license payload.");
        invalidLicensePayload->plaintextBytes = { 0xc3u, 0x28u };
        const auto invalidLicenseBytesWrite = drs::engine::writePerformancePackage(
            invalidLicenseBytesPlan, cryptoProvider);
        require(!invalidLicenseBytesWrite.written
                    && package_support::containsIssue(invalidLicenseBytesWrite.issues, "UTF-8"),
                "The conventional writer must reject invalid UTF-8 license bytes.");

        auto duplicatePayloadPlan = drs::engine::buildPerformancePackageWritePlan(packagePlan);
        duplicatePayloadPlan.outputPackagePath = (scratchDirectory / "duplicate-payloads.drpkg").generic_string();
        duplicatePayloadPlan.payloads[0].payloadId = duplicatePayloadPlan.payloads[1].payloadId;
        const auto duplicatePayloadWrite = drs::engine::writePerformancePackage(duplicatePayloadPlan, cryptoProvider);
        require(!duplicatePayloadWrite.written, "Package writer should reject duplicate payload ids.");
        require(package_support::containsIssue(duplicatePayloadWrite.issues, "duplicate id"),
                "Duplicate payload rejection should explain the duplicate id.");

        auto invalidGainManifestPlan = drs::engine::buildPerformancePackageWritePlan(packagePlan);
        invalidGainManifestPlan.outputPackagePath = (scratchDirectory / "invalid-gain-manifest.drpkg").generic_string();
        invalidGainManifestPlan.manifest.masterGainDb = std::numeric_limits<double>::infinity();
        const auto invalidGainManifestWrite = drs::engine::writePerformancePackage(invalidGainManifestPlan, cryptoProvider);
        require(!invalidGainManifestWrite.written,
                "Package writer should reject package manifests with non-finite master gain.");
        require(package_support::containsIssue(invalidGainManifestWrite.issues, "manifest.masterGainDb"),
                "Package writer should explain why a non-finite package master gain was rejected.");

        const auto referencePackageBytes = package_support::readBinaryFile(fs::path(packagePlan.outputPackagePath));

        auto tocMismatchBytes = referencePackageBytes;
        const auto payloadCountOffset = drs::engine::getPerformancePackageHeaderPayloadCountOffsetBytes();
        require(payloadCountOffset + sizeof(std::uint32_t) <= tocMismatchBytes.size(),
                "Package header payload-count offset should remain inside the package file.");
        std::uint32_t corruptedPayloadCount = generatedInspection.header.payloadCount + 1;
        std::memcpy(tocMismatchBytes.data() + payloadCountOffset,
                    &corruptedPayloadCount,
                    sizeof(corruptedPayloadCount));
        const auto tocMismatchPath = scratchDirectory / "toc-mismatch.drpkg";
        package_support::writeBinaryFile(tocMismatchPath, tocMismatchBytes);
        const auto tocMismatchInspection = drs::engine::inspectPerformancePackage(tocMismatchPath.generic_string(),
                                                                                  cryptoProvider,
                                                                                  drs::engine::performancePackageSchemaVersion);
        require(!tocMismatchInspection.valid, "TOC-mismatch performance package should fail inspection.");
        require(package_support::containsIssue(tocMismatchInspection.issues, "cleartext header and sealed TOC no longer agree"),
                "TOC-mismatch performance package should report a cleartext-header versus TOC mismatch.");

        std::cout << "Phase 1 performance package tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 performance package tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
