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

std::size_t countFilesWithExtension(const std::filesystem::path& root, const std::string& extension)
{
    std::size_t count = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (entry.is_regular_file() && entry.path().extension() == extension)
            ++count;
    }

    return count;
}
} // namespace

int main()
{
    try
    {
        const auto corpusRoot = resolveFixturePath("DemoSFVInstruments/jlearman.jRhodes3d-master-rr");
        const auto generatedNotePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/GENERATED_PSEUDO_RR.md");
        const auto readmePath = resolveFixturePath("DemoSFVInstruments/jlearman.jRhodes3d-master-rr/README.md");
        const auto monoFixturePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz");
        const auto monoNoXfadePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-no-xfade-flac.sfz");
        const auto stereoFixturePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st/_jRhodes3d-st-flac.sfz");
        const auto stereoNoXfadePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st/_jRhodes3d-st-no-xfade-flac.sfz");
        const auto svFixturePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-sv/_jRhodes3d-sv-flac.sfz");

        const auto generatedNoteText = readText(generatedNotePath);
        const auto readmeText = readText(readmePath);
        const auto monoFixtureText = readText(monoFixturePath);
        const auto monoNoXfadeText = readText(monoNoXfadePath);
        const auto stereoFixtureText = readText(stereoFixturePath);
        const auto stereoNoXfadeText = readText(stereoNoXfadePath);
        const auto svFixtureText = readText(svFixturePath);

        require(countFilesWithExtension(corpusRoot, ".sfz") == 12,
                "The pseudo-RR corpus SFZ count changed unexpectedly.");
        require(countFilesWithExtension(corpusRoot, ".flac") == 585,
                "The pseudo-RR corpus FLAC count changed unexpectedly.");

        require(countFilesWithExtension(corpusRoot / "jRhodes3d-mono", ".flac") == 195
                    && countFilesWithExtension(corpusRoot / "jRhodes3d-st", ".flac") == 195
                    && countFilesWithExtension(corpusRoot / "jRhodes3d-sv", ".flac") == 195,
                "Each generated RR branch should continue to contain 195 FLAC samples.");

        require(generatedNoteText.find("round-robin variants per source sample: `3`") != std::string::npos
                    && generatedNoteText.find("source FLAC count: `195`") != std::string::npos
                    && generatedNoteText.find("rewritten SFZ count: `12`") != std::string::npos,
                "The generated pseudo-RR inventory note changed unexpectedly.");

        require(readmeText.find("* jRhodes3d-mono.sfz - mono only") != std::string::npos
                    && readmeText.find("* jRhodes3d-st.sfz - stereo effect") != std::string::npos
                    && readmeText.find("* jRhodes3d-sv.sfz - stereo vibrato effect") != std::string::npos,
                "The pseudo-RR README should keep the six-root-wrapper inventory.");

        require(countMatches(monoFixtureText, std::regex(R"(<region>)")) == 225
                    && countMatches(stereoFixtureText, std::regex(R"(<region>)")) == 225
                    && countMatches(svFixtureText, std::regex(R"(<region>)")) == 225,
                "Primary RR branch fixtures should remain 225-region documents.");

        require(countMatches(monoFixtureText, std::regex(R"(seq_length=3)")) == 225
                    && countMatches(monoFixtureText, std::regex(R"(seq_position=1)")) == 75
                    && countMatches(monoFixtureText, std::regex(R"(seq_position=2)")) == 75
                    && countMatches(monoFixtureText, std::regex(R"(seq_position=3)")) == 75,
                "The primary mono RR fixture should keep balanced 3-way slot distribution.");
        require(countMatches(stereoFixtureText, std::regex(R"(seq_length=3)")) == 225
                    && countMatches(stereoFixtureText, std::regex(R"(seq_position=1)")) == 75
                    && countMatches(stereoFixtureText, std::regex(R"(seq_position=2)")) == 75
                    && countMatches(stereoFixtureText, std::regex(R"(seq_position=3)")) == 75,
                "The primary stereo RR fixture should keep balanced 3-way slot distribution.");

        require(countMatches(monoFixtureText, std::regex(R"(xfin_lovel=)")) == 4
                    && countMatches(monoFixtureText, std::regex(R"(xfout_lovel=)")) == 4
                    && countMatches(monoNoXfadeText, std::regex(R"(xfin_lovel=|xfout_lovel=)")) == 0,
                "Mono RR crossfade and no-crossfade baselines changed unexpectedly.");
        require(countMatches(stereoFixtureText, std::regex(R"(xfin_lovel=)")) == 4
                    && countMatches(stereoFixtureText, std::regex(R"(xfout_lovel=)")) == 4
                    && countMatches(stereoNoXfadeText, std::regex(R"(xfin_lovel=|xfout_lovel=)")) == 0,
                "Stereo RR crossfade and no-crossfade baselines changed unexpectedly.");

        require(monoFixtureText.find("A_029__F1_5_rr1.flac") != std::string::npos,
                "The primary mono RR fixture should remain rooted in the generated mono sample set.");
        require(stereoFixtureText.find("A_029__F1_5-ST_rr1.flac") != std::string::npos,
                "The primary stereo RR fixture should remain rooted in the generated stereo sample set.");
        require(svFixtureText.find("A_029__F1_5-SV_rr1.flac") != std::string::npos,
                "The stereo-vibrato RR fixture should remain rooted in the generated SV sample set.");

        std::cout << "Phase 3 Round Robin Sprint 1 fixture characterization tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 3 Round Robin Sprint 1 fixture characterization tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
