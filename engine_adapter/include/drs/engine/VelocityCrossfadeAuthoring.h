#pragma once

#include "drs/engine/ProjectDocument.h"

#include <string>
#include <vector>

namespace drs::engine
{
enum class VelocityCrossfadeDirection : std::uint8_t
{
    fadeIn,
    fadeOut
};

// These states are deliberately author-facing.  They distinguish a selection
// problem from a topology problem so later UI can explain why an action is
// unavailable without reproducing playback's matching rules.
enum class VelocityCrossfadeAuthoringState : std::uint8_t
{
    eligible,
    noChanges,
    missingPartner,
    ambiguousPartner,
    incompatibleMapping,
    incompleteRoundRobinPool,
    mixedRoundRobinSlotCount,
    duplicateRoundRobinSlot,
    invalidOverlap,
    invalidExistingCrossfade,
    invalidTopology
};

struct VelocityCrossfadePartnerDiscovery
{
    VelocityCrossfadeAuthoringState state = VelocityCrossfadeAuthoringState::missingPartner;
    std::string anchorZoneId;
    VelocityCrossfadeDirection direction = VelocityCrossfadeDirection::fadeOut;
    std::vector<std::string> partnerZoneIds;
    std::vector<std::string> blockingIssues;

    bool eligible() const noexcept { return state == VelocityCrossfadeAuthoringState::eligible; }
};

struct VelocityCrossfadePairRequest
{
    std::string lowerZoneId;
    std::string upperZoneId;
    int overlapLowVelocity = 0;
    int overlapHighVelocity = 0;
};

struct VelocityCrossfadeAuthoringPlan
{
    VelocityCrossfadeAuthoringState state = VelocityCrossfadeAuthoringState::invalidTopology;
    RuntimeProjectModel proposedProject;
    std::vector<std::string> affectedZoneIds;
    std::vector<std::string> warnings;
    std::vector<std::string> blockingIssues;

    bool valid() const noexcept
    {
        return state == VelocityCrossfadeAuthoringState::eligible
            || state == VelocityCrossfadeAuthoringState::noChanges;
    }

    bool changesProject() const noexcept { return state == VelocityCrossfadeAuthoringState::eligible; }
};

VelocityCrossfadePartnerDiscovery discoverVelocityCrossfadePartner(
    const RuntimeProjectModel& project,
    const std::string& anchorZoneId,
    VelocityCrossfadeDirection direction);

VelocityCrossfadeAuthoringPlan planVelocityCrossfadePair(
    const RuntimeProjectModel& project,
    const VelocityCrossfadePairRequest& request);

VelocityCrossfadeAuthoringPlan planVelocityCrossfadeRemoval(
    const RuntimeProjectModel& project,
    const std::string& lowerZoneId,
    const std::string& upperZoneId);
} // namespace drs::engine
