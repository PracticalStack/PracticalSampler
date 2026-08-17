#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/PlaybackRegionContract.h"

#include <json/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <algorithm>
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

std::vector<char> readBinaryFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

void requireAnyContains(const std::vector<std::string>& messages,
                        const std::string& needle,
                        const std::string& failureMessage)
{
    const auto containsNeedle = std::any_of(messages.begin(),
                                            messages.end(),
                                            [&](const std::string& message)
                                            {
                                                return message.find(needle) != std::string::npos;
                                            });
    require(containsNeedle, failureMessage);
}

fs::path getReferenceDirectory()
{
    return fs::path(drs::engine::getPhase1ReferenceInstrumentManifestPath()).parent_path();
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
    require(sineImport.accepted, "Reference sine sample must inspect successfully before compile tests run.");

    const auto triangleImport = drs::engine::inspectSampleFile(trianglePath.generic_string());
    require(triangleImport.accepted, "Reference triangle sample must inspect successfully before compile tests run.");

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
    plan.projectNotes = {
        "Sprint 1 project fixture used to validate the product-owned runtime model and loader seam.",
        "Sprint 2 finalizes the compiled stream path with a deterministic binary payload writer."
    };
    plan.instrumentValidationNotes = {
        "Uses the existing open HISE sample assets as stand-in sample sources for Sprint 1.",
        "Exercises two articulations, two groups, velocity-layer routing, and explicit prefetch metadata.",
        "Acts as the canonical loader fixture for the deterministic stream index and payload writer."
    };

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

    drs::engine::RuntimeMacroDefinition tone;
    tone.id = "tone";
    tone.name = "Tone";
    tone.defaultValue = 0.35;
    tone.minValue = 0.0;
    tone.maxValue = 1.0;
    plan.macros.push_back(std::move(tone));

    drs::engine::RuntimeMacroDefinition motion;
    motion.id = "motion";
    motion.name = "Motion";
    motion.defaultValue = 0.15;
    motion.minValue = 0.0;
    motion.maxValue = 1.0;
    plan.macros.push_back(std::move(motion));

    drs::engine::RuntimeArticulationDefinition sustain;
    sustain.id = "sustain";
    sustain.name = "Sustain";
    sustain.isDefault = true;
    plan.articulations.push_back(std::move(sustain));

    drs::engine::RuntimeArticulationDefinition lead;
    lead.id = "lead";
    lead.name = "Triangle Lead";
    lead.isDefault = false;
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

void addPackagedFxRoutingGraph(drs::engine::RuntimeCompilePlan& plan)
{
    for (auto& group : plan.groups)
        group.routingBusId = group.id == "pad-core" ? "bus-group-pad-core" : "master";

    drs::engine::RuntimeProjectFxSlotDefinition drive;
    drive.id = "drive";
    drive.displayName = "Drive";
    drive.effectType = "drs.saturator";
    drive.effectVersion = 1;
    drive.parameters = {
        { "character", 0.0 },
        { "driveDb", 7.5 },
        { "tone", 0.55 },
        { "mix", 0.8 },
        { "outputDb", -1.0 }
    };

    drs::engine::RuntimeProjectFxSlotDefinition room;
    room.id = "room";
    room.displayName = "Room";
    room.effectType = "drs.algorithmicReverb";
    room.effectVersion = 1;
    room.parameters = {
        { "preDelayMs", 18.0 },
        { "size", 0.62 },
        { "decaySeconds", 2.5 },
        { "damping", 0.45 },
        { "width", 1.0 },
        { "mix", 0.18 }
    };

    plan.fxSlots = { std::move(drive), std::move(room) };
    plan.routingBuses = {
        { "bus-group-pad-core", "Pad Core Insert", "groups/pad-core", { "drive" }, false },
        { "bus-master", "Master Insert", "master", { "room" }, false }
    };
}

void runPackagedFxRoutingCompileContract(const fs::path& outputDirectory)
{
    auto graphCompilePlan = buildReferenceCompilePlan(outputDirectory / "fx-routing-v4");
    addPackagedFxRoutingGraph(graphCompilePlan);
    const auto graphCompile = drs::engine::compileRuntimeInstrument(graphCompilePlan);
    require(graphCompile.compiled
                && graphCompile.instrument.schemaVersion
                    == drs::engine::runtimeInstrumentFxRoutingSchemaVersion,
            "A graph-bearing compile plan must produce runtime instrument schema v4.");
    require(graphCompile.instrument.fxSlots.size() == 2
                && graphCompile.instrument.fxSlots.at(0).id == "drive"
                && graphCompile.instrument.fxSlots.at(0).parameters.at(1).id == "driveDb"
                && graphCompile.instrument.routingBuses.size() == 2
                && graphCompile.instrument.routingBuses.at(0).fxSlotIds
                    == std::vector<std::string> { "drive" }
                && graphCompile.instrument.groups.at(0).routingBusId == "bus-group-pad-core"
                && graphCompile.instrument.groups.at(1).routingBusId == "master",
            "Runtime compilation must preserve ordered slots, parameters, buses, chains, and group routing.");

    const auto graphInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(
        graphCompile.instrument, graphCompilePlan.outputInstrumentPath);
    const auto graphRoundTrip = drs::engine::parseRuntimeInstrumentManifest(
        graphInstrumentJson, graphCompilePlan.outputInstrumentPath, false);
    require(graphRoundTrip.loaded
                && drs::engine::serializeRuntimeInstrumentManifest(
                       graphRoundTrip.instrument, graphCompilePlan.outputInstrumentPath)
                    == graphInstrumentJson,
            "A compiled runtime instrument v4 graph must round-trip deterministically.");

    auto invalidGraphPlan = graphCompilePlan;
    invalidGraphPlan.groups.at(0).routingBusId = "missing-group-bus";
    invalidGraphPlan.routingBuses.at(0).inputSourceId = "group/pad-core";
    invalidGraphPlan.routingBuses.at(1).fxSlotIds = { "drive", "missing-slot" };
    const auto invalidGraphCompile = drs::engine::compileRuntimeInstrument(invalidGraphPlan);
    require(!invalidGraphCompile.compiled,
            "Runtime compilation must fail closed for malformed v4 graph topology.");
    requireAnyContains(invalidGraphCompile.issues, "graph-invalid-owner-source",
                       "Compile validation must report non-canonical graph owners.");
    requireAnyContains(invalidGraphCompile.issues, "graph-unknown-group-bus",
                       "Compile validation must report unknown group routing buses.");
    requireAnyContains(invalidGraphCompile.issues, "graph-duplicate-slot-owner",
                       "Compile validation must report duplicate slot ownership.");
    requireAnyContains(invalidGraphCompile.issues, "graph-unknown-slot",
                       "Compile validation must report unknown slot references.");
}
} // namespace

