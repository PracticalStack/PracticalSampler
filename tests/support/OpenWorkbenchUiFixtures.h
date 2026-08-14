#pragma once

#include "drs/engine/AuthoringSession.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace drs::test
{
struct OpenWorkbenchZoneMapFixture
{
    std::vector<engine::AuthoringZoneSummary> zones;
    // AuthoringZoneSummary intentionally remains a compact map projection and
    // does not currently carry group identity. Keep deterministic parallel
    // metadata for the semantic-zoom/group-tint phases.
    std::vector<std::string> groupIdsByZone;
};

inline OpenWorkbenchZoneMapFixture makeOpenWorkbenchZoneMapFixture(
    const std::size_t zoneCount = 642)
{
    OpenWorkbenchZoneMapFixture fixture;
    fixture.zones.reserve(zoneCount);
    fixture.groupIdsByZone.reserve(zoneCount);

    constexpr int velocityLayerCount = 8;
    for (std::size_t index = 0; index < zoneCount; ++index)
    {
        const auto key = static_cast<int>(index % 128u);
        const auto layer = static_cast<int>((index / 128u) % velocityLayerCount);
        const auto low = layer == 0 ? 1 : std::max(1, layer * 16 - 2);
        const auto high = layer == velocityLayerCount - 1
            ? 127
            : std::min(127, (layer + 1) * 16 + 2);

        engine::AuthoringZoneSummary zone;
        zone.id = "open-workbench-zone-" + std::to_string(index);
        zone.displayName = index % 73u == 0u
            ? "Open Workbench deliberately long sample label for truncation qualification "
                + std::to_string(index)
            : "Zone " + std::to_string(index) + " / key " + std::to_string(key)
                + " / layer " + std::to_string(layer + 1);
        zone.sampleSourceId = "open-workbench-source-" + std::to_string(index);
        zone.articulationId = index % 5u == 0u ? "staccato" : "sustain";
        zone.rootKey = key;
        zone.keyLow = key;
        zone.keyHigh = key;
        zone.velocityLow = low;
        zone.velocityHigh = high;
        if (layer > 0)
        {
            zone.velocityCrossfade.fadeInLowVelocity = low;
            zone.velocityCrossfade.fadeInHighVelocity = std::min(high, low + 5);
        }
        if (layer < velocityLayerCount - 1)
        {
            zone.velocityCrossfade.fadeOutLowVelocity = std::max(low, high - 5);
            zone.velocityCrossfade.fadeOutHighVelocity = high;
        }
        zone.gainDb = static_cast<double>(static_cast<int>(index % 9u) - 4) * 0.5;
        zone.pan = static_cast<double>(static_cast<int>(index % 7u) - 3) / 10.0;
        zone.loopEnabled = index % 11u == 0u;

        const auto groupIndex = std::min(11, key / 11);
        fixture.groupIdsByZone.push_back("open-workbench-group-" + std::to_string(groupIndex));
        zone.groupId = fixture.groupIdsByZone.back();
        fixture.zones.push_back(std::move(zone));
    }

    // Reserve the final pair for a stable pathological overlap: identical
    // full-map zones with long names and crossfades. These exercise paint order,
    // overlap simplification, hit testing, and label suppression.
    if (zoneCount >= 2)
    {
        for (std::size_t offset = 0; offset < 2; ++offset)
        {
            const auto index = zoneCount - 2 + offset;
            auto& zone = fixture.zones[index];
            zone.id = "open-workbench-pathological-overlap-" + std::to_string(offset + 1);
            zone.displayName = "Pathological identical full-range overlap with a deliberately long label "
                + std::to_string(offset + 1);
            zone.rootKey = 60;
            zone.keyLow = 0;
            zone.keyHigh = 127;
            zone.velocityLow = 1;
            zone.velocityHigh = 127;
            zone.velocityCrossfade.fadeInLowVelocity = 1;
            zone.velocityCrossfade.fadeInHighVelocity = 16;
            zone.velocityCrossfade.fadeOutLowVelocity = 112;
            zone.velocityCrossfade.fadeOutHighVelocity = 127;
            fixture.groupIdsByZone[index] = "open-workbench-group-11";
            fixture.zones[index].groupId = fixture.groupIdsByZone[index];
        }
    }

    return fixture;
}
} // namespace drs::test
