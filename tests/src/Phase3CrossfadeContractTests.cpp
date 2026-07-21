#include "drs/engine/VelocityCrossfade.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

drs::engine::VelocityCrossfadeZoneDefinition makeZone(int velocityLow,
                                                      int velocityHigh,
                                                      int fadeInLow = 0,
                                                      int fadeInHigh = 0,
                                                      int fadeOutLow = 0,
                                                      int fadeOutHigh = 0)
{
    drs::engine::VelocityCrossfadeZoneDefinition zone;
    zone.velocityLow = velocityLow;
    zone.velocityHigh = velocityHigh;
    zone.crossfade.fadeInLowVelocity = fadeInLow;
    zone.crossfade.fadeInHighVelocity = fadeInHigh;
    zone.crossfade.fadeOutLowVelocity = fadeOutLow;
    zone.crossfade.fadeOutHighVelocity = fadeOutHigh;
    return zone;
}

std::size_t countActiveLayers(const std::vector<drs::engine::VelocityCrossfadeZoneDefinition>& zones,
                              int velocity)
{
    std::size_t count = 0;
    for (const auto& zone : zones)
        if (drs::engine::participatesInFirstPassVelocityCrossfade(zone, velocity))
            ++count;
    return count;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        static_assert(VelocityCrossfadeFirstPassFixtureCharacterization::expectedLayerCount == 5,
                      "Sprint 1 fixture layer count changed unexpectedly.");
        static_assert(VelocityCrossfadeFirstPassFixtureCharacterization::expectedOverlapCount == 4,
                      "Sprint 1 overlap count changed unexpectedly.");

        const std::vector<VelocityCrossfadeZoneDefinition> rhodesLayers {
            makeZone(1, 60, 0, 0, 25, 60),
            makeZone(25, 84, 25, 60, 61, 84),
            makeZone(61, 103, 61, 84, 85, 103),
            makeZone(85, 119, 85, 103, 104, 119),
            makeZone(104, 127, 104, 119)
        };

        for (const auto& zone : rhodesLayers)
            require(validateFirstPassVelocityCrossfadeZone(zone) == VelocityCrossfadeZoneIssue::none,
                    "The canonical Rhodes-style zone shape must remain supported.");

        for (std::size_t index = 0; index + 1 < rhodesLayers.size(); ++index)
            require(validateFirstPassVelocityCrossfadePair(rhodesLayers[index], rhodesLayers[index + 1])
                        == VelocityCrossfadePairIssue::none,
                    "Adjacent Rhodes-style overlap pairs must remain supported.");

        require(effectiveVelocityLow(rhodesLayers[1]) == 25 && effectiveVelocityHigh(rhodesLayers[1]) == 84,
                "Effective audible window should be anchored by fade endpoints.");

        require(countActiveLayers(rhodesLayers, 24) == 1,
                "Velocities before the first overlap should trigger exactly one layer.");
        require(countActiveLayers(rhodesLayers, 25) == 1,
                "The fade-start velocity should stay owned by the outgoing layer only.");
        require(countActiveLayers(rhodesLayers, 26) == 2,
                "Interior overlap velocities should start two adjacent layers.");
        require(countActiveLayers(rhodesLayers, 60) == 1,
                "The fade-end velocity should stay owned by the incoming layer only.");
        require(countActiveLayers(rhodesLayers, 61) == 1,
                "The next fade-start velocity should collapse back to one layer.");
        require(countActiveLayers(rhodesLayers, 70) == 2,
                "Later interior overlaps should still resolve to two adjacent layers.");
        require(countActiveLayers(rhodesLayers, 127) == 1,
                "The top layer should own the terminal velocity.");

        const auto lowerGainAtBoundary = computeFirstPassVelocityCrossfadeGain(rhodesLayers.front(), 25);
        const auto upperGainAtBoundary = computeFirstPassVelocityCrossfadeGain(rhodesLayers[1], 25);
        require(std::abs(lowerGainAtBoundary - 1.0) < 1.0e-9 && upperGainAtBoundary == 0.0,
                "Fade-start ownership must not start a zero-gain incoming layer.");

        const auto lowerMidGain = computeFirstPassVelocityCrossfadeGain(rhodesLayers.front(), 43);
        const auto upperMidGain = computeFirstPassVelocityCrossfadeGain(rhodesLayers[1], 43);
        require(std::abs((lowerMidGain + upperMidGain) - 1.0) < 1.0e-9,
                "Mirrored linear overlaps must sum to unity at the midpoint.");
        require(lowerMidGain > 0.0 && upperMidGain > 0.0,
                "Interior overlap velocities must keep both adjacent layers active.");

        require(computeFirstPassVelocityCrossfadeGain(rhodesLayers.front(), 61) == 0.0,
                "The outgoing layer must be silent after its fade-out endpoint.");
        require(std::abs(computeFirstPassVelocityCrossfadeGain(rhodesLayers[1], 60) - 1.0) < 1.0e-9,
                "The incoming layer must fully own the fade end.");

        const auto invalidPartialFade = makeZone(25, 84, 25, 0, 61, 84);
        require(validateFirstPassVelocityCrossfadeZone(invalidPartialFade) == VelocityCrossfadeZoneIssue::fadeInPartial,
                "Partial fade-in metadata must remain unsupported.");

        const auto invalidInvertedFade = makeZone(25, 84, 25, 60, 84, 84);
        require(validateFirstPassVelocityCrossfadeZone(invalidInvertedFade)
                    == VelocityCrossfadeZoneIssue::fadeOutInverted,
                "Inverted fade-out metadata must remain unsupported.");

        const auto invalidOverlappingZone = makeZone(25, 84, 25, 61, 61, 84);
        require(validateFirstPassVelocityCrossfadeZone(invalidOverlappingZone)
                    == VelocityCrossfadeZoneIssue::fadeWindowsOverlap,
                "A zone whose fade-in and fade-out touch or overlap must remain unsupported.");

        const auto mismatchedUpper = makeZone(26, 84, 26, 60, 61, 84);
        require(validateFirstPassVelocityCrossfadePair(rhodesLayers.front(), mismatchedUpper)
                    == VelocityCrossfadePairIssue::overlapMismatch,
                "Asymmetric overlap intervals must remain unsupported in the first release.");

        std::cout << "Phase 3 crossfade Sprint 1 contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 3 crossfade Sprint 1 contract tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
