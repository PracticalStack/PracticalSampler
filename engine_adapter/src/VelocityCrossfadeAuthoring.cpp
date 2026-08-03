#include "drs/engine/VelocityCrossfadeAuthoring.h"

#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <sstream>

namespace drs::engine
{
namespace
{
std::uint64_t computeFnv1a64(const std::string& text) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }
    return hash;
}

std::optional<std::size_t> findZoneIndex(const RuntimeProjectModel& project, const std::string& id)
{
    const auto iterator = std::find_if(project.authoring.zones.begin(), project.authoring.zones.end(),
                                       [&](const RuntimeProjectZoneDefinition& zone) { return zone.id == id; });
    if (iterator == project.authoring.zones.end())
        return std::nullopt;
    return static_cast<std::size_t>(std::distance(project.authoring.zones.begin(), iterator));
}

bool sameRoundRobinIdentity(const RuntimeProjectZoneDefinition& left,
                            const RuntimeProjectZoneDefinition& right) noexcept
{
    const auto leftUsesRoundRobin = left.roundRobinLength > 0 && left.roundRobinPosition > 0;
    const auto rightUsesRoundRobin = right.roundRobinLength > 0 && right.roundRobinPosition > 0;
    if (leftUsesRoundRobin != rightUsesRoundRobin)
        return false;
    if (!leftUsesRoundRobin)
        return true;

    const auto leftPool = left.roundRobin.has_value() ? left.roundRobin->poolId : std::string {};
    const auto rightPool = right.roundRobin.has_value() ? right.roundRobin->poolId : std::string {};
    return leftPool == rightPool && left.roundRobinLength == right.roundRobinLength
        && left.roundRobinPosition == right.roundRobinPosition;
}

bool sameCrossfadeIdentity(const RuntimeProjectZoneDefinition& left,
                           const RuntimeProjectZoneDefinition& right)
{
    return computeVelocityCrossfadePairingKey(left.articulationId, left.rootKey, left.keyLow,
                                              left.keyHigh, static_cast<int>(left.triggerMode))
            == computeVelocityCrossfadePairingKey(right.articulationId, right.rootKey, right.keyLow,
                                                   right.keyHigh, static_cast<int>(right.triggerMode))
        && sameRoundRobinIdentity(left, right);
}

VelocityCrossfadeZoneDefinition validationZone(const RuntimeProjectZoneDefinition& zone)
{
    return { zone.velocityLow, zone.velocityHigh, zone.velocityCrossfade };
}

void addIssue(VelocityCrossfadeAuthoringPlan& plan, const std::string& issue)
{
    plan.blockingIssues.push_back(issue);
}

VelocityCrossfadeAuthoringPlan rejectedPlan(const RuntimeProjectModel& project,
                                            VelocityCrossfadeAuthoringState state,
                                            const std::string& issue)
{
    VelocityCrossfadeAuthoringPlan plan;
    plan.state = state;
    plan.proposedProject = project;
    addIssue(plan, issue);
    return plan;
}

