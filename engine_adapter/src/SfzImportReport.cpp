#include "drs/engine/SfzImportReport.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

struct OpcodeClassification
{
    SfzImportSupportDisposition disposition = SfzImportSupportDisposition::converted;
    std::string nativeTarget;
    std::string rationale;
    std::string findingCode;
    std::string findingSummary;
    std::string findingDetail;
};

struct SupportKey
{
    SfzOpcodeScope scope = SfzOpcodeScope::unknown;
    std::string opcodeName;

    bool operator<(const SupportKey& other) const noexcept
    {
        if (scope != other.scope)
            return scope < other.scope;

        return opcodeName < other.opcodeName;
    }
};

std::string toLowerAscii(const std::string& text)
{
    std::string lowered = text;
    std::transform(lowered.begin(),
                   lowered.end(),
                   lowered.begin(),
                   [](unsigned char character)
                   {
                       return static_cast<char>(std::tolower(character));
                   });
    return lowered;
}

bool isCurveValueOpcode(const std::string& opcodeName)
{
    if (opcodeName.size() < 2 || opcodeName.front() != 'v')
        return false;

    return std::all_of(opcodeName.begin() + 1,
                       opcodeName.end(),
                       [](unsigned char character)
                       {
                           return std::isdigit(character) != 0;
                       });
}

std::size_t dispositionRank(const SfzImportSupportDisposition disposition) noexcept
{
    switch (disposition)
    {
        case SfzImportSupportDisposition::converted:
            return 0;
        case SfzImportSupportDisposition::approximated:
            return 1;
        case SfzImportSupportDisposition::reportedOnly:
            return 2;
        case SfzImportSupportDisposition::blocking:
            return 3;
    }

    return 0;
}

std::size_t countParsedOpcodes(const SfzParsedDocument& document) noexcept
{
    auto count = std::size_t { 0 };
    for (const auto& section : document.sections)
        count += section.opcodes.size();

    return count;
}

std::string findEffectiveSampleReference(const SfzNormalizedSection& section)
{
    if (const auto* sample = findEffectiveOpcode(section, "sample"))
        return sample->value;

    return {};
}

void incrementDispositionCount(SfzImportReportSummary& summary,
                               const SfzImportSupportDisposition disposition) noexcept
{
    switch (disposition)
    {
        case SfzImportSupportDisposition::converted:
            ++summary.convertedOpcodeCount;
            break;
        case SfzImportSupportDisposition::approximated:
            ++summary.approximatedOpcodeCount;
            break;
        case SfzImportSupportDisposition::reportedOnly:
            ++summary.reportedOnlyOpcodeCount;
            break;
        case SfzImportSupportDisposition::blocking:
            ++summary.blockingOpcodeCount;
            break;
    }
}

void accumulateFindingSeverities(SfzImportReportSummary& summary,
                                 const std::vector<SfzImportFinding>& findings) noexcept
{
    for (const auto& finding : findings)
    {
        switch (finding.severity)
        {
            case SfzImportFindingSeverity::information:
                ++summary.informationFindingCount;
                break;
            case SfzImportFindingSeverity::warning:
                ++summary.warningFindingCount;
                break;
            case SfzImportFindingSeverity::error:
                ++summary.errorFindingCount;
                break;
        }
    }
}

void addClassificationFinding(std::vector<SfzImportFinding>& findings,
                              const OpcodeClassification& classification,
                              const SfzResolvedOpcode& opcode,
                              const std::string& sampleReference)
{
    if (classification.disposition == SfzImportSupportDisposition::converted)
        return;

    SfzImportFinding finding;
    finding.severity = classification.disposition == SfzImportSupportDisposition::blocking
        ? SfzImportFindingSeverity::error
        : SfzImportFindingSeverity::warning;
    finding.disposition = classification.disposition;
    finding.code = classification.findingCode;
    finding.summary = classification.findingSummary;
    finding.detail = classification.findingDetail;
    if (!sampleReference.empty())
        finding.detail += " Context sample: '" + sampleReference + "'.";
    finding.location = opcode.location;
    findings.push_back(std::move(finding));
}

