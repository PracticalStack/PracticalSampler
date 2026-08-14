#include "Phase1PerformancePackageSupport.h"
#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/PackageReader.h"

#include <json/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;

std::string readText(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return {};
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

void printIssues(const std::vector<std::string>& issues)
{
    for (const auto& issue : issues)
        std::cerr << "  - " << issue << '\n';
}

int checkPackageOpenAuthoredGraph()
{
    const auto tempRoot = fs::temp_directory_path() / "drs-px01-package-open-red";
    std::error_code cleanupError;
    fs::remove_all(tempRoot, cleanupError);
    fs::create_directories(tempRoot);

    const auto outputPath = tempRoot / "graph-bearing-v4-package.drpkg";
    const auto compileWritePlan = drs::tests::performance_package::buildPackagePlan(
        tempRoot / "compiled", outputPath);
    auto writePlan = drs::engine::buildPerformancePackageWritePlan(compileWritePlan);

    const auto runtimePayload = std::find_if(
        writePlan.payloads.begin(), writePlan.payloads.end(), [](const auto& payload)
        {
            return payload.kind == drs::engine::PerformancePackagePayloadKind::runtimeInstrument;
        });
    if (runtimePayload == writePlan.payloads.end())
    {
        std::cerr << "PX-01 setup error: package plan did not contain a runtime instrument payload.\n";
        fs::remove_all(tempRoot, cleanupError);
        return 2;
    }

    auto runtimeJson = json::parse(std::string(runtimePayload->plaintextBytes.begin(),
                                               runtimePayload->plaintextBytes.end()));
    const auto graphFixture = json::parse(readText(
        fs::path(DRS_PACKAGED_INSTRUMENT_FX_ROUTING_FIXTURE_ROOT)
            / "runtime-instrument-v4-ordered-graph.json"));

    runtimeJson["schemaVersion"] = drs::engine::runtimeInstrumentFxRoutingSchemaVersion;
    runtimeJson["fxSlots"] = graphFixture.at("fxSlots");
    auto buses = graphFixture.at("routingBuses");
    buses.at(0)["id"] = "bus-zone-pad-a3";
    buses.at(0)["inputSourceId"] = "zones/pad-a3";
    buses.at(1)["id"] = "bus-group-pad-core";
    buses.at(1)["inputSourceId"] = "groups/pad-core";
    runtimeJson["routingBuses"] = std::move(buses);
    for (auto& group : runtimeJson.at("groups"))
    {
        group["routingBusId"] = group.at("id") == "pad-core"
            ? "bus-group-pad-core"
            : "master";
    }
    runtimePayload->plaintextBytes = drs::tests::performance_package::toBytes(
        runtimeJson.dump(2) + "\n");

    writePlan.manifest.schemaVersion = drs::engine::performancePackageFxRoutingSchemaVersion;
    writePlan.manifest.minimumReaderSchemaVersion
        = drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion;
    const auto manifestPayload = std::find_if(
        writePlan.payloads.begin(), writePlan.payloads.end(), [](const auto& payload)
        {
            return payload.kind == drs::engine::PerformancePackagePayloadKind::packageManifest;
        });
    if (manifestPayload == writePlan.payloads.end())
    {
        std::cerr << "PX-04 setup error: package plan did not contain a package manifest payload.\n";
        fs::remove_all(tempRoot, cleanupError);
        return 2;
    }
    auto manifestJson = json::parse(std::string(manifestPayload->plaintextBytes.begin(),
                                                manifestPayload->plaintextBytes.end()));
    manifestJson["schemaVersion"] = drs::engine::performancePackageFxRoutingSchemaVersion;
    manifestJson["minimumReaderSchemaVersion"]
        = drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion;
    manifestPayload->plaintextBytes = drs::tests::performance_package::toBytes(
        manifestJson.dump(2) + "\n");

    const auto writeResult = drs::engine::writePerformancePackage(writePlan);
    if (!writeResult.written)
    {
        std::cerr << "PX-01 setup error: graph-bearing characterization package did not write.\n";
        printIssues(writeResult.issues);
        fs::remove_all(tempRoot, cleanupError);
        return 2;
    }

    const auto packageLoad = drs::engine::loadPerformancePackage(
        outputPath.generic_string(),
        drs::engine::getDeterministicPackageCryptoProvider(),
        drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion);
    if (!packageLoad.loaded)
    {
        std::cerr << "PX-01 setup error: graph-bearing characterization package did not load.\n";
        printIssues(packageLoad.issues);
        fs::remove_all(tempRoot, cleanupError);
        return 2;
    }

    const auto prepared = drs::engine::preparePerformancePackageActivation(packageLoad);
    if (!prepared.prepared || prepared.activationPayload == nullptr
        || prepared.activationPayload->snapshot == nullptr)
    {
        std::cerr << "PX-04 failure: graph-bearing package did not prepare for activation.\n";
        printIssues(prepared.issues);
        fs::remove_all(tempRoot, cleanupError);
        return 1;
    }

    const auto& snapshot = *prepared.activationPayload->snapshot;
    const auto padRoute = std::find_if(
        snapshot.groupRoutes.begin(), snapshot.groupRoutes.end(), [](const auto& route)
        {
            return route.groupId == "pad-core";
        });
    const auto graphPlan = drs::engine::compileDspGraphPlan(snapshot);
    const auto hydrated = snapshot.fxSlots.size() == 3
        && snapshot.routingBuses.size() == 3
        && std::all_of(snapshot.fxSlots.begin(), snapshot.fxSlots.end(), [](const auto& slot)
        {
            return slot.catalogResolved;
        })
        && padRoute != snapshot.groupRoutes.end()
        && padRoute->routingSourceId == "groups/pad-core"
        && padRoute->routingBusId == "bus-group-pad-core"
        && graphPlan.compiled
        && graphPlan.plan.nodes.size() == 2
        && !graphPlan.plan.directFastPath;
    fs::remove_all(tempRoot, cleanupError);

    if (!hydrated)
    {
        std::cerr << "PX-04 failure: package open did not reconstruct the authored executable graph.\n";
        return 1;
    }

    std::cout << "Packaged instrument FX/routing activation regression passed.\n";
    return 0;
}
} // namespace

int main()
{
    return checkPackageOpenAuthoredGraph();
}
