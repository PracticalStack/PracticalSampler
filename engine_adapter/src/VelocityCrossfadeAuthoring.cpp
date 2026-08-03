#include "drs/engine/VelocityCrossfadeAuthoring.h"

#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <unordered_set>

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

bool sameCrossfadeBaseIdentity(const RuntimeProjectZoneDefinition& left,
                               const RuntimeProjectZoneDefinition& right)
{
    return computeVelocityCrossfadePairingKey(left.articulationId, left.rootKey, left.keyLow,
                                              left.keyHigh, static_cast<int>(left.triggerMode))
        == computeVelocityCrossfadePairingKey(right.articulationId, right.rootKey, right.keyLow,
                                              right.keyHigh, static_cast<int>(right.triggerMode));
}

bool usesRoundRobin(const RuntimeProjectZoneDefinition& zone) noexcept
{
    return zone.roundRobinLength > 0 || zone.roundRobinPosition > 0 || zone.roundRobin.has_value();
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
        case VelocityCrossfadeAuthoringState::incompleteLayerStack:
            return "Select every layer in the velocity stack before changing its crossfades.";
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

VelocityCrossfadeAuthoringPlan planVelocityCrossfadeStack(
    const RuntimeProjectModel& project,
    const VelocityCrossfadeStackRequest& request)
{
    if (request.requestedOverlapWidth < 1 || request.requestedOverlapWidth > 126)
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidOverlap,
                            "Stack overlap width must be between 1 and 126 MIDI velocity steps.");

    std::vector<std::size_t> selectedIndices;
    std::unordered_set<std::string> selectedIds;
    for (const auto& id : request.zoneIds)
    {
        if (!selectedIds.insert(id).second)
            return rejectedPlan(project, VelocityCrossfadeAuthoringState::incompleteLayerStack,
                                "Each selected velocity layer must appear only once.");
        const auto index = findZoneIndex(project, id);
        if (!index.has_value())
            return rejectedPlan(project, VelocityCrossfadeAuthoringState::missingPartner,
                                "One of the selected velocity layers no longer exists.");
        selectedIndices.push_back(*index);
    }
    if (selectedIndices.size() < 2)
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::incompleteLayerStack,
                            "Select at least two compatible velocity layers to create a stack.");

    const auto& anchor = project.authoring.zones[selectedIndices.front()];
    const auto hasRoundRobin = usesRoundRobin(anchor);
    for (const auto index : selectedIndices)
    {
        const auto& zone = project.authoring.zones[index];
        if (!sameCrossfadeBaseIdentity(anchor, zone) || usesRoundRobin(zone) != hasRoundRobin)
            return rejectedPlan(project, VelocityCrossfadeAuthoringState::incompatibleMapping,
                                "Every layer in a crossfade stack must share its playback mapping and Round Robin mode.");
    }

    std::vector<std::vector<std::size_t>> layers;
    if (!hasRoundRobin)
    {
        std::sort(selectedIndices.begin(), selectedIndices.end(), [&](const auto left, const auto right)
        {
            const auto& leftZone = project.authoring.zones[left];
            const auto& rightZone = project.authoring.zones[right];
            return leftZone.velocityLow != rightZone.velocityLow
                ? leftZone.velocityLow < rightZone.velocityLow : leftZone.id < rightZone.id;
        });
        for (std::size_t index = 1; index < selectedIndices.size(); ++index)
            if (project.authoring.zones[selectedIndices[index - 1]].velocityLow
                    == project.authoring.zones[selectedIndices[index]].velocityLow)
                return rejectedPlan(project, VelocityCrossfadeAuthoringState::ambiguousPartner,
                                    "Two selected layers start at the same velocity, so their order is ambiguous.");
        for (const auto index : selectedIndices)
            layers.push_back({ index });
    }
    else
    {
        const auto topologyState = roundRobinTopologyState(project, anchor);
        if (topologyState != VelocityCrossfadeAuthoringState::eligible)
            return rejectedPlan(project, topologyState, stateIssue(topologyState));
        const auto poolId = anchor.roundRobin.has_value() ? anchor.roundRobin->poolId : std::string {};
        for (const auto& candidate : project.authoring.zones)
        {
            const auto candidatePoolId = candidate.roundRobin.has_value() ? candidate.roundRobin->poolId : std::string {};
            if (sameCrossfadeBaseIdentity(anchor, candidate) && candidatePoolId == poolId
                && usesRoundRobin(candidate) && !selectedIds.count(candidate.id))
                return rejectedPlan(project, VelocityCrossfadeAuthoringState::incompleteRoundRobinPool,
                                    "Select every Round Robin slot in every velocity layer before creating a stack.");
        }

        std::map<int, std::vector<std::size_t>> layersByLow;
        for (const auto index : selectedIndices)
            layersByLow[project.authoring.zones[index].velocityLow].push_back(index);
        for (auto& [velocityLow, layer] : layersByLow)
        {
            (void) velocityLow;
            std::sort(layer.begin(), layer.end(), [&](const auto left, const auto right)
            {
                return project.authoring.zones[left].roundRobinPosition
                    < project.authoring.zones[right].roundRobinPosition;
            });
            if (static_cast<int>(layer.size()) != anchor.roundRobinLength)
                return rejectedPlan(project, VelocityCrossfadeAuthoringState::incompleteRoundRobinPool,
                                    "Each Round Robin velocity layer must contain every pool slot exactly once.");
            const auto expectedHigh = project.authoring.zones[layer.front()].velocityHigh;
            const auto expectedCrossfade = project.authoring.zones[layer.front()].velocityCrossfade;
            for (int slot = 1; slot <= anchor.roundRobinLength; ++slot)
            {
                const auto& zone = project.authoring.zones[layer[static_cast<std::size_t>(slot - 1)]];
                if (zone.roundRobinPosition != slot)
                    return rejectedPlan(project, VelocityCrossfadeAuthoringState::duplicateRoundRobinSlot,
                                        "A Round Robin layer must contain one ordered copy of every slot.");
                if (zone.velocityHigh != expectedHigh
                    || zone.velocityCrossfade.fadeInLowVelocity != expectedCrossfade.fadeInLowVelocity
                    || zone.velocityCrossfade.fadeInHighVelocity != expectedCrossfade.fadeInHighVelocity
                    || zone.velocityCrossfade.fadeOutLowVelocity != expectedCrossfade.fadeOutLowVelocity
                    || zone.velocityCrossfade.fadeOutHighVelocity != expectedCrossfade.fadeOutHighVelocity)
                    return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidExistingCrossfade,
                                        "Every Round Robin slot must have the same velocity-layer shape.");
            }
            layers.push_back(std::move(layer));
        }
    }

    if (layers.size() < 2)
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::incompleteLayerStack,
                            "The selected zones resolve to fewer than two velocity layers.");

    std::vector<int> overlapLows(layers.size() - 1);
    std::vector<int> overlapHighs(layers.size() - 1);
    for (std::size_t index = 0; index + 1 < layers.size(); ++index)
    {
        const auto& lower = project.authoring.zones[layers[index].front()];
        const auto& upper = project.authoring.zones[layers[index + 1].front()];
        const auto seam = (lower.velocityHigh + upper.velocityLow) / 2;
        overlapLows[index] = std::clamp(seam - request.requestedOverlapWidth / 2, 1, 126);
        overlapHighs[index] = std::min(127, overlapLows[index] + request.requestedOverlapWidth);
        if (index > 0)
        {
            overlapLows[index] = std::max(overlapLows[index], overlapHighs[index - 1] + 1);
            overlapHighs[index] = std::max(overlapHighs[index], overlapLows[index] + 1);
        }
        if (overlapHighs[index] > 127)
            return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidOverlap,
                                "The selected layers leave no room for the requested crossfade widths.");
    }
    for (std::size_t index = overlapHighs.size(); index-- > 1;)
    {
        overlapHighs[index - 1] = std::min(overlapHighs[index - 1], overlapLows[index] - 1);
        if (overlapHighs[index - 1] <= overlapLows[index - 1])
            return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidOverlap,
                                "The selected layers leave no room for disjoint adjacent crossfade windows.");
    }

    auto proposed = project;
    VelocityCrossfadeAuthoringPlan plan;
    plan.proposedProject = project;
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex)
    {
        auto& orderedIds = plan.orderedLayerZoneIds.emplace_back();
        for (const auto zoneIndex : layers[layerIndex])
        {
            auto& zone = proposed.authoring.zones[zoneIndex];
            orderedIds.push_back(zone.id);
            plan.affectedZoneIds.push_back(zone.id);
            if (layerIndex > 0)
                zone.velocityLow = overlapLows[layerIndex - 1];
            if (layerIndex + 1 < layers.size())
                zone.velocityHigh = overlapHighs[layerIndex];
            zone.velocityCrossfade = {};
            if (layerIndex > 0)
            {
                zone.velocityCrossfade.fadeInLowVelocity = overlapLows[layerIndex - 1];
                zone.velocityCrossfade.fadeInHighVelocity = overlapHighs[layerIndex - 1];
            }
            if (layerIndex + 1 < layers.size())
            {
                zone.velocityCrossfade.fadeOutLowVelocity = overlapLows[layerIndex];
                zone.velocityCrossfade.fadeOutHighVelocity = overlapHighs[layerIndex];
            }
            if (layerIndex > 0 || layerIndex + 1 < layers.size())
                zone.velocityCrossfade.curve = VelocityCrossfadeCurve::linear;
        }
    }
    for (std::size_t index = 0; index < overlapLows.size(); ++index)
    {
        VelocityCrossfadeStackOverlap overlap;
        overlap.lowVelocity = overlapLows[index];
        overlap.highVelocity = overlapHighs[index];
        overlap.widthClamped = overlap.highVelocity - overlap.lowVelocity != request.requestedOverlapWidth;
        for (const auto zoneIndex : layers[index]) overlap.lowerZoneIds.push_back(project.authoring.zones[zoneIndex].id);
        for (const auto zoneIndex : layers[index + 1]) overlap.upperZoneIds.push_back(project.authoring.zones[zoneIndex].id);
        if (overlap.widthClamped)
            plan.warnings.push_back("A stack overlap was clamped to keep adjacent fade windows disjoint.");
        plan.stackOverlaps.push_back(std::move(overlap));
    }

    const auto validation = validateRuntimeProjectModel(proposed);
    if (!validation.valid)
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidTopology,
                            validation.issues.empty() ? "The proposed crossfade stack is invalid."
                                                      : validation.issues.front());

    bool changed = false;
    for (const auto zoneIndex : selectedIndices)
    {
        const auto& before = project.authoring.zones[zoneIndex];
        const auto& after = proposed.authoring.zones[zoneIndex];
        changed = changed || before.velocityLow != after.velocityLow || before.velocityHigh != after.velocityHigh
            || before.velocityCrossfade.fadeInLowVelocity != after.velocityCrossfade.fadeInLowVelocity
            || before.velocityCrossfade.fadeInHighVelocity != after.velocityCrossfade.fadeInHighVelocity
            || before.velocityCrossfade.fadeOutLowVelocity != after.velocityCrossfade.fadeOutLowVelocity
            || before.velocityCrossfade.fadeOutHighVelocity != after.velocityCrossfade.fadeOutHighVelocity;
    }
    plan.proposedProject = std::move(proposed);
    plan.state = changed ? VelocityCrossfadeAuthoringState::eligible : VelocityCrossfadeAuthoringState::noChanges;
    return plan;
}