VelocityCrossfadeAuthoringState roundRobinTopologyState(const RuntimeProjectModel& project,
                                                         const RuntimeProjectZoneDefinition& anchor)
{
    if (anchor.roundRobinLength <= 0 || anchor.roundRobinPosition <= 0)
        return VelocityCrossfadeAuthoringState::eligible;

    const auto pairingKey = computeVelocityCrossfadePairingKey(anchor.articulationId, anchor.rootKey,
                                                                 anchor.keyLow, anchor.keyHigh,
                                                                 static_cast<int>(anchor.triggerMode));
    const auto poolId = anchor.roundRobin.has_value() ? anchor.roundRobin->poolId : std::string {};
    std::vector<bool> coveredSlots(static_cast<std::size_t>(anchor.roundRobinLength) + 1u, false);
    std::vector<const RuntimeProjectZoneDefinition*> matchingZones;
    for (const auto& candidate : project.authoring.zones)
    {
        if (computeVelocityCrossfadePairingKey(candidate.articulationId, candidate.rootKey,
                                                candidate.keyLow, candidate.keyHigh,
                                                static_cast<int>(candidate.triggerMode)) != pairingKey)
            continue;
        const auto candidatePoolId = candidate.roundRobin.has_value() ? candidate.roundRobin->poolId : std::string {};
        if (candidatePoolId != poolId)
            continue;
        if (candidate.roundRobinLength != anchor.roundRobinLength)
            return VelocityCrossfadeAuthoringState::mixedRoundRobinSlotCount;
        if (candidate.roundRobinPosition < 1 || candidate.roundRobinPosition > candidate.roundRobinLength)
            return VelocityCrossfadeAuthoringState::incompleteRoundRobinPool;
        coveredSlots[static_cast<std::size_t>(candidate.roundRobinPosition)] = true;
        matchingZones.push_back(&candidate);
    }

    for (std::size_t leftIndex = 0; leftIndex < matchingZones.size(); ++leftIndex)
    {
        const auto& left = *matchingZones[leftIndex];
        for (std::size_t rightIndex = leftIndex + 1; rightIndex < matchingZones.size(); ++rightIndex)
        {
            const auto& right = *matchingZones[rightIndex];
            if (left.roundRobinPosition == right.roundRobinPosition
                && left.velocityLow == right.velocityLow && left.velocityHigh == right.velocityHigh
                && left.velocityCrossfade.fadeInLowVelocity == right.velocityCrossfade.fadeInLowVelocity
                && left.velocityCrossfade.fadeInHighVelocity == right.velocityCrossfade.fadeInHighVelocity
                && left.velocityCrossfade.fadeOutLowVelocity == right.velocityCrossfade.fadeOutLowVelocity
                && left.velocityCrossfade.fadeOutHighVelocity == right.velocityCrossfade.fadeOutHighVelocity)
            {
                return VelocityCrossfadeAuthoringState::duplicateRoundRobinSlot;
            }
        }
    }

    for (int slot = 1; slot <= anchor.roundRobinLength; ++slot)
        if (!coveredSlots[static_cast<std::size_t>(slot)])
            return VelocityCrossfadeAuthoringState::incompleteRoundRobinPool;
    return VelocityCrossfadeAuthoringState::eligible;
}

std::string stateIssue(const VelocityCrossfadeAuthoringState state)
{
    switch (state)
    {
        case VelocityCrossfadeAuthoringState::incompleteRoundRobinPool:
            return "The Round Robin pool does not cover every slot.";
        case VelocityCrossfadeAuthoringState::mixedRoundRobinSlotCount:
            return "The Round Robin pool uses mixed slot counts.";
        case VelocityCrossfadeAuthoringState::duplicateRoundRobinSlot:
            return "The Round Robin pool contains a duplicate slot for this layer.";
        default:
            return "The selected zones cannot form a valid velocity crossfade.";
    }
}
} // namespace

std::uint64_t computeVelocityCrossfadePairingKey(const std::string& articulationId,
                                                 const int rootKey,
                                                 const int keyLow,
                                                 const int keyHigh,
                                                 const int triggerMode)
{
    std::ostringstream stream;
    stream << articulationId << '|' << rootKey << '|' << keyLow << '|' << keyHigh << '|' << triggerMode;
    return computeFnv1a64(stream.str());
}

