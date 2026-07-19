#pragma once

#include "drs/engine/SamplerVoice.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace drs::engine
{
class SamplerEventBlock final
{
public:
    static constexpr std::size_t capacity = 128;

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
    int sourceMidiNote = 0;
    int effectiveMidiNote = 0;
};

struct SamplerVoicePoolRenderResult
{
    bool accepted = false;
    SamplerRenderResult render;
    std::uint32_t activeVoiceCount = 0;
    std::uint32_t releasingVoiceCount = 0;
    std::uint32_t finishedVoiceCount = 0;
    std::uint32_t resetVoiceCount = 0;
};

class SamplerVoicePool final
{
public:
    static constexpr std::size_t capacity = 24;

    bool prepare(const SamplerRenderModel& model, double outputSampleRate) noexcept;
    bool activateModel(const SamplerRenderModel& model, double outputSampleRate) noexcept;
    void clearRenderModel() noexcept;
    SamplerVoicePoolRenderResult renderBlock(SamplerAudioBufferView output,
                                             SamplerRenderEventView events) noexcept;
    void resetVoices() noexcept;

    std::size_t activeVoiceCount() const noexcept;
    std::size_t releasingVoiceCount() const noexcept;
    std::size_t finishedVoiceCount() const noexcept;
    std::size_t voiceCountUsingModel(const SamplerRenderModel* model) const noexcept;
    SamplerVoiceSlotSnapshot getSlotSnapshot(std::size_t index) const noexcept;

private:
    struct Slot
    {
        SamplerVoice voice;
        SamplerVoiceSlotState state = SamplerVoiceSlotState::free;
    };

    void renderRange(SamplerAudioBufferView output,
                     std::uint32_t startFrame,
                     std::uint32_t frameCount,
                     SamplerVoicePoolRenderResult& result) noexcept;
    void applyEvent(const SamplerRenderEvent& event,
                    SamplerVoicePoolRenderResult& result) noexcept;
    std::size_t selectRouteIndex(int midiNote, int velocity) const noexcept;
    std::size_t acquireSlot(bool& stolen) noexcept;
    void updateCounts(SamplerVoicePoolRenderResult& result) const noexcept;

    std::array<Slot, capacity> slots {};
    const SamplerRenderModel* renderModel = nullptr;
    double sampleRate = 0.0;
    std::uint64_t nextVoiceId = 1;
};
} // namespace drs::engine