VelocityCrossfadeAuthoringPlan planVelocityCrossfadeStackRemoval(
    const RuntimeProjectModel& project,
    const std::vector<std::string>& zoneIds)
{
    const auto topology = planVelocityCrossfadeStack(project, { zoneIds, 16 });
    if (!topology.valid())
        return topology;

    auto proposed = project;
    bool changed = false;
    for (std::size_t layer = 0; layer + 1 < topology.orderedLayerZoneIds.size(); ++layer)
    {
        const auto& lowerIds = topology.orderedLayerZoneIds[layer];
        const auto& upperIds = topology.orderedLayerZoneIds[layer + 1];
        if (lowerIds.size() != upperIds.size())
            return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidExistingCrossfade,
                                "The selected stack does not have matching Round Robin slots.");
        for (std::size_t slot = 0; slot < lowerIds.size(); ++slot)
        {
            const auto lowerIndex = findZoneIndex(project, lowerIds[slot]);
            const auto upperIndex = findZoneIndex(project, upperIds[slot]);
            if (!lowerIndex.has_value() || !upperIndex.has_value()
                || validateFirstPassVelocityCrossfadePair(validationZone(project.authoring.zones[*lowerIndex]),
                                                           validationZone(project.authoring.zones[*upperIndex]))
                    != VelocityCrossfadePairIssue::none)
                return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidExistingCrossfade,
                                    "Every adjacent layer must own a valid shared crossfade before stack removal.");
            auto& lower = proposed.authoring.zones[*lowerIndex];
            auto& upper = proposed.authoring.zones[*upperIndex];
            changed = changed || hasAnyVelocityCrossfadeValue(lower.velocityCrossfade)
                || hasAnyVelocityCrossfadeValue(upper.velocityCrossfade);
            lower.velocityCrossfade.fadeOutLowVelocity = 0;
            lower.velocityCrossfade.fadeOutHighVelocity = 0;
            upper.velocityCrossfade.fadeInLowVelocity = 0;
            upper.velocityCrossfade.fadeInHighVelocity = 0;
        }
    }
    if (!changed)
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::noChanges,
                            "The selected stack has no crossfade relationships to remove.");
    const auto validation = validateRuntimeProjectModel(proposed);
    if (!validation.valid)
        return rejectedPlan(project, VelocityCrossfadeAuthoringState::invalidTopology,
                            validation.issues.empty() ? "The proposed stack removal is invalid."
                                                      : validation.issues.front());
    auto plan = topology;
    plan.proposedProject = std::move(proposed);
    plan.state = VelocityCrossfadeAuthoringState::eligible;
    return plan;
}
} // namespace drs::engine
