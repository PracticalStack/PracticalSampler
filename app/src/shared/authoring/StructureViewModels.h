#pragma once

#include "shared/authoring/AuthoringStructureSelection.h"
#include "drs/engine/RuntimeModel.h"
#include "shared/authoring/StructureOverlapPolicy.h"
#include "shared/authoring/StructureViewState.h"

#include <string>
#include <vector>

namespace drs::app::authoring
{
struct StructureLayerRowViewModel
{
    std::string id;
    std::string title;
    std::string statusText;
    int displayOrder = 0;
    int groupCount = 0;
    int zoneCount = 0;
    int keyLow = 0;
    int keyHigh = 127;
    bool workspaceVisible = true;
    bool selected = false;
    bool primary = false;
    std::string routingBusId;
    std::string auditionAnchorId;
    std::string accessibilityText;
};

struct StructureViewProjectionOptions
{
    std::string searchText;
    StructureSortMode sortMode = StructureSortMode::authoredOrder;
    bool showOverlapsOnly = false;
    bool showPotentialCollisionsOnly = false;
    bool showExactStacksOnly = false;
    bool visibleOnly = false;
    bool includeDiagnostics = false;
    std::string articulationFilter;
    std::optional<drs::engine::PerformanceEventKind> performanceEventFilter;
};

struct StructureGroupRowViewModel
{
    std::string id;
    std::string layerId;
    std::string title;
    std::string statusText;
    int displayOrder = 0;
    int zoneCount = 0;
    int keyLow = 0;
    int keyHigh = 127;
    bool workspaceVisible = true;
    bool selected = false;
    bool primary = false;
    std::string routingBusId;
    std::string auditionAnchorId;
    std::string accessibilityText;
};

struct StructureZoneRowViewModel
{
    std::string id;
    std::string groupId;
    std::string title;
    std::string sampleSourceId;
    std::string articulationId;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    float keyStartNormalized = 0.0f;
    float keyEndNormalized = 1.0f;
    float rootNormalized = 0.5f;
    int roundRobinPosition = 0;
    int roundRobinLength = 0;
    drs::engine::ZoneTriggerMode triggerMode = drs::engine::ZoneTriggerMode::gated;
    drs::engine::PerformanceEventKind performanceEvent = drs::engine::PerformanceEventKind::noteOn;
    bool selected = false;
    bool primary = false;
    std::string accessibilityText;
    StructureOverlapKind overlapKind = StructureOverlapKind::none;
    int overlapCount = 0;
    bool hasPotentialCollision = false;
    std::string overlapReason;
    bool hasVelocityCrossfade = false;
};

struct StructureHierarchyViewModel
{
    std::vector<StructureLayerRowViewModel> layers;
    std::vector<StructureGroupRowViewModel> groups;
    std::vector<StructureZoneRowViewModel> zones;
};

class StructureOverlapCache
{
public:
    void refreshIfNeeded(const drs::engine::RuntimeProjectModel& project);
    const std::vector<StructureOverlapInfo>& getInfo() const noexcept { return info; }

private:
    std::size_t fingerprint = 0;
    std::vector<StructureOverlapInfo> info;
};

// Pure projection of authored hierarchy into lightweight UI rows. The optional
// primary parent limits children to the selected layer or group, matching the
// first-release navigation contract.
StructureHierarchyViewModel buildStructureHierarchyViewModel(
    const drs::engine::RuntimeProjectModel& project,
    const AuthoringStructureSelection& selection,
    const std::string& primaryLayerId = {},
    const std::string& primaryGroupId = {},
    const StructureViewProjectionOptions& options = {},
    const StructureOverlapCache* overlapCache = nullptr);
} // namespace drs::app::authoring
