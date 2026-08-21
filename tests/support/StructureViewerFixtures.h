#pragma once

#include "drs/engine/RuntimeModel.h"

#include <cstddef>
#include <string>
#include <utility>

namespace drs::tests
{
inline drs::engine::RuntimeProjectModel makeStructureViewerFixture(const std::size_t additionalZones = 0)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 10;
    project.projectId = "structure-viewer-fixture";
    project.displayName = "Structure Viewer Fixture";
    project.contentRootPath = ".";
    project.defaultInstrumentManifestPath = "structure-viewer-fixture.json";
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 9;
    project.authoring.articulations.push_back({ "default", "Default", true, {} });
    project.authoring.articulations.push_back({ "legato", "Legato", false, {} });
    project.authoring.articulations.push_back({ "release", "Release", false, {} });

    project.authoring.layers = {
        { "layer-piano", "Piano", 0, true, 0.0, 0.0, {}, "group-piano-sustain", {} },
        { "layer-strings", "Strings", 1, true, -1.0, 0.0, {}, "group-strings-sustain", {} },
        { "layer-empty", "Empty Layer", 2, true, 0.0, 0.0, {}, {}, {} }
    };
    project.authoring.groups = {
        { "group-piano-sustain", "layer-piano", "Piano Sustain", 0, true, 0.0, 0.0, {}, {} },
        { "group-piano-release", "layer-piano", "Piano Release", 1, true, 0.0, 0.0, {}, {} },
        { "group-strings-sustain", "layer-strings", "Strings Sustain", 0, true, 0.0, 0.0, {}, {} },
        { "group-empty", "layer-strings", "Empty Group", 1, true, 0.0, 0.0, {}, {} }
    };

    auto addZone = [&](std::string id,
                       std::string displayName,
                       std::string groupId,
                       int keyLow,
                       int keyHigh,
                       int velocityLow,
                       int velocityHigh,
                       int rootKey,
                       int roundRobinPosition = 0,
                       int roundRobinLength = 0)
    {
        drs::engine::RuntimeProjectZoneDefinition zone;
        zone.id = std::move(id);
        zone.displayName = std::move(displayName);
        zone.sampleSourceId = "sample-" + zone.id;
        project.sampleSources.push_back({ zone.sampleSourceId, zone.sampleSourceId + ".wav", "fixture" });
        zone.groupId = std::move(groupId);
        zone.articulationId = "default";
        zone.rootKey = rootKey;
        zone.keyLow = keyLow;
        zone.keyHigh = keyHigh;
        zone.velocityLow = velocityLow;
        zone.velocityHigh = velocityHigh;
        zone.roundRobinLength = roundRobinLength;
        zone.roundRobinPosition = roundRobinPosition;
        if (roundRobinLength > 0)
            zone.roundRobin = drs::engine::RoundRobinDescriptor { "fixture-" + zone.groupId,
                                                                   roundRobinLength,
                                                                   roundRobinPosition,
                                                                   drs::engine::RoundRobinMode::sequential };
        project.authoring.zones.push_back(std::move(zone));
    };

    addZone("zone-piano-low", "Piano Low", "group-piano-sustain", 36, 60, 1, 63, 48);
    addZone("zone-piano-high", "Piano High", "group-piano-sustain", 36, 60, 64, 127, 48);
    addZone("zone-piano-release", "Piano Release", "group-piano-release", 36, 60, 1, 127, 48);
    addZone("zone-strings-a", "Strings A", "group-strings-sustain", 48, 84, 1, 127, 60, 1, 2);
    addZone("zone-strings-b", "Strings B", "group-strings-sustain", 48, 84, 1, 127, 60, 2, 2);
    project.authoring.zones[3].articulationId = "legato";
    project.authoring.zones[4].articulationId = "release";
    project.authoring.zones[4].performance.event = drs::engine::PerformanceEventKind::noteOff;

    for (std::size_t index = 0; index < additionalZones; ++index)
    {
        const auto id = "zone-large-" + std::to_string(index);
        addZone(id,
                "Large Zone " + std::to_string(index),
                index % 2 == 0 ? "group-piano-sustain" : "group-strings-sustain",
                static_cast<int>(index % 48) + 36,
                static_cast<int>(index % 48) + 60,
                static_cast<int>(index % 4) * 32 + 1,
                static_cast<int>(index % 4 + 1) * 32,
                static_cast<int>(index % 48) + 48);
    }

    project.authoring.selectedLayerId = "layer-piano";
    project.authoring.selectedGroupId = "group-piano-sustain";
    project.authoring.selectedZoneId = "zone-piano-low";
    return project;
}
} // namespace drs::tests
