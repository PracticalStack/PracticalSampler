#include "drs/engine/SfzImport.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
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
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto fixturePath = resolveFirstFixturePath();
        const auto result = parseSfzDocument(fixturePath.generic_string());

        require(result.parsed, "The first SFZ fixture should parse without syntax failure.");
        require(result.complete, "The first SFZ fixture should parse completely in Sprint 3.1.2.");
        require(result.state == "Parsed", "The first SFZ fixture parse state changed unexpectedly.");
        require(result.document.sourceFiles.size() == 1,
                "The first SFZ fixture should currently resolve to exactly one source file.");
        require(result.document.sections.size() == 233,
                "The first SFZ fixture section count changed unexpectedly.");

        require(std::count_if(result.document.sections.begin(),
                              result.document.sections.end(),
                              [](const SfzParsedSection& section)
                              {
                                  return section.scope == SfzOpcodeScope::control;
                              }) == SfzFirstFixtureCharacterization::expectedControlHeaderCount,
                "The first SFZ fixture control-section count changed unexpectedly.");
        require(std::count_if(result.document.sections.begin(),
                              result.document.sections.end(),
                              [](const SfzParsedSection& section)
                              {
                                  return section.scope == SfzOpcodeScope::master;
                              }) == SfzFirstFixtureCharacterization::expectedMasterHeaderCount,
                "The first SFZ fixture master-section count changed unexpectedly.");
        require(std::count_if(result.document.sections.begin(),
                              result.document.sections.end(),
                              [](const SfzParsedSection& section)
                              {
                                  return section.scope == SfzOpcodeScope::group;
                              }) == SfzFirstFixtureCharacterization::expectedGroupHeaderCount,
                "The first SFZ fixture group-section count changed unexpectedly.");
        require(std::count_if(result.document.sections.begin(),
                              result.document.sections.end(),
                              [](const SfzParsedSection& section)
                              {
                                  return section.scope == SfzOpcodeScope::region;
                              }) == SfzFirstFixtureCharacterization::expectedRegionCount,
                "The first SFZ fixture region-section count changed unexpectedly.");
        require(std::count_if(result.document.sections.begin(),
                              result.document.sections.end(),
                              [](const SfzParsedSection& section)
                              {
                                  return section.scope == SfzOpcodeScope::curve;
                              }) == SfzFirstFixtureCharacterization::expectedCurveHeaderCount,
                "The first SFZ fixture curve-section count changed unexpectedly.");

        require(!result.document.sections.empty(), "The first SFZ fixture should contain parsed sections.");
        const auto& controlSection = result.document.sections.front();
        require(controlSection.scope == SfzOpcodeScope::control,
                "The first parsed section should remain the control header.");
        require(controlSection.opcodes.size() == 2,
                "The first control section opcode count changed unexpectedly.");
        require(controlSection.opcodes[0].name == "label_cc1"
                    && controlSection.opcodes[0].value == "Mod:width",
                "The first control opcode changed unexpectedly.");
        require(controlSection.opcodes[1].name == "set_hdcc1"
                    && controlSection.opcodes[1].value == "0.5",
                "The second control opcode changed unexpectedly.");

        const auto firstRegionIterator = std::find_if(result.document.sections.begin(),
                                                      result.document.sections.end(),
                                                      [](const SfzParsedSection& section)
                                                      {
                                                          return section.scope == SfzOpcodeScope::region;
                                                      });
        require(firstRegionIterator != result.document.sections.end(),
                "The first SFZ fixture should still contain region sections.");
        require(firstRegionIterator->opcodes.size() >= 6,
                "The first region should keep its expected opcode density.");
        require(firstRegionIterator->opcodes.front().name == "sample"
                    && firstRegionIterator->opcodes.front().value == "A_029__F1_5_rr1.flac",
                "The first parsed region sample changed unexpectedly.");

        const auto curveIterator = std::find_if(result.document.sections.begin(),
                                                result.document.sections.end(),
                                                [](const SfzParsedSection& section)
                                                {
                                                    return section.scope == SfzOpcodeScope::curve;
                                                });
        require(curveIterator != result.document.sections.end(),
                "The first SFZ fixture should still contain one curve section.");
        require(curveIterator->opcodes.size() == 5,
                "The first SFZ fixture curve section opcode count changed unexpectedly.");
        require(curveIterator->opcodes[0].name == "curve_index"
                    && curveIterator->opcodes[0].value == "99",
                "The first curve opcode changed unexpectedly.");
        require(curveIterator->opcodes.back().name == "v127"
                    && curveIterator->opcodes.back().value == "2",
                "The final curve opcode changed unexpectedly.");

        require(std::none_of(result.findings.begin(),
                             result.findings.end(),
                             [](const SfzImportFinding& finding)
                             {
                                 return finding.severity == SfzImportFindingSeverity::error;
                             }),
                "The first SFZ fixture should not emit parser errors in Sprint 3.1.2.");

        std::cout << "Sprint 3.1.2 SFZ parser tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.2 SFZ parser tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
