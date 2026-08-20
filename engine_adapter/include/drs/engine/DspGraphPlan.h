#pragma once

#include "drs/engine/PlaybackSnapshot.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
enum class DspGraphOwnerKind : std::uint8_t { zone, group, layer, master };

struct DspGraphParameterSlot
{
    std::string id;
    double value = 0.0;
};

struct DspGraphNode
{
    DspGraphOwnerKind ownerKind = DspGraphOwnerKind::zone;
    std::string ownerId;
    std::string inputSourceId;
    std::string outputDestinationId;
    std::string slotId;
    std::string effectType;
    std::uint32_t effectVersion = 0;
    std::size_t parameterStart = 0;
    std::size_t parameterCount = 0;
    std::size_t scratchOffsetBytes = 0;
    std::size_t scratchBytes = 0;
    std::size_t stateBytes = 0;
    std::size_t delayMemoryBytes = 0;
    std::uint32_t costUnits = 0;
};

struct ImmutableDspGraphPlan
{
    std::string authoredGraphDigest;
    std::string planDigest;
    std::vector<DspGraphNode> nodes;
    std::vector<DspGraphParameterSlot> parameters;
    std::size_t scratchBytes = 0;
    std::size_t stateBytes = 0;
    std::size_t delayMemoryBytes = 0;
    std::uint32_t costUnits = 0;
    bool directFastPath = true;
};

struct DspGraphPlanFinding
{
    std::string code;
    std::string path;
    std::string message;
};

struct DspGraphPlanBuildResult
{
    bool compiled = false;
    ImmutableDspGraphPlan plan;
    std::vector<DspGraphPlanFinding> findings;
};

DspGraphPlanBuildResult compileDspGraphPlan(const ImmutablePlaybackSnapshot& snapshot);
} // namespace drs::engine
