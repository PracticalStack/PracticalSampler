#pragma once

#include "drs/engine/PackageWriter.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/NativeContent.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
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

inline std::vector<std::uint8_t> decodeBase64(std::string_view encoded)
{
    auto decodeValue = [](const char character) -> std::optional<std::uint8_t>
    {
        if (character >= 'A' && character <= 'Z')
            return static_cast<std::uint8_t>(character - 'A');
        if (character >= 'a' && character <= 'z')
            return static_cast<std::uint8_t>(character - 'a' + 26);
        if (character >= '0' && character <= '9')
            return static_cast<std::uint8_t>(character - '0' + 52);
        if (character == '+')
            return static_cast<std::uint8_t>(62);
        if (character == '/')
            return static_cast<std::uint8_t>(63);
        if (character == '=')
            return std::nullopt;
        throw std::runtime_error("Invalid base64 character in package test fixture.");
    };

    std::vector<std::uint8_t> decoded;
    decoded.reserve((encoded.size() * 3) / 4);

    for (std::size_t index = 0; index < encoded.size(); index += 4)
    {
        const auto a = decodeValue(encoded.at(index));
        const auto b = decodeValue(encoded.at(index + 1));
        const auto c = decodeValue(encoded.at(index + 2));
        const auto d = decodeValue(encoded.at(index + 3));
        if (!a.has_value() || !b.has_value())
            throw std::runtime_error("Invalid padded base64 in package test fixture.");

        decoded.push_back(static_cast<std::uint8_t>((*a << 2u) | (*b >> 4u)));
        if (c.has_value())
        {
            decoded.push_back(static_cast<std::uint8_t>(((*b & 0x0Fu) << 4u) | (*c >> 2u)));
            if (d.has_value())
            {
                decoded.push_back(static_cast<std::uint8_t>(((*c & 0x03u) << 6u) | *d));
            }
        }
    }

    return decoded;
}

inline std::vector<std::uint8_t> buildBackgroundImageJpegFixture()
{
    static const auto bytes = decodeBase64(
        "/9j/4AAQSkZJRgABAQEAYABgAAD/2wBDAAMCAgMCAgMDAwMEAwMEBQgFBQQEBQoHBwYIDAoMDAsKCwsNDhIQDQ4RDgsLEBYQERMUFRUVDA8XGBYUGBIUFRT/"
        "2wBDAQMEBAUEBQkFBQkUDQsNFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBT/wAARCAABAAEDASIAAhEBAxEB/"
        "8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2Jygg"
        "kKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXG"
        "x8jJytLT1NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECA"
        "xEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhY"
        "aHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwD1Oiiivyc/TT//2Q==");
    return bytes;
}

inline std::vector<std::uint8_t> buildLicenseTextFixture()
{
    return {
        0xefu, 0xbbu, 0xbfu,
        'D', 'e', 'c', 'e', 'n', 't', ' ', 'R', 'h', 'a', 'p', 's', 'o', 'd', 'y', '\n',
        'T', 'e', 's', 't', ' ', 'L', 'i', 'c', 'e', 'n', 's', 'e', '\n'
    };
}

