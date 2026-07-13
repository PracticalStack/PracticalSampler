#include "drs/engine/RuntimeLoadProfile.h"

#include <algorithm>

namespace drs::engine
{
std::vector<RuntimeLoadProfileDefinition> getPhase1RuntimeLoadProfiles()
{
    return {
        {
            "eco",
            "Eco",
            8192,
            2,
            "Minimizes prefetch and cache residency so dormant content yields memory quickly."
        },
        {
            "balanced",
            "Balanced",
            16384,
            4,
            "Default Phase 1 profile that keeps the reference instrument responsive without hoarding pages."
        },
        {
            "performance",
            "Performance",
            32768,
            8,
            "Favors larger resident working sets so active playback is less likely to wait on follow-up pages."
        }
    };
}

std::optional<RuntimeLoadProfileDefinition> findPhase1RuntimeLoadProfile(const std::string& id)
{
    const auto profiles = getPhase1RuntimeLoadProfiles();
    const auto iterator = std::find_if(profiles.begin(),
                                       profiles.end(),
                                       [&](const RuntimeLoadProfileDefinition& profile)
                                       {
                                           return profile.id == id;
                                       });

    if (iterator == profiles.end())
        return std::nullopt;

    return *iterator;
}

RuntimeStreamingServiceOptions buildRuntimeStreamingServiceOptions(const RuntimeLoadProfileDefinition& profile,
                                                                  std::uint64_t simulatedReadLatencyMicros)
{
    RuntimeStreamingServiceOptions options;
    options.loadProfileId = profile.id;
    options.maxCachedPages = profile.maxCachedPages;
    options.simulatedReadLatencyMicros = simulatedReadLatencyMicros;
    return options;
}

std::uint64_t clampPrefetchBytesForLoadProfile(const RuntimeLoadProfileDefinition& profile,
                                               std::uint64_t requestedPrefetchBytes,
                                               std::uint64_t availablePrefetchBytes)
{
    return std::min({ requestedPrefetchBytes,
                      availablePrefetchBytes,
                      profile.maxPrefetchBytesPerVoice });
}
} // namespace drs::engine
