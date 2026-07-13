#include "drs/engine/RuntimeLoader.h"

#include <json/json.hpp>

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

bool containsIssue(const drs::engine::RuntimeManifestLoadResult& result, const std::string& needle)
{
    for (const auto& issue : result.issues)
    {
        if (issue.find(needle) != std::string::npos)
            return true;
    }

    return false;
}

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

fs::path getPhase1RuntimeRoot()
{
    return fs::path(drs::engine::getPhase1RuntimeRootPath());
}
} // namespace

int main()
{
    try
    {
        const auto referenceProject = drs::engine::loadPhase1ReferenceProjectManifest();
        require(referenceProject.loaded, "Reference Phase 1 project must load cleanly.");
        require(referenceProject.project.sampleSources.size() == 2,
                "Reference project sample-source count changed unexpectedly.");

        const auto referenceProjectPath = fs::path(drs::engine::getPhase1ReferenceProjectManifestPath());
        const auto serializedProject = drs::engine::serializeRuntimeProjectManifest(referenceProject.project,
                                                                                   referenceProjectPath.generic_string());
        require(serializedProject == readTextFile(referenceProjectPath),
                "Reference project manifest did not round-trip back to the checked-in golden file.");

        const auto referenceResult = drs::engine::loadPhase1ReferenceInstrumentManifest();

        require(referenceResult.loaded, "Reference Phase 1 manifest must load cleanly.");
        require(referenceResult.metrics.manifestSizeBytes > 0, "Reference manifest size must be recorded.");
        require(referenceResult.metrics.loadDurationMicros > 0, "Reference load duration must be recorded.");
        require(referenceResult.metrics.sourceProjectResolved, "Reference source project must resolve successfully.");
        require(referenceResult.metrics.compiledStreamAssetResolved, "Reference stream asset must resolve successfully.");
        require(referenceResult.metrics.macroCount == 2, "Reference macro count changed unexpectedly.");
        require(referenceResult.metrics.zoneCount == 4, "Reference zone count changed unexpectedly.");

        const auto referenceManifestPath = fs::path(drs::engine::getPhase1ReferenceInstrumentManifestPath());
        const auto serializedManifest = drs::engine::serializeRuntimeInstrumentManifest(referenceResult.instrument,
                                                                                        referenceManifestPath.generic_string());
        require(serializedManifest == readTextFile(referenceManifestPath),
                "Reference instrument manifest did not round-trip back to the checked-in golden file.");

        const auto missingDefaultManifest = getPhase1RuntimeRoot()
            / "negative-corpus"
            / "missing-default-articulation"
            / "missing-default-articulation.drinst";
        const auto missingDefaultResult = drs::engine::loadRuntimeInstrumentManifest(missingDefaultManifest.generic_string());

        require(!missingDefaultResult.loaded, "Missing-default negative fixture should fail validation.");
        require(containsIssue(missingDefaultResult, "default articulation"),
                "Missing-default negative fixture must report the missing default articulation issue.");

        const auto missingSampleManifest = getPhase1RuntimeRoot()
            / "negative-corpus"
            / "missing-sample-file"
            / "missing-sample-file.drinst";
        const auto missingSampleResult = drs::engine::loadRuntimeInstrumentManifest(missingSampleManifest.generic_string());

        require(!missingSampleResult.loaded, "Missing-sample negative fixture should fail validation.");
        require(containsIssue(missingSampleResult, "Zone sample does not exist"),
                "Missing-sample negative fixture must report the missing sample-path issue.");

        const auto malformedJsonManifest = getPhase1RuntimeRoot()
            / "negative-corpus"
            / "malformed-json"
            / "malformed-json.drinst";
        const auto malformedJsonResult = drs::engine::loadRuntimeInstrumentManifest(malformedJsonManifest.generic_string());

        require(!malformedJsonResult.loaded, "Malformed-json corrupt fixture should fail validation.");
        require(containsIssue(malformedJsonResult, "JSON parse failed"),
                "Malformed-json corrupt fixture must report a parse failure issue.");

        const auto corpusIndexPath = fs::path(drs::engine::getPhase1ReferenceCorpusIndexPath());
        require(fs::exists(corpusIndexPath), "Reference corpus index must exist.");

        const auto corpusIndexText = readTextFile(corpusIndexPath);
        require(corpusIndexText.find("\"negative-missing-default-articulation\"") != std::string::npos,
                "Reference corpus index must list the missing-default negative fixture.");
        require(corpusIndexText.find("\"negative-missing-sample-file\"") != std::string::npos,
                "Reference corpus index must list the missing-sample negative fixture.");
        require(corpusIndexText.find("\"corrupt-malformed-json\"") != std::string::npos,
                "Reference corpus index must list the malformed-json corrupt fixture.");

        const auto baselinePath = fs::path(drs::engine::getPhase1ReferenceBaselinePath());
        require(fs::exists(baselinePath), "Checked-in Phase 1 baseline snapshot must exist.");

        const auto baselineJson = json::parse(readTextFile(baselinePath));
        require(baselineJson.at("schemaName").get<std::string>() == "drs.runtimeBaseline",
                "Checked-in baseline schema name changed unexpectedly.");
        require(baselineJson.at("schemaVersion").get<int>() == 1,
                "Checked-in baseline schema version changed unexpectedly.");
        require(baselineJson.at("baselineId").get<std::string>() == "drs.phase1.tiny-open-instrument",
                "Checked-in baseline id changed unexpectedly.");
        require(baselineJson.at("timingUnits").get<std::string>() == "microseconds",
                "Checked-in baseline timing units changed unexpectedly.");

        const auto& staticExpectations = baselineJson.at("staticExpectations");
        require(staticExpectations.at("manifestBytes").get<std::uint64_t>() == referenceResult.metrics.manifestSizeBytes,
                "Checked-in baseline manifest size does not match the reference fixture.");
        require(staticExpectations.at("sourceProjectResolved").get<bool>() == referenceResult.metrics.sourceProjectResolved,
                "Checked-in baseline source-project resolution flag does not match the reference fixture.");
        require(staticExpectations.at("compiledStreamAssetResolved").get<bool>() == referenceResult.metrics.compiledStreamAssetResolved,
                "Checked-in baseline stream-asset resolution flag does not match the reference fixture.");
        require(staticExpectations.at("macroCount").get<std::size_t>() == referenceResult.metrics.macroCount,
                "Checked-in baseline macro count does not match the reference fixture.");
        require(staticExpectations.at("articulationCount").get<std::size_t>() == referenceResult.metrics.articulationCount,
                "Checked-in baseline articulation count does not match the reference fixture.");
        require(staticExpectations.at("groupCount").get<std::size_t>() == referenceResult.metrics.groupCount,
                "Checked-in baseline group count does not match the reference fixture.");
        require(staticExpectations.at("zoneCount").get<std::size_t>() == referenceResult.metrics.zoneCount,
                "Checked-in baseline zone count does not match the reference fixture.");
        require(staticExpectations.at("referencedSampleCount").get<std::size_t>() == referenceResult.metrics.referencedSampleCount,
                "Checked-in baseline referenced-sample count does not match the reference fixture.");
        require(staticExpectations.at("totalPrefetchBytes").get<std::uint64_t>() == referenceResult.metrics.totalPrefetchBytes,
                "Checked-in baseline prefetch total does not match the reference fixture.");

        const auto& latestObserved = baselineJson.at("latestObserved");
        require(latestObserved.at("coldLoadMicros").get<std::uint64_t>() > 0,
                "Checked-in baseline must carry a positive cold-load observation.");
        require(latestObserved.at("warmLoadMicros").get<std::uint64_t>() > 0,
                "Checked-in baseline must carry a positive warm-load observation.");

        std::cout << "Phase 1 runtime contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 runtime contract tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
