#include "drs/engine/DspGraphPlan.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace drs::engine
{
namespace
{
constexpr std::size_t maximumNodes = 128;
constexpr std::size_t maximumParameters = 1024;
constexpr std::size_t maximumScratchBytes = 8u * 1024u * 1024u;
constexpr std::size_t maximumStateBytes = 16u * 1024u * 1024u;
constexpr std::uint32_t maximumCostUnits = 128;

void addFinding(DspGraphPlanBuildResult& result, std::string code, std::string path, std::string message)
{
    result.findings.push_back({ std::move(code), std::move(path), std::move(message) });
}

bool checkedAdd(std::size_t& target, const std::size_t value)
{
    if (value > std::numeric_limits<std::size_t>::max() - target)
        return false;
    target += value;
    return true;
}

std::string fnv(const std::string& text)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text) { hash ^= static_cast<unsigned char>(character); hash *= 1099511628211ull; }
    std::ostringstream stream;
    stream << "fnv1a64:" << std::hex << hash;
    return stream.str();
}
} // namespace

DspGraphPlanBuildResult compileDspGraphPlan(const ImmutablePlaybackSnapshot& snapshot)
{
    DspGraphPlanBuildResult result;
    result.plan.authoredGraphDigest = snapshot.dspGraphDigest;
    std::unordered_map<std::string, const PlaybackSnapshotFxSlotReference*> slots;
    for (const auto& slot : snapshot.fxSlots) slots.emplace(slot.id, &slot);
    std::unordered_map<std::string, std::size_t> slotOwnerCounts;
    std::unordered_map<std::string, std::string> zoneGroups;
    for (const auto& zone : snapshot.zones) zoneGroups.emplace(zone.id, zone.groupId);

    std::vector<std::size_t> orderedBusIndices;
    orderedBusIndices.reserve(snapshot.routingBuses.size());
    for (std::size_t index = 0; index < snapshot.routingBuses.size(); ++index)
        orderedBusIndices.push_back(index);
    const auto ownerDepth = [&](const std::size_t index)
    {
        const auto& source = snapshot.routingBuses[index].inputSourceId;
        if (source.rfind("zones/", 0) == 0) return 0;
        if (source.rfind("groups/", 0) == 0) return 1;
        if (source == "master") return 2;
        return 3;
    };
    std::stable_sort(orderedBusIndices.begin(), orderedBusIndices.end(),
                     [&](const auto left, const auto right)
                     {
                         return ownerDepth(left) < ownerDepth(right);
                     });

    for (const auto busIndex : orderedBusIndices)
    {
        const auto& bus = snapshot.routingBuses[busIndex];
        if (bus.chainBypassed) continue;
        DspGraphOwnerKind kind = DspGraphOwnerKind::master;
        std::string ownerId;
        std::string destination = "output";
        if (bus.inputSourceId.rfind("zones/", 0) == 0)
        {
            kind = DspGraphOwnerKind::zone;
            ownerId = bus.inputSourceId.substr(6);
            const auto zone = zoneGroups.find(ownerId);
            if (zone == zoneGroups.end())
                addFinding(result, "graph-invalid-zone-owner", "routingBuses[" + std::to_string(busIndex) + "]", "Zone owner is absent from snapshot zones.");
            else destination = "groups/" + zone->second;
        }
        else if (bus.inputSourceId.rfind("groups/", 0) == 0)
        {
            kind = DspGraphOwnerKind::group;
            ownerId = bus.inputSourceId.substr(7);
            destination = "master";
        }
        else if (bus.inputSourceId == "master") { ownerId = "master"; }
        else addFinding(result, "graph-invalid-owner-source", "routingBuses[" + std::to_string(busIndex) + "]", "Only canonical zone, group, or master sources are compilable.");

        for (const auto& slotId : bus.fxSlotIds)
        {
            if (++slotOwnerCounts[slotId] > 1)
            {
                addFinding(result, "graph-duplicate-slot-owner", "routingBuses[" + std::to_string(busIndex) + "]",
                           "A DSP slot may belong to only one compiled chain.");
                continue;
            }
            const auto slot = slots.find(slotId);
            if (slot == slots.end()) { addFinding(result, "graph-unknown-slot", "routingBuses[" + std::to_string(busIndex) + "]", "Chain references a missing snapshot slot."); continue; }
            const auto& effect = *slot->second;
            if (effect.bypassed || effect.unavailable || effect.legacyInert) continue;
            if (!effect.catalogResolved) { addFinding(result, "graph-unresolved-effect", "fxSlots." + slotId, "Active effects must resolve to a curated catalog entry."); continue; }
            if (result.plan.nodes.size() >= maximumNodes) { addFinding(result, "graph-node-budget", "fxSlots." + slotId, "Graph exceeds the 128-node limit."); continue; }
            DspGraphNode node;
            node.ownerKind = kind; node.ownerId = ownerId; node.inputSourceId = bus.inputSourceId;
            node.outputDestinationId = destination; node.slotId = effect.id; node.effectType = effect.effectType;
            node.effectVersion = effect.effectVersion; node.parameterStart = result.plan.parameters.size();
            node.parameterCount = effect.parameters.size(); node.scratchOffsetBytes = result.plan.scratchBytes;
            node.scratchBytes = effect.cost.scratchBytes; node.stateBytes = effect.cost.stateBytes; node.costUnits = effect.cost.costUnits;
            node.delayMemoryBytes = effect.stateClass == CuratedDspStateClass::delay ? effect.cost.stateBytes : 0;
            if (node.parameterCount > maximumParameters - result.plan.parameters.size())
            {
                addFinding(result, "graph-parameter-budget", "fxSlots." + slotId, "Graph exceeds the 1,024-parameter limit.");
                continue;
            }
            auto nextScratchBytes = result.plan.scratchBytes;
            if (!checkedAdd(nextScratchBytes, node.scratchBytes) || nextScratchBytes > maximumScratchBytes)
            {
                addFinding(result, "graph-scratch-budget", "fxSlots." + slotId, "Graph scratch exceeds the 8 MiB limit.");
                continue;
            }
            auto nextStateBytes = result.plan.stateBytes;
            if (!checkedAdd(nextStateBytes, node.stateBytes) || nextStateBytes > maximumStateBytes)
            {
                addFinding(result, "graph-state-budget", "fxSlots." + slotId, "Graph mutable state exceeds the 16 MiB limit.");
                continue;
            }
            if (node.costUnits > std::numeric_limits<std::uint32_t>::max() - result.plan.costUnits)
            {
                addFinding(result, "graph-cost-overflow", "fxSlots." + slotId, "Graph effect cost overflowed its bounded total.");
                continue;
            }
            if (node.costUnits > maximumCostUnits - result.plan.costUnits)
            {
                addFinding(result, "graph-cost-budget", "fxSlots." + slotId,
                           "Graph DSP cost exceeds the 128-unit callback budget.");
                continue;
            }
            auto nextDelayMemoryBytes = result.plan.delayMemoryBytes;
            if (!checkedAdd(nextDelayMemoryBytes, node.delayMemoryBytes))
            {
                addFinding(result, "graph-delay-memory-overflow", "fxSlots." + slotId,
                           "Graph delay-memory request overflowed its bounded total.");
                continue;
            }
            result.plan.scratchBytes = nextScratchBytes;
            result.plan.stateBytes = nextStateBytes;
            result.plan.delayMemoryBytes = nextDelayMemoryBytes;
            result.plan.costUnits += node.costUnits;
            for (const auto& parameter : effect.parameters)
                result.plan.parameters.push_back({ parameter.id, parameter.value });
            result.plan.nodes.push_back(std::move(node));
        }
    }
    result.plan.directFastPath = result.plan.nodes.empty();
    std::ostringstream identity;
    identity << result.plan.authoredGraphDigest << ':' << result.plan.nodes.size() << ':' << result.plan.parameters.size()
             << ':' << result.plan.scratchBytes << ':' << result.plan.stateBytes << ':'
             << result.plan.delayMemoryBytes << ':' << result.plan.costUnits;
    for (const auto& node : result.plan.nodes) identity << ':' << node.inputSourceId << ':' << node.slotId;
    result.plan.planDigest = fnv(identity.str());
    result.compiled = result.findings.empty();
    return result;
}
} // namespace drs::engine
