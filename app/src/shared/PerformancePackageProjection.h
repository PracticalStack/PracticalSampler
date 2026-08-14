#pragma once

#include "drs/engine/PerformancePackage.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeModel.h"
#include "drs/engine/RuntimePresetState.h"

#include <string>
#include <vector>

namespace drs::app
{
struct PerformancePackageProjectionContext
{
    std::string fallbackPackageName;
    std::string outputProjectPath;
    std::string outputInstrumentPath;
    std::string outputStreamPath;
    std::vector<drs::engine::RuntimeCompileSourceDefinition> sampleSources;
};

struct PerformancePackageProjectionResult
{
    bool projected = false;
    std::string state;
    std::vector<std::string> issues;
    drs::engine::RuntimeCompilePlan compilePlan;
    drs::engine::PerformancePackageManifest manifest;
};

PerformancePackageProjectionResult projectPerformancePackage(
    const drs::engine::RuntimeProjectModel& project,
    const drs::engine::RuntimeSessionStateSnapshot& sessionState,
    PerformancePackageProjectionContext context);
} // namespace drs::app
