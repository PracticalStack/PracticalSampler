#pragma once

#include "drs/engine/SamplerPlaybackContext.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace drs::tests
{
inline constexpr double offlineSampleTolerance = 1.0e-6;
inline constexpr double offlineSummaryTolerance = 1.0e-8;
inline constexpr double offlineChecksumQuantum = 1.0e-7;

struct OfflineTimelineEvent
{
    std::uint64_t frame = 0;
    engine::SamplerRenderEventType type = engine::SamplerRenderEventType::noteOn;
    std::uint8_t midiNote = 60;
    float velocity = 1.0f;
};

struct OfflineRenderRequest
{
    std::string scenarioId;
    engine::SamplerRenderModelPtr model;
    double sampleRate = 48000.0;
    std::uint64_t frameCount = 0;
    std::uint32_t partitionSize = 64;
    std::vector<OfflineTimelineEvent> events;
};

struct OfflineRenderSummary
{
    std::uint64_t frameCount = 0;
    std::string quantizedChecksum;
    double peak = 0.0;
    double rms = 0.0;
    std::int64_t firstNonZeroFrame = -1;
    std::int64_t lastNonZeroFrame = -1;
    engine::SamplerPlaybackContextCounters counters;
    std::uint32_t activeVoiceCount = 0;
    std::uint32_t releasingVoiceCount = 0;
    std::uint32_t finishedVoiceCount = 0;
};

struct OfflineRenderArtifact
{
    std::string scenarioId;
    std::uint32_t partitionSize = 0;
    std::array<std::vector<float>, 2> channels;
    OfflineRenderSummary summary;
};

struct OfflineArtifactComparison
{
    bool equivalent = false;
    std::string message;
    std::size_t channel = 0;
    std::uint64_t frame = 0;
    double expected = 0.0;
    double actual = 0.0;
};

OfflineRenderArtifact renderOffline(const OfflineRenderRequest& request);
OfflineArtifactComparison compareOfflineArtifacts(const OfflineRenderArtifact& expected,
                                                   const OfflineRenderArtifact& actual,
                                                   double sampleTolerance = offlineSampleTolerance);
std::string serializeOfflineArtifactJson(const OfflineRenderArtifact& artifact);
void writeOfflineMismatchArtifacts(const std::filesystem::path& outputDirectory,
                                   const OfflineRenderArtifact& expected,
                                   const OfflineRenderArtifact& actual,
                                   const OfflineArtifactComparison& comparison);
} // namespace drs::tests