VelocityCrossfadePartnerDiscovery discoverVelocityCrossfadePartner(
    const RuntimeProjectModel& project,
    const std::string& anchorZoneId,
    const VelocityCrossfadeDirection direction)
{
    VelocityCrossfadePartnerDiscovery result;
    result.anchorZoneId = anchorZoneId;
    result.direction = direction;
    const auto anchorIndex = findZoneIndex(project, anchorZoneId);
    if (!anchorIndex.has_value())
    {
        result.blockingIssues.push_back("The selected zone does not exist in this project.");
        return result;
    }

    const auto& anchor = project.authoring.zones[*anchorIndex];
    const auto roundRobinState = roundRobinTopologyState(project, anchor);
    if (roundRobinState != VelocityCrossfadeAuthoringState::eligible)
    {
        result.state = roundRobinState;
        result.blockingIssues.push_back(stateIssue(roundRobinState));
        return result;
    }

    for (const auto& candidate : project.authoring.zones)
    {
        if (candidate.id == anchor.id || !sameCrossfadeIdentity(anchor, candidate))
            continue;
        const auto isLower = candidate.velocityLow < anchor.velocityLow;
        const auto isUpper = candidate.velocityLow > anchor.velocityLow;
        if ((direction == VelocityCrossfadeDirection::fadeIn && isLower)
            || (direction == VelocityCrossfadeDirection::fadeOut && isUpper))
        {
            result.partnerZoneIds.push_back(candidate.id);
        }
    }

    std::sort(result.partnerZoneIds.begin(), result.partnerZoneIds.end());
    if (result.partnerZoneIds.empty())
    {
        result.state = VelocityCrossfadeAuthoringState::missingPartner;
        result.blockingIssues.push_back("No compatible adjacent velocity layer was found.");
    }
    else if (result.partnerZoneIds.size() != 1)
    {
        result.state = VelocityCrossfadeAuthoringState::ambiguousPartner;
        result.blockingIssues.push_back("More than one compatible adjacent velocity layer was found.");
    }
    else
    {
        result.state = VelocityCrossfadeAuthoringState::eligible;
    }
    return result;
}

VelocityCrossfadeAuthoringPlan planVelocityCrossfadePair(
    const RuntimeProjectModel& project,
    const VelocityCrossfadePairRequest& request)
{
    if (!isVelocityValueValid(request.overlapLowVelocity)
        || !isVelocityValueValid(request.overlapHighVelocity)
        || request.overlapLowVelocity >= request.overlapHighVelocity)
    {
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidOverlap,
                            "Crossfade overlap must be an ordered two-step window within MIDI velocities 1-127.");
    }

    const auto lowerIndex = findZoneIndex(project, request.lowerZoneId);
    const auto upperIndex = findZoneIndex(project, request.upperZoneId);
    if (!lowerIndex.has_value() || !upperIndex.has_value() || *lowerIndex == *upperIndex)
    {
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::missingPartner,
                            "Choose two distinct existing zones for a velocity crossfade.");
    }

    const auto& lower = project.authoring.zones[*lowerIndex];
    const auto& upper = project.authoring.zones[*upperIndex];
    if (!sameCrossfadeIdentity(lower, upper))
    {
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::incompatibleMapping,
                            "Crossfade partners must share articulation, root, key range, trigger mode, and Round Robin identity.");
    }

    const auto lowerDiscovery = discoverVelocityCrossfadePartner(project, lower.id,
                                                                  VelocityCrossfadeDirection::fadeOut);
    if (!lowerDiscovery.eligible() || lowerDiscovery.partnerZoneIds.front() != upper.id)
    {
        return rejectedPlan(project, lowerDiscovery.state,
                            lowerDiscovery.blockingIssues.empty()
                                ? "The lower zone does not resolve to the requested upper partner."
                                : lowerDiscovery.blockingIssues.front());
    }

    auto proposed = project;
    auto& proposedLower = proposed.authoring.zones[*lowerIndex];
    auto& proposedUpper = proposed.authoring.zones[*upperIndex];
    proposedLower.velocityHigh = request.overlapHighVelocity;
    proposedLower.velocityCrossfade.fadeOutLowVelocity = request.overlapLowVelocity;
    proposedLower.velocityCrossfade.fadeOutHighVelocity = request.overlapHighVelocity;
    proposedLower.velocityCrossfade.curve = VelocityCrossfadeCurve::linear;
    proposedUpper.velocityLow = request.overlapLowVelocity;
    proposedUpper.velocityCrossfade.fadeInLowVelocity = request.overlapLowVelocity;
    proposedUpper.velocityCrossfade.fadeInHighVelocity = request.overlapHighVelocity;
    proposedUpper.velocityCrossfade.curve = VelocityCrossfadeCurve::linear;

    if (validateFirstPassVelocityCrossfadeZone(validationZone(proposedLower)) != VelocityCrossfadeZoneIssue::none
        || validateFirstPassVelocityCrossfadeZone(validationZone(proposedUpper)) != VelocityCrossfadeZoneIssue::none
        || validateFirstPassVelocityCrossfadePair(validationZone(proposedLower), validationZone(proposedUpper))
               != VelocityCrossfadePairIssue::none)
    {
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidExistingCrossfade,
                            "The requested overlap would make one of the selected zones' crossfade windows invalid.");
    }

    const auto validation = validateRuntimeProjectModel(proposed);
    if (!validation.valid)
    {
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidTopology,
                            validation.issues.empty() ? "The proposed crossfade topology is invalid."
                                                      : validation.issues.front());
    }

    VelocityCrossfadeAuthoringPlan plan;
    plan.proposedProject = std::move(proposed);
    plan.affectedZoneIds = { request.lowerZoneId, request.upperZoneId };
    plan.state = plan.proposedProject.authoring.zones[*lowerIndex].velocityHigh == lower.velocityHigh
            && plan.proposedProject.authoring.zones[*upperIndex].velocityLow == upper.velocityLow
            && plan.proposedProject.authoring.zones[*lowerIndex].velocityCrossfade.fadeOutLowVelocity
                    == lower.velocityCrossfade.fadeOutLowVelocity
            && plan.proposedProject.authoring.zones[*lowerIndex].velocityCrossfade.fadeOutHighVelocity
                    == lower.velocityCrossfade.fadeOutHighVelocity
            && plan.proposedProject.authoring.zones[*upperIndex].velocityCrossfade.fadeInLowVelocity
                    == upper.velocityCrossfade.fadeInLowVelocity
            && plan.proposedProject.authoring.zones[*upperIndex].velocityCrossfade.fadeInHighVelocity
                    == upper.velocityCrossfade.fadeInHighVelocity
        ? VelocityCrossfadeAuthoringState::noChanges
        : VelocityCrossfadeAuthoringState::eligible;
    return plan;
}

