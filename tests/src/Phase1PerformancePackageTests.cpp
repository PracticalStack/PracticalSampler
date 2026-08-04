#include "drs/engine/PackageCrypto.h"
#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageWriter.h"
#include "Phase1PerformancePackageSupport.h"

#include <json/json.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
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
        require(inspection.header.payloadCount == 4, "Reference performance package payload count changed unexpectedly.");
        require(inspection.payloads.size() == 4, "Reference performance package should expose four decrypted payloads.");
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
        const auto packageWrite = drs::engine::writePerformancePackage(packagePlan, cryptoProvider);
        require(packageWrite.written, "Performance package writer should succeed for the generated reference compile result.");

        auto duplicatePayloadPlan = drs::engine::buildPerformancePackageWritePlan(packagePlan);
        duplicatePayloadPlan.outputPackagePath = (scratchDirectory / "duplicate-payloads.drpkg").generic_string();
        duplicatePayloadPlan.payloads[0].payloadId = duplicatePayloadPlan.payloads[1].payloadId;
        const auto duplicatePayloadWrite = drs::engine::writePerformancePackage(duplicatePayloadPlan, cryptoProvider);
        require(!duplicatePayloadWrite.written, "Package writer should reject duplicate payload ids.");
        require(package_support::containsIssue(duplicatePayloadWrite.issues, "duplicate id"),
                "Duplicate payload rejection should explain the duplicate id.");

        const auto referencePackageBytes = package_support::readBinaryFile(fs::path(packagePlan.outputPackagePath));

        auto tocMismatchBytes = referencePackageBytes;
        const auto payloadCountOffset = drs::engine::getPerformancePackageHeaderPayloadCountOffsetBytes();
        require(payloadCountOffset + sizeof(std::uint32_t) <= tocMismatchBytes.size(),
                "Package header payload-count offset should remain inside the package file.");
        std::uint32_t corruptedPayloadCount = inspection.header.payloadCount + 1;
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
