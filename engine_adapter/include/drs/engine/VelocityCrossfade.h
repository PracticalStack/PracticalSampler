#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

namespace drs::engine
{
enum class VelocityCrossfadeCurve : uint8_t
{
    linear = 0
};

struct VelocityCrossfadeDescriptor
{
    int fadeInLowVelocity = 0;
    int fadeInHighVelocity = 0;
    int fadeOutLowVelocity = 0;
    int fadeOutHighVelocity = 0;
    VelocityCrossfadeCurve curve = VelocityCrossfadeCurve::linear;
};

struct VelocityCrossfadeZoneDefinition
{
    int velocityLow = 1;
    int velocityHigh = 127;
    VelocityCrossfadeDescriptor crossfade;
};

struct VelocityCrossfadeRuntimeDescriptor
{
    int effectiveLowVelocity = 0;
    int effectiveHighVelocity = 0;
    std::string fadeInNeighborZoneId;
    std::string fadeOutNeighborZoneId;
    int fadeInOverlapLowVelocity = 0;
    int fadeInOverlapHighVelocity = 0;
    int fadeOutOverlapLowVelocity = 0;
    int fadeOutOverlapHighVelocity = 0;
};

enum class VelocityCrossfadeZoneIssue : uint8_t
{
    none = 0,
    velocityRangeInvalid,
    unsupportedCurve,
    fadeInPartial,
    fadeInOutOfRange,
    fadeInInverted,
    fadeOutPartial,
    fadeOutOutOfRange,
    fadeOutInverted,
    fadeWindowsOverlap
};

enum class VelocityCrossfadePairIssue : uint8_t
{
    none = 0,
    lowerZoneUnsupported,
    upperZoneUnsupported,
    overlapMismatch
};

enum class VelocityCrossfadeTopologyIssue : uint8_t
{
    none = 0,
    fadeInMissingPartner,
    fadeInAmbiguousPartner,
    fadeOutMissingPartner,
    fadeOutAmbiguousPartner
};

struct VelocityCrossfadeTopologyZoneDefinition
{
    uint64_t pairingKey = 0;
    int velocityLow = 1;
    int velocityHigh = 127;
    int roundRobinLength = 0;
    int roundRobinPosition = 0;
    VelocityCrossfadeDescriptor crossfade;
};

struct VelocityCrossfadeRuntimeTopology
{
    int effectiveLowVelocity = 0;
    int effectiveHighVelocity = 0;
    int fadeInNeighborZoneIndex = -1;
    int fadeOutNeighborZoneIndex = -1;
    int fadeInOverlapLowVelocity = 0;
    int fadeInOverlapHighVelocity = 0;
    int fadeOutOverlapLowVelocity = 0;
    int fadeOutOverlapHighVelocity = 0;
};

struct VelocityCrossfadeTopologyFinding
{
    size_t zoneIndex = 0;
    VelocityCrossfadeTopologyIssue issue = VelocityCrossfadeTopologyIssue::none;
};

struct VelocityCrossfadeFirstPassFixtureCharacterization
{
    static constexpr size_t expectedLayerCount = 5;
    static constexpr size_t expectedOverlapCount = 4;

    static constexpr int expectedOverlapLowVelocity(size_t index) noexcept
    {
        return index == 0 ? 25
            : index == 1 ? 61
            : index == 2 ? 85
                         : 104;
    }

    static constexpr int expectedOverlapHighVelocity(size_t index) noexcept
    {
        return index == 0 ? 60
            : index == 1 ? 84
            : index == 2 ? 103
                         : 119;
    }

    static constexpr int expectedControlVelocityLow(size_t index) noexcept
    {
        return index == 0 ? 1
            : index == 1 ? 48
            : index == 2 ? 73
            : index == 3 ? 96
                         : 112;
    }

