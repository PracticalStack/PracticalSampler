#pragma once

#include "drs/engine/RuntimeStreamingService.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace drs::engine
{
struct RuntimeLoadProfileDefinition
{
    std::string id;
    std::string displayName;
    std::uint64_t maxPrefetchBytesPerVoice = 0;
    std::size_t maxCachedPages = 0;
    std::string summary;
};

std::vector<RuntimeLoadProfileDefinition> getPhase1RuntimeLoadProfiles();
std::optional<RuntimeLoadProfileDefinition> findPhase1RuntimeLoadProfile(const std::string& id);
RuntimeStreamingServiceOptions buildRuntimeStreamingServiceOptions(const RuntimeLoadProfileDefinition& profile,
                                                                  std::uint64_t simulatedReadLatencyMicros = 0);
std::uint64_t clampPrefetchBytesForLoadProfile(const RuntimeLoadProfileDefinition& profile,
                                               std::uint64_t requestedPrefetchBytes,
                                               std::uint64_t availablePrefetchBytes);
} // namespace drs::engine
