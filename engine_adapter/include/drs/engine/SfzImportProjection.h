#pragma once

#include "drs/engine/AuthoringSession.h"
#include "drs/engine/SfzImportReport.h"

#include <cstddef>
#include <string>
#include <vector>

namespace drs::engine
{
struct SfzImportOmittedRegionSummary
{
    SfzImportSemanticDependencyKind dependencyKind
        = SfzImportSemanticDependencyKind::none;
    int controllerNumber = -1;
    SfzOpcodeScope sourceScope = SfzOpcodeScope::unknown;
    std::string sourcePath;
    std::size_t firstSourceLineNumber = 0;
    std::string feature;
    std::size_t affectedRegionCount = 0;
};

struct SfzImportProjectionResult
{
    bool projected = false;
    bool playable = false;
    bool lossy = false;
    bool blocking = false;
    std::size_t semanticAnalyzedRegionCount = 0;
    std::size_t unsafeUnconditionalRegionCount = 0;
    std::vector<std::size_t> unsafeUnconditionalRegionDocumentOrders;
    std::size_t omittedUnsafeRegionCount = 0;
    std::vector<SfzImportOmittedRegionSummary> omittedRegionSummaries;
    std::string state;
    std::vector<std::string> issues;
    std::vector<RuntimeProjectSampleSource> sampleSources;
    double masterGainDb = 0.0;
    std::vector<RuntimeProjectGroupDefinition> groups;
    std::vector<RuntimeProjectZoneDefinition> zones;
    std::vector<RuntimeControllerDefault> controllerDefaults;
    std::vector<RuntimeProjectInstrumentControlDefinition> instrumentControls;
    std::vector<RuntimeProjectInstrumentControlTargetDefinition> instrumentControlTargets;
    std::vector<RuntimeProjectMidiControlBindingDefinition> midiControlBindings;
    std::vector<std::string> projectNotes;
    std::vector<std::string> authoringNotes;
    SfzImportExecutionState execution;
};

SfzImportProjectionResult projectSfzImportAnalysis(const RuntimeProjectModel& baseProject,
                                                   const SfzImportAnalysisResult& analysis);
SfzImportProjectionResult projectSfzImportAnalysis(const RuntimeProjectModel& baseProject,
                                                   const SfzImportAnalysisResult& analysis,
                                                   const SfzImportExecutionContext& context);
SfzImportProjectionResult projectSfzImportDocument(const RuntimeProjectModel& baseProject,
                                                   const std::string& sfzPath);
SfzImportProjectionResult projectSfzImportDocument(const RuntimeProjectModel& baseProject,
                                                   const std::string& sfzPath,
                                                   const SfzImportExecutionContext& context);
RuntimeProjectDocumentActionResult applySfzImportProjection(AuthoringSession& authoringSession,
                                                            SfzImportProjectionResult projection,
                                                            const std::string& label);
} // namespace drs::engine