inline fs::path getReferenceContentRoot()
{
    return fs::path(drs::engine::getNativeContentRoots().samplesRoot);
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

    const auto sinePath = (contentRoot / "DRS_Sine_A3.wav").lexically_normal();
    const auto trianglePath = (contentRoot / "DRS_TriangleLead_A4.wav").lexically_normal();

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
    plan.masterGainDb = -1.5;
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
    padCore.gainDb = -3.0;
    plan.groups.push_back(std::move(padCore));

    drs::engine::RuntimeGroupDefinition leadCore;
    leadCore.id = "lead-core";
    leadCore.name = "Lead Core";
    leadCore.articulationIds = { "lead" };
    leadCore.gainDb = 1.5;
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
    padZone.gainDb = -0.75;
    padZone.prefetchBytes = 16384;
    padZone.fineTuneCents = 17.0;
    padZone.amplitudeVelocityTracking = 37.0;
    padZone.controllerConditions = { { 23, 0, 63 } };
    padZone.performance.event = drs::engine::PerformanceEventKind::release;
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
    padAccentZone.gainDb = 1.25;
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
    leadZone.gainDb = -2.0;
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
    leadAccentZone.gainDb = 0.5;
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
    manifest.masterGainDb = compileResult.masterGainDb;
    for (const auto& group : compileResult.instrument.groups)
        manifest.groupRoutes.push_back({ group.id, group.gainDb });
    manifest.backgroundImage.payloadId = "background-image";
    manifest.license.payloadId = drs::engine::playableInstrumentLicensePayloadId;
    manifest.notes = {
        "Sprint 7 checked-in performance package corpus.",
        "Contains runtime-only payloads and omits the authored project manifest."
    };

    drs::engine::PerformancePackageCompileWritePlan packagePlan;
    packagePlan.manifest = std::move(manifest);
    packagePlan.compiledRuntime = std::move(compileResult);
    packagePlan.outputPackagePath = outputPackagePath.generic_string();
    packagePlan.minimumCompatibleAppVersion = "0.5.0-internal";
    packagePlan.additionalPayloads.push_back({
        "background-image",
        drs::engine::PerformancePackagePayloadKind::backgroundImage,
        "images/background.jpg",
        "image/jpeg",
        buildBackgroundImageJpegFixture()
    });
    packagePlan.additionalPayloads.push_back({
        drs::engine::playableInstrumentLicensePayloadId,
        drs::engine::PerformancePackagePayloadKind::licenseText,
        drs::engine::playableInstrumentLicenseLogicalPath,
        drs::engine::playableInstrumentLicenseMediaType,
        buildLicenseTextFixture()
    });
    return packagePlan;
}

inline drs::engine::RuntimeProjectModel buildAuthoringProjectFixture()
{
    const auto contentRoot = getReferenceContentRoot().lexically_normal().generic_string();

    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "drs.phase1.package-export-project";
    project.displayName = "Package Export Fixture";
    project.contentRootPath = contentRoot;
    project.defaultInstrumentManifestPath
        = (getReferenceContentRoot() / "PackageExportFixture.drinst").generic_string();
    project.notes = { "Sprint 6 playable package export fixture." };

    project.sampleSources.push_back({ "sine-a3", "DRS_Sine_A3.wav", "core-sustain" });
    project.sampleSources.push_back({ "triangle-a4", "DRS_TriangleLead_A4.wav", "core-lead" });

    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    project.authoring.masterGainDb = -1.5;
    project.authoring.articulations.push_back({ "sustain", "Sustain", true, 0, std::nullopt });
    project.authoring.articulations.push_back({ "lead", "Lead", false, 1, std::nullopt });
    project.authoring.groups.push_back({ "pad-core", "Pad Core", 0, true, -3.0, 0.0, {}, {} });
    project.authoring.groups.push_back({ "lead-core", "Lead Core", 1, true, 1.5, 0.0, {}, {} });

    drs::engine::RuntimeProjectZoneDefinition padZone;
    padZone.id = "pad-a3";
    padZone.sampleSourceId = "sine-a3";
    padZone.displayName = "Pad A3";
    padZone.groupId = "pad-core";
    padZone.articulationId = "sustain";
    padZone.rootKey = 57;
    padZone.keyLow = 36;
    padZone.keyHigh = 76;
    padZone.velocityLow = 1;
    padZone.velocityHigh = 95;
    padZone.gainDb = -0.75;
    project.authoring.zones.push_back(std::move(padZone));

    drs::engine::RuntimeProjectZoneDefinition leadZone;
    leadZone.id = "lead-a4";
    leadZone.sampleSourceId = "triangle-a4";
    leadZone.displayName = "Lead A4";
    leadZone.groupId = "lead-core";
    leadZone.articulationId = "lead";
    leadZone.rootKey = 69;
    leadZone.keyLow = 60;
    leadZone.keyHigh = 96;
    leadZone.velocityLow = 1;
    leadZone.velocityHigh = 127;
    leadZone.gainDb = 0.5;
    leadZone.sampleStartFrame = 64;
    project.authoring.zones.push_back(std::move(leadZone));

    return project;
}
} // namespace drs::tests::performance_package
