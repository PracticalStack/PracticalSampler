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

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "Could not open JSON file: " + path.generic_string());
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

json readJsonFile(const fs::path& path)
{
    return json::parse(readTextFile(path));
}

std::uint64_t readUnsignedField(const json& object, const char* fieldName)
{
    require(object.contains(fieldName), std::string("Missing required field '") + fieldName + "'.");
    return object.at(fieldName).get<std::uint64_t>();
}
void compareStaticExpectation(const json& generatedReport,
                             const json& checkedInBaseline,
                             const char* generatedField,
                             const char* baselineField)
{
    const auto& staticExpectations = checkedInBaseline.at("staticExpectations");
    require(generatedReport.at(generatedField) == staticExpectations.at(baselineField),
            std::string("Generated baseline field '") + generatedField
                + "' diverged from checked-in static expectation '" + baselineField
                + "'. Review the reference fixture change before refreshing the checked-in snapshot.");
}

void compareTimingWithPolicy(const json& generatedReport,
                             const json& checkedInBaseline,
                             const char* generatedField)
{
    const auto& latestObserved = checkedInBaseline.at("latestObserved");
    const auto& driftPolicy = checkedInBaseline.at("driftPolicy");

    const auto generatedValue = readUnsignedField(generatedReport, generatedField);
    const auto checkedInValue = readUnsignedField(latestObserved, generatedField);
    const auto allowedPositiveDriftMicros = readUnsignedField(driftPolicy, "allowedPositiveDriftMicros");
    const auto allowedNegativeDriftMicros = readUnsignedField(driftPolicy, "allowedNegativeDriftMicros");

    const auto upperBound = checkedInValue + allowedPositiveDriftMicros;
    require(generatedValue <= upperBound,
            std::string("Generated timing field '") + generatedField
                + "' exceeded the checked-in baseline by more than the allowed positive drift. Review the generated artifact and refresh the checked-in baseline only if the drift is intentional.");

    require(generatedValue + allowedNegativeDriftMicros >= checkedInValue,
            std::string("Generated timing field '") + generatedField
                + "' was lower than the checked-in baseline beyond the allowed negative drift. Refresh the checked-in baseline only after reviewing the change intentionally.");
}
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        require(argc >= 2, "Expected path to generated baseline artifact as the first argument.");

        const auto generatedReportPath = fs::path(argv[1]);
        const auto checkedInBaselinePath = fs::path(drs::engine::getPhase1ReferenceBaselinePath());

        require(fs::exists(generatedReportPath), "Generated baseline artifact file does not exist.");
        require(fs::exists(checkedInBaselinePath), "Checked-in baseline snapshot does not exist.");

        const auto generatedReport = readJsonFile(generatedReportPath);
        const auto checkedInBaseline = readJsonFile(checkedInBaselinePath);

        require(generatedReport.at("report").get<std::string>() == "drs.phase1.runtimeBaseline",
                "Generated baseline artifact report type changed unexpectedly.");
        require(checkedInBaseline.at("schemaName").get<std::string>() == "drs.runtimeBaseline",
                "Checked-in baseline schema name changed unexpectedly.");
        require(checkedInBaseline.at("schemaVersion").get<int>() == 1,
                "Checked-in baseline schema version changed unexpectedly.");
        require(checkedInBaseline.at("timingUnits").get<std::string>() == "microseconds",
                "Checked-in baseline timing units changed unexpectedly.");
        require(checkedInBaseline.at("baselineId").get<std::string>() == generatedReport.at("instrumentId").get<std::string>(),
                "Checked-in baseline baselineId no longer matches the generated artifact instrument id.");

        compareStaticExpectation(generatedReport, checkedInBaseline, "manifestBytes", "manifestBytes");
        compareStaticExpectation(generatedReport, checkedInBaseline, "sourceProjectResolved", "sourceProjectResolved");
        compareStaticExpectation(generatedReport, checkedInBaseline, "compiledStreamAssetResolved", "compiledStreamAssetResolved");
        compareStaticExpectation(generatedReport, checkedInBaseline, "macroCount", "macroCount");
        compareStaticExpectation(generatedReport, checkedInBaseline, "articulationCount", "articulationCount");
        compareStaticExpectation(generatedReport, checkedInBaseline, "groupCount", "groupCount");
        compareStaticExpectation(generatedReport, checkedInBaseline, "zoneCount", "zoneCount");
        compareStaticExpectation(generatedReport, checkedInBaseline, "referencedSampleCount", "referencedSampleCount");
        compareStaticExpectation(generatedReport, checkedInBaseline, "totalPrefetchBytes", "totalPrefetchBytes");

        compareTimingWithPolicy(generatedReport, checkedInBaseline, "coldLoadMicros");
        compareTimingWithPolicy(generatedReport, checkedInBaseline, "warmLoadMicros");

        std::cout << "Phase 1 runtime baseline guard passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 runtime baseline guard failed: " << exception.what() << std::endl;
        return 1;
    }
}
