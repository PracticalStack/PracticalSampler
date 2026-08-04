#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageWriter.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"

#include <json/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
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

std::vector<std::uint8_t> toBytes(const std::string& text)
{
    return std::vector<std::uint8_t>(text.begin(), text.end());
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
    require(sineImport.accepted, "Reference sine sample must inspect successfully before package-loader tests run.");

    const auto triangleImport = drs::engine::inspectSampleFile(trianglePath.generic_string());
    require(triangleImport.accepted, "Reference triangle sample must inspect successfully before package-loader tests run.");

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
    require(compileResult.compiled, "Reference compile plan should compile successfully for package-loader tests.");

    const auto writeResult = drs::engine::writeCompiledStreamAssets(compileResult);
    require(writeResult.written, "Compiled stream assets should write successfully for package-loader tests.");

    drs::engine::PerformancePackageManifest manifest;
    manifest.packageId = "drs.phase1.tiny-open-instrument.package";
    manifest.displayName = "DRS Tiny Open Instrument Package";
    manifest.instrumentId = compilePlan.instrumentId;
    manifest.defaultLoadProfile = compilePlan.defaultLoadProfile;
    manifest.minimumReaderSchemaVersion = minimumReaderSchemaVersion;
    manifest.notes = {
        "Sprint 4 package loader fixture."
    };

    drs::engine::PerformancePackageCompileWritePlan packagePlan;
    packagePlan.manifest = std::move(manifest);
    packagePlan.compiledRuntime = std::move(compileResult);
    packagePlan.outputPackagePath = (scratchDirectory / "tiny-open-instrument.drpkg").generic_string();
    packagePlan.minimumCompatibleAppVersion = "0.4.0-internal";
    return packagePlan;
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

        const auto packagePlan = buildPackagePlan(scratchDirectory / "success");
        const auto packageWrite = drs::engine::writePerformancePackage(packagePlan, cryptoProvider);
        require(packageWrite.written, "Performance package writer should succeed for loader tests.");

        const auto loadedPackage = drs::engine::loadPerformancePackage(packagePlan.outputPackagePath,
                                                                       cryptoProvider,
                                                                       drs::engine::performancePackageSchemaVersion);
        require(loadedPackage.loaded, "Performance package should load through the dedicated runtime loader path.");
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

        auto checksumMismatchWritePlan = drs::engine::buildPerformancePackageWritePlan(
            buildPackagePlan(scratchDirectory / "checksum-mismatch"));
        checksumMismatchWritePlan.outputPackagePath = (scratchDirectory / "checksum-mismatch" / "bad-payload.drpkg").generic_string();
        auto payloadIterator = std::find_if(checksumMismatchWritePlan.payloads.begin(),
                                            checksumMismatchWritePlan.payloads.end(),
                                            [](const drs::engine::PerformancePackagePayloadSource& payload)
                                            {
                                                return payload.kind == drs::engine::PerformancePackagePayloadKind::runtimeStreamPayload;
                                            });
        require(payloadIterator != checksumMismatchWritePlan.payloads.end(),
                "Checksum-mismatch package plan should include the runtime stream payload.");
        require(!payloadIterator->plaintextBytes.empty(),
                "Checksum-mismatch runtime stream payload should include bytes to corrupt.");
        payloadIterator->plaintextBytes[0] ^= 0x7fu;
        const auto checksumMismatchWrite = drs::engine::writePerformancePackage(checksumMismatchWritePlan, cryptoProvider);
        require(checksumMismatchWrite.written, "Checksum-mismatch package should still write successfully.");
        const auto checksumMismatchLoad = drs::engine::loadPerformancePackage(checksumMismatchWritePlan.outputPackagePath,
                                                                              cryptoProvider,
                                                                              drs::engine::performancePackageSchemaVersion);
        require(!checksumMismatchLoad.loaded, "Package loader should reject a package whose embedded stream payload no longer matches the stream index.");
        require(containsIssue(checksumMismatchLoad.issues, "payload checksum mismatch"),
                "Checksum-mismatch package load should report a payload checksum mismatch.");

        const auto futureReaderPackagePlan = buildPackagePlan(scratchDirectory / "future-reader",
                                                              drs::engine::performancePackageSchemaVersion + 1);
        const auto futureReaderWrite = drs::engine::writePerformancePackage(futureReaderPackagePlan, cryptoProvider);
        require(futureReaderWrite.written, "Future-reader package write should succeed.");
        const auto futureReaderLoad = drs::engine::loadPerformancePackage(futureReaderPackagePlan.outputPackagePath,
                                                                          cryptoProvider,
                                                                          drs::engine::performancePackageSchemaVersion);
        require(!futureReaderLoad.loaded, "Package loader should reject unsupported reader schema versions.");
        require(containsIssue(futureReaderLoad.issues, "requires reader schema version"),
                "Unsupported-reader package load should explain the reader schema version mismatch.");

        auto missingPayloadWritePlan = drs::engine::buildPerformancePackageWritePlan(
            buildPackagePlan(scratchDirectory / "missing-payload"));
        missingPayloadWritePlan.outputPackagePath = (scratchDirectory / "missing-payload" / "missing-stream-index.drpkg").generic_string();
        missingPayloadWritePlan.payloads.erase(
            std::remove_if(missingPayloadWritePlan.payloads.begin(),
                           missingPayloadWritePlan.payloads.end(),
                           [](const drs::engine::PerformancePackagePayloadSource& payload)
                           {
                               return payload.kind == drs::engine::PerformancePackagePayloadKind::runtimeStreamIndex;
                           }),
            missingPayloadWritePlan.payloads.end());
        const auto missingPayloadWrite = drs::engine::writePerformancePackage(missingPayloadWritePlan, cryptoProvider);
        require(missingPayloadWrite.written, "Missing-payload package should still write successfully.");
        const auto missingPayloadLoad = drs::engine::loadPerformancePackage(missingPayloadWritePlan.outputPackagePath,
                                                                            cryptoProvider,
                                                                            drs::engine::performancePackageSchemaVersion);
        require(!missingPayloadLoad.loaded, "Package loader should reject packages missing required runtime payloads.");
        require(containsIssue(missingPayloadLoad.issues, "runtimeStreamIndex payload"),
                "Missing-payload package load should report the missing runtime stream-index payload.");

        auto wrongSchemaWritePlan = drs::engine::buildPerformancePackageWritePlan(
            buildPackagePlan(scratchDirectory / "wrong-schema"));
        wrongSchemaWritePlan.outputPackagePath = (scratchDirectory / "wrong-schema" / "wrong-schema.drpkg").generic_string();
        auto manifestPayloadIterator = std::find_if(wrongSchemaWritePlan.payloads.begin(),
                                                    wrongSchemaWritePlan.payloads.end(),
                                                    [](const drs::engine::PerformancePackagePayloadSource& payload)
                                                    {
                                                        return payload.kind == drs::engine::PerformancePackagePayloadKind::packageManifest;
                                                    });
        require(manifestPayloadIterator != wrongSchemaWritePlan.payloads.end(),
                "Wrong-schema package plan should include a package-manifest payload.");
        auto manifestJson = json::parse(std::string(manifestPayloadIterator->plaintextBytes.begin(),
                                                    manifestPayloadIterator->plaintextBytes.end()));
        manifestJson["schemaName"] = "drs.notPerformancePackage";
        manifestPayloadIterator->plaintextBytes = toBytes(manifestJson.dump(2) + "\n");
        const auto wrongSchemaWrite = drs::engine::writePerformancePackage(wrongSchemaWritePlan, cryptoProvider);
        require(wrongSchemaWrite.written, "Wrong-schema package should still write successfully.");
        const auto wrongSchemaLoad = drs::engine::loadPerformancePackage(wrongSchemaWritePlan.outputPackagePath,
                                                                         cryptoProvider,
                                                                         drs::engine::performancePackageSchemaVersion);
        require(!wrongSchemaLoad.loaded, "Package loader should reject a package-manifest payload with the wrong schema.");
        require(containsIssue(wrongSchemaLoad.issues, "Package manifest payload schemaName"),
                "Wrong-schema package load should report the package-manifest schema mismatch.");

        std::cout << "Phase 1 performance package loader tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 performance package loader tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
