#include "drs/engine/SfzImportContract.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <string>

namespace
{
std::string readText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Could not read " + path.generic_string());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::filesystem::path resolveFirstFixturePath()
{
    const auto sourceRoot = std::filesystem::path(DRS_SOURCE_ROOT);
    const auto relativeFixturePath =
        std::filesystem::path("DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz");

    const auto localFixturePath = sourceRoot / relativeFixturePath;
    if (std::filesystem::exists(localFixturePath))
        return localFixturePath;

    const auto workspaceFixturePath = sourceRoot.parent_path() / relativeFixturePath;
    if (std::filesystem::exists(workspaceFixturePath))
        return workspaceFixturePath;

    throw std::runtime_error("Could not locate " + relativeFixturePath.generic_string());
}

std::size_t countMatches(const std::string& text, const std::regex& pattern)
{
    return static_cast<std::size_t>(
        std::distance(std::sregex_iterator(text.begin(), text.end(), pattern), std::sregex_iterator()));
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto fixturePath = resolveFirstFixturePath();
        const auto text = readText(fixturePath);

        require(countMatches(text, std::regex(R"(<control>)"))
                    == SfzFirstFixtureCharacterization::expectedControlHeaderCount,
                "The first SFZ fixture control-header count changed unexpectedly.");
        require(countMatches(text, std::regex(R"(<master>)"))
                    == SfzFirstFixtureCharacterization::expectedMasterHeaderCount,
                "The first SFZ fixture master-header count changed unexpectedly.");
        require(countMatches(text, std::regex(R"(<group>)"))
                    == SfzFirstFixtureCharacterization::expectedGroupHeaderCount,
                "The first SFZ fixture group-header count changed unexpectedly.");
        require(countMatches(text, std::regex(R"(<curve>)"))
                    == SfzFirstFixtureCharacterization::expectedCurveHeaderCount,
                "The first SFZ fixture curve-header count changed unexpectedly.");
        require(countMatches(text, std::regex(R"(<region>)"))
                    == SfzFirstFixtureCharacterization::expectedRegionCount,
                "The first SFZ fixture region count changed unexpectedly.");

        require(countMatches(text, std::regex(R"(seq_length=3)"))
                    == SfzFirstFixtureCharacterization::expectedRegionCount,
                "The first SFZ fixture should keep a 3-way round-robin declaration on every region.");
        require(countMatches(text, std::regex(R"(seq_position=1)")) == 75
                    && countMatches(text, std::regex(R"(seq_position=2)")) == 75
                    && countMatches(text, std::regex(R"(seq_position=3)")) == 75,
                "The first SFZ fixture round-robin positions changed unexpectedly.");

        require(countMatches(text, std::regex(R"(sample=[^ \r\n]+\.flac)"))
                    == SfzFirstFixtureCharacterization::expectedRegionCount,
                "The first SFZ fixture should continue to reference one FLAC sample per region.");
        require(countMatches(text, std::regex(R"(sample=[^/\\ \r\n]+\.flac)"))
                    == SfzFirstFixtureCharacterization::expectedRegionCount,
                "The first SFZ fixture sample references should remain local relative FLAC filenames.");

        require(countMatches(text, std::regex(R"(xfin_lovel=)")) == 4
                    && countMatches(text, std::regex(R"(xfin_hivel=)")) == 4
                    && countMatches(text, std::regex(R"(xfout_lovel=)")) == 4
                    && countMatches(text, std::regex(R"(xfout_hivel=)")) == 4,
                "The first SFZ fixture velocity-crossfade group declarations changed unexpectedly.");
        require(text.find("label_cc1=Mod:width") != std::string::npos
                    && text.find("set_hdcc1=0.5") != std::string::npos
                    && text.find("width_oncc1=100") != std::string::npos
                    && text.find("width_curvecc1=99") != std::string::npos,
                "The first SFZ fixture CC labeling or width modulation declarations changed unexpectedly.");
        require(text.find("<curve> curve_index=99") != std::string::npos,
                "The first SFZ fixture should keep its width-control curve declaration.");

        std::cout << "Sprint 3.1.1 SFZ fixture characterization tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.1 SFZ fixture characterization tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
