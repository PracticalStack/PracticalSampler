#include "drs/engine/PackageCrypto.h"
#include "drs/engine/PackageWriter.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"

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

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::vector<std::uint8_t> readBinaryFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void writeBinaryFile(const fs::path& path, const std::vector<std::uint8_t>& bytes)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good(), "Could not open package test fixture for writing: " + path.generic_string());
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    require(output.good(), "Could not finish writing package test fixture: " + path.generic_string());
}

bool containsIssue(const std::vector<std::string>& issues, const std::string& needle)
{
    return std::any_of(issues.begin(),
                       issues.end(),
                       [&](const std::string& issue)
                       {
                           return issue.find(needle) != std::string::npos;
                       });
}

drs::engine::RuntimeCompilePlan buildReferenceCompilePlan(const fs::path& outputDirectory)
{
    const auto projectPath = outputDirectory / "tiny-open-instrument.drsproj";
    const auto instrumentPath = outputDirectory / "tiny-open-instrument.drinst";
    const auto streamPath = outputDirectory / "tiny-open-instrument.drstrm";
    const auto contentRoot = fs::path(drs::engine::getPhase1ReferenceProjectManifestPath()).parent_path()
        / ".." / ".." / ".." / ".." / "hise_project";

    const auto sinePath = (contentRoot / "Samples" / "DRS_Sine_A3.wav").lexically_normal();
    const auto trianglePath = (contentRoot / "Samples" / "DRS_TriangleLead_A4.wav").lexically_normal();

    const auto sineImport = drs::engine::inspectSampleFile(sinePath.generic_string());
    require(sineImport.accepted, "Reference sine sample must inspect successfully before package tests run.");

    const auto triangleImport = drs::engine::inspectSampleFile(trianglePath.generic_string());
    require(triangleImport.accepted, "Reference triangle sample must inspect successfully before package tests run.");

    drs::engine::RuntimeCompilePlan plan;
    plan.outputProjectPath = projectPath.generic_string();
    plan.outputInstrumentPath = instrumentPath.generic_string();
    plan.outputStreamPath = streamPath.generic_string();
    plan.projectId = "drs.phase1.tiny-open-project";
    plan.projectDisplayName = "DRS Tiny Open Project";
    plan.contentRootPath = contentRoot.lexically_normal().generic_string();
    plan.instrumentId = "drs.phase1.tiny-open-instrument";
    plan.instrumentDisplayName = "DRS Tiny Open Instrument";
    plan.defaultLoadProfile = "balanced";
    plan.pageSizeBytes = 65536;

    drs::engine::RuntimeCompileSourceDefinition sineSource;
    sineSource.id = "sine-a3";
    sineSource.sourcePath = sinePath.generic_string();
    sineSource.role = "core-sustain";
    sineSource.metadata = sineImport.metadata;
    plan.sampleSources.push_back(std::move(sineSource));

    drs::engine::RuntimeCompileSourceDefinition triangleSource;
    triangleSource.id = "triangle-a4";
    triangleSource.sourcePath = trianglePath.generic_string();
    triangleSource.role = "core-lead";
    triangleSource.metadata = triangleImport.metadata;
    plan.sampleSources.push_back(std::move(triangleSource));

    drs::engine::RuntimeArticulationDefinition sustain;
    sustain.id = "sustain";
    sustain.name = "Sustain";
    sustain.isDefault = true;
    plan.articulations.push_back(std::move(sustain));

    drs::engine::RuntimeArticulationDefinition lead;
    lead.id = "lead";
    lead.name = "Lead";
    plan.articulations.push_back(std::move(lead));

    drs::engine::RuntimeGroupDefinition padCore;
    padCore.id = "pad-core";
    padCore.name = "Pad Core";
    padCore.articulationIds = { "sustain" };
    plan.groups.push_back(std::move(padCore));

    drs::engine::RuntimeGroupDefinition leadCore;
    leadCore.id = "lead-core";
    leadCore.name = "Lead Core";
    leadCore.articulationIds = { "lead" };
    plan.groups.push_back(std::move(leadCore));

    drs::engine::RuntimeCompileZoneDefinition padZone;
    padZone.id = "pad-a3";
    padZone.sourceId = "sine-a3";
    padZone.groupId = "pad-core";
    padZone.articulationId = "sustain";
    padZone.rootKey = 57;
    padZone.keyLow = 36;
    padZone.keyHigh = 76;
    padZone.velocityLow = 1;
    padZone.velocityHigh = 95;
    padZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(padZone));

    drs::engine::RuntimeCompileZoneDefinition padAccentZone;
    padAccentZone.id = "pad-a3-accent";
    padAccentZone.sourceId = "sine-a3";
    padAccentZone.groupId = "pad-core";
    padAccentZone.articulationId = "sustain";
    padAccentZone.rootKey = 57;
    padAccentZone.keyLow = 36;
    padAccentZone.keyHigh = 76;
    padAccentZone.velocityLow = 96;
    padAccentZone.velocityHigh = 127;
    padAccentZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(padAccentZone));

    drs::engine::RuntimeCompileZoneDefinition leadZone;
    leadZone.id = "lead-a4";
    leadZone.sourceId = "triangle-a4";
    leadZone.groupId = "lead-core";
    leadZone.articulationId = "lead";
    leadZone.rootKey = 69;
    leadZone.keyLow = 60;
    leadZone.keyHigh = 96;
    leadZone.velocityLow = 1;
    leadZone.velocityHigh = 95;
    leadZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(leadZone));

    drs::engine::RuntimeCompileZoneDefinition leadAccentZone;
    leadAccentZone.id = "lead-a4-accent";
    leadAccentZone.sourceId = "triangle-a4";
    leadAccentZone.groupId = "lead-core";
    leadAccentZone.articulationId = "lead";
    leadAccentZone.rootKey = 69;
    leadAccentZone.keyLow = 60;
    leadAccentZone.keyHigh = 96;
    leadAccentZone.velocityLow = 96;
    leadAccentZone.velocityHigh = 127;
    leadAccentZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(leadAccentZone));

    return plan;
}

