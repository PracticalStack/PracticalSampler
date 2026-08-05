#include "drs/engine/SfzImportReport.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
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

const drs::engine::SfzImportOpcodeSupportSummary* findSupportSummary(
    const drs::engine::SfzImportReport& report,
    const drs::engine::SfzOpcodeScope scope,
    const std::string& opcodeName)
{
    const auto iterator = std::find_if(report.opcodeSupport.begin(),
                                       report.opcodeSupport.end(),
                                       [&](const drs::engine::SfzImportOpcodeSupportSummary& summary)
                                       {
                                           return summary.scope == scope && summary.opcodeName == opcodeName;
                                       });
    return iterator == report.opcodeSupport.end() ? nullptr : &(*iterator);
}

std::size_t countFindingsWithCode(const drs::engine::SfzImportReport& report,
                                  const std::string& code)
{
    return static_cast<std::size_t>(
        std::count_if(report.findings.begin(),
                      report.findings.end(),
                      [&](const drs::engine::SfzImportFinding& finding)
                      {
                          return finding.code == code;
                      }));
}

std::filesystem::path resolveFirstSamplePath(const std::filesystem::path& fixturePath)
{
    constexpr std::array extensions { ".flac", ".wav", ".aif", ".aiff" };
    for (const auto& entry : std::filesystem::recursive_directory_iterator(fixturePath.parent_path()))
    {
        if (!entry.is_regular_file())
            continue;

        const auto extension = entry.path().extension().generic_string();
        if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end())
            return entry.path();
    }

    throw std::runtime_error("Could not locate a sample asset next to the first SFZ fixture.");
}

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto fixturePath = resolveFirstFixturePath();
        const auto analysis = analyzeSfzImportDocument(fixturePath.generic_string());

        require(analysis.analyzed, "Sprint 3.1.3 should return an analyzed SFZ result.");
        require(analysis.parseResult.parsed && analysis.parseResult.complete,
                "The first SFZ fixture should still parse completely in Sprint 3.1.3.");
        require(analysis.normalizeResult.normalized,
                "The first SFZ fixture should normalize before compatibility classification.");
        require(analysis.report.available,
                "Sprint 3.1.3 should publish an import report for the first fixture.");
        require(analysis.report.stage == SfzImportStage::reviewReady,
                "The first SFZ fixture should currently produce a review-ready report.");
        require(analysis.report.reviewDisposition == SfzImportReviewDisposition::confirmationRequired,
                "The first SFZ fixture should require confirmation because it contains lossy or report-first features.");
        require(!analysis.report.blocking,
                "The first SFZ fixture should not be blocked when all referenced samples resolve.");
        require(analysis.report.state == "Review Ready",
                "The first SFZ fixture report state changed unexpectedly.");

        require(analysis.report.summary.sourceFileCount == 1,
                "The first SFZ fixture should still report one source file.");
        require(analysis.report.summary.sectionCount == 233,
                "The first SFZ fixture report section count changed unexpectedly.");
        require(analysis.report.summary.opcodeCount == 1608,
                "The first SFZ fixture report opcode count changed unexpectedly.");
        require(analysis.report.summary.convertedOpcodeCount == 1599,
                "The first SFZ fixture converted-opcode count changed unexpectedly.");
        require(analysis.report.summary.approximatedOpcodeCount == 0,
                "The first SFZ fixture approximated-opcode count changed unexpectedly.");
        require(analysis.report.summary.reportedOnlyOpcodeCount == 9,
                "The first SFZ fixture reported-only opcode count changed unexpectedly.");
        require(analysis.report.summary.blockingOpcodeCount == 0,
                "The first SFZ fixture should not currently report blocking opcode counts.");
        require(analysis.report.summary.warningFindingCount == 9
                    && analysis.report.summary.errorFindingCount == 0,
                "The first SFZ fixture finding severity counts changed unexpectedly.");

        require(analysis.report.traceEntries.size() == analysis.report.summary.opcodeCount,
                "The SFZ trace map should currently contain one entry per local opcode.");
        require(analysis.report.findings.size() == 9,
                "The first SFZ fixture finding count changed unexpectedly.");

        require(countFindingsWithCode(analysis.report, "sfz.velocity_crossfade.approximated") == 0,
                "Supported velocity crossfades should no longer emit approximation findings.");
        require(countFindingsWithCode(analysis.report, "sfz.cc.label.reported") == 1,
                "The first SFZ fixture should still report exactly one CC label finding.");
        require(countFindingsWithCode(analysis.report, "sfz.cc.default.reported") == 1,
                "The first SFZ fixture should still report exactly one controller-default finding.");
        require(countFindingsWithCode(analysis.report, "sfz.cc.width.reported") == 1,
                "The first SFZ fixture should still report exactly one width-on-CC finding.");
        require(countFindingsWithCode(analysis.report, "sfz.cc.width_curve.reported") == 1,
                "The first SFZ fixture should still report exactly one width-curve binding finding.");
        require(countFindingsWithCode(analysis.report, "sfz.curve.reported") == 5,
                "The first SFZ fixture should still report every curve opcode for transparency.");

        const auto* crossfadeSummary = findSupportSummary(analysis.report, SfzOpcodeScope::group, "xfin_lovel");
        require(crossfadeSummary != nullptr
                    && crossfadeSummary->disposition == SfzImportSupportDisposition::converted
                    && crossfadeSummary->occurrenceCount == 4,
                "The support matrix should classify supported group xfin_lovel as converted.");
        const auto* seqLengthSummary = findSupportSummary(analysis.report, SfzOpcodeScope::region, "seq_length");
        require(seqLengthSummary != nullptr
                    && seqLengthSummary->disposition == SfzImportSupportDisposition::converted
                    && seqLengthSummary->occurrenceCount == 225
                    && seqLengthSummary->nativeTarget == "zone.roundRobin.slotCount",
                "The support matrix should keep round-robin sequence length as a converted region feature.");
        const auto* masterVolumeSummary = findSupportSummary(analysis.report, SfzOpcodeScope::master, "volume");
        require(masterVolumeSummary != nullptr
                    && masterVolumeSummary->disposition == SfzImportSupportDisposition::converted
                    && masterVolumeSummary->occurrenceCount == 1
                    && masterVolumeSummary->nativeTarget == "authoring.masterGainDb",
                "The support matrix should classify master-scope volume as authored master gain.");
        const auto* curveSummary = findSupportSummary(analysis.report, SfzOpcodeScope::curve, "curve_index");
        require(curveSummary != nullptr
                    && curveSummary->disposition == SfzImportSupportDisposition::reportedOnly
                    && curveSummary->occurrenceCount == 1,
                "The support matrix should keep the width-control curve as a report-first feature.");

        const auto traceIterator = std::find_if(
            analysis.report.traceEntries.begin(),
            analysis.report.traceEntries.end(),
            [](const SfzImportTraceEntry& trace)
            {
                return trace.scope == SfzOpcodeScope::control && trace.opcodeName == "label_cc1";
            });
        require(traceIterator != analysis.report.traceEntries.end()
                    && traceIterator->disposition == SfzImportSupportDisposition::reportedOnly
                    && traceIterator->findingCode == "sfz.cc.label.reported",
                "The trace map should preserve the CC1 label as a report-first control opcode.");

        const auto samplePath = resolveFirstSamplePath(fixturePath).lexically_normal();
        const auto invalidFixtureDirectory =
            std::filesystem::temp_directory_path() / "drs-sprint31-sfz-crossfade-review";
        const auto invalidFixturePath = invalidFixtureDirectory / "unsupported-topology.sfz";
        writeTextFile(
            invalidFixturePath,
            "<group>\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "lovel=25\n"
            "hivel=84\n"
            "xfin_lovel=25\n"
            "xfin_hivel=60\n"
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n");
        const auto invalidAnalysis = analyzeSfzImportDocument(invalidFixturePath.generic_string());
        require(invalidAnalysis.analyzed && invalidAnalysis.report.available,
                "Unsupported crossfade topology should still produce a review report.");
        require(countFindingsWithCode(invalidAnalysis.report, "sfz.velocity_crossfade.approximated") == 2,
                "Unsupported crossfade topology should remain visible through approximation findings.");
        const auto* invalidCrossfadeSummary =
            findSupportSummary(invalidAnalysis.report, SfzOpcodeScope::group, "xfin_lovel");
        require(invalidCrossfadeSummary != nullptr
                    && invalidCrossfadeSummary->disposition == SfzImportSupportDisposition::approximated,
                "Unsupported crossfade topology should remain classified as approximated.");

        const auto scopedGainFixturePath = invalidFixtureDirectory / "scoped-gain-report.sfz";
        writeTextFile(
            scopedGainFixturePath,
            "<global>\n"
            "volume=-1\n"
            "<master>\n"
            "volume=2\n"
            "<group>\n"
            "volume=-3\n"
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "volume=4\n");
        const auto scopedGainAnalysis = analyzeSfzImportDocument(scopedGainFixturePath.generic_string());
        require(scopedGainAnalysis.analyzed && scopedGainAnalysis.report.available,
                "Scoped gain topology should still produce a compatibility report.");
        require(countFindingsWithCode(scopedGainAnalysis.report, "sfz.gain.global_volume.approximated") == 1,
                "Global SFZ volume should now surface a scoped-gain approximation finding.");
        const auto* globalVolumeSummary =
            findSupportSummary(scopedGainAnalysis.report, SfzOpcodeScope::global, "volume");
        require(globalVolumeSummary != nullptr
                    && globalVolumeSummary->disposition == SfzImportSupportDisposition::approximated
                    && globalVolumeSummary->nativeTarget == "report.gain.globalScope",
                "Global SFZ volume should remain visible as a review-time approximation.");
        const auto* scopedMasterVolumeSummary =
            findSupportSummary(scopedGainAnalysis.report, SfzOpcodeScope::master, "volume");
        require(scopedMasterVolumeSummary != nullptr
                    && scopedMasterVolumeSummary->disposition == SfzImportSupportDisposition::converted
                    && scopedMasterVolumeSummary->nativeTarget == "authoring.masterGainDb",
                "Master-scope volume should map into authored master gain.");
        const auto* scopedGroupVolumeSummary =
            findSupportSummary(scopedGainAnalysis.report, SfzOpcodeScope::group, "volume");
        require(scopedGroupVolumeSummary != nullptr
                    && scopedGroupVolumeSummary->disposition == SfzImportSupportDisposition::converted
                    && scopedGroupVolumeSummary->nativeTarget == "authoring.groups.gainDb",
                "Group-scope volume should map into authored group gain.");
        const auto* scopedRegionVolumeSummary =
            findSupportSummary(scopedGainAnalysis.report, SfzOpcodeScope::region, "volume");
        require(scopedRegionVolumeSummary != nullptr
                    && scopedRegionVolumeSummary->disposition == SfzImportSupportDisposition::converted
                    && scopedRegionVolumeSummary->nativeTarget == "authoring.zones.gainDb",
                "Region-local volume should map into authored zone gain.");

        const auto sparseFixturePath = invalidFixtureDirectory / "sparse-round-robin.sfz";
        writeTextFile(
            sparseFixturePath,
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "seq_length=3\n"
            "seq_position=1\n"
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "seq_length=3\n"
            "seq_position=3\n");
        const auto sparseAnalysis = analyzeSfzImportDocument(sparseFixturePath.generic_string());
        require(countFindingsWithCode(sparseAnalysis.report, "sfz.round_robin.sparse_slots.reported") >= 1,
                "Sparse sequential round-robin pools should surface a typed review finding.");
        const auto* sparseSummary = findSupportSummary(sparseAnalysis.report, SfzOpcodeScope::region, "seq_length");
        require(sparseSummary != nullptr
                    && sparseSummary->disposition == SfzImportSupportDisposition::reportedOnly,
                "Sparse sequential round-robin pools should keep seq_length in the review-only bucket.");

        const auto mixedLengthFixturePath = invalidFixtureDirectory / "mixed-round-robin-lengths.sfz";
        writeTextFile(
            mixedLengthFixturePath,
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "seq_length=2\n"
            "seq_position=1\n"
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "seq_length=3\n"
            "seq_position=2\n");
        const auto mixedLengthAnalysis = analyzeSfzImportDocument(mixedLengthFixturePath.generic_string());
        require(countFindingsWithCode(mixedLengthAnalysis.report, "sfz.round_robin.mixed_lengths.reported") >= 1,
                "Mixed sequential round-robin lengths should surface a typed review finding.");

        const auto conflictingFixturePath = invalidFixtureDirectory / "conflicting-round-robin-slots.sfz";
        writeTextFile(
            conflictingFixturePath,
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "seq_length=2\n"
            "seq_position=1\n"
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "seq_length=2\n"
            "seq_position=1\n");
        const auto conflictingAnalysis = analyzeSfzImportDocument(conflictingFixturePath.generic_string());
        require(countFindingsWithCode(conflictingAnalysis.report, "sfz.round_robin.conflicting_group.reported") >= 1,
                "Conflicting sequential round-robin slot assignments should surface a typed review finding.");

        const auto randomPolicyFixturePath = invalidFixtureDirectory / "random-round-robin-policy.sfz";
        writeTextFile(
            randomPolicyFixturePath,
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "lorand=0\n"
            "hirand=0.5\n");
        const auto randomPolicyAnalysis = analyzeSfzImportDocument(randomPolicyFixturePath.generic_string());
        const auto* lorandSummary = findSupportSummary(randomPolicyAnalysis.report, SfzOpcodeScope::region, "lorand");
        require(lorandSummary != nullptr
                    && lorandSummary->disposition == SfzImportSupportDisposition::reportedOnly,
                "Unsupported random round-robin policy opcodes should stay review-only.");
        require(countFindingsWithCode(randomPolicyAnalysis.report, "sfz.round_robin.random_policy.reported") >= 1,
                "Unsupported random round-robin policy opcodes should surface a typed review finding.");

        std::cout << "Sprint 3.1.3 SFZ compatibility tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.3 SFZ compatibility tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
