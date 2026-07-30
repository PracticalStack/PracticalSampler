#include "drs/engine/DspRenderGeneration.h"
#include "drs/engine/DspGain.h"
#include "drs/engine/DspSaturator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
namespace
{
constexpr std::size_t maximumRoutingScratchBytes = 8u * 1024u * 1024u;
std::atomic<std::uint64_t> nextControlGenerationIdentity { 1 };
}

bool DspRenderGeneration::publishControlValue(const std::uint64_t generationIdentity,
                                               const std::uint32_t controlIndex,
                                               const double value) noexcept
{
    if (generationIdentity != controlGenerationIdentity || controlIndex >= controlLayout.controls.size()
        || !std::isfinite(value)) return false;
    const auto& descriptor = controlLayout.controls[controlIndex];
    if (value < descriptor.minimum || value > descriptor.maximum) return false;
    publishedControlValues[controlIndex].store(static_cast<float>(value), std::memory_order_release);
    return true;
}

std::optional<std::uint32_t> DspRenderGeneration::findControlIndex(const std::string& slotId,
                                                                    const std::string& parameterId) const noexcept
{
    const auto control = std::find_if(controlLayout.controls.begin(), controlLayout.controls.end(),
                                      [&](const auto& candidate)
                                      { return candidate.slotId == slotId && candidate.parameterId == parameterId; });
    return control == controlLayout.controls.end() ? std::optional<std::uint32_t> {}
                                                   : std::optional<std::uint32_t> { control->controlIndex };
}

bool DspRenderGeneration::publishNodeBypass(const std::uint64_t generationIdentity,
                                            const std::uint32_t nodeIndex,
                                            const bool bypassed) noexcept
{
    if (generationIdentity != controlGenerationIdentity || nodeIndex >= graphPlan.nodes.size()) return false;
    publishedNodeBypass[nodeIndex].store(bypassed ? 1u : 0u, std::memory_order_release);
    return true;
}

bool DspRenderGeneration::publishChainBypass(const std::uint64_t generationIdentity,
                                             const std::uint32_t chainIndex,
                                             const bool bypassed) noexcept
{
    if (generationIdentity != controlGenerationIdentity || chainIndex >= chainStartIndices.size()) return false;
    for (std::uint32_t node = chainStartIndices[chainIndex]; node <= chainEndIndices[chainIndex]; ++node)
        publishedNodeBypass[node].store(bypassed ? 1u : 0u, std::memory_order_release);
    return true;
}

void DspRenderGeneration::setControlSampleRate(const double sampleRate) noexcept
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0) return;
    controlSmoothingFrames = std::max<std::uint32_t>(1, static_cast<std::uint32_t>(std::lround(sampleRate * 0.01)));
    bypassSmoothingFrames = std::max<std::uint32_t>(1, static_cast<std::uint32_t>(std::lround(sampleRate * 0.005)));
    for (auto& state : saturatorStates)
        state.prepare(sampleRate);
    for (auto& state : delayStates)
        if (!state.left.empty()) state.prepare(sampleRate);
    resetControlSmoothing();
}

void DspRenderGeneration::resetControlSmoothing() noexcept
{
    for (std::size_t index = 0; index < controlLayout.controls.size(); ++index)
    {
        const auto value = publishedControlValues[index].load(std::memory_order_acquire);
        controlCurrentValues[index] = value;
        controlTargetValues[index] = value;
        controlFramesRemaining[index] = 0;
        const auto parameterIndex = controlLayout.controls[index].graphParameterIndex;
        graphParameterStartValues[parameterIndex] = value;
        graphParameterValues[parameterIndex] = value;
    }
    for (std::size_t index = 0; index < graphPlan.nodes.size(); ++index)
    {
        const auto wet = publishedNodeBypass[index].load(std::memory_order_acquire) == 0 ? 1.0f : 0.0f;
        nodeWetCurrent[index] = wet;
        nodeWetTarget[index] = wet;
        nodeWetStart[index] = wet;
        nodeBypassFramesRemaining[index] = 0;
    }
}

void DspRenderGeneration::resetEffectState() noexcept
{
    for (auto& state : saturatorStates)
        state.reset();
    for (auto& state : delayStates)
        state.reset();
    clearTail();
}

void DspRenderGeneration::setTransport(const SamplerRenderControlValues::TransportView value) noexcept
{
    transport = { value.tempoBpm, value.valid, value.isPlaying };
}