drs::engine::PerformancePackageCompileWritePlan buildPackagePlan(const fs::path& scratchDirectory,
                                                                 const int minimumReaderSchemaVersion = drs::engine::performancePackageSchemaVersion)
{
    auto compilePlan = buildReferenceCompilePlan(scratchDirectory / "compiled-runtime");
    auto compileResult = drs::engine::compileRuntimeInstrument(compilePlan);
    require(compileResult.compiled, "Reference compile plan should compile successfully for package tests.");

    const auto writeResult = drs::engine::writeCompiledStreamAssets(compileResult);
    require(writeResult.written, "Compiled stream assets should write successfully for package tests.");

    drs::engine::PerformancePackageManifest manifest;
    manifest.packageId = "drs.phase1.tiny-open-instrument.package";
    manifest.displayName = "DRS Tiny Open Instrument Package";
    manifest.instrumentId = compilePlan.instrumentId;
    manifest.defaultLoadProfile = compilePlan.defaultLoadProfile;
    manifest.minimumReaderSchemaVersion = minimumReaderSchemaVersion;
    manifest.notes = {
        "Sprint 3 sealed package fixture.",
        "Contains runtime-only payloads and omits the authored project manifest."
    };

    drs::engine::PerformancePackageCompileWritePlan packagePlan;
    packagePlan.manifest = std::move(manifest);
    packagePlan.compiledRuntime = std::move(compileResult);
    packagePlan.outputPackagePath = (scratchDirectory / "tiny-open-instrument.drpkg").generic_string();
    packagePlan.minimumCompatibleAppVersion = "0.3.0-internal";
    return packagePlan;
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

        const auto packagePlan = buildPackagePlan(scratchDirectory);
        const auto packageWrite = drs::engine::writePerformancePackage(packagePlan, cryptoProvider);
        require(packageWrite.written, "Performance package writer should succeed for the reference compile result.");

        const auto inspection = drs::engine::inspectPerformancePackage(packagePlan.outputPackagePath,
                                                                       cryptoProvider,
                                                                       drs::engine::performancePackageSchemaVersion);
        require(inspection.valid, "Reference performance package should inspect successfully.");
        require(inspection.cleartextManifest.displayName == "DRS Tiny Open Instrument Package",
                "Cleartext package display name changed unexpectedly.");
        require(inspection.header.payloadCount == 4, "Reference performance package payload count changed unexpectedly.");
        require(inspection.payloads.size() == 4, "Reference performance package should expose four decrypted payloads.");
        require(inspection.cryptoAlgorithm == cryptoProvider.algorithmId(),
                "Reference performance package crypto algorithm changed unexpectedly.");
        require(inspection.minimumCompatibleAppVersion == "0.3.0-internal",
                "Reference performance package minimumCompatibleAppVersion changed unexpectedly.");

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

        auto duplicatePayloadPlan = drs::engine::buildPerformancePackageWritePlan(packagePlan);
        duplicatePayloadPlan.outputPackagePath = (scratchDirectory / "duplicate-payloads.drpkg").generic_string();
        duplicatePayloadPlan.payloads[0].payloadId = duplicatePayloadPlan.payloads[1].payloadId;
        const auto duplicatePayloadWrite = drs::engine::writePerformancePackage(duplicatePayloadPlan, cryptoProvider);
        require(!duplicatePayloadWrite.written, "Package writer should reject duplicate payload ids.");
        require(containsIssue(duplicatePayloadWrite.issues, "duplicate id"),
                "Duplicate payload rejection should explain the duplicate id.");

        const auto futureReaderPackagePlan =
            buildPackagePlan(scratchDirectory / "future-reader", drs::engine::performancePackageSchemaVersion + 1);
        const auto futureReaderWrite = drs::engine::writePerformancePackage(futureReaderPackagePlan, cryptoProvider);
        require(futureReaderWrite.written, "Future-reader package write should still succeed.");
        const auto futureReaderInspection = drs::engine::inspectPerformancePackage(
            futureReaderPackagePlan.outputPackagePath,
            cryptoProvider,
            drs::engine::performancePackageSchemaVersion);
        require(!futureReaderInspection.valid, "Future-reader package should fail version-skew inspection.");
        require(containsIssue(futureReaderInspection.issues, "requires reader schema version"),
                "Version-skew inspection should explain the minimum reader schema version.");

        const auto referencePackageBytes = readBinaryFile(fs::path(packagePlan.outputPackagePath));

        auto truncatedBytes = referencePackageBytes;
        truncatedBytes.resize(truncatedBytes.size() - 32);
        const auto truncatedPath = scratchDirectory / "truncated.drpkg";
        writeBinaryFile(truncatedPath, truncatedBytes);
        const auto truncatedInspection = drs::engine::inspectPerformancePackage(truncatedPath.generic_string(),
                                                                                cryptoProvider,
                                                                                drs::engine::performancePackageSchemaVersion);
        require(!truncatedInspection.valid, "Truncated performance package should fail inspection.");
        require(!truncatedInspection.issues.empty(), "Truncated performance package should report at least one issue.");

        auto badTagBytes = referencePackageBytes;
        const auto tocTagOffset = inspection.header.tocOffsetBytes + cryptoProvider.nonceSizeBytes();
        require(tocTagOffset < badTagBytes.size(), "TOC tag offset should remain inside the package file.");
        badTagBytes[static_cast<std::size_t>(tocTagOffset)] ^= 0x5au;
        const auto badTagPath = scratchDirectory / "bad-tag.drpkg";
        writeBinaryFile(badTagPath, badTagBytes);
        const auto badTagInspection = drs::engine::inspectPerformancePackage(badTagPath.generic_string(),
                                                                             cryptoProvider,
                                                                             drs::engine::performancePackageSchemaVersion);
        require(!badTagInspection.valid, "Bad-tag performance package should fail inspection.");
        require(containsIssue(badTagInspection.issues, "authentication failed"),
                "Bad-tag performance package should report TOC authentication failure.");

        auto tocMismatchBytes = referencePackageBytes;
        const auto payloadCountOffset = drs::engine::getPerformancePackageHeaderPayloadCountOffsetBytes();
        require(payloadCountOffset + sizeof(std::uint32_t) <= tocMismatchBytes.size(),
                "Package header payload-count offset should remain inside the package file.");
        std::uint32_t corruptedPayloadCount = inspection.header.payloadCount + 1;
        std::memcpy(tocMismatchBytes.data() + payloadCountOffset,
                    &corruptedPayloadCount,
                    sizeof(corruptedPayloadCount));
        const auto tocMismatchPath = scratchDirectory / "toc-mismatch.drpkg";
        writeBinaryFile(tocMismatchPath, tocMismatchBytes);
        const auto tocMismatchInspection = drs::engine::inspectPerformancePackage(tocMismatchPath.generic_string(),
                                                                                  cryptoProvider,
                                                                                  drs::engine::performancePackageSchemaVersion);
        require(!tocMismatchInspection.valid, "TOC-mismatch performance package should fail inspection.");
        require(containsIssue(tocMismatchInspection.issues, "cleartext header and sealed TOC no longer agree"),
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
