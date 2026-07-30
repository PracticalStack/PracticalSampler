#include "drs/engine/DspParameterControl.h"

#include <algorithm>
#include <cmath>

namespace drs::engine
{
DspParameterControlBuildResult compileDspParameterControlLayout(const ImmutableDspGraphPlan& plan)
{
    DspParameterControlBuildResult result;
    result.layout.graphPlanDigest = plan.planDigest;
    for (std::size_t nodeIndex = 0; nodeIndex < plan.nodes.size(); ++nodeIndex)
    {
        const auto& node = plan.nodes[nodeIndex];
        const auto* effect = findCuratedDspEffect(node.effectType, node.effectVersion);
        if (effect == nullptr)
        {
            result.findings.push_back("control-unresolved-effect:" + node.slotId);
            continue;
        }
        for (std::size_t offset = 0; offset < node.parameterCount; ++offset)
        {
            const auto parameterIndex = node.parameterStart + offset;
            if (parameterIndex >= plan.parameters.size())
            {
                result.findings.push_back("control-parameter-index-invalid:" + node.slotId);
                continue;
            }
            const auto& graphParameter = plan.parameters[parameterIndex];
            const auto definition = std::find_if(effect->parameters.begin(), effect->parameters.end(),
                                                 [&](const auto& parameter)
                                                 { return parameter.id == graphParameter.id; });
            if (definition == effect->parameters.end() || !std::isfinite(graphParameter.value)
                || graphParameter.value < definition->minimum || graphParameter.value > definition->maximum)
            {
                result.findings.push_back("control-parameter-invalid:" + node.slotId + ":" + graphParameter.id);
                continue;
            }
            DspParameterControlDescriptor descriptor;
            descriptor.controlIndex = static_cast<std::uint32_t>(result.layout.controls.size());
            descriptor.nodeIndex = static_cast<std::uint32_t>(nodeIndex);
            descriptor.graphParameterIndex = static_cast<std::uint32_t>(parameterIndex);
            descriptor.slotId = node.slotId;
            descriptor.parameterId = graphParameter.id;
            descriptor.minimum = definition->minimum;
            descriptor.maximum = definition->maximum;
            descriptor.defaultValue = definition->defaultValue;
            descriptor.smoothing = definition->smoothing;
            result.layout.controls.push_back(std::move(descriptor));
        }
    }
    result.compiled = result.findings.empty();
    return result;
}
} // namespace drs::engine
