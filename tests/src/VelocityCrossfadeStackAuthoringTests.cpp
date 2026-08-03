#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/VelocityCrossfadeAuthoring.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
void require(const bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

drs::engine::RuntimeProjectModel makeBaseProject()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Stack fixture requires the Phase 2 project.");
    const auto curated = drs::engine::migrateRuntimeProjectToCuratedDspSchema(loaded.project);
    const auto migrated = drs::engine::migrateRuntimeProjectToPerformanceArticulationSchema(curated.project);
    require(curated.valid && migrated.valid && migrated.project.authoring.zones.size() >= 2,
            "Stack fixture must migrate to the current project schema.");
    auto project = migrated.project;
    for (std::size_t index = 2; index < project.authoring.zones.size(); ++index)
        project.authoring.zones[index].rootKey += 12;
    return project;
}

drs::engine::RuntimeProjectModel makeStackProject(const int layerCount)
{
    auto project = makeBaseProject();
    auto prototype = project.authoring.zones.front();
    const auto originalZoneCount = project.authoring.zones.size();
    for (int layer = 0; layer < layerCount; ++layer)
    {
        auto zone = static_cast<std::size_t>(layer) < originalZoneCount
            ? project.authoring.zones[static_cast<std::size_t>(layer)] : prototype;
        zone.articulationId = prototype.articulationId;
        zone.rootKey = prototype.rootKey;
        zone.keyLow = prototype.keyLow;
        zone.keyHigh = prototype.keyHigh;
        zone.triggerMode = prototype.triggerMode;
        if (static_cast<std::size_t>(layer) >= originalZoneCount)
            zone.id = "stack-layer-" + std::to_string(layer);
        zone.displayName = "Stack layer " + std::to_string(layer + 1);
        zone.velocityLow = 1 + layer * (127 / layerCount);
        zone.velocityHigh = layer + 1 == layerCount ? 127 : (1 + (layer + 1) * (127 / layerCount) - 1);
        zone.velocityCrossfade = {};
        zone.roundRobin.reset();
        zone.roundRobinLength = 0;
        zone.roundRobinPosition = 0;
        if (static_cast<std::size_t>(layer) < originalZoneCount)
            project.authoring.zones[static_cast<std::size_t>(layer)] = zone;
        else
            project.authoring.zones.push_back(std::move(zone));
    }
    project.authoring.selectedZoneId = project.authoring.zones.front().id;
    const auto validation = drs::engine::validateRuntimeProjectModel(project);
    if (!validation.valid)
    {
        std::string issue;
        for (const auto& entry : validation.issues)
            issue += (issue.empty() ? "" : " | ") + entry;
        throw std::runtime_error("Stack fixture must be valid: " + issue);
    }
    return project;
}

std::vector<std::string> stackIds(const drs::engine::RuntimeProjectModel& project, const int count)
{
    std::vector<std::string> ids;
    for (int index = 0; index < count; ++index)
        ids.push_back(project.authoring.zones[static_cast<std::size_t>(index)].id);
    return ids;
}

