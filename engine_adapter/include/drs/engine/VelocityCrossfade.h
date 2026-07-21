#pragma once

#include <stddef.h>
#include <stdint.h>

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
} // namespace drs::engine
