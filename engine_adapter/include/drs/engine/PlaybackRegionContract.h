#pragma once

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
} // namespace drs::engine
