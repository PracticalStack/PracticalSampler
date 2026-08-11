#pragma once

#include "drs/engine/SamplerVoice.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace drs::engine
{
class SamplerEventBlock final
{
public:
    static constexpr std::size_t capacity = 512;

    bool push(SamplerRenderEvent event) noexcept;
    void clear() noexcept;
    SamplerRenderEventView view() const noexcept { return { events.data(), eventCount }; }
    std::size_t size() const noexcept { return eventCount; }
    std::size_t droppedEventCount() const noexcept { return droppedEvents; }

private:
    std::array<SamplerRenderEvent, capacity> events {};
    std::size_t eventCount = 0;
    std::size_t droppedEvents = 0;
};

enum class SamplerVoiceSlotState : std::uint8_t
{
    free,
    active,
    releasing,
    finished
};

struct SamplerVoiceSlotSnapshot
{
    SamplerVoiceSlotState state = SamplerVoiceSlotState::free;
    std::uint64_t voiceId = 0;
    std::uint64_t activationGeneration = 0;
    std::size_t modelRevision = 0;
    int sourceMidiNote = 0;
    int effectiveMidiNote = 0;
    double incrementFrames = 0.0;
    float baseGain = 0.0f;
    bool loopActive = false;
    bool sustainDeferred = false;
};

struct SamplerVoicePoolRenderResult
{
    bool accepted = false;
    SamplerRenderResult render;
    std::uint32_t activeVoiceCount = 0;
    std::uint32_t releasingVoiceCount = 0;
    std::uint32_t finishedVoiceCount = 0;
    std::uint32_t activeGenerationVoiceCount = 0;
    std::uint32_t retiredGenerationVoiceCount = 0;
    std::uint32_t sustainDeferredVoiceCount = 0;
    std::uint32_t resetVoiceCount = 0;
};

struct SamplerRouteEligibilityQuery
{
    int midiNote = 60;
    int velocity = 64;
    PerformanceEventKind performanceEvent = PerformanceEventKind::noteOn;
    bool sustainPedalDown = false;
    std::uint32_t articulationIndex = kInvalidPerformanceProgramIndex;
    std::array<std::uint8_t, 128> controllerValues {};
};

struct SamplerRouteEligibilityResult
{
    bool evaluated = false;
    std::vector<std::size_t> eligibleRouteIndices;
};

// Qualification-side read-only evaluator. It intentionally reports every semantically eligible
// route before crossfade weighting or one-slot round-robin selection.
SamplerRouteEligibilityResult evaluateSamplerRouteEligibility(
    const SamplerRenderModel& model,
    const SamplerRouteEligibilityQuery& query);

class SamplerVoicePool final
{
public:
    static constexpr std::size_t capacity = 24;

    bool prepare(const SamplerRenderModel& model,
                 double outputSampleRate,
                 std::uint64_t activationGeneration = 0) noexcept;
    bool activateModel(const SamplerRenderModel& model,
                       double outputSampleRate,
                       std::uint64_t activationGeneration = 0) noexcept;
    void clearRenderModel() noexcept;
    SamplerVoicePoolRenderResult renderBlock(SamplerAudioBufferView output,
                                             SamplerRenderEventView events,
                                             SamplerRenderControlValues controls = {},
                                             const SamplerAudioBufferView* routeTargets = nullptr,
                                             std::size_t routeTargetCount = 0) noexcept;
    void resetVoices() noexcept;

    std::size_t activeVoiceCount() const noexcept;
    std::size_t releasingVoiceCount() const noexcept;
    std::size_t finishedVoiceCount() const noexcept;
    std::size_t voiceCountUsingModel(const SamplerRenderModel* model) const noexcept;
    std::size_t voiceCountUsingGeneration(std::uint64_t activationGeneration) const noexcept;
    std::size_t retiredGenerationVoiceCount() const noexcept;
    std::size_t sustainDeferredVoiceCount() const noexcept;
    std::uint64_t getActiveGeneration() const noexcept { return activeGeneration; }
    SamplerVoiceSlotSnapshot getSlotSnapshot(std::size_t index) const noexcept;

private:
    struct RoundRobinPoolKey
    {
        std::string_view poolId;
        int slotCount = 0;
        bool usesLegacyScalarKey = false;
        RoundRobinMode mode = RoundRobinMode::sequential;
    };

    struct RoundRobinPoolState
    {
        RoundRobinPoolKey key;
        std::uint64_t stableId = 0;
        std::uint64_t initialRandomState = 0;
        int nextSlotIndex = 1;
        std::uint64_t randomState = 0;
    };

    static constexpr std::size_t roundRobinPoolCapacity = 256;

    struct Slot
    {
        SamplerVoice voice;
        SamplerVoiceSlotState state = SamplerVoiceSlotState::free;
        bool sustainDeferred = false;
    };

    void renderRange(SamplerAudioBufferView output,
                     std::uint32_t startFrame,
                     std::uint32_t frameCount,
                     SamplerVoicePoolRenderResult& result,
                     const SamplerAudioBufferView* routeTargets,
                     std::size_t routeTargetCount) noexcept;
    void applyEvent(const SamplerRenderEvent& event,
                    SamplerVoicePoolRenderResult& result,
                    const SamplerRenderControlValues& controls) noexcept;
    void applyChokes(const SamplerRenderRoute& sourceRoute,
                     SamplerVoicePoolRenderResult& result,
                     std::uint64_t preEventVoiceIdLimit) noexcept;
    void resetRoundRobinPools() noexcept;
    void resetRoundRobinPool(RoundRobinPoolState& pool) noexcept;
    void applyRoundRobinResets(RoundRobinResetEvent event) noexcept;
    void rebuildRoundRobinPools(const SamplerRenderModel& model) noexcept;
    bool peekRoundRobinSlot(std::string_view poolId,
                            int slotCount,
                            bool usesLegacyScalarKey,
                            RoundRobinMode mode,
                            int& slotIndex) const noexcept;
    bool advanceRoundRobinSlot(std::string_view poolId,
                               int slotCount,
                               bool usesLegacyScalarKey,
                               RoundRobinMode mode) noexcept;
    std::size_t acquireSlot(bool& stolen,
                            bool& generationStolen,
                            bool& releasingStolen) noexcept;
    void updateCounts(SamplerVoicePoolRenderResult& result) const noexcept;

    std::array<Slot, capacity> slots {};
    std::array<RoundRobinPoolState, roundRobinPoolCapacity> roundRobinPools {};
    std::size_t roundRobinPoolCount = 0;
    const SamplerRenderModel* renderModel = nullptr;
    double sampleRate = 0.0;
    std::uint64_t nextVoiceId = 1;
    std::uint64_t nextTriggerId = 1;
    std::uint64_t activeGeneration = 0;
    std::uint64_t nextGeneratedActivation = 1;
    bool sustainPedalDown = false;
    std::array<std::uint8_t, 128> controllerValues {};
};
} // namespace drs::engine
