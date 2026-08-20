#include "drs/engine/SfzRegionContract.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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
    return fs::path(DRS_SOURCE_ROOT) / "tests" / "fixtures" / "sfz-region-contract"
        / "loop-prestart-baseline.sfz";
}

drs::engine::SfzNormalizedSection makeRegion(
    const std::vector<std::pair<std::string, std::string>>& values)
{
    drs::engine::SfzNormalizedSection section;
    section.scope = drs::engine::SfzOpcodeScope::region;
    section.headerName = "region";
    std::size_t column = 1;
    for (const auto& [name, value] : values)
    {
        drs::engine::SfzResolvedOpcode opcode;
        opcode.name = name;
        opcode.value = value;
        opcode.location = { "synthetic-loop-prestart.sfz",
                            1,
                            column++,
                            drs::engine::SfzOpcodeScope::region,
                            name };
        section.localOpcodes.push_back(opcode);
        section.effectiveOpcodes.push_back(std::move(opcode));
    }
    section.localOpcodeCount = section.localOpcodes.size();
    return section;
}

const drs::engine::SfzNormalizedSection& regionAt(
    const drs::engine::SfzNormalizedDocument& document,
    const std::size_t regionIndex)
{
    std::size_t index = 0;
    for (const auto& section : document.sections)
    {
        if (section.scope != drs::engine::SfzOpcodeScope::region)
            continue;
        if (index++ == regionIndex)
            return section;
    }
    throw std::runtime_error("The requested loop pre-start baseline region does not exist.");
}

bool hasFindingCode(const drs::engine::SfzRegionResolutionResult& result,
                   const std::string& code)
{
    for (const auto& finding : result.findings)
    {
        if (finding.code == code)
            return true;
    }
    return false;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto path = fixturePath();
        require(fs::exists(path),
                "The Sprint 0 loop pre-start fixture must be checked in.");

        const auto parsed = parseSfzDocument(path.generic_string());
        require(parsed.parsed && parsed.complete,
                "The Sprint 0 loop pre-start fixture must parse completely.");
        const auto normalized = normalizeSfzDocument(parsed.document);
        require(normalized.normalized,
                "The Sprint 0 loop pre-start fixture must normalize completely.");

        const auto earlierLoop = resolveSfzRegionContract(
            regionAt(normalized.document, 0),
            { std::uint64_t { 435175 }, std::nullopt });
        require(earlierLoop.region.playbackStart.frame == 30767
                    && earlierLoop.region.loopStart.frame == 30549
                    && earlierLoop.region.loopEndExclusive.frame == 435175
                    && earlierLoop.region.loopMode.mode == SfzRegionLoopMode::loopContinuous,
                "The baseline fixture must resolve the authored offset, loop points, and inherited continuous mode.");
        require(earlierLoop.region.loopStart.frame < earlierLoop.region.playbackStart.frame
                    && earlierLoop.region.playbackStart.frame < earlierLoop.region.loopEndExclusive.frame,
                "The baseline fixture must exercise a loop head before the playback start while remaining enterable.");
        require(earlierLoop.valid
                    && earlierLoop.disposition == SfzRegionMappingDisposition::normalized
                    && !hasFindingCode(earlierLoop, "sfz.region.loop_range.outside_playback"),
                "Phase 1 must accept a loop head before playback start when the loop is enterable.");

        const auto ordinaryLoop = resolveSfzRegionContract(
            regionAt(normalized.document, 1),
            { std::uint64_t { 1000 }, std::nullopt });
        require(ordinaryLoop.valid
                    && ordinaryLoop.region.playbackStart.frame == 120
                    && ordinaryLoop.region.loopStart.frame == 240
                    && ordinaryLoop.region.loopEndExclusive.frame == 800,
                "An ordinary loop fully inside playback must remain valid in the baseline.");

        const auto missingBoundaries = resolveSfzRegionContract(
            makeRegion({ { "loop_mode", "loop_continuous" } }));
        require(missingBoundaries.valid
                    && missingBoundaries.disposition == SfzRegionMappingDisposition::unsupported
                    && !missingBoundaries.region.hasResolvedLoopRange(),
                "A loop without resolvable boundaries must remain reported as unsupported, not invalid.");

        const auto collapsedLoop = resolveSfzRegionContract(
            makeRegion({ { "offset", "40" },
                         { "end", "120" },
                         { "loop_mode", "loop_continuous" },
                         { "loop_start", "80" },
                         { "loop_end", "79" } }),
            { std::uint64_t { 121 }, std::nullopt });
        require(!collapsedLoop.valid
                    && hasFindingCode(collapsedLoop, "sfz.region.loop_range.invalid"),
                "A collapsed loop must remain a blocking invalid range.");

        const auto loopEndsBeforePlayback = resolveSfzRegionContract(
            makeRegion({ { "offset", "100" },
                         { "end", "200" },
                         { "loop_mode", "loop_continuous" },
                         { "loop_start", "20" },
                         { "loop_end", "99" } }),
            { std::uint64_t { 201 }, std::nullopt });
        require(!loopEndsBeforePlayback.valid
                    && hasFindingCode(loopEndsBeforePlayback, "sfz.region.loop_range.outside_playback"),
                "A loop that ends before playback starts must remain a blocking non-enterable range.");

        const auto exactStartAndEnd = resolveSfzRegionContract(
            makeRegion({ { "offset", "100" },
                         { "end", "199" },
                         { "loop_mode", "loop_continuous" },
                         { "loop_start", "100" },
                         { "loop_end", "199" } }),
            { std::uint64_t { 200 }, std::nullopt });
        require(exactStartAndEnd.valid
                    && exactStartAndEnd.region.loopStart.frame
                        == exactStartAndEnd.region.playbackStart.frame
                    && exactStartAndEnd.region.loopEndExclusive.frame
                        == exactStartAndEnd.region.playbackEndExclusive.frame,
                "Loop start equal to playback start and loop end equal to playback end must remain valid.");

        const auto loopEndsBeyondPlayback = resolveSfzRegionContract(
            makeRegion({ { "offset", "100" },
                         { "end", "199" },
                         { "loop_mode", "loop_continuous" },
                         { "loop_start", "80" },
                         { "loop_end", "200" } }),
            { std::uint64_t { 201 }, std::nullopt });
        require(!loopEndsBeyondPlayback.valid
                    && hasFindingCode(loopEndsBeyondPlayback, "sfz.region.loop_range.outside_playback"),
                "A loop extending beyond playback end must remain a blocking out-of-range contract.");

        std::cout << "SFZ loop pre-start baseline characterization passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "SFZ loop pre-start baseline characterization failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