bool DspRenderGeneration::beginScopedRender(const SamplerAudioBufferView output) noexcept
{
    if (output.channels == nullptr || output.channelCount == 0 || output.frameCount == 0
        || output.frameCount > maximumBlockFrames || graphPlan.directFastPath) return false;

    activeFrameCount = output.frameCount;
    activeChannelCount = std::min<std::uint32_t>(output.channelCount, 2);
    for (std::size_t controlIndex = 0; controlIndex < controlLayout.controls.size(); ++controlIndex)
    {
        const auto& control = controlLayout.controls[controlIndex];
        const auto published = publishedControlValues[controlIndex].load(std::memory_order_acquire);
        if (published != controlTargetValues[controlIndex])
        {
            controlTargetValues[controlIndex] = published;
            controlFramesRemaining[controlIndex] = control.smoothing == CuratedDspSmoothing::none
                ? 0 : controlSmoothingFrames;
        }
        const auto parameterIndex = control.graphParameterIndex;
        graphParameterStartValues[parameterIndex] = controlCurrentValues[controlIndex];
        if (controlFramesRemaining[controlIndex] == 0)
        {
            controlCurrentValues[controlIndex] = controlTargetValues[controlIndex];
        }
        else
        {
            const auto frames = std::min<std::uint32_t>(activeFrameCount, controlFramesRemaining[controlIndex]);
            const auto fraction = static_cast<float>(frames) / static_cast<float>(controlFramesRemaining[controlIndex]);
            if (control.smoothing == CuratedDspSmoothing::logarithmic
                && controlCurrentValues[controlIndex] > 0.0f && controlTargetValues[controlIndex] > 0.0f)
            {
                controlCurrentValues[controlIndex] *= std::pow(
                    controlTargetValues[controlIndex] / controlCurrentValues[controlIndex], fraction);
            }
            else
            {
                controlCurrentValues[controlIndex] += (controlTargetValues[controlIndex]
                    - controlCurrentValues[controlIndex]) * fraction;
            }
            controlFramesRemaining[controlIndex] -= frames;
            if (controlFramesRemaining[controlIndex] == 0)
                controlCurrentValues[controlIndex] = controlTargetValues[controlIndex];
        }
        graphParameterValues[parameterIndex] = controlCurrentValues[controlIndex];
    }
    for (std::size_t nodeIndex = 0; nodeIndex < graphPlan.nodes.size(); ++nodeIndex)
    {
        const auto target = publishedNodeBypass[nodeIndex].load(std::memory_order_acquire) == 0 ? 1.0f : 0.0f;
        if (target != nodeWetTarget[nodeIndex])
        {
            nodeWetTarget[nodeIndex] = target;
            nodeBypassFramesRemaining[nodeIndex] = bypassSmoothingFrames;
        }
        nodeWetStart[nodeIndex] = nodeWetCurrent[nodeIndex];
        if (nodeBypassFramesRemaining[nodeIndex] == 0)
            nodeWetCurrent[nodeIndex] = nodeWetTarget[nodeIndex];
        else
        {
            const auto frames = std::min(activeFrameCount, nodeBypassFramesRemaining[nodeIndex]);
            nodeWetCurrent[nodeIndex] += (nodeWetTarget[nodeIndex] - nodeWetCurrent[nodeIndex])
                * static_cast<float>(frames) / static_cast<float>(nodeBypassFramesRemaining[nodeIndex]);
            nodeBypassFramesRemaining[nodeIndex] -= frames;
            if (nodeBypassFramesRemaining[nodeIndex] == 0)
                nodeWetCurrent[nodeIndex] = nodeWetTarget[nodeIndex];
        }
    }
    for (std::size_t nodeIndex = 0; nodeIndex < graphPlan.nodes.size(); ++nodeIndex)
    for (std::uint32_t channel = 0; channel < activeChannelCount; ++channel)
    {
        const auto offset = (nodeIndex * 2u + channel) * maximumBlockFrames;
        std::fill_n(routingScratch.data() + offset, activeFrameCount, 0.0f);
    }

    for (std::size_t routeIndex = 0; routeIndex < routeOutputStorage.size(); ++routeIndex)
    {
        auto& routeOutput = routeOutputStorage[routeIndex];
        const auto destination = routeDestinationNodeIndices[routeIndex];
        if (destination == noGraphDestination)
        {
            routeOutput.view = output;
            routeOutputViews[routeIndex] = routeOutput.view;
            continue;
        }
        for (std::uint32_t channel = 0; channel < activeChannelCount; ++channel)
        {
            const auto offset = (static_cast<std::size_t>(destination) * 2u + channel)
                * maximumBlockFrames;
            routeOutput.channels[channel] = routingScratch.data() + offset;
        }
        routeOutput.view = { routeOutput.channels.data(), activeChannelCount, activeFrameCount };
        routeOutputViews[routeIndex] = routeOutput.view;
    }
    return true;
}

