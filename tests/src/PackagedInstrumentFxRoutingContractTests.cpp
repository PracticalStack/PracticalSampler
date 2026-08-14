#include "drs/engine/PerformancePackage.h"

#include <json/json.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readText(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Could not read fixture " + path.generic_string());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

std::vector<std::string> stringArray(const json& array)
{
    std::vector<std::string> result;
    for (const auto& value : array)
        result.push_back(value.get<std::string>());
    return result;
}
} // namespace

int main()
{
    try
    {
        using namespace drs::engine;

        require(performancePackageLegacySchemaVersion == 1,
                "The legacy package contract must remain schema v1.");
        require(performancePackageFxRoutingSchemaVersion == 2
                    && performancePackageFxRoutingMinimumReaderSchemaVersion == 2,
                "DSP-bearing packages must require package reader schema v2.");
        require(runtimeInstrumentFxRoutingSchemaVersion == 4,
                "Packaged FX/routing must use runtime instrument schema v4.");
        require(performancePackageSchemaVersion == performancePackageLegacySchemaVersion,
                "PX-01 must not change the production writer or reader default.");

        const auto fixtureRoot = fs::path(DRS_PACKAGED_INSTRUMENT_FX_ROUTING_FIXTURE_ROOT);
        const auto valid = json::parse(readText(fixtureRoot / "runtime-instrument-v4-ordered-graph.json"));

        require(valid.at("schemaName") == "drs.instrument"
                    && valid.at("schemaVersion") == runtimeInstrumentFxRoutingSchemaVersion,
                "The canonical graph fixture must identify runtime instrument schema v4.");
        require(valid.contains("fxSlots") && valid.at("fxSlots").is_array()
                    && valid.contains("routingBuses") && valid.at("routingBuses").is_array(),
                "Instrument v4 must carry explicit FX slot and routing bus arrays.");
        require(valid.at("groups").at(0).at("routingBusId") == "bus-group-piano",
                "Instrument v4 groups must explicitly preserve their routing bus assignment.");

        const auto& slots = valid.at("fxSlots");
        require(slots.size() == 3
                    && slots.at(0).at("id") == "air"
                    && slots.at(1).at("id") == "drive"
                    && slots.at(2).at("id") == "room",
                "FX slot array order is part of the frozen serialization contract.");
        require(slots.at(0).at("bypassed").get<bool>()
                    && !slots.at(1).at("bypassed").get<bool>(),
                "Authored slot bypass state must remain explicit.");
        require(stringArray(json::array({ slots.at(1).at("parameters").at(0).at("id"),
                                         slots.at(1).at("parameters").at(1).at("id"),
                                         slots.at(1).at("parameters").at(2).at("id"),
                                         slots.at(1).at("parameters").at(3).at("id"),
                                         slots.at(1).at("parameters").at(4).at("id") }))
                    == std::vector<std::string> { "character", "driveDb", "tone", "mix", "outputDb" },
                "FX parameter array order is part of the frozen serialization contract.");

        const auto& buses = valid.at("routingBuses");
        require(buses.size() == 3
                    && buses.at(0).at("inputSourceId") == "zones/piano-c4"
                    && buses.at(1).at("inputSourceId") == "groups/piano"
                    && buses.at(2).at("inputSourceId") == "master",
                "The fixture must freeze zone, group, and master canonical owners in order.");
        require(buses.at(0).at("chainBypassed").get<bool>()
                    && !buses.at(1).at("chainBypassed").get<bool>(),
                "Authored chain bypass state must remain explicit.");
        require(stringArray(buses.at(0).at("fxSlotIds")) == std::vector<std::string> { "air" }
                    && stringArray(buses.at(1).at("fxSlotIds")) == std::vector<std::string> { "drive" }
                    && stringArray(buses.at(2).at("fxSlotIds")) == std::vector<std::string> { "room" },
                "Each slot must have exactly one canonical chain owner.");

        const auto invalid = json::parse(readText(fixtureRoot / "runtime-instrument-v4-invalid-graph.json"));
        const auto validationExpectations = json::parse(
            readText(fixtureRoot / "validation-expectations.json"));
        const std::unordered_set<std::string> expectedFindingCodes {
            "graph-invalid-owner-source",
            "graph-unknown-group-bus",
            "graph-duplicate-slot-owner",
            "graph-unknown-slot"
        };
        require(validationExpectations.at("fixture") == "runtime-instrument-v4-invalid-graph.json"
                    && invalid.at("schemaVersion") == runtimeInstrumentFxRoutingSchemaVersion,
                "Validation expectations must identify the standalone invalid v4 payload.");
        const auto invalidFindingCodes = stringArray(validationExpectations.at("expectedFindingCodes"));
        require(std::unordered_set<std::string>(invalidFindingCodes.begin(), invalidFindingCodes.end())
                    == expectedFindingCodes,
                "The invalid graph fixture must freeze all fail-closed validation findings.");

        const auto matrix = json::parse(readText(fixtureRoot / "compatibility-matrix.json"));
        require(matrix.at("policyId") == "drs.performancePackage.fxRouting.v1"
                    && matrix.at("cases").size() == 5,
                "The fail-closed compatibility matrix must retain its five reviewed cases.");

        std::unordered_set<std::string> matrixResults;
        for (const auto& entry : matrix.at("cases"))
            matrixResults.insert(entry.at("expectedResult").get<std::string>());
        require(matrixResults == std::unordered_set<std::string> {
                    "load-empty-graph",
                    "reject-before-activation",
                    "load-compiled-graph",
                    "playback-compatibility-failure"
                },
                "The compatibility matrix must preserve legacy loading and fail closed for DSP skew.");

        const auto& oldReaderCase = matrix.at("cases").at(2);
        require(oldReaderCase.at("packageSchemaVersion") == performancePackageFxRoutingSchemaVersion
                    && oldReaderCase.at("instrumentSchemaVersions")
                        == json::array({ runtimeInstrumentFxRoutingSchemaVersion })
                    && oldReaderCase.at("minimumReaderSchemaVersion")
                        == performancePackageFxRoutingMinimumReaderSchemaVersion
                    && oldReaderCase.at("readerSchemaVersion") == performancePackageLegacySchemaVersion
                    && oldReaderCase.at("expectedResult") == "reject-before-activation",
                "A legacy reader must reject a DSP-bearing package before activation.");
        require(matrix.at("cases").at(0).at("instrumentSchemaVersions")
                    == json::array({ 1, 2, 3 }),
                "Legacy package compatibility must cover runtime instrument schemas v1-v3.");

        std::cout << "Packaged instrument FX/routing PX-01 contract tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Packaged instrument FX/routing PX-01 contract tests failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
