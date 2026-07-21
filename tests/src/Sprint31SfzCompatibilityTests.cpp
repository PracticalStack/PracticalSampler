#include "drs/engine/SfzImportReport.h"

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
        require(analysis.report.summary.convertedOpcodeCount == 1583,
                "The first SFZ fixture converted-opcode count changed unexpectedly.");
        require(analysis.report.summary.approximatedOpcodeCount == 16,
                "The first SFZ fixture approximated-opcode count changed unexpectedly.");
        require(analysis.report.summary.reportedOnlyOpcodeCount == 9,
                "The first SFZ fixture reported-only opcode count changed unexpectedly.");
        require(analysis.report.summary.blockingOpcodeCount == 0,
                "The first SFZ fixture should not currently report blocking opcode counts.");
        require(analysis.report.summary.warningFindingCount == 25
                    && analysis.report.summary.errorFindingCount == 0,
                "The first SFZ fixture finding severity counts changed unexpectedly.");

        require(analysis.report.traceEntries.size() == analysis.report.summary.opcodeCount,
                "The SFZ trace map should currently contain one entry per local opcode.");
        require(analysis.report.findings.size() == 25,
                "The first SFZ fixture finding count changed unexpectedly.");

        require(countFindingsWithCode(analysis.report, "sfz.velocity_crossfade.approximated") == 16,
                "Velocity crossfade warnings should currently land on every crossfade opcode occurrence.");
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
                    && crossfadeSummary->disposition == SfzImportSupportDisposition::approximated
                    && crossfadeSummary->occurrenceCount == 4,
                "The support matrix should classify group xfin_lovel as a repeated lossy crossfade mapping.");
        const auto* seqLengthSummary = findSupportSummary(analysis.report, SfzOpcodeScope::region, "seq_length");
        require(seqLengthSummary != nullptr
                    && seqLengthSummary->disposition == SfzImportSupportDisposition::converted
                    && seqLengthSummary->occurrenceCount == 225,
                "The support matrix should keep round-robin sequence length as a converted region feature.");
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
