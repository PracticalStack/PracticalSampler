#pragma once

#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/DspAlgorithmicReverb.h"
#include "drs/engine/DspCompactEq.h"
#include "drs/engine/DspChorus.h"
#include "drs/engine/DspParameterControl.h"
#include "drs/engine/DspSaturator.h"
#include "drs/engine/DspStereoDelay.h"
#include "drs/engine/SamplerRenderModel.h"

#include <cstddef>
#include <atomic>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace drs::engine
{
struct DspRenderGenerationDiagnostics
{
    std::string graphPlanDigest;
    std::size_t nodeCount = 0;
    std::size_t effectCount = 0;
    std::size_t scratchBytes = 0;
    std::size_t stateBytes = 0;
    std::size_t delayMemoryBytes = 0;
};

class DspRenderGeneration final
{
public:
    static constexpr std::uint32_t noGraphDestination = std::numeric_limits<std::uint32_t>::max();
    const SamplerRenderModelPtr& getSamplerModel() const noexcept { return samplerModel; }
    const ImmutableDspGraphPlan& getGraphPlan() const noexcept { return graphPlan; }
    const DspRenderGenerationDiagnostics& getDiagnostics() const noexcept { return diagnostics; }
    const DspParameterControlLayout& getControlLayout() const noexcept { return controlLayout; }
    std::uint64_t getControlGenerationIdentity() const noexcept { return controlGenerationIdentity; }
    std::optional<std::uint32_t> findControlIndex(const std::string& slotId,
                                                  const std::string& parameterId) const noexcept;
    bool publishControlValue(std::uint64_t generationIdentity,
                             std::uint32_t controlIndex,
                             double value) noexcept;
    bool publishNodeBypass(std::uint64_t generationIdentity,
                           std::uint32_t nodeIndex,
                           bool bypassed) noexcept;
    bool publishChainBypass(std::uint64_t generationIdentity,
                            std::uint32_t chainIndex,
                            bool bypassed) noexcept;
    void setControlSampleRate(double sampleRate) noexcept;
    void resetControlSmoothing() noexcept;
    void resetEffectState() noexcept;
    void setTransport(SamplerRenderControlValues::TransportView transport) noexcept;
    // Called around voice mixing. All routing buffers and route views were allocated
    // during construction, so the callback exchanges only numeric route targets.
    bool beginScopedRender(SamplerAudioBufferView output) noexcept;
    bool beginRetiredTailRender(SamplerAudioBufferView output) noexcept;
    const SamplerAudioBufferView* getRouteOutputViews() const noexcept
    {
        return routeOutputViews.empty() ? nullptr : routeOutputViews.data();
    }
    std::size_t getRouteOutputViewCount() const noexcept { return routeOutputViews.size(); }
    bool executeScopedGraph(SamplerAudioBufferView output) noexcept;
    void requestRetirementTailFade(std::uint32_t frames) noexcept;
    // Indexed by immutable sampler route. This is built off-audio and lets the
    // voice path select its future graph input without IDs, hashes, or strings.
    std::uint32_t getRouteDestinationNodeIndex(std::size_t routeIndex) const noexcept
    {
        return routeIndex < routeDestinationNodeIndices.size()
            ? routeDestinationNodeIndices[routeIndex] : noGraphDestination;
    }
    bool tailActive() const noexcept { return tailFramesRemaining.load(std::memory_order_relaxed) != 0; }
    void clearTail() noexcept { tailFramesRemaining.store(0, std::memory_order_release); }
    void setTailFramesRemaining(std::uint64_t frames) noexcept { tailFramesRemaining.store(frames, std::memory_order_release); }
    void advanceTail(std::uint32_t frames) noexcept
    {
        auto remaining = tailFramesRemaining.load(std::memory_order_relaxed);
        while (remaining != 0)
        {
            const auto next = frames >= remaining ? 0 : remaining - frames;
            if (tailFramesRemaining.compare_exchange_weak(remaining, next, std::memory_order_release,
                                                          std::memory_order_relaxed))
                break;
        }
    }

private:
    friend std::shared_ptr<DspRenderGeneration> createDspRenderGeneration(
        SamplerRenderModelPtr, ImmutableDspGraphPlan, std::uint32_t, std::string*);

    SamplerRenderModelPtr samplerModel;
    ImmutableDspGraphPlan graphPlan;
    std::vector<std::uint8_t> mutableEffectState;
    std::vector<float> scratch;
    struct RouteOutputView
    {
        std::array<float*, 2> channels {};
        SamplerAudioBufferView view;
    };
    std::vector<float> routingScratch;
    std::vector<RouteOutputView> routeOutputStorage;
    std::vector<SamplerAudioBufferView> routeOutputViews;
    std::vector<std::uint32_t> routeDestinationNodeIndices;
    std::vector<std::uint32_t> chainStartIndices;
    std::vector<std::uint32_t> chainEndIndices;
    std::vector<std::uint32_t> chainDownstreamStartIndices;
    DspParameterControlLayout controlLayout;
    std::unique_ptr<std::atomic<float>[]> publishedControlValues;
    std::vector<float> graphParameterStartValues;
    std::vector<float> graphParameterValues;
    std::vector<float> controlCurrentValues;
    std::vector<float> controlTargetValues;
    std::vector<std::uint32_t> controlFramesRemaining;
    std::unique_ptr<std::atomic<std::uint8_t>[]> publishedNodeBypass;
    std::vector<float> nodeWetCurrent;
    std::vector<float> nodeWetTarget;
    std::vector<float> nodeWetStart;
    std::vector<std::uint32_t> nodeBypassFramesRemaining;
    std::vector<DspSaturatorState> saturatorStates;
    std::vector<DspStereoDelayState> delayStates;
    std::vector<DspAlgorithmicReverbState> reverbStates;
    std::vector<DspCompactEqState> compactEqStates;
    std::vector<DspChorusState> chorusStates;
    DspStereoDelayTransport transport;
    std::uint64_t controlGenerationIdentity = 0;
    double controlSampleRate = 48000.0;
    std::uint32_t controlSmoothingFrames = 480;
    std::uint32_t bypassSmoothingFrames = 240;
    std::uint32_t maximumBlockFrames = 0;
    std::uint32_t activeFrameCount = 0;
    std::uint32_t activeChannelCount = 0;
    DspRenderGenerationDiagnostics diagnostics;
    std::atomic<std::uint64_t> tailFramesRemaining { 0 };
    std::atomic<std::uint32_t> requestedRetirementTailFadeFrames { 0 };
    std::uint32_t retirementTailFadeFramesRemaining = 0;
    float retirementTailGain = 1.0f;
    bool renderingRetiredTail = false;
    bool beginRender(SamplerAudioBufferView output) noexcept;
};

std::shared_ptr<DspRenderGeneration> createDspRenderGeneration(
    SamplerRenderModelPtr samplerModel,
    ImmutableDspGraphPlan graphPlan,
    std::uint32_t maximumBlockFrames,
    std::string* failureReason = nullptr);
} // namespace drs::engine
