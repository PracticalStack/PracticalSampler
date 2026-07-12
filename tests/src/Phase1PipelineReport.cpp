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
#include <vector>

namespace
{
namespace fs = std::filesystem;
using ordered_json = nlohmann::ordered_json;

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
    require(output.good(), "Could not open pipeline report artifact for writing: " + path.generic_string());
    output << text;
    require(output.good(), "Could not finish writing pipeline report artifact: " + path.generic_string());
}

bool containsText(const std::vector<std::string>& messages, const std::string& needle)
{
    return std::any_of(messages.begin(),
                       messages.end(),
                       [&](const std::string& message)
                       {
                           return message.find(needle) != std::string::npos;
                       });
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

    const auto sineImport = drs::engine::importSampleFile(sinePath.generic_string());
    require(sineImport.imported, "Reference sine sample must import before the pipeline report runs.");

    const auto triangleImport = drs::engine::importSampleFile(trianglePath.generic_string());
    require(triangleImport.imported, "Reference triangle sample must import before the pipeline report runs.");

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
        "Exercises two articulations, two groups, and explicit prefetch metadata.",
        "Acts as the canonical loader fixture until the import compiler lands in Sprint 2."
    };

    drs::engine::RuntimeCompileSourceDefinition sineSource;
    sineSource.id = "sine-a3";
    sineSource.sourcePath = sinePath.generic_string();
    sineSource.role = "core-sustain";
    sineSource.metadata = sineImport.sample.metadata;
    plan.sampleSources.push_back(std::move(sineSource));

    drs::engine::RuntimeCompileSourceDefinition triangleSource;
    triangleSource.id = "triangle-a4";
    triangleSource.sourcePath = trianglePath.generic_string();
    triangleSource.role = "core-lead";
    triangleSource.metadata = triangleImport.sample.metadata;
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
    padZone.velocityHigh = 127;
    padZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(padZone));

    drs::engine::RuntimeCompileZoneDefinition leadZone;
    leadZone.id = "lead-a4";
    leadZone.sourceId = "triangle-a4";
    leadZone.groupId = "lead-core";
    leadZone.articulationId = "lead";
    leadZone.rootKey = 69;
    leadZone.keyLow = 60;
    leadZone.keyHigh = 96;
    leadZone.velocityLow = 1;
    leadZone.velocityHigh = 127;
    leadZone.prefetchBytes = 16384;
    plan.zones.push_back(std::move(leadZone));

    return plan;
}

ordered_json buildImportEntry(const drs::engine::SampleImportResult& result)
{
    ordered_json entry;
    entry["sourcePath"] = result.sourcePath;
    entry["imported"] = result.imported;
    entry["state"] = result.state;
    entry["warningCount"] = result.warnings.size();
    entry["issueCount"] = result.issues.size();
    entry["warnings"] = result.warnings;
    entry["issues"] = result.issues;

    if (result.imported)
    {
        entry["formatName"] = result.sample.metadata.formatName;
        entry["sampleRate"] = result.sample.metadata.sampleRate;
        entry["frameCount"] = result.sample.metadata.frameCount;
        entry["channelCount"] = result.sample.metadata.channelCount;
    }

    return entry;
}

ordered_json buildCompileEntry(const drs::engine::RuntimeCompileResult& result)
{
    ordered_json entry;
    entry["compiled"] = result.compiled;
    entry["state"] = result.state;
    entry["warningCount"] = result.warnings.size();
    entry["issueCount"] = result.issues.size();
    entry["warnings"] = result.warnings;
    entry["issues"] = result.issues;
    entry["streamSampleCount"] = result.streamSamples.size();
    entry["totalPayloadBytes"] = result.totalPayloadBytes;
    return entry;
}
} // namespace

