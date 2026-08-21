#include "shared/authoring/ZoneMapOverlapIndex.h"

#include <algorithm>

namespace drs::app::authoring
{
void ZoneMapOverlapIndex::rebuild(const std::vector<drs::engine::AuthoringZoneSummary>& nextZones)
{
    zones = nextZones;
}

std::vector<ZoneMapOverlapCandidate> ZoneMapOverlapIndex::query(
    const int key, const int velocity, const std::string& excludeZoneId) const
{
    std::vector<ZoneMapOverlapCandidate> result;
    const auto target = std::find_if(zones.begin(), zones.end(),
                                     [&](const auto& zone) { return zone.id == excludeZoneId; });
    for (const auto& zone : zones)
    {
        if (zone.id == excludeZoneId
            || key < zone.keyLow || key > zone.keyHigh
            || velocity < zone.velocityLow || velocity > zone.velocityHigh)
            continue;
        ZoneMapOverlapCandidate candidate;
        candidate.zoneId = zone.id;
        candidate.displayName = zone.displayName.empty() ? zone.id : zone.displayName;
        candidate.keyLow = zone.keyLow;
        candidate.keyHigh = zone.keyHigh;
        candidate.velocityLow = zone.velocityLow;
        candidate.velocityHigh = zone.velocityHigh;
        candidate.overlapArea = (zone.keyHigh - zone.keyLow + 1)
            * (zone.velocityHigh - zone.velocityLow + 1);
        candidate.exactStack = target != zones.end()
            && zone.keyLow == target->keyLow && zone.keyHigh == target->keyHigh
            && zone.velocityLow == target->velocityLow && zone.velocityHigh == target->velocityHigh;
        result.push_back(std::move(candidate));
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right)
    {
        if (left.exactStack != right.exactStack) return left.exactStack > right.exactStack;
        if (left.overlapArea != right.overlapArea) return left.overlapArea > right.overlapArea;
        return left.zoneId < right.zoneId;
    });
    return result;
}
} // namespace drs::app::authoring
