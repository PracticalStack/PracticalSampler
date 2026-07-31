#pragma once

#include "drs/engine/SfzImportReport.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace drs::app
{
inline bool reportHasVelocityCrossfadeWithDisposition(
    const drs::engine::SfzImportReport& report,
    drs::engine::SfzImportSupportDisposition disposition)
{
    return std::any_of(report.opcodeSupport.begin(),
                       report.opcodeSupport.end(),
                       [disposition](const drs::engine::SfzImportOpcodeSupportSummary& summary)
                       {
                           return (summary.opcodeName == "xfin_lovel"
                                      || summary.opcodeName == "xfin_hivel"
                                      || summary.opcodeName == "xfout_lovel"
                                      || summary.opcodeName == "xfout_hivel")
                               && summary.disposition == disposition;
                       });
}

inline bool reportHasRoundRobinWithDisposition(const drs::engine::SfzImportReport& report,
                                               drs::engine::SfzImportSupportDisposition disposition)
{
    return std::any_of(report.opcodeSupport.begin(),
                       report.opcodeSupport.end(),
                       [disposition](const drs::engine::SfzImportOpcodeSupportSummary& summary)
                       {
                           return (summary.opcodeName == "seq_length"
                                      || summary.opcodeName == "seq_position")
                               && summary.disposition == disposition;
                       });
}

struct SfzImportReportModel
{
    bool available = false;
    bool commitAllowed = false;
    bool confirmationRequired = false;
    drs::engine::SfzImportStage stage = drs::engine::SfzImportStage::idle;
    drs::engine::SfzImportReviewDisposition reviewDisposition
        = drs::engine::SfzImportReviewDisposition::noneRequired;
    std::string documentPath;
    std::string headline;
    std::string guidance;
    std::size_t convertedCount = 0;
    std::size_t approximatedCount = 0;
    std::size_t reportedOnlyCount = 0;
    std::size_t blockingCount = 0;
    drs::engine::SfzImportReport report;
};

inline SfzImportReportModel makeSfzImportReportModel(
    const drs::engine::SfzImportAnalysisResult& analysis)
{
    SfzImportReportModel model;
    model.available = analysis.report.available;
    model.stage = analysis.report.stage;
    model.reviewDisposition = analysis.report.reviewDisposition;
    model.commitAllowed = model.reviewDisposition != drs::engine::SfzImportReviewDisposition::blocked;
    model.confirmationRequired
        = model.reviewDisposition == drs::engine::SfzImportReviewDisposition::confirmationRequired;
    model.documentPath = analysis.report.rootDocumentPath;
    model.convertedCount = analysis.report.summary.convertedOpcodeCount;
    model.approximatedCount = analysis.report.summary.approximatedOpcodeCount;
    model.reportedOnlyCount = analysis.report.summary.reportedOnlyOpcodeCount;
    model.blockingCount = analysis.report.summary.blockingOpcodeCount;
    model.report = analysis.report;
    const auto& report = analysis.report;

    if (!model.available)
    {
        model.headline = "No SFZ import report";
        model.guidance = "Analyze an SFZ document to build a compatibility report.";
        return model;
    }

    if (model.reviewDisposition == drs::engine::SfzImportReviewDisposition::blocked)
    {
        model.headline = "SFZ import blocked";
        model.guidance = "Resolve blocking findings before the project can be mutated.";
    }
    else if (model.confirmationRequired)
    {
        model.headline = "Review SFZ import";
        if (reportHasVelocityCrossfadeWithDisposition(report, drs::engine::SfzImportSupportDisposition::approximated))
        {
            model.guidance =
                "Some velocity crossfades still fall outside the supported linear-adjacent contract, or other review-only features remain. Confirm the findings before final import.";
        }
        else
        {
            const auto preservesCrossfades = reportHasVelocityCrossfadeWithDisposition(
                report,
                drs::engine::SfzImportSupportDisposition::converted);
            const auto preservesRoundRobin = reportHasRoundRobinWithDisposition(
                report,
                drs::engine::SfzImportSupportDisposition::converted);

            if (preservesCrossfades && preservesRoundRobin)
            {
                model.guidance =
                    "Supported linear velocity crossfades will be preserved, and supported sequential round robins will be grouped into native Round Robin pools. Review-only findings still need acknowledgement before final import.";
            }
            else if (preservesCrossfades)
            {
                model.guidance =
                    "Supported linear velocity crossfades will be preserved and play back natively. Review-only findings still need acknowledgement before final import.";
            }
            else if (preservesRoundRobin)
            {
                model.guidance =
                    "Supported sequential round robins will be preserved and grouped into native Round Robin pools. Review-only findings still need acknowledgement before final import.";
            }
            else
            {
                model.guidance =
                    "This SFZ uses lossy or report-first features that must be acknowledged before final import.";
            }
        }
    }
    else
    {
        model.headline = "SFZ import ready";
        model.guidance = reportHasVelocityCrossfadeWithDisposition(
                             report,
                             drs::engine::SfzImportSupportDisposition::converted)
            ? "No blocking or lossy findings were detected. Supported linear velocity crossfades will be preserved."
            : reportHasRoundRobinWithDisposition(
                  report,
                  drs::engine::SfzImportSupportDisposition::converted)
            ? "No blocking or lossy findings were detected. Supported sequential round robins will be preserved."
            : "No blocking or lossy findings were detected.";
    }

    return model;
}
} // namespace drs::app
