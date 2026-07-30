#pragma once

#include "drs/engine/SfzImport.h"

#include <cstddef>
#include <string>
#include <vector>

namespace drs::engine
{
struct SfzImportTraceEntry
{
    std::size_t documentOrder = 0;
    SfzOpcodeScope scope = SfzOpcodeScope::unknown;
    std::string headerName;
    std::string opcodeName;
    std::string opcodeValue;
    std::string nativeTarget;
    std::string sampleReference;
    SfzImportSupportDisposition disposition = SfzImportSupportDisposition::converted;
    std::string findingCode;
    SfzImportSourceLocation location;
};

struct SfzImportOpcodeSupportSummary
{
    SfzOpcodeScope scope = SfzOpcodeScope::unknown;
    std::string opcodeName;
    SfzImportSupportDisposition disposition = SfzImportSupportDisposition::converted;
    std::string nativeTarget;
    std::string rationale;
    std::size_t occurrenceCount = 0;
};

struct SfzImportReportSummary
{
    std::size_t sourceFileCount = 0;
    std::size_t sectionCount = 0;
    std::size_t opcodeCount = 0;
    std::size_t convertedOpcodeCount = 0;
    std::size_t approximatedOpcodeCount = 0;
    std::size_t reportedOnlyOpcodeCount = 0;
    std::size_t blockingOpcodeCount = 0;
    std::size_t informationFindingCount = 0;
    std::size_t warningFindingCount = 0;
    std::size_t errorFindingCount = 0;
};

struct SfzImportReport
{
    bool available = false;
    bool blocking = false;
    SfzImportStage stage = SfzImportStage::idle;
    SfzImportReviewDisposition reviewDisposition = SfzImportReviewDisposition::noneRequired;
    std::string state;
    std::string rootDocumentPath;
    std::vector<std::string> sourceFiles;
    SfzImportReportSummary summary;
    std::vector<SfzImportFinding> findings;
    std::vector<SfzImportTraceEntry> traceEntries;
    std::vector<SfzImportOpcodeSupportSummary> opcodeSupport;
    SfzImportExecutionState execution;
};

struct SfzImportAnalysisResult
{
    bool analyzed = false;
    SfzDocumentParseResult parseResult;
    SfzDocumentNormalizeResult normalizeResult;
    SfzImportReport report;
    SfzImportExecutionState execution;
};

SfzImportAnalysisResult analyzeSfzImportDocument(const std::string& sfzPath);
SfzImportAnalysisResult analyzeSfzImportDocument(const std::string& sfzPath,
                                                const SfzImportExecutionContext& context);
} // namespace drs::engine
