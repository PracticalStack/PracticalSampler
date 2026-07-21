#pragma once

#include "drs/engine/SfzImportReport.h"

#include <cstddef>
#include <string>

namespace drs::app
{
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
        model.guidance = "This SFZ uses lossy or report-first features that must be acknowledged before final import.";
    }
    else
    {
        model.headline = "SFZ import ready";
        model.guidance = "No blocking or lossy findings were detected.";
    }

    return model;
}
} // namespace drs::app
