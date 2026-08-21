#pragma once

#include "shared/authoring/StructureScope.h"
#include "drs/engine/AuthoringSession.h"

#include <string>
#include <vector>

namespace drs::app::authoring
{
struct ScopedZoneProjectionOptions
{
    bool includeHiddenContainers = false;
    bool includeSelectedHidden = true;
    std::string searchText;
    std::string articulationFilter;
    std::optional<drs::engine::PerformanceEventKind> performanceEventFilter;
};

struct ScopedZoneProjection
{
    StructureScope scope = makeInstrumentStructureScope();
    std::vector<drs::engine::AuthoringZoneSummary> zones;
    std::size_t totalInScope = 0;
    std::size_t hiddenCount = 0;
};

ScopedZoneProjection buildScopedZoneProjection(
    const drs::engine::RuntimeProjectModel& project,
    const StructureScope& scope,
    const AuthoringStructureSelection& selection,
    const ScopedZoneProjectionOptions& options = {});
} // namespace drs::app::authoring
