#pragma once

#include <cstddef>

namespace drs::engine
{
// HP-01 contract constants. These reserve the persistence and realtime policy
// for later slices; HP-01 does not emit the reserved schemas or change playback.
inline constexpr int continuousDamperProjectSchemaVersion = 7;
inline constexpr int continuousDamperAuthoringSchemaVersion = 6;
inline constexpr int continuousDamperInstrumentSchemaVersion = 5;

inline constexpr int legacySustainControllerNumber = 64;
inline constexpr double legacySustainThreshold = 64.0;
inline constexpr int sfzDefaultSustainControllerNumber = 64;
inline constexpr double sfzDefaultSustainThreshold = 0.5;

inline constexpr int halfPedalReleaseControllerNumber = 64;
inline constexpr std::size_t continuousDamperCurvePointCount = 128;
inline constexpr double minimumDynamicReleaseSeconds = 0.001;
inline constexpr double maximumDynamicReleaseSeconds = 100.0;
} // namespace drs::engine
