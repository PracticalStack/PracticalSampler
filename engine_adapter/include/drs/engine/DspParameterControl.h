#pragma once

#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/DspGraphPlan.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct DspParameterControlDescriptor
{
    std::uint32_t controlIndex = 0;
    std::uint32_t nodeIndex = 0;
    std::uint32_t graphParameterIndex = 0;
    std::string slotId;
    std::string parameterId;
    double minimum = 0.0;
    double maximum = 0.0;
    double defaultValue = 0.0;
    CuratedDspSmoothing smoothing = CuratedDspSmoothing::none;
};

struct DspParameterControlLayout
{
    std::string graphPlanDigest;
    std::vector<DspParameterControlDescriptor> controls;
};

struct DspParameterControlBuildResult
{
    bool compiled = false;
    DspParameterControlLayout layout;
    std::vector<std::string> findings;
};

// Message/worker-owned only. The callback consumes controlIndex and graphParameterIndex,
// never a slot or parameter string.
DspParameterControlBuildResult compileDspParameterControlLayout(const ImmutableDspGraphPlan& plan);
} // namespace drs::engine
