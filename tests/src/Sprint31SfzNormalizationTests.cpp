#include "drs/engine/SfzImport.h"

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

fs::path getScratchDirectory()
{
    auto path = fs::temp_directory_path() / "drs-sprint31-sfz-normalization-tests";
    fs::create_directories(path);
    return path;
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    std::ofstream output(path, std::ios::binary);
    require(output.good(), "Could not open fixture file for writing: " + path.generic_string());
    output << text;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto scratchDirectory = getScratchDirectory();
        const auto sharedPath = scratchDirectory / "shared.sfz";
        const auto mainPath = scratchDirectory / "main.sfz";

        writeTextFile(sharedPath,
                      "<control> label_cc1=Mod:width\n"
                      "<master> volume=6 width_oncc1=100\n");
        writeTextFile(mainPath,
                      "#include \"shared.sfz\"\n"
                      "<group> lovel=1 hivel=63\n"
                      "<region> sample=A.wav lokey=C1\n"
                      "<region> sample=B.wav lokey=D1 hivel=40\n"
                      "<group> lovel=64\n"
                      "<region> sample=C.wav lokey=E1\n");

        const auto parseResult = parseSfzDocument(mainPath.generic_string());
        require(parseResult.parsed && parseResult.complete,
                "The synthetic SFZ include fixture should parse completely.");
        require(parseResult.document.sourceFiles.size() == 2,
                "The synthetic SFZ include fixture should report both source files.");
        require(parseResult.document.sections.size() == 7,
                "The synthetic SFZ include fixture section count changed unexpectedly.");

        const auto normalizeResult = normalizeSfzDocument(parseResult.document);
        require(normalizeResult.normalized,
                "The synthetic SFZ include fixture should normalize successfully.");
        require(normalizeResult.state == "Normalized",
                "The synthetic SFZ normalization state changed unexpectedly.");
        require(normalizeResult.document.sections.size() == parseResult.document.sections.size(),
                "Normalization should preserve section count.");

        const auto& firstRegion = normalizeResult.document.sections[3];
        require(firstRegion.scope == SfzOpcodeScope::region,
                "The first synthetic normalized region moved unexpectedly.");
        require(findEffectiveOpcode(firstRegion, "label_cc1") != nullptr,
                "The first synthetic region should inherit the control opcode.");
        require(findEffectiveOpcode(firstRegion, "volume") != nullptr
                    && findEffectiveOpcode(firstRegion, "volume")->value == "6",
                "The first synthetic region should inherit the master volume.");
        require(findEffectiveOpcode(firstRegion, "width_oncc1") != nullptr
                    && findEffectiveOpcode(firstRegion, "width_oncc1")->value == "100",
                "The first synthetic region should inherit the master CC width mapping.");
        require(findEffectiveOpcode(firstRegion, "lovel") != nullptr
                    && findEffectiveOpcode(firstRegion, "lovel")->value == "1",
                "The first synthetic region should inherit the group low velocity.");
        require(findEffectiveOpcode(firstRegion, "hivel") != nullptr
                    && findEffectiveOpcode(firstRegion, "hivel")->value == "63",
                "The first synthetic region should inherit the group high velocity.");
        require(findEffectiveOpcode(firstRegion, "sample") != nullptr
                    && findEffectiveOpcode(firstRegion, "sample")->value == "A.wav",
                "The first synthetic region sample mapping changed unexpectedly.");

        const auto& secondRegion = normalizeResult.document.sections[4];
        require(findEffectiveOpcode(secondRegion, "hivel") != nullptr
                    && findEffectiveOpcode(secondRegion, "hivel")->value == "40",
                "The second synthetic region should override inherited high velocity.");
        require(findEffectiveOpcode(secondRegion, "sample") != nullptr
                    && findEffectiveOpcode(secondRegion, "sample")->value == "B.wav",
                "The second synthetic region sample mapping changed unexpectedly.");

        const auto& thirdRegion = normalizeResult.document.sections[6];
        require(findEffectiveOpcode(thirdRegion, "lovel") != nullptr
                    && findEffectiveOpcode(thirdRegion, "lovel")->value == "64",
                "The third synthetic region should inherit the second group low velocity.");
        require(findEffectiveOpcode(thirdRegion, "hivel") == nullptr,
                "A new group should reset the prior group's high-velocity value in Sprint 3.1.2 normalization.");
        require(findEffectiveOpcode(thirdRegion, "sample") != nullptr
                    && findEffectiveOpcode(thirdRegion, "sample")->value == "C.wav",
                "The third synthetic region sample mapping changed unexpectedly.");

        std::cout << "Sprint 3.1.2 SFZ normalization tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.2 SFZ normalization tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
