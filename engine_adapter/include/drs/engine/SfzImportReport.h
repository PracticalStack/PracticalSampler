#pragma once

#include "drs/engine/SfzImport.h"

#include <cstddef>
#include <string>
#include <vector>

namespace drs::engine
{
enum class SfzImportSemanticDependencyKind : std::uint8_t
{
    none = 0,
    controllerRange,
    controllerTriggerRange,
    sustainPedalState,
    triggerEvent,
    randomPolicy,
    switchCondition,
    controllerDefault,
    controllerModulation,
    presentationMetadata
};

enum class SfzImportSemanticImpact : std::uint8_t
{
    none = 0,
    presentationOnly,
    soundCritical
};

enum class SfzImportSemanticSupport : std::uint8_t
{
    native = 0,
    partial,
    unsupported
};

struct SfzImportSemanticDependency
{
    SfzImportSemanticDependencyKind kind = SfzImportSemanticDependencyKind::none;
    SfzImportSemanticImpact impact = SfzImportSemanticImpact::none;
    SfzImportSemanticSupport support = SfzImportSemanticSupport::native;
    bool affectsRegionEligibility = false;
    int controllerNumber = -1;
    std::string opcodeName;
    std::string opcodeValue;
    bool inherited = false;
    SfzImportSourceLocation location;
};

struct SfzImportRegionSemanticAnalysis
{
    std::size_t documentOrder = 0;
    std::string sampleReference;
    std::vector<SfzImportSemanticDependency> dependencies;
    bool hasSoundCriticalDependencies = false;
    bool hasIncompleteSoundCriticalDependencies = false;
    bool safeToProjectUnconditionally = true;
};

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
    SfzImportSemanticDependencyKind semanticDependencyKind
        = SfzImportSemanticDependencyKind::none;
    SfzImportSemanticImpact semanticImpact = SfzImportSemanticImpact::none;
    SfzImportSemanticSupport semanticSupport = SfzImportSemanticSupport::native;
    bool affectsRegionEligibility = false;
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

// Stable, UI-ready report buckets.  The detailed trace remains the source of
// truth; these summaries let an importer present a useful review surface
// without making the editor understand SFZ opcode taxonomy.
struct SfzImportReportSection
{
    std::string name;
    std::size_t itemCount = 0;
    std::vector<std::string> entries;
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
    std::size_t suppressedFindingCount = 0;
    std::size_t semanticAnalyzedRegionCount = 0;
    std::size_t semanticDependencyCount = 0;
    std::size_t soundCriticalDependencyCount = 0;
    std::size_t incompleteSoundCriticalDependencyCount = 0;
    std::size_t presentationOnlyDependencyCount = 0;
    std::size_t unsafeUnconditionalRegionCount = 0;
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
    std::vector<SfzImportReportSection> sections;
    std::vector<SfzImportRegionSemanticAnalysis> regionSemanticAnalysis;
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
