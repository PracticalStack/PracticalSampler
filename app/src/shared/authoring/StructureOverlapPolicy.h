#pragma once

#include "drs/engine/RuntimeModel.h"

#include <string>
#include <vector>

namespace drs::app::authoring
{
enum class StructureOverlapKind
{
    none,
    keyRangeOverlap,
    velocityStackCandidate,
    potentialCollision,
    exactKeyStack,
    exactStack
};

struct StructureOverlapInfo
{
    StructureOverlapKind kind = StructureOverlapKind::none;
    int overlapCount = 0;
    bool overlapsKeyRange = false;
    bool sharesVelocity = false;
    bool sharesTrigger = false;
    // A row may have a stronger intentional-stack classification as well as
    // a partial-overlap pair. Preserve that fact for the potential-collision
    // filter instead of collapsing all pair evidence to one enum value.
    bool hasPotentialCollision = false;
    std::string reason;
};

// Shared diagnostic policy for the structure viewer. It is deliberately pure
// so the same classification can drive row badges, filters and qualification
// tests without mutating the authored project.
StructureOverlapInfo classifyStructureOverlap(const drs::engine::RuntimeProjectZoneDefinition& zone,
                                               const drs::engine::RuntimeProjectZoneDefinition& other) noexcept;

std::vector<StructureOverlapInfo> analyzeStructureOverlaps(
    const std::vector<drs::engine::RuntimeProjectZoneDefinition>& zones);
} // namespace drs::app::authoring