int main(int argc, char* argv[])
{
    const auto outputPath = argc >= 2
        ? fs::path(argv[1])
        : fs::temp_directory_path() / "drs-phase1-pipeline-report.json";

    ordered_json report;
    report["report"] = "drs.phase1.pipelineStatus";
    report["referenceCorpusId"] = "tiny-open-instrument";

    try
    {
        const auto referenceProject = drs::engine::loadPhase1ReferenceProjectManifest();
        const auto referenceInstrument = drs::engine::loadPhase1ReferenceInstrumentManifest();

        ordered_json loaderSection;
        loaderSection["projectLoaded"] = referenceProject.loaded;
        loaderSection["instrumentLoaded"] = referenceInstrument.loaded;
        loaderSection["projectIssues"] = referenceProject.issues;
        loaderSection["instrumentIssues"] = referenceInstrument.issues;
        loaderSection["sampleSourceCount"] = referenceProject.project.sampleSources.size();
        loaderSection["macroCount"] = referenceInstrument.metrics.macroCount;
        loaderSection["zoneCount"] = referenceInstrument.metrics.zoneCount;
        loaderSection["sourceProjectResolved"] = referenceInstrument.metrics.sourceProjectResolved;
        loaderSection["compiledStreamAssetResolved"] = referenceInstrument.metrics.compiledStreamAssetResolved;
        const bool loaderPassed = referenceProject.loaded
            && referenceInstrument.loaded
            && referenceInstrument.metrics.sourceProjectResolved
            && referenceInstrument.metrics.compiledStreamAssetResolved;
        loaderSection["passed"] = loaderPassed;
        report["loader"] = std::move(loaderSection);

        const auto referencePlan = buildReferenceCompilePlan(getReferenceDirectory());
        const auto tempDirectory = fs::temp_directory_path() / "drs-phase1-pipeline-report";
        const auto tempPlan = buildReferenceCompilePlan(tempDirectory);

        ordered_json importerSection;
        importerSection["samples"] = ordered_json::array();
        bool importerPassed = true;
        for (const auto& source : referencePlan.sampleSources)
        {
            const auto importResult = drs::engine::importSampleFile(source.sourcePath);
            importerSection["samples"].push_back(buildImportEntry(importResult));
            importerPassed = importerPassed && importResult.imported && importResult.issues.empty();
        }
        importerSection["passed"] = importerPassed;
        report["importer"] = std::move(importerSection);

        const auto referenceCompile = drs::engine::compileRuntimeInstrument(referencePlan);
        const auto secondReferenceCompile = drs::engine::compileRuntimeInstrument(referencePlan);
        const auto tempCompile = drs::engine::compileRuntimeInstrument(tempPlan);

        const auto referenceProjectJson = drs::engine::serializeRuntimeProjectManifest(referenceCompile.project,
                                                                                       referencePlan.outputProjectPath);
        const auto referenceInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(referenceCompile.instrument,
                                                                                            referencePlan.outputInstrumentPath);
        const auto referenceStreamJson = drs::engine::serializePrototypeStreamContainer(referenceCompile,
                                                                                         referencePlan.outputStreamPath);

        const auto deterministicProject = referenceProjectJson
            == drs::engine::serializeRuntimeProjectManifest(secondReferenceCompile.project,
                                                            referencePlan.outputProjectPath);
        const auto deterministicInstrument = referenceInstrumentJson
            == drs::engine::serializeRuntimeInstrumentManifest(secondReferenceCompile.instrument,
                                                               referencePlan.outputInstrumentPath);
        const auto deterministicStream = referenceStreamJson
            == drs::engine::serializePrototypeStreamContainer(secondReferenceCompile,
                                                              referencePlan.outputStreamPath);

        const auto checkedInProjectPath = getReferenceDirectory() / "tiny-open-instrument.drsproj";
        const auto checkedInInstrumentPath = getReferenceDirectory() / "tiny-open-instrument.drinst";
        const auto checkedInStreamPath = getReferenceDirectory() / "tiny-open-instrument.drstrm";

        const bool goldenProjectMatch = referenceProjectJson == readTextFile(checkedInProjectPath);
        const bool goldenInstrumentMatch = referenceInstrumentJson == readTextFile(checkedInInstrumentPath);
        const bool goldenStreamMatch = referenceStreamJson == readTextFile(checkedInStreamPath);

        const auto tempProjectJson = drs::engine::serializeRuntimeProjectManifest(tempCompile.project,
                                                                                  tempPlan.outputProjectPath);
        const auto tempInstrumentJson = drs::engine::serializeRuntimeInstrumentManifest(tempCompile.instrument,
                                                                                       tempPlan.outputInstrumentPath);
        const auto tempStreamJson = drs::engine::serializePrototypeStreamContainer(tempCompile,
                                                                                   tempPlan.outputStreamPath);

        writeTextFile(fs::path(tempPlan.outputProjectPath), tempProjectJson);
        writeTextFile(fs::path(tempPlan.outputInstrumentPath), tempInstrumentJson);
        writeTextFile(fs::path(tempPlan.outputStreamPath), tempStreamJson);

        const auto tempLoadedProject = drs::engine::loadRuntimeProjectManifest(tempPlan.outputProjectPath);
        const auto tempLoadedInstrument = drs::engine::loadRuntimeInstrumentManifest(tempPlan.outputInstrumentPath);

        ordered_json compileSection = buildCompileEntry(referenceCompile);
        compileSection["deterministicProject"] = deterministicProject;
        compileSection["deterministicInstrument"] = deterministicInstrument;
        compileSection["deterministicStream"] = deterministicStream;
        compileSection["goldenProjectMatch"] = goldenProjectMatch;
        compileSection["goldenInstrumentMatch"] = goldenInstrumentMatch;
        compileSection["goldenStreamMatch"] = goldenStreamMatch;
        compileSection["tempProjectLoaded"] = tempLoadedProject.loaded;
        compileSection["tempInstrumentLoaded"] = tempLoadedInstrument.loaded;
        const bool compilePassed = referenceCompile.compiled
            && secondReferenceCompile.compiled
            && tempCompile.compiled
            && deterministicProject
            && deterministicInstrument
            && deterministicStream
            && goldenProjectMatch
            && goldenInstrumentMatch
            && goldenStreamMatch
            && tempLoadedProject.loaded
            && tempLoadedInstrument.loaded;
        compileSection["passed"] = compilePassed;
        report["compilePath"] = std::move(compileSection);

        ordered_json corruptionSection;
        const auto missingDefaultManifest = fs::path(drs::engine::getPhase1RuntimeRootPath())
            / "negative-corpus"
            / "missing-default-articulation"
            / "missing-default-articulation.drinst";
        const auto missingSampleManifest = fs::path(drs::engine::getPhase1RuntimeRootPath())
            / "negative-corpus"
            / "missing-sample-file"
            / "missing-sample-file.drinst";
        const auto malformedJsonManifest = fs::path(drs::engine::getPhase1RuntimeRootPath())
            / "negative-corpus"
            / "malformed-json"
            / "malformed-json.drinst";

        const auto missingDefaultResult = drs::engine::loadRuntimeInstrumentManifest(missingDefaultManifest.generic_string());
        const auto missingSampleResult = drs::engine::loadRuntimeInstrumentManifest(missingSampleManifest.generic_string());
        const auto malformedJsonResult = drs::engine::loadRuntimeInstrumentManifest(malformedJsonManifest.generic_string());

        fs::remove(fs::path(tempPlan.outputStreamPath));
        const auto missingGeneratedStreamResult = drs::engine::loadRuntimeInstrumentManifest(tempPlan.outputInstrumentPath);

        const auto malformedGeneratedInstrumentPath = tempDirectory / "tiny-open-instrument-corrupt.drinst";
        writeTextFile(malformedGeneratedInstrumentPath, "{ invalid json\n");
        const auto malformedGeneratedResult = drs::engine::loadRuntimeInstrumentManifest(malformedGeneratedInstrumentPath.generic_string());

        const bool missingDefaultRejected = !missingDefaultResult.loaded
            && containsText(missingDefaultResult.issues, "default articulation");
        const bool missingSampleRejected = !missingSampleResult.loaded
            && containsText(missingSampleResult.issues, "Zone sample does not exist");
        const bool malformedFixtureRejected = !malformedJsonResult.loaded
            && containsText(malformedJsonResult.issues, "JSON parse failed");
        const bool missingGeneratedStreamRejected = !missingGeneratedStreamResult.loaded
            && containsText(missingGeneratedStreamResult.issues, "Compiled stream asset must exist");
        const bool malformedGeneratedRejected = !malformedGeneratedResult.loaded
            && containsText(malformedGeneratedResult.issues, "JSON parse failed");

        corruptionSection["missingDefaultRejected"] = missingDefaultRejected;
        corruptionSection["missingSampleRejected"] = missingSampleRejected;
        corruptionSection["malformedFixtureRejected"] = malformedFixtureRejected;
        corruptionSection["missingGeneratedStreamRejected"] = missingGeneratedStreamRejected;
        corruptionSection["malformedGeneratedRejected"] = malformedGeneratedRejected;
        corruptionSection["passed"] = missingDefaultRejected
            && missingSampleRejected
            && malformedFixtureRejected
            && missingGeneratedStreamRejected
            && malformedGeneratedRejected;
        report["corruptionChecks"] = std::move(corruptionSection);

        const bool overallPassed = loaderPassed
            && importerPassed
            && compilePassed
            && report["corruptionChecks"].at("passed").get<bool>();
        report["passed"] = overallPassed;
        writeTextFile(outputPath, report.dump(2) + "\n");
        std::cout << report.dump(2) << std::endl;

        require(overallPassed, "Phase 1 pipeline report detected one or more failing sections.");
        return 0;
    }
    catch (const std::exception& exception)
    {
        report["passed"] = false;
        report["fatalError"] = exception.what();
        try
        {
            writeTextFile(outputPath, report.dump(2) + "\n");
        }
        catch (const std::exception&)
        {
        }

        std::cerr << "Phase 1 pipeline report failed: " << exception.what() << std::endl;
        return 1;
    }
}