int main(const int argc, char** argv)
{
    try
    {
        const auto compileOutputDirectory = fs::temp_directory_path() / "drs-phase1-compile-path-tests";
        if (argc == 2 && std::string(argv[1]) == "--px02-fx-routing")
        {
            runPackagedFxRoutingCompileContract(compileOutputDirectory);
            std::cout << "PX-02 packaged instrument FX/routing compile contract passed." << std::endl;
            return 0;
        }
        require(argc == 1, "Usage: drs_phase1_compile_path_tests [--px02-fx-routing]");

        const auto referenceDirectory = getReferenceDirectory();
        const auto referenceCompilePlan = buildReferenceCompilePlan(referenceDirectory);
        const auto tempCompilePlan = buildReferenceCompilePlan(compileOutputDirectory);

        auto firstCompile = drs::engine::compileRuntimeInstrument(referenceCompilePlan);
        require(firstCompile.compiled, "Reference compile plan should compile successfully.");
        auto firstWriteCompile = firstCompile;
        firstWriteCompile.payloadFilePath = (compileOutputDirectory / "reference-pass-1" / "tiny-open-instrument.drstrm.bin").generic_string();
        const auto firstWrite = drs::engine::writeCompiledStreamAssets(firstWriteCompile);
        require(firstWrite.written, "Reference compile plan should write compiled stream assets successfully.");
        firstCompile = firstWriteCompile;
        firstCompile.payloadFilePath = drs::engine::buildCompiledStreamPayloadPath(referenceCompilePlan.outputStreamPath);

        auto secondCompile = drs::engine::compileRuntimeInstrument(referenceCompilePlan);
        require(secondCompile.compiled, "Reference compile plan should compile successfully on the second pass.");
        auto secondWriteCompile = secondCompile;
        secondWriteCompile.payloadFilePath = (compileOutputDirectory / "reference-pass-2" / "tiny-open-instrument.drstrm.bin").generic_string();
        const auto secondWrite = drs::engine::writeCompiledStreamAssets(secondWriteCompile);
        require(secondWrite.written, "Reference compile plan should write compiled stream assets successfully on the second pass.");
        secondCompile = secondWriteCompile;
        secondCompile.payloadFilePath = drs::engine::buildCompiledStreamPayloadPath(referenceCompilePlan.outputStreamPath);

        const auto generatedProjectJson = drs::engine::serializeRuntimeProjectManifest(firstCompile.project,
                                                                                       referenceCompilePlan.outputProjectPath);
        const auto generatedInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(firstCompile.instrument,
                                                                                            referenceCompilePlan.outputInstrumentPath);
        const auto generatedStreamJson = drs::engine::serializeCompiledStreamIndex(firstCompile,
                                                                                   referenceCompilePlan.outputStreamPath);

        require(generatedProjectJson == drs::engine::serializeRuntimeProjectManifest(secondCompile.project,
                                                                                     referenceCompilePlan.outputProjectPath),
                "Project manifest generation must be deterministic.");
        require(generatedInstrumentJson == drs::engine::serializeRuntimeInstrumentManifest(secondCompile.instrument,
                                                                                           referenceCompilePlan.outputInstrumentPath),
                "Instrument manifest generation must be deterministic.");
        require(generatedStreamJson == drs::engine::serializeCompiledStreamIndex(secondCompile,
                                                                                 referenceCompilePlan.outputStreamPath),
                "Stream index generation must be deterministic.");
        require(firstWrite.payloadFileChecksumHex == secondWrite.payloadFileChecksumHex,
                "Compiled stream payload generation must be deterministic.");

        auto tempCompile = drs::engine::compileRuntimeInstrument(tempCompilePlan);
        require(tempCompile.compiled, "Temp compile plan should compile successfully.");

        auto playbackRegionPlan = tempCompilePlan;
        playbackRegionPlan.zones.front().sampleStartFrame = 10;
        playbackRegionPlan.zones.front().sampleEndFrame = 100;
        const auto playbackRegionCompile = drs::engine::compileRuntimeInstrument(playbackRegionPlan);
        require(playbackRegionCompile.compiled
                    && playbackRegionCompile.instrument.schemaVersion
                        == drs::engine::playbackRegionInstrumentSchemaVersion
                    && playbackRegionCompile.instrument.zones.front().sampleStartFrame == 10
                    && playbackRegionCompile.instrument.zones.front().sampleEndFrame == 100,
                "Runtime compilation must retain authored playback bounds in instrument schema 6.");
        const auto playbackRegionInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(
            playbackRegionCompile.instrument, playbackRegionPlan.outputInstrumentPath);
        const auto playbackRegionInstrumentRoundTrip = drs::engine::parseRuntimeInstrumentManifest(
            playbackRegionInstrumentJson, playbackRegionPlan.outputInstrumentPath, false);
        require(playbackRegionInstrumentRoundTrip.loaded
                    && playbackRegionInstrumentRoundTrip.instrument.zones.front().sampleEndFrame == 100,
                "Packaged runtime-instrument serialization must round-trip the playback end.");

        auto maxBoundaryPlan = tempCompilePlan;
        maxBoundaryPlan.zones.front().sampleEndFrame
            = maxBoundaryPlan.sampleSources.front().metadata.frameCount;
        require(drs::engine::compileRuntimeInstrument(maxBoundaryPlan).compiled,
                "The physical source frame count must remain the maximum legal exclusive boundary.");
        auto beyondBoundaryPlan = maxBoundaryPlan;
        ++beyondBoundaryPlan.zones.front().sampleEndFrame;
        require(!drs::engine::compileRuntimeInstrument(beyondBoundaryPlan).compiled,
                "Runtime compilation must reject playback ends beyond the physical source.");

        runPackagedFxRoutingCompileContract(compileOutputDirectory);

        const auto tempWrite = drs::engine::writeCompiledStreamAssets(tempCompile);
        require(tempWrite.written, "Temp compile plan should write compiled stream assets successfully.");

        const auto tempProjectJson = drs::engine::serializeRuntimeProjectManifest(tempCompile.project,
                                                                                  tempCompilePlan.outputProjectPath);
        const auto tempInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(tempCompile.instrument,
                                                                                       tempCompilePlan.outputInstrumentPath);
        const auto tempStreamJson = drs::engine::serializeCompiledStreamIndex(tempCompile,
                                                                              tempCompilePlan.outputStreamPath);

        writeTextFile(fs::path(tempCompilePlan.outputProjectPath), tempProjectJson);
        writeTextFile(fs::path(tempCompilePlan.outputInstrumentPath), tempInstrumentJson);
        writeTextFile(fs::path(tempCompilePlan.outputStreamPath), tempStreamJson);

        const auto checkedInProjectPath = referenceDirectory / "tiny-open-instrument.drsproj";
        const auto checkedInInstrumentPath = referenceDirectory / "tiny-open-instrument.drinst";
        const auto checkedInStreamPath = referenceDirectory / "tiny-open-instrument.drstrm";
        const auto checkedInPayloadPath = fs::path(drs::engine::buildCompiledStreamPayloadPath(referenceCompilePlan.outputStreamPath));
        require(generatedProjectJson == readTextFile(checkedInProjectPath),
                "Compiler-generated project manifest no longer matches the checked-in reference artifact.");
        require(generatedInstrumentJson == readTextFile(checkedInInstrumentPath),
                "Compiler-generated instrument manifest no longer matches the checked-in reference artifact.");
        require(generatedStreamJson == readTextFile(checkedInStreamPath),
                "Compiler-generated stream index no longer matches the checked-in reference artifact.");
        require(readBinaryFile(firstWrite.payloadPath) == readBinaryFile(checkedInPayloadPath),
                "Compiler-generated stream payload no longer matches the checked-in reference artifact.");

        const auto generatedProjectLoad = drs::engine::loadRuntimeProjectManifest(tempCompilePlan.outputProjectPath);
        require(generatedProjectLoad.loaded, "Loader must open the compiler-generated project manifest.");

        const auto generatedInstrumentLoad = drs::engine::loadRuntimeInstrumentManifest(tempCompilePlan.outputInstrumentPath);
        require(generatedInstrumentLoad.loaded, "Loader must open the compiler-generated instrument manifest.");
        require(generatedInstrumentLoad.metrics.macroCount == 2, "Compiler-generated manifest macro count changed unexpectedly.");
        require(generatedInstrumentLoad.metrics.groupCount == 2, "Compiler-generated manifest group count changed unexpectedly.");
        require(generatedInstrumentLoad.metrics.zoneCount == 4, "Compiler-generated manifest zone count changed unexpectedly.");
        require(generatedInstrumentLoad.metrics.totalPrefetchBytes == 65536,
                "Compiler-generated manifest total prefetch bytes changed unexpectedly.");
        const auto generatedStreamLoad = drs::engine::loadRuntimeStreamContainer(tempCompilePlan.outputStreamPath);
        require(generatedStreamLoad.loaded, "Loader must open the compiler-generated stream index and payload.");
        require(generatedStreamLoad.metrics.payloadAssetResolved,
                "Compiler-generated stream payload asset should resolve successfully.");
        require(generatedStreamLoad.metrics.payloadChecksumValidatedCount == 3,
                "Compiler-generated stream payload checksum validation count changed unexpectedly.");

        const auto streamJson = json::parse(generatedStreamJson);
        require(streamJson.at("schemaName").get<std::string>() == "drs.streamContainer",
                "Compiler-generated stream index schema name changed unexpectedly.");
        require(streamJson.at("sampleCount").get<int>() == 2,
                "Compiler-generated stream index sample count changed unexpectedly.");
        require(streamJson.at("pageSizeBytes").get<std::uint64_t>() == 65536,
                "Compiler-generated stream index page size changed unexpectedly.");
        require(streamJson.at("payloadAssetPath").get<std::string>() == "tiny-open-instrument.drstrm.bin",
                "Compiler-generated stream index payload asset path changed unexpectedly.");
        require(streamJson.at("payloadFileChecksumHex").get<std::string>() == firstWrite.payloadFileChecksumHex,
                "Compiler-generated stream index payload checksum changed unexpectedly.");

        const auto& samples = streamJson.at("samples");
        require(samples.is_array() && samples.size() == 2, "Compiler-generated stream index must list two samples.");
        require(samples.at(0).at("sampleId").get<std::string>() == "sine-a3",
                "First compiled stream sample id changed unexpectedly.");
        require(samples.at(1).at("sampleId").get<std::string>() == "triangle-a4",
                "Second compiled stream sample id changed unexpectedly.");
        require(samples.at(0).at("prefetchBytes").get<std::uint64_t>() == 16384,
                "First compiled stream prefetch size changed unexpectedly.");
        require(samples.at(1).at("prefetchBytes").get<std::uint64_t>() == 16384,
                "Second compiled stream prefetch size changed unexpectedly.");
        require(!samples.at(0).at("payloadChecksumHex").get<std::string>().empty(),
                "Compiled stream samples should include payload checksums.");
        require(samples.at(1).at("payloadOffsetBytes").get<std::uint64_t>()
                    > samples.at(0).at("payloadOffsetBytes").get<std::uint64_t>(),
                "Compiled stream payload offsets must advance monotonically.");

        auto invalidSampleRatePlan = tempCompilePlan;
        invalidSampleRatePlan.sampleSources.front().metadata.sampleRate = 96000.0;
        const auto invalidSampleRateCompile = drs::engine::compileRuntimeInstrument(invalidSampleRatePlan);
        require(invalidSampleRateCompile.compiled,
                "Compile plan with unusual sample rate should remain compilable under the warning-only sample-rate policy.");
        requireAnyContains(invalidSampleRateCompile.warnings,
                           "prefers 44100 Hz or 48000 Hz",
                           "Compile policy warning should explain the preferred sample rates.");

        auto invalidLayoutPlan = tempCompilePlan;
        invalidLayoutPlan.sampleSources.front().sourcePath = (compileOutputDirectory / "outside-content-root.wav").generic_string();
        invalidLayoutPlan.sampleSources.front().metadata.sourcePath = invalidLayoutPlan.sampleSources.front().sourcePath;
        const auto invalidLayoutCompile = drs::engine::compileRuntimeInstrument(invalidLayoutPlan);
        require(invalidLayoutCompile.compiled,
                "Compile plan with sample content outside the content root should remain compilable with a warning.");
        requireAnyContains(invalidLayoutCompile.warnings,
                           "content root",
                           "Compile policy warning should explain the content-root layout rule.");

        auto duplicateSourcePlan = tempCompilePlan;
        duplicateSourcePlan.sampleSources.push_back(duplicateSourcePlan.sampleSources.front());
        const auto duplicateSourceCompile = drs::engine::compileRuntimeInstrument(duplicateSourcePlan);
        require(!duplicateSourceCompile.compiled,
                "Compile plan with duplicate source ids should be rejected.");
        requireAnyContains(duplicateSourceCompile.issues,
                           "duplicate id",
                           "Compile rejection should explain duplicate source ids.");

        auto awkwardNamingPlan = tempCompilePlan;
        awkwardNamingPlan.sampleSources.front().sourcePath = (fs::path(awkwardNamingPlan.contentRootPath) / "Samples" / "Bad Name!.wav").generic_string();
        awkwardNamingPlan.sampleSources.front().metadata.sourcePath = awkwardNamingPlan.sampleSources.front().sourcePath;
        const auto awkwardNamingCompile = drs::engine::compileRuntimeInstrument(awkwardNamingPlan);
        require(awkwardNamingCompile.compiled,
                "Compile plan with a non-portable sample name should still compile with warnings.");
        require(!awkwardNamingCompile.warnings.empty(),
                "Compile plan with a non-portable sample name should surface a warning.");
        requireAnyContains(awkwardNamingCompile.warnings,
                           "portable sample names",
                           "Compile warnings should preserve the naming-policy guidance.");

        auto missingPayloadCompile = tempCompile;
        missingPayloadCompile.streamSamples.front().sourcePath = (compileOutputDirectory / "missing-source.wav").generic_string();
        const auto missingPayloadWrite = drs::engine::writeCompiledStreamAssets(missingPayloadCompile);
        require(!missingPayloadWrite.written,
                "Compiled stream writer should reject missing source samples.");
        requireAnyContains(missingPayloadWrite.issues,
                           "could not decode source sample",
                           "Compiled stream writer rejection should explain missing source samples.");

        std::cout << "Phase 1 compile path tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 compile path tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