    static constexpr int expectedControlVelocityHigh(size_t index) noexcept
    {
        return index == 0 ? 47
            : index == 1 ? 72
            : index == 2 ? 95
            : index == 3 ? 111
                         : 127;
    }
};

constexpr bool isVelocityValueValid(int velocity) noexcept
{
    return velocity >= 1 && velocity <= 127;
}

constexpr bool isVelocityRangeValid(int low, int high) noexcept
{
    return isVelocityValueValid(low) && isVelocityValueValid(high) && low <= high;
}

constexpr bool hasFadeIn(const VelocityCrossfadeDescriptor& descriptor) noexcept
{
    return descriptor.fadeInLowVelocity > 0 || descriptor.fadeInHighVelocity > 0;
}

constexpr bool hasFadeOut(const VelocityCrossfadeDescriptor& descriptor) noexcept
{
    return descriptor.fadeOutLowVelocity > 0 || descriptor.fadeOutHighVelocity > 0;
}

constexpr bool hasAnyVelocityCrossfadeValue(const VelocityCrossfadeDescriptor& descriptor) noexcept
{
    return hasFadeIn(descriptor) || hasFadeOut(descriptor);
}

inline bool hasAnyVelocityCrossfadeRuntimeValue(const VelocityCrossfadeRuntimeDescriptor& descriptor) noexcept
{
    return descriptor.effectiveLowVelocity > 0
        || descriptor.effectiveHighVelocity > 0
        || !descriptor.fadeInNeighborZoneId.empty()
        || !descriptor.fadeOutNeighborZoneId.empty()
        || descriptor.fadeInOverlapLowVelocity > 0
        || descriptor.fadeInOverlapHighVelocity > 0
        || descriptor.fadeOutOverlapLowVelocity > 0
        || descriptor.fadeOutOverlapHighVelocity > 0;
}

constexpr bool hasCompleteFadeIn(const VelocityCrossfadeDescriptor& descriptor) noexcept
{
    return descriptor.fadeInLowVelocity > 0 && descriptor.fadeInHighVelocity > 0;
}

constexpr bool hasCompleteFadeOut(const VelocityCrossfadeDescriptor& descriptor) noexcept
{
    return descriptor.fadeOutLowVelocity > 0 && descriptor.fadeOutHighVelocity > 0;
}

constexpr int effectiveVelocityLow(const VelocityCrossfadeZoneDefinition& zone) noexcept
{
    return hasCompleteFadeIn(zone.crossfade) ? zone.crossfade.fadeInLowVelocity : zone.velocityLow;
}

constexpr int effectiveVelocityHigh(const VelocityCrossfadeZoneDefinition& zone) noexcept
{
    return hasCompleteFadeOut(zone.crossfade) ? zone.crossfade.fadeOutHighVelocity : zone.velocityHigh;
}

constexpr VelocityCrossfadeZoneIssue validateFirstPassVelocityCrossfadeZone(
    const VelocityCrossfadeZoneDefinition& zone) noexcept
{
    if (!isVelocityRangeValid(zone.velocityLow, zone.velocityHigh))
        return VelocityCrossfadeZoneIssue::velocityRangeInvalid;

    if (zone.crossfade.curve != VelocityCrossfadeCurve::linear)
        return VelocityCrossfadeZoneIssue::unsupportedCurve;

    if (hasFadeIn(zone.crossfade) && !hasCompleteFadeIn(zone.crossfade))
        return VelocityCrossfadeZoneIssue::fadeInPartial;

    if (hasFadeOut(zone.crossfade) && !hasCompleteFadeOut(zone.crossfade))
        return VelocityCrossfadeZoneIssue::fadeOutPartial;

    if (hasCompleteFadeIn(zone.crossfade))
    {
        if (!isVelocityValueValid(zone.crossfade.fadeInLowVelocity)
            || !isVelocityValueValid(zone.crossfade.fadeInHighVelocity)
            || zone.crossfade.fadeInLowVelocity != zone.velocityLow)
            return VelocityCrossfadeZoneIssue::fadeInOutOfRange;

        if (zone.crossfade.fadeInLowVelocity >= zone.crossfade.fadeInHighVelocity)
            return VelocityCrossfadeZoneIssue::fadeInInverted;

        if (zone.crossfade.fadeInHighVelocity > zone.velocityHigh)
            return VelocityCrossfadeZoneIssue::fadeInOutOfRange;
    }

    if (hasCompleteFadeOut(zone.crossfade))
    {
        if (!isVelocityValueValid(zone.crossfade.fadeOutLowVelocity)
            || !isVelocityValueValid(zone.crossfade.fadeOutHighVelocity)
            || zone.crossfade.fadeOutHighVelocity != zone.velocityHigh)
            return VelocityCrossfadeZoneIssue::fadeOutOutOfRange;

        if (zone.crossfade.fadeOutLowVelocity >= zone.crossfade.fadeOutHighVelocity)
            return VelocityCrossfadeZoneIssue::fadeOutInverted;

        if (zone.crossfade.fadeOutLowVelocity < zone.velocityLow)
            return VelocityCrossfadeZoneIssue::fadeOutOutOfRange;
    }

    if (hasCompleteFadeIn(zone.crossfade)
        && hasCompleteFadeOut(zone.crossfade)
        && zone.crossfade.fadeInHighVelocity >= zone.crossfade.fadeOutLowVelocity)
        return VelocityCrossfadeZoneIssue::fadeWindowsOverlap;

    return VelocityCrossfadeZoneIssue::none;
}

constexpr VelocityCrossfadePairIssue validateFirstPassVelocityCrossfadePair(
    const VelocityCrossfadeZoneDefinition& lower,
    const VelocityCrossfadeZoneDefinition& upper) noexcept
{
    if (validateFirstPassVelocityCrossfadeZone(lower) != VelocityCrossfadeZoneIssue::none)
        return VelocityCrossfadePairIssue::lowerZoneUnsupported;

    if (validateFirstPassVelocityCrossfadeZone(upper) != VelocityCrossfadeZoneIssue::none)
        return VelocityCrossfadePairIssue::upperZoneUnsupported;

    if (!hasCompleteFadeOut(lower.crossfade)
        || !hasCompleteFadeIn(upper.crossfade)
        || lower.crossfade.fadeOutLowVelocity != upper.crossfade.fadeInLowVelocity
        || lower.crossfade.fadeOutHighVelocity != upper.crossfade.fadeInHighVelocity)
        return VelocityCrossfadePairIssue::overlapMismatch;

    return VelocityCrossfadePairIssue::none;
}

inline double computeFirstPassVelocityCrossfadeGain(const VelocityCrossfadeZoneDefinition& zone,
                                                    int velocity) noexcept
{
    if (validateFirstPassVelocityCrossfadeZone(zone) != VelocityCrossfadeZoneIssue::none)
        return 0.0;

    if (velocity < zone.velocityLow || velocity > zone.velocityHigh)
        return 0.0;

    auto gain = 1.0;

    if (hasCompleteFadeIn(zone.crossfade))
    {
        if (velocity <= zone.crossfade.fadeInLowVelocity)
            gain = 0.0;
        else if (velocity < zone.crossfade.fadeInHighVelocity)
        {
            const auto fadeInGain =
                static_cast<double>(velocity - zone.crossfade.fadeInLowVelocity)
                / static_cast<double>(zone.crossfade.fadeInHighVelocity
                                      - zone.crossfade.fadeInLowVelocity);
            gain = fadeInGain < gain ? fadeInGain : gain;
        }
    }

    if (hasCompleteFadeOut(zone.crossfade))
    {
        if (velocity >= zone.crossfade.fadeOutHighVelocity)
            gain = 0.0;
        else if (velocity > zone.crossfade.fadeOutLowVelocity)
        {
            const auto fadeOutGain =
                static_cast<double>(zone.crossfade.fadeOutHighVelocity - velocity)
                / static_cast<double>(zone.crossfade.fadeOutHighVelocity
                                      - zone.crossfade.fadeOutLowVelocity);
            gain = fadeOutGain < gain ? fadeOutGain : gain;
        }
    }

    if (gain < 0.0)
        return 0.0;
    if (gain > 1.0)
        return 1.0;
    return gain;
}

inline bool participatesInFirstPassVelocityCrossfade(const VelocityCrossfadeZoneDefinition& zone,
                                                     int velocity) noexcept
{
    return computeFirstPassVelocityCrossfadeGain(zone, velocity) > 0.0;
}

inline std::vector<VelocityCrossfadeRuntimeTopology> buildFirstPassVelocityCrossfadeRuntimeTopology(
    const std::vector<VelocityCrossfadeTopologyZoneDefinition>& zones,
    std::vector<VelocityCrossfadeTopologyFinding>* findings = nullptr)
{
    std::vector<VelocityCrossfadeRuntimeTopology> topology(zones.size());

    for (size_t zoneIndex = 0; zoneIndex < zones.size(); ++zoneIndex)
    {
        const auto& zone = zones[zoneIndex];
        auto& runtime = topology[zoneIndex];
        if (!hasAnyVelocityCrossfadeValue(zone.crossfade))
            continue;

        const VelocityCrossfadeZoneDefinition validationZone { zone.velocityLow, zone.velocityHigh, zone.crossfade };
        runtime.effectiveLowVelocity = effectiveVelocityLow(validationZone);
        runtime.effectiveHighVelocity = effectiveVelocityHigh(validationZone);

        if (hasCompleteFadeIn(zone.crossfade))
        {
            std::vector<size_t> candidates;
            for (size_t candidateIndex = 0; candidateIndex < zones.size(); ++candidateIndex)
            {
                if (candidateIndex == zoneIndex)
                    continue;

                const auto& candidate = zones[candidateIndex];
                if (candidate.pairingKey != zone.pairingKey
                    || candidate.roundRobinLength != zone.roundRobinLength
                    || candidate.roundRobinPosition != zone.roundRobinPosition)
                {
                    continue;
                }

                const VelocityCrossfadeZoneDefinition lower {
                    candidate.velocityLow,
                    candidate.velocityHigh,
                    candidate.crossfade
                };
                if (validateFirstPassVelocityCrossfadePair(lower, validationZone)
                    == VelocityCrossfadePairIssue::none)
                {
                    candidates.push_back(candidateIndex);
                }
            }

            if (candidates.size() == 1)
            {
                runtime.fadeInNeighborZoneIndex = static_cast<int>(candidates.front());
                runtime.fadeInOverlapLowVelocity = zone.crossfade.fadeInLowVelocity;
                runtime.fadeInOverlapHighVelocity = zone.crossfade.fadeInHighVelocity;
            }
            else if (findings != nullptr)
            {
                findings->push_back({
                    zoneIndex,
                    candidates.empty()
                        ? VelocityCrossfadeTopologyIssue::fadeInMissingPartner
                        : VelocityCrossfadeTopologyIssue::fadeInAmbiguousPartner
                });
            }
        }

        if (hasCompleteFadeOut(zone.crossfade))
        {
            std::vector<size_t> candidates;
            for (size_t candidateIndex = 0; candidateIndex < zones.size(); ++candidateIndex)
            {
                if (candidateIndex == zoneIndex)
                    continue;

                const auto& candidate = zones[candidateIndex];
                if (candidate.pairingKey != zone.pairingKey
                    || candidate.roundRobinLength != zone.roundRobinLength
                    || candidate.roundRobinPosition != zone.roundRobinPosition)
                {
                    continue;
                }

                const VelocityCrossfadeZoneDefinition upper {
                    candidate.velocityLow,
                    candidate.velocityHigh,
                    candidate.crossfade
                };
                if (validateFirstPassVelocityCrossfadePair(validationZone, upper)
                    == VelocityCrossfadePairIssue::none)
                {
                    candidates.push_back(candidateIndex);
                }
            }

            if (candidates.size() == 1)
            {
                runtime.fadeOutNeighborZoneIndex = static_cast<int>(candidates.front());
                runtime.fadeOutOverlapLowVelocity = zone.crossfade.fadeOutLowVelocity;
                runtime.fadeOutOverlapHighVelocity = zone.crossfade.fadeOutHighVelocity;
            }
            else if (findings != nullptr)
            {
                findings->push_back({
                    zoneIndex,
                    candidates.empty()
                        ? VelocityCrossfadeTopologyIssue::fadeOutMissingPartner
                        : VelocityCrossfadeTopologyIssue::fadeOutAmbiguousPartner
                });
            }
        }
    }

    return topology;
}
} // namespace drs::engine