drs::engine::RuntimeProjectModel makeRoundRobinStackProject(const int layers, const int slots)
{
    auto project = makeBaseProject();
    auto prototype = project.authoring.zones.front();
    const auto originalZoneCount = project.authoring.zones.size();
    int index = 0;
    for (int layer = 0; layer < layers; ++layer)
    {
        for (int slot = 1; slot <= slots; ++slot)
        {
            auto zone = static_cast<std::size_t>(index) < originalZoneCount
                ? project.authoring.zones[static_cast<std::size_t>(index)] : prototype;
            zone.articulationId = prototype.articulationId;
            zone.rootKey = prototype.rootKey;
            zone.keyLow = prototype.keyLow;
            zone.keyHigh = prototype.keyHigh;
            zone.triggerMode = prototype.triggerMode;
            if (static_cast<std::size_t>(index) >= originalZoneCount)
                zone.id = "rr-stack-" + std::to_string(layer) + "-" + std::to_string(slot);
            zone.displayName = zone.id;
            zone.velocityLow = 1 + layer * (127 / layers);
            zone.velocityHigh = layer + 1 == layers ? 127 : 1 + (layer + 1) * (127 / layers) - 1;
            zone.velocityCrossfade = {};
            zone.roundRobin = drs::engine::RoundRobinDescriptor { "stack-pool", slots, slot,
                                                                    drs::engine::RoundRobinMode::sequential };
            zone.roundRobinLength = slots;
            zone.roundRobinPosition = slot;
            if (static_cast<std::size_t>(index) < originalZoneCount)
                project.authoring.zones[static_cast<std::size_t>(index)] = zone;
            else
                project.authoring.zones.push_back(std::move(zone));
            ++index;
        }
    }
    project.authoring.selectedZoneId = project.authoring.zones.front().id;
    require(drs::engine::validateRuntimeProjectModel(project).valid, "Round Robin stack fixture must be valid.");
    return project;
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        for (const auto layerCount : { 2, 5, 10 })
        {
            auto project = makeStackProject(layerCount);
            const auto ids = stackIds(project, layerCount);
            const auto plan = planVelocityCrossfadeStack(project, { ids, 16 });
            if (!plan.changesProject() || static_cast<int>(plan.stackOverlaps.size()) != layerCount - 1)
            {
                const auto issue = plan.blockingIssues.empty() ? std::string("no issue") : plan.blockingIssues.front();
                throw std::runtime_error("Stack planning must create exactly N-1 adjacent crossfades: " + issue);
            }
            require(plan.proposedProject.authoring.zones[0].velocityLow == 1,
                    "Stack planning must preserve the lower stack endpoint.");
            const auto lastIndex = static_cast<std::size_t>(layerCount - 1);
            require(plan.proposedProject.authoring.zones[lastIndex].velocityHigh == 127,
                    "Stack planning must preserve the upper stack endpoint.");
            require(validateRuntimeProjectModel(plan.proposedProject).valid,
                    "Every planned stack must pass project topology validation.");

            AuthoringSession session(project);
            const auto result = session.createVelocityCrossfadeStack({ ids, 16 }, "Create stack crossfades");
            require(result.applied && session.getDocumentState().undoDepth == 1,
                    "An entire stack must commit in one undo snapshot.");
            require(session.undo().applied && session.redo().applied,
                    "A stack authoring transaction must round-trip through undo and redo.");
        }

        const auto denseProject = makeStackProject(10);
        const auto densePlan = planVelocityCrossfadeStack(denseProject, { stackIds(denseProject, 10), 16 });
        require(std::any_of(densePlan.stackOverlaps.begin(), densePlan.stackOverlaps.end(),
                            [](const auto& overlap) { return overlap.widthClamped; }),
                "Dense stacks must report deterministic overlap-width clamping in their preview.");

        auto gapped = makeStackProject(3);
        gapped.authoring.zones[0].velocityHigh = 20;
        gapped.authoring.zones[1].velocityLow = 48;
        gapped.authoring.zones[1].velocityHigh = 84;
        gapped.authoring.zones[2].velocityLow = 108;
        const auto gapPlan = planVelocityCrossfadeStack(gapped, { stackIds(gapped, 3), 16 });
        require(gapPlan.changesProject() && validateRuntimeProjectModel(gapPlan.proposedProject).valid,
                "Gapped hard splits must receive deterministic valid adjacent overlap windows.");

        auto existing = makeStackProject(5);
        AuthoringSession existingSession(existing);
        const auto existingIds = stackIds(existing, 5);
        require(existingSession.createVelocityCrossfadeStack({ existingIds, 16 }, "Create stack").applied,
                "A pre-existing stack fixture must create successfully.");
        std::vector<std::pair<int, int>> ranges;
        for (const auto& zone : existingSession.getProject().authoring.zones)
            if (zone.id.rfind("stack-layer-", 0) == 0)
                ranges.emplace_back(zone.velocityLow, zone.velocityHigh);
        require(existingSession.removeVelocityCrossfadeStack(existingIds, "Remove stack").applied,
                "Stack removal must be an atomic operation.");
        std::size_t rangeIndex = 0;
        for (const auto& zone : existingSession.getProject().authoring.zones)
            if (zone.id.rfind("stack-layer-", 0) == 0)
            {
                require(ranges[rangeIndex++] == std::make_pair(zone.velocityLow, zone.velocityHigh)
                            && !hasAnyVelocityCrossfadeValue(zone.velocityCrossfade),
                        "Stack removal must preserve velocity ranges and clear only crossfade descriptors.");
            }

        auto ambiguous = makeStackProject(3);
        ambiguous.authoring.zones[1].velocityLow = ambiguous.authoring.zones[0].velocityLow;
        require(planVelocityCrossfadeStack(ambiguous, { stackIds(ambiguous, 3), 16 }).state
                    == VelocityCrossfadeAuthoringState::ambiguousPartner,
                "Duplicate layer starts must be rejected rather than resolved by id order.");

        auto roundRobin = makeRoundRobinStackProject(3, 2);
        std::vector<std::string> roundRobinIds;
        for (int layer = 0; layer < 3; ++layer)
            for (int slot = 1; slot <= 2; ++slot)
                roundRobinIds.push_back(roundRobin.authoring.zones[static_cast<std::size_t>(layer * 2 + slot - 1)].id);
        const auto rrPlan = planVelocityCrossfadeStack(roundRobin, { roundRobinIds, 16 });
        require(rrPlan.changesProject() && rrPlan.orderedLayerZoneIds.size() == 3
                    && rrPlan.orderedLayerZoneIds.front().size() == 2,
                "A complete Round Robin selection must create matching slot bundles.");
        auto partialIds = roundRobinIds;
        partialIds.pop_back();
        require(planVelocityCrossfadeStack(roundRobin, { partialIds, 16 }).state
                    == VelocityCrossfadeAuthoringState::incompleteRoundRobinPool,
                "Partial Round Robin selections must be rejected.");
        roundRobin.authoring.zones[1].roundRobinLength = 3;
        require(planVelocityCrossfadeStack(roundRobin, { roundRobinIds, 16 }).state
                    == VelocityCrossfadeAuthoringState::mixedRoundRobinSlotCount,
                "Mixed Round Robin slot counts must be rejected.");

        auto fourSlotRoundRobin = makeRoundRobinStackProject(3, 4);
        std::vector<std::string> fourSlotIds;
        for (int layer = 0; layer < 3; ++layer)
            for (int slot = 1; slot <= 4; ++slot)
                fourSlotIds.push_back(fourSlotRoundRobin.authoring.zones[
                    static_cast<std::size_t>(layer * 4 + slot - 1)].id);
        require(planVelocityCrossfadeStack(fourSlotRoundRobin, { fourSlotIds, 16 }).changesProject(),
                "Complete four-slot Round Robin bundles must produce matching stack crossfades.");

        std::cout << "Velocity crossfade stack authoring tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Velocity crossfade stack authoring tests failed: " << exception.what() << '\n';
        return 1;
    }
}
