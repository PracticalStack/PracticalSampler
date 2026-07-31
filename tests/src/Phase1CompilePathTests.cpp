#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"

#include <json/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <algorithm>

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
        "The native importer and compiled stream writer are intentionally deferred to Sprint 2."
    };
    plan.instrumentValidationNotes = {
        "Uses the existing open HISE sample assets as stand-in sample sources for Sprint 1.",
        "Exercises two articulations, two groups, velocity-layer routing, and explicit prefetch metadata.",
        "Acts as the canonical loader fixture until the import compiler lands in Sprint 2."
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
} // namespace

int main()
{
    try
    {
        const auto referenceDirectory = getReferenceDirectory();
        const auto compileOutputDirectory = fs::temp_directory_path() / "drs-phase1-compile-path-tests";
        const auto referenceCompilePlan = buildReferenceCompilePlan(referenceDirectory);
        const auto tempCompilePlan = buildReferenceCompilePlan(compileOutputDirectory);

        const auto firstCompile = drs::engine::compileRuntimeInstrument(referenceCompilePlan);
        require(firstCompile.compiled, "Reference compile plan should compile successfully.");

        const auto secondCompile = drs::engine::compileRuntimeInstrument(referenceCompilePlan);
        require(secondCompile.compiled, "Reference compile plan should compile successfully on the second pass.");

        const auto generatedProjectJson = drs::engine::serializeRuntimeProjectManifest(firstCompile.project,
                                                                                       referenceCompilePlan.outputProjectPath);
        const auto generatedInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(firstCompile.instrument,
                                                                                            referenceCompilePlan.outputInstrumentPath);
        const auto generatedStreamJson = drs::engine::serializePrototypeStreamContainer(firstCompile,
                                                                                         referenceCompilePlan.outputStreamPath);

        require(generatedProjectJson == drs::engine::serializeRuntimeProjectManifest(secondCompile.project,
                                                                                     referenceCompilePlan.outputProjectPath),
                "Project manifest generation must be deterministic.");
        require(generatedInstrumentJson == drs::engine::serializeRuntimeInstrumentManifest(secondCompile.instrument,
                                                                                           referenceCompilePlan.outputInstrumentPath),
                "Instrument manifest generation must be deterministic.");
        require(generatedStreamJson == drs::engine::serializePrototypeStreamContainer(secondCompile,
                                                                                      referenceCompilePlan.outputStreamPath),
                "Stream container generation must be deterministic.");

        const auto tempCompile = drs::engine::compileRuntimeInstrument(tempCompilePlan);
        require(tempCompile.compiled, "Temp compile plan should compile successfully.");

        const auto tempProjectJson = drs::engine::serializeRuntimeProjectManifest(tempCompile.project,
                                                                                  tempCompilePlan.outputProjectPath);
        const auto tempInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(tempCompile.instrument,
                                                                                       tempCompilePlan.outputInstrumentPath);
        const auto tempStreamJson = drs::engine::serializePrototypeStreamContainer(tempCompile,
                                                                                   tempCompilePlan.outputStreamPath);

        writeTextFile(fs::path(tempCompilePlan.outputProjectPath), tempProjectJson);
        writeTextFile(fs::path(tempCompilePlan.outputInstrumentPath), tempInstrumentJson);
        writeTextFile(fs::path(tempCompilePlan.outputStreamPath), tempStreamJson);

        const auto checkedInProjectPath = referenceDirectory / "tiny-open-instrument.drsproj";
        const auto checkedInInstrumentPath = referenceDirectory / "tiny-open-instrument.drinst";
        const auto checkedInStreamPath = referenceDirectory / "tiny-open-instrument.drstrm";
        require(generatedProjectJson == readTextFile(checkedInProjectPath),
                "Compiler-generated project manifest no longer matches the checked-in reference artifact.");
        require(generatedInstrumentJson == readTextFile(checkedInInstrumentPath),
                "Compiler-generated instrument manifest no longer matches the checked-in reference artifact.");
        require(generatedStreamJson == readTextFile(checkedInStreamPath),
                "Compiler-generated stream descriptor no longer matches the checked-in reference artifact.");

        const auto generatedProjectLoad = drs::engine::loadRuntimeProjectManifest(tempCompilePlan.outputProjectPath);
        require(generatedProjectLoad.loaded, "Loader must open the compiler-generated project manifest.");

        const auto generatedInstrumentLoad = drs::engine::loadRuntimeInstrumentManifest(tempCompilePlan.outputInstrumentPath);
        require(generatedInstrumentLoad.loaded, "Loader must open the compiler-generated instrument manifest.");
        require(generatedInstrumentLoad.metrics.macroCount == 2, "Compiler-generated manifest macro count changed unexpectedly.");
        require(generatedInstrumentLoad.metrics.groupCount == 2, "Compiler-generated manifest group count changed unexpectedly.");
        require(generatedInstrumentLoad.metrics.zoneCount == 4, "Compiler-generated manifest zone count changed unexpectedly.");
        require(generatedInstrumentLoad.metrics.totalPrefetchBytes == 65536,
                "Compiler-generated manifest total prefetch bytes changed unexpectedly.");

        const auto streamJson = json::parse(generatedStreamJson);
        require(streamJson.at("schemaName").get<std::string>() == "drs.streamContainer",
                "Compiler-generated stream descriptor schema name changed unexpectedly.");
        require(streamJson.at("sampleCount").get<int>() == 2,
                "Compiler-generated stream descriptor sample count changed unexpectedly.");
        require(streamJson.at("pageSizeBytes").get<std::uint64_t>() == 65536,
                "Compiler-generated stream descriptor page size changed unexpectedly.");

        const auto& samples = streamJson.at("samples");
        require(samples.is_array() && samples.size() == 2, "Compiler-generated stream descriptor must list two samples.");
        require(samples.at(0).at("sampleId").get<std::string>() == "sine-a3",
                "First compiled stream sample id changed unexpectedly.");
        require(samples.at(1).at("sampleId").get<std::string>() == "triangle-a4",
                "Second compiled stream sample id changed unexpectedly.");
        require(samples.at(0).at("prefetchBytes").get<std::uint64_t>() == 16384,
                "First compiled stream prefetch size changed unexpectedly.");
        require(samples.at(1).at("prefetchBytes").get<std::uint64_t>() == 16384,
                "Second compiled stream prefetch size changed unexpectedly.");
        require(samples.at(1).at("payloadOffsetBytes").get<std::uint64_t>()
                    > samples.at(0).at("payloadOffsetBytes").get<std::uint64_t>(),
                "Compiled stream payload offsets must advance monotonically.");

        auto invalidSampleRatePlan = tempCompilePlan;
        invalidSampleRatePlan.sampleSources.front().metadata.sampleRate = 96000.0;
        const auto invalidSampleRateCompile = drs::engine::compileRuntimeInstrument(invalidSampleRatePlan);
        require(!invalidSampleRateCompile.compiled,
                "Compile plan with unsupported sample rate should be rejected.");
        requireAnyContains(invalidSampleRateCompile.issues,
                           "44100 Hz and 48000 Hz",
                           "Compile policy rejection should explain the supported sample rates.");

        auto invalidLayoutPlan = tempCompilePlan;
        invalidLayoutPlan.sampleSources.front().sourcePath = (compileOutputDirectory / "outside-content-root.wav").generic_string();
        invalidLayoutPlan.sampleSources.front().metadata.sourcePath = invalidLayoutPlan.sampleSources.front().sourcePath;
        const auto invalidLayoutCompile = drs::engine::compileRuntimeInstrument(invalidLayoutPlan);
        require(!invalidLayoutCompile.compiled,
                "Compile plan with sample content outside the content root should be rejected.");
        requireAnyContains(invalidLayoutCompile.issues,
                           "content root",
                           "Compile policy rejection should explain the content-root layout rule.");

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

        std::cout << "Phase 1 compile path tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 compile path tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
