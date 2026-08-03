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
    incompleteLayerStack,
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

// A stack is planned as one topology operation.  This deliberately avoids
// composing pair operations: once three or more compatible layers exist, a
// pair-wise partner lookup is intentionally ambiguous.
struct VelocityCrossfadeStackRequest
{
    std::vector<std::string> zoneIds;
    int requestedOverlapWidth = 16;
};

struct VelocityCrossfadeStackOverlap
{
    std::vector<std::string> lowerZoneIds;
    std::vector<std::string> upperZoneIds;
    int lowVelocity = 0;
    int highVelocity = 0;
    bool widthClamped = false;
};

struct VelocityCrossfadeAuditionStep
{
    std::string label;
    int velocity = 1;
    double lowerGain = 0.0;
    double upperGain = 0.0;
};

struct VelocityCrossfadeAuditionPlan
{
    VelocityCrossfadeAuthoringState state = VelocityCrossfadeAuthoringState::invalidTopology;
    std::string lowerZoneId;
    std::string upperZoneId;
    std::vector<VelocityCrossfadeAuditionStep> steps;
    std::vector<std::string> blockingIssues;

    bool valid() const noexcept { return state == VelocityCrossfadeAuthoringState::eligible; }
};

struct VelocityCrossfadeAuthoringPlan
{
    VelocityCrossfadeAuthoringState state = VelocityCrossfadeAuthoringState::invalidTopology;
    RuntimeProjectModel proposedProject;
    std::vector<std::string> affectedZoneIds;
    std::vector<std::string> warnings;
    std::vector<std::string> blockingIssues;
    // Ordered low-to-high.  A Round Robin layer contains every slot in its
    // pool; ordinary layers contain exactly one zone.
    std::vector<std::vector<std::string>> orderedLayerZoneIds;
    std::vector<VelocityCrossfadeStackOverlap> stackOverlaps;

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

VelocityCrossfadeAuthoringPlan planVelocityCrossfadeStack(
    const RuntimeProjectModel& project,
    const VelocityCrossfadeStackRequest& request);

VelocityCrossfadeAuthoringPlan planVelocityCrossfadeStackRemoval(
    const RuntimeProjectModel& project,
    const std::vector<std::string>& zoneIds);

// Builds the author-facing five-point audition sequence from the same gain
// function used by playback.  No UI-specific crossfade math is permitted.
VelocityCrossfadeAuditionPlan planVelocityCrossfadeAudition(
    const RuntimeProjectModel& project,
    const std::string& lowerZoneId,
    const std::string& upperZoneId);
} // namespace drs::engine