void updateSupportSummary(std::map<SupportKey, SfzImportOpcodeSupportSummary>& summaries,
                          const SfzResolvedOpcode& opcode,
                          const OpcodeClassification& classification)
{
    const SupportKey key { opcode.location.scope, opcode.name };
    auto& summary = summaries[key];
    if (summary.occurrenceCount == 0)
    {
        summary.scope = opcode.location.scope;
        summary.opcodeName = opcode.name;
        summary.disposition = classification.disposition;
        summary.nativeTarget = classification.nativeTarget;
        summary.rationale = classification.rationale;
    }
    else if (dispositionRank(classification.disposition) > dispositionRank(summary.disposition))
    {
        summary.disposition = classification.disposition;
        summary.nativeTarget = classification.nativeTarget;
        summary.rationale = classification.rationale;
    }

    ++summary.occurrenceCount;
}

OpcodeClassification classifyOpcode(const SfzResolvedOpcode& opcode)
{
    const auto opcodeName = toLowerAscii(opcode.name);

    if (opcodeName == "sample")
    {
        const auto resolvedSamplePath =
            fs::path(opcode.location.sourcePath).parent_path() / fs::path(opcode.value);
        if (!fs::exists(resolvedSamplePath))
        {
            return { SfzImportSupportDisposition::blocking,
                     "zone.samplePath",
                     "Sample file resolution is required before any native zone can be created.",
                     "sfz.sample.missing",
                     "Referenced sample file is missing",
                     "The importer could not resolve sample '" + opcode.value + "' next to the declaring SFZ file." };
        }

        return { SfzImportSupportDisposition::converted,
                 "zone.samplePath",
                 "Relative sample references can map directly into native zone source paths." };
    }

    if (opcodeName == "lokey")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.keyRange.lowNote",
                 "Lower key bounds map directly into native zone key ranges." };
    }

    if (opcodeName == "hikey")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.keyRange.highNote",
                 "Upper key bounds map directly into native zone key ranges." };
    }

    if (opcodeName == "pitch_keycenter")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.rootKey",
                 "Pitch centers map directly into native root-key metadata." };
    }

    if (opcodeName == "lovel")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.velocityRange.lowVelocity",
                 "Lower velocity bounds map directly into native velocity ranges." };
    }

    if (opcodeName == "hivel")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.velocityRange.highVelocity",
                 "Upper velocity bounds map directly into native velocity ranges." };
    }

    if (opcodeName == "seq_length")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.roundRobin.length",
                 "Round-robin sequence length remains part of the required Phase 3.1 import contract." };
    }

    if (opcodeName == "seq_position")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.roundRobin.position",
                 "Round-robin sequence position remains part of the required Phase 3.1 import contract." };
    }

    if (opcodeName == "volume")
    {
        return { SfzImportSupportDisposition::converted,
                 "instrument.gainDb",
                 "Document-level gain can map into native authored gain metadata." };
    }

    if (opcodeName == "ampeg_release")
    {
        return { SfzImportSupportDisposition::converted,
                 "ampEnvelope.releaseSeconds",
                 "Per-zone and inherited release times can map into native envelope release controls." };
    }

    if (opcodeName == "xfin_lovel" || opcodeName == "xfin_hivel"
        || opcodeName == "xfout_lovel" || opcodeName == "xfout_hivel")
    {
        return { SfzImportSupportDisposition::approximated,
                 "zone.velocityCrossfade",
                 "Velocity-crossfade edges can be preserved, but true crossfade playback is not yet guaranteed.",
                 "sfz.velocity_crossfade.approximated",
                 "Velocity crossfade will be approximated",
                 "This SFZ uses velocity-crossfade boundaries that Phase 3.1 currently reports as a lossy import." };
    }

    if (opcodeName == "label_cc1")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.controls.cc1Label",
                 "CC labels are preserved for creator review before any final import.",
                 "sfz.cc.label.reported",
                 "CC label will be reported",
                 "The importer preserves the CC1 label for review, but does not yet apply it to a native modulation surface." };
    }

    if (opcodeName == "set_hdcc1")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.controls.cc1Default",
                 "Controller defaults are surfaced in the review report even when the native control path is not ready.",
                 "sfz.cc.default.reported",
                 "Controller default will be reported",
                 "The importer preserves the CC1 default value for review, but does not yet apply it to a native modulation surface." };
    }

    if (opcodeName == "width_oncc1")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.modulation.widthOnCc1",
                 "CC-driven width modulation is important to disclose before project mutation.",
                 "sfz.cc.width.reported",
                 "CC-driven width modulation will be reported",
                 "The importer preserves width-on-CC1 modulation for creator review, but does not yet recreate the stereo-width behavior." };
    }

    if (opcodeName == "width_curvecc1")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.modulation.widthCurveCc1",
                 "Curve-linked width modulation is surfaced for transparency until the native behavior lands.",
                 "sfz.cc.width_curve.reported",
                 "CC width curve will be reported",
                 "The importer preserves the width-control curve binding for review, but does not yet recreate the stereo-width behavior." };
    }

    if (opcodeName == "curve_index" || isCurveValueOpcode(opcodeName))
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.curves",
                 "Curve definitions are preserved for creator review until a native curve path exists.",
                 "sfz.curve.reported",
                 "Curve definition will be reported",
                 "The importer preserves SFZ curve definitions for transparency, but does not yet convert them into native modulation curves." };
    }

    return { SfzImportSupportDisposition::reportedOnly,
             {},
             "Recognized SFZ opcodes remain visible in the report even when native conversion is not implemented yet.",
             "sfz.opcode.unmapped",
             "Opcode will be reported instead of converted",
             "The importer recognizes opcode '" + opcode.name + "' but does not yet have a native conversion target for it." };
}
} // namespace

