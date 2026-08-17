#pragma once

namespace drs::engine
{
// Sprint 4 introduces the authored exclusive playback-end boundary while
// preserving zero as the historical "physical source end" sentinel.
inline constexpr int sfzRegionPlaybackContractSchemaVersion = 2;
inline constexpr int playbackRegionProjectSchemaVersion = 8;
inline constexpr int playbackRegionAuthoringSchemaVersion = 7;
inline constexpr int playbackRegionInstrumentSchemaVersion = 6;
} // namespace drs::engine