bool DspRenderGeneration::executeScopedGraph(const SamplerAudioBufferView output) noexcept
{
    if (activeFrameCount != output.frameCount || activeChannelCount == 0 || graphPlan.directFastPath)
        return false;

    for (std::size_t chainIndex = 0; chainIndex < chainStartIndices.size(); ++chainIndex)
    {
        const auto start = chainStartIndices[chainIndex];
        const auto end = chainEndIndices[chainIndex];
        std::array<float*, 2> channels {};
        for (std::uint32_t channel = 0; channel < activeChannelCount; ++channel)
        {
            const auto offset = (static_cast<std::size_t>(start) * 2u + channel) * maximumBlockFrames;
            channels[channel] = routingScratch.data() + offset;
        }
        const SamplerAudioBufferView chainBuffer { channels.data(), activeChannelCount, activeFrameCount };
        for (std::uint32_t nodeIndex = start; nodeIndex <= end; ++nodeIndex)
        {
            const auto& node = graphPlan.nodes[nodeIndex];
            // A fully bypassed stateless node has no tail to render; leave it suspended
            // until its published wet target changes, while the chain continues to pass audio.
            if (nodeWetStart[nodeIndex] == 0.0f && nodeWetCurrent[nodeIndex] == 0.0f)
                continue;
            DspGainParameters gainStart;
            DspGainParameters gainEnd;
            DspSaturatorParameters saturatorStart;
            DspSaturatorParameters saturatorEnd;
            DspStereoDelayParameters delayParameters;
            for (std::size_t parameterOffset = 0; parameterOffset < node.parameterCount; ++parameterOffset)
            {
                const auto parameterIndex = node.parameterStart + parameterOffset;
                if (parameterIndex >= graphParameterValues.size()) break;
                const auto& parameterId = graphPlan.parameters[parameterIndex].id;
                const auto startValue = graphParameterStartValues[parameterIndex];
                const auto endValue = graphParameterValues[parameterIndex];
                if (parameterId == "gainDb") { gainStart.gainDb = startValue; gainEnd.gainDb = endValue; }
                else if (parameterId == "polarity") { gainStart.polarity = startValue; gainEnd.polarity = endValue; }
                else if (parameterId == "mute") { gainStart.mute = startValue; gainEnd.mute = endValue; }
                else if (parameterId == "character") { saturatorStart.character = startValue; saturatorEnd.character = endValue; }
                else if (parameterId == "driveDb") { saturatorStart.driveDb = startValue; saturatorEnd.driveDb = endValue; }
                else if (parameterId == "tone") { saturatorStart.tone = startValue; saturatorEnd.tone = endValue; delayParameters.tone = endValue; }
                else if (parameterId == "mix") { saturatorStart.mix = startValue; saturatorEnd.mix = endValue; delayParameters.mix = endValue; }
                else if (parameterId == "outputDb") { saturatorStart.outputDb = startValue; saturatorEnd.outputDb = endValue; }
                else if (parameterId == "timeMs") delayParameters.timeMs = endValue;
                else if (parameterId == "sync") delayParameters.sync = endValue;
                else if (parameterId == "divisionBeats") delayParameters.divisionBeats = endValue;
                else if (parameterId == "feedback") delayParameters.feedback = endValue;
                else if (parameterId == "pingPong") delayParameters.pingPong = endValue;
                else if (parameterId == "width") delayParameters.width = endValue;
            }
            if (node.effectType == "drs.gain")
                processDspGainBypassRamp(chainBuffer, gainStart, gainEnd,
                                         nodeWetStart[nodeIndex], nodeWetCurrent[nodeIndex]);
            else if (node.effectType == "drs.saturator")
                processDspSaturatorBypassRamp(chainBuffer, saturatorStates[nodeIndex],
                                              saturatorStart, saturatorEnd,
                                              nodeWetStart[nodeIndex], nodeWetCurrent[nodeIndex]);
            else if (node.effectType == "drs.stereoDelay")
            {
                processDspStereoDelay(chainBuffer, delayStates[nodeIndex], delayParameters, transport);
                if (delayStates[nodeIndex].inputPeak > 1.0e-5f && delayParameters.feedback > 0.0)
                    setTailFramesRemaining(std::min<std::uint64_t>(30u * 96000u,
                        static_cast<std::uint64_t>(std::ceil(2.0 * 96000.0))));
            }
        }

        const auto downstream = chainDownstreamStartIndices[chainIndex];
        for (std::uint32_t channel = 0; channel < activeChannelCount; ++channel)
        {
            auto* destination = downstream == noGraphDestination
                ? output.channels[channel]
                : routingScratch.data() + (static_cast<std::size_t>(downstream) * 2u + channel)
                    * maximumBlockFrames;
            const auto* source = channels[channel];
            for (std::uint32_t frame = 0; frame < activeFrameCount; ++frame)
                destination[frame] += source[frame];
        }
    }
    return !chainStartIndices.empty();
}

