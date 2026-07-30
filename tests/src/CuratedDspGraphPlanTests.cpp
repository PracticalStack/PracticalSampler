#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/DspRenderGeneration.h"
#include "drs/engine/DspParameterControl.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

bool hasFinding(const drs::engine::DspGraphPlanBuildResult& result, const char* code)
{
    return std::any_of(result.findings.begin(), result.findings.end(),
                       [code](const auto& finding) { return finding.code == code; });
}

drs::engine::PlaybackSnapshotFxSlotReference makeSlot(std::string id,
                                                       std::string type,
                                                       std::string parameterId,
                                                       double value,
                                                       std::uint32_t cost)
{
    drs::engine::PlaybackSnapshotFxSlotReference slot;
    slot.id = std::move(id);
    slot.effectType = std::move(type);
    slot.effectVersion = 1;
    slot.catalogResolved = true;
    slot.parameters = { { std::move(parameterId), value } };
    slot.cost.costUnits = cost;
    return slot;
}

drs::engine::ImmutablePlaybackSnapshot makeSnapshot()
{
    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.dspGraphDigest = "fnv1a64:graph-plan-fixture";
    snapshot.zones.push_back({ "zone-a", "sample", "Zone", "group-a" });
    snapshot.fxSlots = {
        makeSlot("zone-gain", "drs.gain", "gainDb", 0.0, 1),
        makeSlot("group-saturator", "drs.saturator", "driveDb", 6.0, 3),
        makeSlot("master-delay", "drs.stereoDelay", "timeMs", 375.0, 12),
        makeSlot("unknown", "vendor.future", "future", 0.5, 0)
    };
    snapshot.fxSlots.back().catalogResolved = false;
    snapshot.fxSlots.back().unavailable = true;
    snapshot.routingBuses = {
        { "zone", "Zone", "zones/zone-a", { "zone-gain" } },
        { "group", "Group", "groups/group-a", { "group-saturator" } },
        { "master", "Master", "master", { "master-delay", "unknown" } }
    };
    return snapshot;
}
}

