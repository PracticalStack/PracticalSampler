#include "drs/engine/VelocityCrossfade.h"

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

std::filesystem::path resolveFixturePath(const std::filesystem::path& relativeFixturePath)
{
    const auto sourceRoot = std::filesystem::path(DRS_SOURCE_ROOT);

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
        const auto monoCrossfadePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz");
        const auto stereoCrossfadePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st.sfz");
        const auto monoControlPath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono-no-xfade.sfz");

        const auto monoCrossfadeText = readText(monoCrossfadePath);
        const auto stereoCrossfadeText = readText(stereoCrossfadePath);
        const auto monoControlText = readText(monoControlPath);

        require(countMatches(monoCrossfadeText, std::regex(R"(<group>)"))
                    == VelocityCrossfadeFirstPassFixtureCharacterization::expectedLayerCount,
                "The primary mono crossfade fixture group count changed unexpectedly.");
        require(countMatches(stereoCrossfadeText, std::regex(R"(<group>)"))
                    == VelocityCrossfadeFirstPassFixtureCharacterization::expectedLayerCount,
                "The stereo crossfade fixture group count changed unexpectedly.");
        require(countMatches(monoControlText, std::regex(R"(<group>)"))
                    == VelocityCrossfadeFirstPassFixtureCharacterization::expectedLayerCount,
                "The no-crossfade control fixture group count changed unexpectedly.");

        require(countMatches(monoCrossfadeText, std::regex(R"(xfin_lovel=)"))
                    == VelocityCrossfadeFirstPassFixtureCharacterization::expectedOverlapCount
                    && countMatches(monoCrossfadeText, std::regex(R"(xfout_lovel=)"))
                           == VelocityCrossfadeFirstPassFixtureCharacterization::expectedOverlapCount,
                "The primary mono crossfade fixture overlap count changed unexpectedly.");
        require(countMatches(stereoCrossfadeText, std::regex(R"(xfin_lovel=)"))
                    == VelocityCrossfadeFirstPassFixtureCharacterization::expectedOverlapCount
                    && countMatches(stereoCrossfadeText, std::regex(R"(xfout_lovel=)"))
                           == VelocityCrossfadeFirstPassFixtureCharacterization::expectedOverlapCount,
                "The stereo crossfade fixture overlap count changed unexpectedly.");
        require(countMatches(monoControlText, std::regex(R"(xfin_lovel=|xfout_lovel=)")) == 0,
                "The no-crossfade control fixture must stay free of crossfade declarations.");

        for (std::size_t index = 0;
             index < VelocityCrossfadeFirstPassFixtureCharacterization::expectedOverlapCount;
             ++index)
        {
            const auto low = VelocityCrossfadeFirstPassFixtureCharacterization::expectedOverlapLowVelocity(index);
            const auto high = VelocityCrossfadeFirstPassFixtureCharacterization::expectedOverlapHighVelocity(index);
            const auto overlapPattern = std::regex("xfin_lovel=" + std::to_string(low)
                                                   + " xfin_hivel=" + std::to_string(high));
            const auto outgoingPattern = std::regex("xfout_lovel=" + std::to_string(low)
                                                    + " xfout_hivel=" + std::to_string(high));

            require(countMatches(monoCrossfadeText, overlapPattern) == 1
                        && countMatches(monoCrossfadeText, outgoingPattern) == 1,
                    "The mono fixture overlap intervals changed unexpectedly.");
            require(countMatches(stereoCrossfadeText, overlapPattern) == 1
                        && countMatches(stereoCrossfadeText, outgoingPattern) == 1,
                    "The stereo fixture overlap intervals changed unexpectedly.");
        }

        for (std::size_t index = 0;
             index < VelocityCrossfadeFirstPassFixtureCharacterization::expectedLayerCount;
             ++index)
        {
            const auto low = VelocityCrossfadeFirstPassFixtureCharacterization::expectedControlVelocityLow(index);
            const auto high = VelocityCrossfadeFirstPassFixtureCharacterization::expectedControlVelocityHigh(index);
            const auto controlPattern = std::regex("lovel=" + std::to_string(low)
                                                   + " hivel=" + std::to_string(high));
            require(countMatches(monoControlText, controlPattern) == 1,
                    "The no-crossfade control layer boundaries changed unexpectedly.");
        }

        require(monoCrossfadeText.find("<group> lovel=1 xfout_lovel=25 xfout_hivel=60") != std::string::npos,
                "The mono fixture must keep the first outgoing crossfade layer.");
        require(monoCrossfadeText.find("<group> xfin_lovel=104 xfin_hivel=119 hivel=127") != std::string::npos,
                "The mono fixture must keep the terminal incoming crossfade layer.");
        require(stereoCrossfadeText.find("sample=A_029__F1_5-ST_rr1.flac") != std::string::npos,
                "The stereo fixture should remain rooted in the stereo sample set.");

        std::cout << "Phase 3 crossfade Sprint 1 fixture characterization tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 3 crossfade Sprint 1 fixture characterization tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
