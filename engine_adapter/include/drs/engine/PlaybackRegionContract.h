#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace drs::engine
{
// Sprint 4 introduces the authored exclusive playback-end boundary while
// preserving zero as the historical "physical source end" sentinel.
inline constexpr int sfzRegionPlaybackContractSchemaVersion = 2;
inline constexpr int playbackRegionProjectSchemaVersion = 8;
inline constexpr int playbackRegionAuthoringSchemaVersion = 7;
inline constexpr int playbackRegionInstrumentSchemaVersion = 6;
// Phase 6 carries the complete typed SFZ region contract through compiled
// instruments and playable packages. Schema v6 only introduced sampleEndFrame;
// v7 adds loop mode and its half-open loop range without changing old readers.
inline constexpr int sfzRegionInstrumentSchemaVersion = 7;
inline constexpr int loopCrossfadeProjectSchemaVersion = 9;
inline constexpr int loopCrossfadeAuthoringSchemaVersion = 8;
inline constexpr int loopCrossfadeInstrumentSchemaVersion = 8;

struct PlaybackRegionPrewarmPlan
{
    std::array<std::uint64_t, 4> frames {};
    std::size_t count = 0;
};

inline PlaybackRegionPrewarmPlan buildPlaybackRegionPrewarmPlan(
    const std::uint64_t sampleStartFrame,
    const bool loopEnabled,
    const std::uint64_t loopStartFrame,
    const std::uint64_t loopEndFrame,
    const std::uint64_t loopCrossfadeFrames) noexcept
{
    PlaybackRegionPrewarmPlan plan;
    const auto appendUnique = [&](const std::uint64_t frame) noexcept
    {
        for (std::size_t index = 0; index < plan.count; ++index)
            if (plan.frames[index] == frame)
                return;
        if (plan.count < plan.frames.size())
            plan.frames[plan.count++] = frame;
    };

    appendUnique(sampleStartFrame);
    if (loopEnabled && loopStartFrame < loopEndFrame)
    {
        appendUnique(loopStartFrame);
        if (loopCrossfadeFrames != 0 && loopCrossfadeFrames <= loopEndFrame - loopStartFrame)
            appendUnique(loopEndFrame - loopCrossfadeFrames);
        appendUnique(loopEndFrame - 1);
    }
    return plan;
}
} // namespace drs::engine