int main()
{
    try
    {
        const auto snapshot = makeSnapshot();
        const auto plan = drs::engine::compileDspGraphPlan(snapshot);
        require(plan.compiled && plan.plan.nodes.size() == 3 && plan.plan.parameters.size() == 3,
                "All executable zone, group, and master nodes must compile into one flat plan.");
        require(plan.plan.nodes[0].ownerKind == drs::engine::DspGraphOwnerKind::zone
                    && plan.plan.nodes[1].ownerKind == drs::engine::DspGraphOwnerKind::group
                    && plan.plan.nodes[2].ownerKind == drs::engine::DspGraphOwnerKind::master,
                "Plan node order must preserve fixed zone -> group -> master topology.");
        require(plan.plan.nodes[0].outputDestinationId == "groups/group-a"
                    && plan.plan.nodes[1].outputDestinationId == "master"
                    && plan.plan.nodes[2].outputDestinationId == "output",
                "Each plan node must resolve its fixed output destination without a project pointer.");
        require(plan.plan.costUnits == 16 && !plan.plan.directFastPath,
                "Unknown unavailable slots must be bypassed while resolved node costs are accumulated.");
        const auto repeatedPlan = drs::engine::compileDspGraphPlan(snapshot);
        require(repeatedPlan.compiled && repeatedPlan.plan.planDigest == plan.plan.planDigest
                    && repeatedPlan.plan.nodes.size() == plan.plan.nodes.size()
                    && repeatedPlan.plan.parameters.size() == plan.plan.parameters.size(),
                "The same immutable snapshot must compile to the same ordered graph plan and digest.");
        const auto controls = drs::engine::compileDspParameterControlLayout(plan.plan);
        require(controls.compiled && controls.layout.controls.size() == 3
                    && controls.layout.controls[0].controlIndex == 0
                    && controls.layout.controls[0].graphParameterIndex == 0
                    && controls.layout.controls[0].slotId == "zone-gain",
                "A graph must compile stable numeric control slots before audio activation.");

        auto delaySnapshot = snapshot;
        delaySnapshot.fxSlots[2].stateClass = drs::engine::CuratedDspStateClass::delay;
        delaySnapshot.fxSlots[2].cost.stateBytes = 4096;
        const auto delayPlan = drs::engine::compileDspGraphPlan(delaySnapshot);
        require(delayPlan.compiled && delayPlan.plan.delayMemoryBytes == 4096
                    && delayPlan.plan.stateBytes == 4096,
                "Delay nodes must request explicit bounded delay memory as part of graph resources.");

        auto bypassed = snapshot;
        for (auto& slot : bypassed.fxSlots) slot.bypassed = true;
        const auto direct = drs::engine::compileDspGraphPlan(bypassed);
        require(direct.compiled && direct.plan.nodes.empty() && direct.plan.directFastPath,
                "A no-DSP graph must select the direct fast path.");

        auto duplicate = snapshot;
        duplicate.routingBuses[1].fxSlotIds.push_back("zone-gain");
        const auto duplicatePlan = drs::engine::compileDspGraphPlan(duplicate);
        require(!duplicatePlan.compiled && hasFinding(duplicatePlan, "graph-duplicate-slot-owner"),
                "A slot attached to two chains must be rejected by graph compilation.");

        auto oversized = snapshot;
        oversized.fxSlots[0].cost.stateBytes = 16u * 1024u * 1024u + 1u;
        const auto oversizedPlan = drs::engine::compileDspGraphPlan(oversized);
        require(!oversizedPlan.compiled && hasFinding(oversizedPlan, "graph-state-budget"),
                "State budget overflow must reject the graph before activation.");

        drs::engine::ImmutablePlaybackSnapshot maximum;
        maximum.dspGraphDigest = "fnv1a64:maximum";
        drs::engine::PlaybackSnapshotRoutingBusReference master { "master", "Master", "master" };
        for (std::size_t index = 0; index < 128; ++index)
        {
            const auto id = "slot-" + std::to_string(index);
            maximum.fxSlots.push_back(makeSlot(id, "drs.gain", "gainDb", 0.0, 1));
            master.fxSlotIds.push_back(id);
        }
        maximum.routingBuses.push_back(master);
        const auto maximumPlan = drs::engine::compileDspGraphPlan(maximum);
        require(maximumPlan.compiled && maximumPlan.plan.nodes.size() == 128,
                "The maximum legal 128-node topology must compile deterministically.");
        maximum.fxSlots.push_back(makeSlot("slot-128", "drs.gain", "gainDb", 0.0, 1));
        maximum.routingBuses.front().fxSlotIds.push_back("slot-128");
        const auto tooManyNodes = drs::engine::compileDspGraphPlan(maximum);
        require(!tooManyNodes.compiled && hasFinding(tooManyNodes, "graph-node-budget"),
                "One node beyond the legal topology must be rejected before activation.");

        drs::engine::ImmutablePlaybackSnapshot costly;
        costly.dspGraphDigest = "fnv1a64:costly";
        drs::engine::PlaybackSnapshotRoutingBusReference costlyMaster { "master", "Master", "master" };
        for (std::size_t index = 0; index < 7; ++index)
        {
            const auto id = "reverb-" + std::to_string(index);
            costly.fxSlots.push_back(makeSlot(id, "drs.algorithmicReverb", "mix", .2, 20));
            costlyMaster.fxSlotIds.push_back(id);
        }
        costly.routingBuses.push_back(costlyMaster);
        const auto costlyPlan = drs::engine::compileDspGraphPlan(costly);
        require(!costlyPlan.compiled && hasFinding(costlyPlan, "graph-cost-budget"),
                "The seventh 20-unit reverb must be rejected before activation by the callback cost budget.");

        std::string generationFailure;
        require(!drs::engine::createDspRenderGeneration({}, plan.plan, 512, &generationFailure)
                    && !generationFailure.empty(),
                "A render generation must reject construction without immutable sampler ownership.");

        std::cout << "Curated DSP graph plan tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Curated DSP graph plan tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