SfzImportAnalysisResult analyzeSfzImportDocument(const std::string& sfzPath)
{
    SfzImportAnalysisResult result;
    result.analyzed = true;
    result.parseResult = parseSfzDocument(sfzPath);

    result.report.available = true;
    result.report.rootDocumentPath = result.parseResult.document.rootDocumentPath;
    result.report.sourceFiles = result.parseResult.document.sourceFiles;
    result.report.summary.sourceFileCount = result.report.sourceFiles.size();
    result.report.summary.sectionCount = result.parseResult.document.sections.size();
    result.report.summary.opcodeCount = countParsedOpcodes(result.parseResult.document);
    result.report.findings = result.parseResult.findings;

    if (result.parseResult.parsed)
    {
        result.normalizeResult = normalizeSfzDocument(result.parseResult.document);
        result.report.findings.insert(result.report.findings.end(),
                                      result.normalizeResult.findings.begin(),
                                      result.normalizeResult.findings.end());

        if (result.normalizeResult.normalized)
        {
            result.report.rootDocumentPath = result.normalizeResult.document.rootDocumentPath;
            result.report.sourceFiles = result.normalizeResult.document.sourceFiles;
            result.report.summary.sourceFileCount = result.report.sourceFiles.size();
            result.report.summary.sectionCount = result.normalizeResult.document.sections.size();
            result.report.summary.opcodeCount = 0;

            std::map<SupportKey, SfzImportOpcodeSupportSummary> supportSummaries;

            for (const auto& section : result.normalizeResult.document.sections)
            {
                const auto sampleReference = findEffectiveSampleReference(section);
                result.report.summary.opcodeCount += section.localOpcodes.size();

                for (const auto& opcode : section.localOpcodes)
                {
                    const auto classification = classifyOpcode(opcode);
                    incrementDispositionCount(result.report.summary, classification.disposition);
                    addClassificationFinding(result.report.findings,
                                             classification,
                                             opcode,
                                             sampleReference);
                    updateSupportSummary(supportSummaries, opcode, classification);

                    result.report.traceEntries.push_back(
                        { section.documentOrder,
                          section.scope,
                          section.headerName,
                          opcode.name,
                          opcode.value,
                          classification.nativeTarget,
                          sampleReference,
                          classification.disposition,
                          classification.findingCode,
                          opcode.location });
                }
            }

            result.report.opcodeSupport.reserve(supportSummaries.size());
            for (const auto& [_, summary] : supportSummaries)
                result.report.opcodeSupport.push_back(summary);
        }
    }

    result.report.reviewDisposition = sfzImportReviewDispositionFor(result.report.findings);
    result.report.blocking = result.report.reviewDisposition == SfzImportReviewDisposition::blocked;
    result.report.stage = result.report.blocking ? SfzImportStage::blocked
                                                 : SfzImportStage::reviewReady;
    result.report.state = result.report.blocking ? "Blocked" : "Review Ready";

    accumulateFindingSeverities(result.report.summary, result.report.findings);
    return result;
}
} // namespace drs::engine
