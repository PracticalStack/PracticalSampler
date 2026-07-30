#pragma once

#include "drs/engine/AuthoringSession.h"
#include "drs/engine/SfzImportReport.h"

#include <string>
#include <vector>

namespace drs::engine
{
struct SfzImportProjectionResult
{
    bool projected = false;
    bool playable = false;
    bool lossy = false;
    bool blocking = false;
    std::string state;
    std::vector<std::string> issues;
    std::vector<RuntimeProjectSampleSource> sampleSources;
    std::vector<RuntimeProjectZoneDefinition> zones;
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