VelocityCrossfadeAuthoringPlan planVelocityCrossfadeRemoval(
    const RuntimeProjectModel& project,
    const std::string& lowerZoneId,
    const std::string& upperZoneId)
{
    const auto lowerIndex = findZoneIndex(project, lowerZoneId);
    const auto upperIndex = findZoneIndex(project, upperZoneId);
    if (!lowerIndex.has_value() || !upperIndex.has_value() || *lowerIndex == *upperIndex)
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::missingPartner,
                            "Choose the two existing zones that own the crossfade relationship.");

    const auto& lower = project.authoring.zones[*lowerIndex];
    const auto& upper = project.authoring.zones[*upperIndex];
    if (!sameCrossfadeIdentity(lower, upper)
        || validateFirstPassVelocityCrossfadePair(validationZone(lower), validationZone(upper))
               != VelocityCrossfadePairIssue::none)
    {
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidExistingCrossfade,
                            "The selected zones do not own one valid shared velocity crossfade.");
    }

    auto proposed = project;
    proposed.authoring.zones[*lowerIndex].velocityCrossfade.fadeOutLowVelocity = 0;
    proposed.authoring.zones[*lowerIndex].velocityCrossfade.fadeOutHighVelocity = 0;
    proposed.authoring.zones[*upperIndex].velocityCrossfade.fadeInLowVelocity = 0;
    proposed.authoring.zones[*upperIndex].velocityCrossfade.fadeInHighVelocity = 0;
    const auto validation = validateRuntimeProjectModel(proposed);
    if (!validation.valid)
    {
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidTopology,
                            validation.issues.empty() ? "The proposed crossfade removal is invalid."
                                                      : validation.issues.front());
    }

    VelocityCrossfadeAuthoringPlan plan;
    plan.state = VelocityCrossfadeAuthoringState::eligible;
    plan.proposedProject = std::move(proposed);
    plan.affectedZoneIds = { lowerZoneId, upperZoneId };
    return plan;
}
} // namespace drs::engine