std::shared_ptr<DspRenderGeneration> createDspRenderGeneration(
    SamplerRenderModelPtr samplerModel,
    ImmutableDspGraphPlan graphPlan,
    const std::uint32_t maximumBlockFrames,
    std::string* failureReason)
{
    const auto reject = [&](const char* reason) -> std::shared_ptr<DspRenderGeneration>
    {
        if (failureReason != nullptr) *failureReason = reason;
        return {};
    };
    if (!samplerModel) return reject("A DSP render generation requires an immutable sampler model.");
    if (maximumBlockFrames == 0) return reject("A DSP render generation requires a nonzero prepared block size.");
    if (graphPlan.scratchBytes > 8u * 1024u * 1024u || graphPlan.stateBytes > 16u * 1024u * 1024u)
        return reject("Graph plan exceeds the render-generation resource limits.");
    if (graphPlan.scratchBytes % sizeof(float) != 0)
        return reject("Graph scratch bytes must be float-aligned before audio activation.");

    auto generation = std::make_shared<DspRenderGeneration>();
    generation->samplerModel = std::move(samplerModel);
    generation->graphPlan = std::move(graphPlan);
    generation->maximumBlockFrames = maximumBlockFrames;
    const auto controls = compileDspParameterControlLayout(generation->graphPlan);
    if (!controls.compiled)
        return reject("Graph parameters could not compile into bounded realtime controls.");
    generation->controlLayout = controls.layout;
    generation->graphParameterValues.reserve(generation->graphPlan.parameters.size());
    for (const auto& parameter : generation->graphPlan.parameters)
        generation->graphParameterValues.push_back(static_cast<float>(parameter.value));
    generation->graphParameterStartValues = generation->graphParameterValues;
    if (!generation->controlLayout.controls.empty())
    {
        generation->publishedControlValues
            = std::make_unique<std::atomic<float>[]>(generation->controlLayout.controls.size());
        for (const auto& control : generation->controlLayout.controls)
            generation->publishedControlValues[control.controlIndex].store(
                generation->graphParameterValues[control.graphParameterIndex], std::memory_order_relaxed);
    }
    generation->controlCurrentValues.resize(generation->controlLayout.controls.size());
    generation->controlTargetValues.resize(generation->controlLayout.controls.size());
    generation->controlFramesRemaining.assign(generation->controlLayout.controls.size(), 0);
    if (!generation->graphPlan.nodes.empty())
    {
        generation->publishedNodeBypass = std::make_unique<std::atomic<std::uint8_t>[]>(generation->graphPlan.nodes.size());
        for (std::size_t node = 0; node < generation->graphPlan.nodes.size(); ++node)
            generation->publishedNodeBypass[node].store(0, std::memory_order_relaxed);
    }
    generation->nodeWetCurrent.resize(generation->graphPlan.nodes.size());
    generation->nodeWetTarget.resize(generation->graphPlan.nodes.size());
    generation->nodeWetStart.resize(generation->graphPlan.nodes.size());
    generation->nodeBypassFramesRemaining.assign(generation->graphPlan.nodes.size(), 0);
    generation->saturatorStates.resize(generation->graphPlan.nodes.size());
    generation->delayStates.resize(generation->graphPlan.nodes.size());
    for (auto& state : generation->saturatorStates)
        state.prepare(48000.0);
    for (std::size_t index = 0; index < generation->graphPlan.nodes.size(); ++index)
        if (generation->graphPlan.nodes[index].effectType == "drs.stereoDelay")
            generation->delayStates[index].prepare(48000.0);
    generation->resetControlSmoothing();
    generation->controlGenerationIdentity = nextControlGenerationIdentity.fetch_add(1, std::memory_order_relaxed);
    if (generation->controlGenerationIdentity == 0)
        generation->controlGenerationIdentity = nextControlGenerationIdentity.fetch_add(1, std::memory_order_relaxed);
    generation->mutableEffectState.assign(generation->graphPlan.stateBytes, 0);
    generation->scratch.assign(generation->graphPlan.scratchBytes / sizeof(float), 0.0f);
    generation->routeDestinationNodeIndices.assign(generation->samplerModel->getRoutes().size(),
                                                    DspRenderGeneration::noGraphDestination);
    generation->routeOutputStorage.resize(generation->samplerModel->getRoutes().size());
    generation->routeOutputViews.resize(generation->samplerModel->getRoutes().size());
    const auto& snapshot = *generation->samplerModel->getRetainedActivationPayload()->snapshot;
    for (std::size_t routeIndex = 0; routeIndex < generation->samplerModel->getRoutes().size(); ++routeIndex)
    {
        const auto& route = generation->samplerModel->getRoutes()[routeIndex];
        const auto zone = std::find_if(snapshot.zones.begin(), snapshot.zones.end(), [&](const auto& candidate)
        {
            return candidate.id == route.zoneId;
        });
        const auto firstNodeFor = [&](DspGraphOwnerKind kind, const std::string& ownerId) noexcept
        {
            const auto node = std::find_if(generation->graphPlan.nodes.begin(), generation->graphPlan.nodes.end(),
                                           [&](const auto& candidate)
                                           { return candidate.ownerKind == kind && candidate.ownerId == ownerId; });
            return node == generation->graphPlan.nodes.end()
                ? DspRenderGeneration::noGraphDestination
                : static_cast<std::uint32_t>(std::distance(generation->graphPlan.nodes.begin(), node));
        };
        auto destination = firstNodeFor(DspGraphOwnerKind::zone, route.zoneId);
        if (destination == DspRenderGeneration::noGraphDestination && zone != snapshot.zones.end())
            destination = firstNodeFor(DspGraphOwnerKind::group, zone->groupId);
        if (destination == DspRenderGeneration::noGraphDestination)
            destination = firstNodeFor(DspGraphOwnerKind::master, "master");
        generation->routeDestinationNodeIndices[routeIndex] = destination;
    }
    for (std::size_t start = 0; start < generation->graphPlan.nodes.size();)
    {
        const auto& source = generation->graphPlan.nodes[start].inputSourceId;
        auto end = start;
        while (end + 1 < generation->graphPlan.nodes.size()
               && generation->graphPlan.nodes[end + 1].inputSourceId == source)
            ++end;
        generation->chainStartIndices.push_back(static_cast<std::uint32_t>(start));
        generation->chainEndIndices.push_back(static_cast<std::uint32_t>(end));
        start = end + 1;
    }
    generation->chainDownstreamStartIndices.assign(generation->chainStartIndices.size(),
                                                    DspRenderGeneration::noGraphDestination);
    for (std::size_t chain = 0; chain < generation->chainStartIndices.size(); ++chain)
    {
        const auto& destination = generation->graphPlan.nodes[generation->chainEndIndices[chain]].outputDestinationId;
        for (std::size_t candidate = 0; candidate < generation->chainStartIndices.size(); ++candidate)
        {
            if (generation->graphPlan.nodes[generation->chainStartIndices[candidate]].inputSourceId == destination)
            {
                generation->chainDownstreamStartIndices[chain] = generation->chainStartIndices[candidate];
                break;
            }
        }
        if (generation->chainDownstreamStartIndices[chain] == DspRenderGeneration::noGraphDestination
            && destination.rfind("groups/", 0) == 0)
        {
            for (std::size_t candidate = 0; candidate < generation->chainStartIndices.size(); ++candidate)
            {
                if (generation->graphPlan.nodes[generation->chainStartIndices[candidate]].inputSourceId == "master")
                {
                    generation->chainDownstreamStartIndices[chain] = generation->chainStartIndices[candidate];
                    break;
                }
            }
        }
    }
    const auto routingValueCount = generation->graphPlan.nodes.size() * 2u
        * static_cast<std::size_t>(maximumBlockFrames);
    if (routingValueCount > maximumRoutingScratchBytes / sizeof(float)
        || generation->graphPlan.scratchBytes > maximumRoutingScratchBytes - routingValueCount * sizeof(float))
        return reject("Graph routing scratch exceeds the render-generation resource limit.");
    generation->routingScratch.assign(routingValueCount, 0.0f);
    generation->diagnostics.graphPlanDigest = generation->graphPlan.planDigest;
    generation->diagnostics.nodeCount = generation->graphPlan.nodes.size();
    generation->diagnostics.effectCount = generation->graphPlan.nodes.size();
    generation->diagnostics.scratchBytes = generation->graphPlan.scratchBytes + routingValueCount * sizeof(float);
    generation->diagnostics.stateBytes = generation->graphPlan.stateBytes;
    generation->diagnostics.delayMemoryBytes = generation->graphPlan.delayMemoryBytes;
    return generation;
}
} // namespace drs::engine
