#include "Phase1PerformancePackageSupport.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/PackageReader.h"

#include <json/json.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;

constexpr std::array<std::string_view, 1> redSeams {
    "package-open-authored-graph"
};

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

    const auto outputPath = tempRoot / "graph-bearing-v3-characterization.drpkg";
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

    // Characterize today's additive-field loss through a schema version the
    // current reader accepts, so the check reaches snapshot reconstruction.
    runtimeJson["schemaVersion"] = 3;
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

    const auto writeResult = drs::engine::writePerformancePackage(writePlan);
    if (!writeResult.written)
    {
        std::cerr << "PX-01 setup error: graph-bearing characterization package did not write.\n";
        printIssues(writeResult.issues);
        fs::remove_all(tempRoot, cleanupError);
        return 2;
    }

    const auto packageLoad = drs::engine::loadPerformancePackage(outputPath.generic_string());
    if (!packageLoad.loaded)
    {
        std::cerr << "PX-01 setup error: graph-bearing characterization package did not load.\n";
        printIssues(packageLoad.issues);
        fs::remove_all(tempRoot, cleanupError);
        return 2;
    }

    const auto prepared = drs::engine::preparePerformancePackageActivation(packageLoad);
    const auto& snapshot = prepared.snapshotResult.snapshot;
    const auto masterOnly = prepared.prepared
        && snapshot.fxSlots.empty()
        && snapshot.routingBuses.empty()
        && std::all_of(snapshot.groupRoutes.begin(), snapshot.groupRoutes.end(), [](const auto& route)
        {
            return route.routingSourceId == "master" && route.routingBusId == "master";
        });
    fs::remove_all(tempRoot, cleanupError);

    if (masterOnly)
    {
        std::cerr << "EXPECTED RED: package open accepted additive graph fields but reconstructed an empty, master-only snapshot.\n";
        return 1;
    }

    return 0;
}
} // namespace

int main(const int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: drs_px01_fx_routing_contract_red_tests <named-missing-seam>\n";
        for (const auto seam : redSeams)
            std::cerr << "  " << seam << '\n';
        return 2;
    }

    const auto seam = std::string_view(argv[1]);
    if (seam == "package-open-authored-graph")
        return checkPackageOpenAuthoredGraph();

    std::cerr << "Unknown PX-01 expected-red seam '" << seam << "'.\n";
    return 2;
}
