#include "shared/authoring/StructureOverlapPolicy.h"

#include <algorithm>

namespace drs::app::authoring
{
namespace
{
bool intersects(const int leftLow, const int leftHigh, const int rightLow, const int rightHigh) noexcept
{
    return leftLow <= rightHigh && rightLow <= leftHigh;
}

bool sameRoundRobinSlot(const drs::engine::RuntimeProjectZoneDefinition& left,
                        const drs::engine::RuntimeProjectZoneDefinition& right) noexcept
{
    return left.roundRobinPosition > 0
        && right.roundRobinPosition > 0
        && left.roundRobinPosition == right.roundRobinPosition;
}
} // namespace

StructureOverlapInfo classifyStructureOverlap(const drs::engine::RuntimeProjectZoneDefinition& zone,
                                               const drs::engine::RuntimeProjectZoneDefinition& other) noexcept
{
    StructureOverlapInfo result;
    result.overlapsKeyRange = intersects(zone.keyLow, zone.keyHigh, other.keyLow, other.keyHigh);
    if (!result.overlapsKeyRange)
        return result;

    result.kind = StructureOverlapKind::keyRangeOverlap;
    result.sharesVelocity = intersects(zone.velocityLow,
                                       zone.velocityHigh,
                                       other.velocityLow,
                                       other.velocityHigh);
    result.sharesTrigger = zone.articulationId == other.articulationId
        && zone.performance.event == other.performance.event;

    const auto exactKeyRange = zone.keyLow == other.keyLow && zone.keyHigh == other.keyHigh;
    if (result.sharesTrigger && exactKeyRange && result.sharesVelocity)
    {
        result.kind = sameRoundRobinSlot(zone, other)
            ? StructureOverlapKind::exactStack
            : StructureOverlapKind::exactKeyStack;
    }
    else if (result.sharesTrigger && exactKeyRange && !result.sharesVelocity)
        result.kind = StructureOverlapKind::velocityStackCandidate;
    else if (result.sharesVelocity && result.sharesTrigger)
        result.kind = StructureOverlapKind::potentialCollision;
    result.reason = result.kind == StructureOverlapKind::exactStack
        ? "same key, velocity, trigger, and round-robin slot"
        : result.kind == StructureOverlapKind::exactKeyStack
            ? "same key and velocity range with the same trigger"
            : result.kind == StructureOverlapKind::velocityStackCandidate
                ? "same key range with adjacent velocity stack"
        : result.kind == StructureOverlapKind::potentialCollision
            ? "key and velocity overlap with the same trigger"
            : result.sharesVelocity
                ? "key and velocity overlap; trigger or articulation differs"
                : "key range overlaps";
    return result;
}

std::vector<StructureOverlapInfo> analyzeStructureOverlaps(
    const std::vector<drs::engine::RuntimeProjectZoneDefinition>& zones)
{
    std::vector<StructureOverlapInfo> result(zones.size());
    for (std::size_t index = 0; index < zones.size(); ++index)
    {
        for (std::size_t otherIndex = index + 1; otherIndex < zones.size(); ++otherIndex)
        {
            const auto classification = classifyStructureOverlap(zones[index], zones[otherIndex]);
            if (!classification.overlapsKeyRange)
                continue;

            ++result[index].overlapCount;
            ++result[otherIndex].overlapCount;
            result[index].overlapsKeyRange = true;
            result[otherIndex].overlapsKeyRange = true;
            result[index].sharesVelocity = result[index].sharesVelocity || classification.sharesVelocity;
            result[otherIndex].sharesVelocity = result[otherIndex].sharesVelocity || classification.sharesVelocity;
            result[index].sharesTrigger = result[index].sharesTrigger || classification.sharesTrigger;
            result[otherIndex].sharesTrigger = result[otherIndex].sharesTrigger || classification.sharesTrigger;
            if (classification.kind == StructureOverlapKind::potentialCollision)
            {
                result[index].hasPotentialCollision = true;
                result[otherIndex].hasPotentialCollision = true;
            }
            if (static_cast<int>(classification.kind) > static_cast<int>(result[index].kind))
            {
                result[index].kind = classification.kind;
                result[index].reason = classification.reason;
            }
            if (static_cast<int>(classification.kind) > static_cast<int>(result[otherIndex].kind))
            {
                result[otherIndex].kind = classification.kind;
                result[otherIndex].reason = classification.reason;
            }
        }
    }
    return result;
}
} // namespace drs::app::authoring
