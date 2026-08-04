#pragma once

#include "drs/engine/PackageWriter.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace drs::tests::performance_package
{
namespace fs = std::filesystem;

struct CheckedInCorpusPaths
{
    fs::path root;
    fs::path index;
    fs::path valid;
    fs::path truncated;
    fs::path tampered;
    fs::path wrongVersion;
    fs::path missingPayload;
    fs::path checksumMismatch;
};

inline std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.good())
        throw std::runtime_error("Could not open text file: " + path.generic_string());

    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

inline std::vector<std::uint8_t> readBinaryFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.good())
        throw std::runtime_error("Could not open binary file: " + path.generic_string());

    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

inline void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.good())
        throw std::runtime_error("Could not open text file for writing: " + path.generic_string());

    output << text;
    if (!output.good())
        throw std::runtime_error("Could not finish writing text file: " + path.generic_string());
}

inline void writeBinaryFile(const fs::path& path, const std::vector<std::uint8_t>& bytes)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.good())
        throw std::runtime_error("Could not open binary file for writing: " + path.generic_string());

    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output.good())
        throw std::runtime_error("Could not finish writing binary file: " + path.generic_string());
}

inline bool containsIssue(const std::vector<std::string>& issues, const std::string& needle)
{
    for (const auto& issue : issues)
    {
        if (issue.find(needle) != std::string::npos)
            return true;
    }

    return false;
}

inline std::vector<std::uint8_t> toBytes(const std::string& text)
{
    return std::vector<std::uint8_t>(text.begin(), text.end());
}

inline fs::path getReferenceContentRoot()
{
    return fs::path(drs::engine::getPhase1ReferenceProjectManifestPath()).parent_path()
        / ".." / ".." / ".." / ".." / "hise_project";
}

inline CheckedInCorpusPaths getCheckedInCorpusPaths()
{
    const auto root = fs::path(drs::engine::getPhase1ReferenceInstrumentManifestPath()).parent_path()
        / "performance-package-corpus";

    return {
        root,
        root / "index.json",
        root / "valid.drpkg",
        root / "truncated.drpkg",
        root / "tampered.drpkg",
        root / "wrong-version.drpkg",
        root / "missing-payload.drpkg",
        root / "checksum-mismatch.drpkg",
    };
}

inline drs::engine::RuntimeCompilePlan buildReferenceCompilePlan(const fs::path& outputDirectory)
{
    const auto projectPath = outputDirectory / "tiny-open-instrument.drsproj";
    const auto instrumentPath = outputDirectory / "tiny-open-instrument.drinst";
    const auto streamPath = outputDirectory / "tiny-open-instrument.drstrm";
    const auto contentRoot = getReferenceContentRoot().lexically_normal();

    const auto sinePath = (contentRoot / "Samples" / "DRS_Sine_A3.wav").lexically_normal();
    const auto trianglePath = (contentRoot / "Samples" / "DRS_TriangleLead_A4.wav").lexically_normal();

    const auto sineImport = drs::engine::inspectSampleFile(sinePath.generic_string());
    if (!sineImport.accepted)
        throw std::runtime_error("Reference sine sample must inspect successfully before package tests run.");

    const auto triangleImport = drs::engine::inspectSampleFile(trianglePath.generic_string());
    if (!triangleImport.accepted)
        throw std::runtime_error("Reference triangle sample must inspect successfully before package tests run.");

    drs::engine::RuntimeCompilePlan plan;
    plan.outputProjectPath = projectPath.generic_string();
    plan.outputInstrumentPath = instrumentPath.generic_string();
    plan.outputStreamPath = streamPath.generic_string();
    plan.projectId = "drs.phase1.tiny-open-project";
    plan.projectDisplayName = "DRS Tiny Open Project";
    plan.contentRootPath = contentRoot.generic_string();
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

inline drs::engine::PerformancePackageCompileWritePlan buildPackagePlan(
    const fs::path& scratchDirectory,
    const fs::path& outputPackagePath,
    const int minimumReaderSchemaVersion = drs::engine::performancePackageSchemaVersion)
{
    auto compilePlan = buildReferenceCompilePlan(scratchDirectory / "compiled-runtime");
    auto compileResult = drs::engine::compileRuntimeInstrument(compilePlan);
    if (!compileResult.compiled)
        throw std::runtime_error("Reference compile plan should compile successfully for package tests.");

    const auto writeResult = drs::engine::writeCompiledStreamAssets(compileResult);
    if (!writeResult.written)
        throw std::runtime_error("Compiled stream assets should write successfully for package tests.");

    drs::engine::PerformancePackageManifest manifest;
    manifest.packageId = "drs.phase1.tiny-open-instrument.package";
    manifest.displayName = "DRS Tiny Open Instrument Package";
    manifest.instrumentId = compilePlan.instrumentId;
    manifest.defaultLoadProfile = compilePlan.defaultLoadProfile;
    manifest.minimumReaderSchemaVersion = minimumReaderSchemaVersion;
    manifest.notes = {
        "Sprint 7 checked-in performance package corpus.",
        "Contains runtime-only payloads and omits the authored project manifest."
    };

    drs::engine::PerformancePackageCompileWritePlan packagePlan;
    packagePlan.manifest = std::move(manifest);
    packagePlan.compiledRuntime = std::move(compileResult);
    packagePlan.outputPackagePath = outputPackagePath.generic_string();
    packagePlan.minimumCompatibleAppVersion = "0.5.0-internal";
    return packagePlan;
}
} // namespace drs::tests::performance_package
