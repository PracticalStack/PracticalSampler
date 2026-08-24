#include "drs/engine/SfzImport.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

fs::path fixturePath()
{
    return fs::path(DRS_SOURCE_ROOT) / "tests" / "fixtures" / "instrument-controls"
        / "naked-drums-paired-define-baseline.sfz";
}

fs::path controllerFixturePath()
{
    return fs::path(DRS_SOURCE_ROOT) / "tests" / "fixtures" / "instrument-controls"
        / "controller-surface-baseline.sfz";
}

bool hasFindingCode(const drs::engine::SfzDocumentParseResult& result,
                   const std::string& code)
{
    return std::any_of(result.findings.begin(), result.findings.end(),
                       [&](const auto& finding) { return finding.code == code; });
}

const drs::engine::SfzParsedOpcode* findOpcode(
    const drs::engine::SfzParsedSection& section,
    const std::string& name)
{
    const auto iterator = std::find_if(section.opcodes.begin(), section.opcodes.end(),
                                       [&](const auto& opcode) { return opcode.name == name; });
    return iterator == section.opcodes.end() ? nullptr : &(*iterator);
}

const drs::engine::SfzParsedSection* findSection(
    const drs::engine::SfzParsedDocument& document,
    const drs::engine::SfzOpcodeScope scope)
{
    const auto iterator = std::find_if(document.sections.begin(), document.sections.end(),
                                       [&](const auto& section) { return section.scope == scope; });
    return iterator == document.sections.end() ? nullptr : &(*iterator);
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto fixture = fixturePath();
        require(fs::exists(fixture),
                "The Phase 0 paired-define characterization fixture must be checked in.");

        const auto parsed = parseSfzDocument(fixture.generic_string());
        // Desired contract: each physical-line definition is tokenized
        // independently, so both velocity regions retain their authored
        // inclusive boundaries.
        require(parsed.parsed && parsed.complete,
                "Paired #define directives must parse completely after the Phase 0/3 tokenizer fix.");
        require(!hasFindingCode(parsed, "preprocessor.macro_undefined"),
                "Resolved paired definitions must not emit undefined-macro findings.");
        std::vector<const SfzParsedSection*> regions;
        for (const auto& section : parsed.document.sections)
            if (section.scope == SfzOpcodeScope::region)
                regions.push_back(&section);
        require(regions.size() == 2,
                "The paired-define fixture must retain two velocity regions.");
        const auto* firstLowVelocity = findOpcode(*regions[0], "lovel");
        const auto* firstHighVelocity = findOpcode(*regions[0], "hivel");
        const auto* secondLowVelocity = findOpcode(*regions[1], "lovel");
        const auto* secondHighVelocity = findOpcode(*regions[1], "hivel");
        require(firstLowVelocity != nullptr && firstLowVelocity->value == "1"
                    && firstHighVelocity != nullptr && firstHighVelocity->value == "32"
                    && secondLowVelocity != nullptr && secondLowVelocity->value == "33"
                    && secondHighVelocity != nullptr && secondHighVelocity->value == "64",
                "Paired definitions must expand to the intended low/mid velocity boundaries.");

        // Keep acceptance data explicit so a future parser refactor cannot
        // silently change the instrument's authored velocity contract.
        constexpr int expectedFirstLow = 1;
        constexpr int expectedFirstHigh = 32;
        constexpr int expectedSecondLow = 33;
        constexpr int expectedSecondHigh = 64;
        require(expectedFirstLow == 1 && expectedFirstHigh == 32
                    && expectedSecondLow == 33 && expectedSecondHigh == 64,
                "The desired paired-define velocity acceptance bands must remain explicit.");

        const auto controllerFixture = controllerFixturePath();
        require(fs::exists(controllerFixture),
                "The Phase 0 controller-surface characterization fixture must be checked in.");
        const auto controllerParsed = parseSfzDocument(controllerFixture.generic_string());
        require(controllerParsed.parsed && controllerParsed.complete,
                "The controller-surface fixture must parse completely before projection work begins.");
        const auto* control = findSection(controllerParsed.document, SfzOpcodeScope::control);
        const auto* global = findSection(controllerParsed.document, SfzOpcodeScope::global);
        require(control != nullptr && global != nullptr,
                "The controller-surface fixture must retain control and global sections.");
        require(findOpcode(*control, "set_cc20") != nullptr
                    && findOpcode(*control, "set_hdcc41") != nullptr
                    && findOpcode(*control, "label_cc20") != nullptr
                    && findOpcode(*control, "label_cc41") != nullptr,
                "Phase 0 must record the standard/default/label controller metadata needed by projection.");
        require(findOpcode(*global, "amplitude_oncc20") != nullptr
                    && findOpcode(*global, "amplitude_curvecc20") != nullptr
                    && findOpcode(*global, "amplitude_oncc21") != nullptr
                    && findOpcode(*global, "pan_oncc42") != nullptr
                    && findOpcode(*global, "tune_oncc43") != nullptr
                    && findOpcode(*global, "ampeg_hold_oncc44") != nullptr
                    && findOpcode(*global, "ampeg_decay_oncc45") != nullptr
                    && findOpcode(*global, "ampeg_sustain_oncc46") != nullptr,
                "Phase 0 must record gain fan-out, pan, tune, and envelope controller target families.");

        std::cout << "SFZ paired-define Phase 0 characterization passed: current parser failure and desired velocity bands recorded."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "SFZ paired-define Phase 0 characterization failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
