#include "drs/engine/Phase1Baseline.h"
#include "drs/engine/RuntimeLoader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void writeReportFile(const fs::path& outputPath, const std::string& reportJson)
{
    fs::create_directories(outputPath.parent_path());

    std::ofstream output(outputPath, std::ios::binary);
    require(output.good(), "Could not open the baseline report output file for writing.");
    output << reportJson;
    require(output.good(), "Could not finish writing the baseline report output file.");
}
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        const auto coldResult = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(coldResult.loaded, "Cold-load baseline could not load the reference manifest.");

        const auto warmResult = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(warmResult.loaded, "Warm-load baseline could not load the reference manifest.");

        const auto reportJson = drs::engine::buildPhase1RuntimeBaselineReportJson(coldResult, warmResult);
        std::cout << reportJson;

        if (argc >= 2)
            writeReportFile(fs::path(argv[1]), reportJson);

        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 baseline report failed: " << exception.what() << std::endl;
        return 1;
    }
}
