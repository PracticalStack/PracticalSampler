#include "drs/engine/RuntimeModel.h"
#include "drs/engine/AuthoringSession.h"
#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/PlaybackSnapshot.h"

#include <json/json.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readTextFile(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.good()) throw std::runtime_error("Could not read fixture: " + path);
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}
}

int main()
{
    try
    {
        drs::engine::RuntimeProjectFxSlotDefinition slot;
        slot.id = "slot-unknown-version";
        slot.displayName = "Unknown preserved effect";
        slot.effectType = "vendor.unknown-effect";
        slot.effectVersion = 73;
        slot.parameters = {
            { "vendor.future-a", -12.5 },
            { "vendor.future-b", 0.625 },
            { "vendor.future-c", 1.0 }
        };

        const auto copied = slot;
        require(copied.effectVersion == 73,
                "A slot must retain an explicit algorithm version without catalog interpretation.");
        require(copied.parameters.size() == 3,
                "Unknown parameter records must be retained rather than collapsed.");
        require(copied.parameters[0].id == "vendor.future-a"
                    && copied.parameters[1].id == "vendor.future-b"
                    && copied.parameters[2].id == "vendor.future-c",
                "Parameter record order and unknown IDs must survive model copies.");
        require(std::abs(copied.parameters[0].value + 12.5) < 1.0e-12
                    && std::abs(copied.parameters[1].value - 0.625) < 1.0e-12
                    && std::abs(copied.parameters[2].value - 1.0) < 1.0e-12,
                "Parameter numeric values must survive model copies exactly.");

        drs::engine::RuntimeProjectRoutingBusDefinition chain;
        require(!chain.chainBypassed, "New chains must default to active.");
        chain.chainBypassed = true;
        require(chain.chainBypassed, "Chain bypass must be an explicit authored state.");

        std::vector<std::string> findings;
        require(drs::engine::validatesCuratedDspCatalog(findings),
                "The initial curated catalog must validate independently of UI.");
        for (const auto type : { "drs.gain", "drs.saturator", "drs.stereoDelay", "drs.algorithmicReverb" })
        {
            const auto* descriptor = drs::engine::findCuratedDspEffect(type, 1);
            require(descriptor != nullptr && !descriptor->supportedScopes.empty()
                        && !descriptor->parameters.empty() && descriptor->cost.costUnits > 0,
                    "Each Wave 1 effect requires a complete version-1 catalog descriptor.");
        }
        require(drs::engine::findCuratedDspEffect("vendor.unknown", 1) == nullptr,
                "Unknown effects must not acquire an executable catalog interpretation.");

        drs::engine::RuntimeProjectModel project;
        project.schemaName = "drs.project";
        project.schemaVersion = 5;
        project.projectId = "curated-dsp-schema-five";
        project.displayName = "Curated DSP Schema Five";
        project.contentRootPath = "content";
        project.defaultInstrumentManifestPath = "instrument.drinst";
        project.authoring.schemaName = "drs.authoring";
        project.authoring.schemaVersion = 4;
        project.authoring.fxSlots = { slot };
        project.authoring.routingBuses = { { "master-chain", "Master", "master", { slot.id }, true } };
        const auto firstSerialization = drs::engine::serializeRuntimeProjectManifest(project, "schema-five.drsproj");
        const auto parsed = drs::engine::parseRuntimeProjectManifest(firstSerialization, "schema-five.drsproj", false);
        if (!parsed.loaded || parsed.project.authoring.fxSlots.size() != 1)
        {
            std::string details = "Schema-5 slots must round-trip through the deterministic project loader.";
            for (const auto& issue : parsed.issues) details += " [" + issue + "]";
            throw std::runtime_error(details);
        }
        const auto& restored = parsed.project.authoring.fxSlots.front();
        require(restored.effectVersion == slot.effectVersion && restored.parameters.size() == slot.parameters.size()
                    && parsed.project.authoring.routingBuses.front().chainBypassed,
                "Schema-5 version, ordered parameter records, and chain bypass must survive parsing.");
        require(restored.unavailable && !parsed.warnings.empty(),
                "Unknown effect versions must remain loadable, reported, and runtime-bypassed.");
        require(firstSerialization == drs::engine::serializeRuntimeProjectManifest(parsed.project, "schema-five.drsproj"),
                "Schema-5 project serialization must be deterministic after a round trip.");

        auto duplicateParameter = firstSerialization;
        const auto firstParameter = duplicateParameter.find("vendor.future-a");
        const auto secondParameter = duplicateParameter.find("vendor.future-b");
        require(firstParameter != std::string::npos && secondParameter != std::string::npos,
                "Schema-5 fixture must contain independently addressable parameter IDs.");
        duplicateParameter.replace(secondParameter, std::string("vendor.future-b").size(), "vendor.future-a");
        require(!drs::engine::parseRuntimeProjectManifest(duplicateParameter, "schema-five.drsproj", false).loaded,
                "Duplicate schema-5 parameter IDs must be rejected with a structural finding.");

        auto missingVersion = firstSerialization;
        const auto version = missingVersion.find("\"effectVersion\": 73");
        require(version != std::string::npos, "Schema-5 fixture must contain its explicit effectVersion.");
        missingVersion.replace(version, std::string("\"effectVersion\": 73").size(), "\"effectVersion\": 0 ");
        require(!drs::engine::parseRuntimeProjectManifest(missingVersion, "schema-five.drsproj", false).loaded,
                "Schema-5 slots with a zero effectVersion must be rejected.");

        const auto legacy = drs::engine::loadPhase2ReferenceProjectManifest();
        require(legacy.loaded, "The checked-in schema-4 project must load before DSP migration.");
        const auto migrated = drs::engine::migrateRuntimeProjectToCuratedDspSchema(legacy.project);
        require(migrated.valid && migrated.migrated && migrated.project.schemaVersion == 5
                    && migrated.project.authoring.schemaVersion == 4,
                "Schema-4 project must migrate to a valid schema-5 / authoring-4 project.");
        require(migrated.project.authoring.fxSlots.size() == legacy.project.authoring.fxSlots.size()
                    && migrated.project.authoring.fxSlots[0].id == "color-eq"
                    && migrated.project.authoring.fxSlots[1].id == "shimmer-delay",
                "Migration must preserve legacy FX slot order and stable IDs.");
        require(migrated.project.authoring.fxSlots[0].effectType == "drs.compactEq"
                    && migrated.project.authoring.fxSlots[1].effectType == "drs.stereoDelay",
                "Migration must map legacy effect names to their curated catalog identities.");
        for (const auto& migratedSlot : migrated.project.authoring.fxSlots)
            require(migratedSlot.effectVersion == 1 && migratedSlot.legacyInert && migratedSlot.bypassed,
                    "Migrated FX must be explicitly legacy-inert and bypassed.");
        require(migrated.project.authoring.routingBuses[1].inputSourceId == "zones/lead-a4-sustain",
                "Migration must canonicalize bare legacy zone routing sources.");
        drs::engine::PlaybackSnapshotBuilder snapshots;
        const auto beforeRender = snapshots.buildSnapshot(snapshots.requestBuild(1, false), legacy.project);
        const auto afterRender = snapshots.buildSnapshot(snapshots.requestBuild(2, false), migrated.project);
        if (!(beforeRender.built && afterRender.built
                    && beforeRender.snapshot.zones.size() == afterRender.snapshot.zones.size()
                    && beforeRender.snapshot.groupRoutes.size() == afterRender.snapshot.groupRoutes.size()))
        {
            std::string details = "Legacy and migrated projects must retain the same dry render topology.";
            for (const auto& finding : beforeRender.findings) details += " [before " + finding.code + ": " + finding.message + "]";
            for (const auto& finding : afterRender.findings) details += " [after " + finding.code + ": " + finding.message + "]";
            throw std::runtime_error(details);
        }
        for (std::size_t index = 0; index < beforeRender.snapshot.zones.size(); ++index)
        {
            const auto& before = beforeRender.snapshot.zones[index];
            const auto& after = afterRender.snapshot.zones[index];
            require(before.id == after.id && before.sampleSourceId == after.sampleSourceId
                        && before.gainDb == after.gainDb && before.pan == after.pan
                        && before.sampleStartFrame == after.sampleStartFrame,
                    "Migration must leave every dry zone render input unchanged.");
        }
        const auto repeatedMigratedRender = snapshots.buildSnapshot(
            snapshots.requestBuild(2, false), migrated.project);
        if (!(repeatedMigratedRender.built
                    && repeatedMigratedRender.snapshot.contentDigest == afterRender.snapshot.contentDigest
                    && repeatedMigratedRender.snapshot.dspGraphDigest == afterRender.snapshot.dspGraphDigest
                    && drs::engine::serializeImmutablePlaybackSnapshot(repeatedMigratedRender.snapshot)
                        == drs::engine::serializeImmutablePlaybackSnapshot(afterRender.snapshot)))
        {
            throw std::runtime_error("Repeated snapshot builds must be byte-equivalent with the same DSP graph identity."
                                     " first=" + afterRender.snapshot.contentDigest + "/" + afterRender.snapshot.dspGraphDigest
                                     + " repeat=" + repeatedMigratedRender.snapshot.contentDigest + "/"
                                     + repeatedMigratedRender.snapshot.dspGraphDigest);
        }
        drs::engine::AuthoringSession migratedAuthoring(migrated.project);
        require(!migratedAuthoring.getDspSelection().fxSlotId.empty()
                    && !migratedAuthoring.getDspSelection().routingBusId.empty(),
                "A migrated project must recover a deterministic editor-only DSP selection.");

        const auto fixture = drs::engine::parseRuntimeProjectManifest(
            readTextFile(std::string(DRS_CURATED_DSP_FIXTURE_ROOT) + "/valid-all-scopes.json"),
            "valid-all-scopes.json", false);
        if (!fixture.loaded || fixture.project.authoring.routingBuses.size() != 3
            || !fixture.project.authoring.fxSlots[1].unavailable)
        {
            std::string details = "All-scopes fixture must load with its preserved unknown effect bypassed.";
            for (const auto& issue : fixture.issues) details += " [" + issue + "]";
            throw std::runtime_error(details);
        }
        drs::engine::PlaybackSnapshotBuilder fixtureSnapshots;
        const auto fixtureSnapshot = fixtureSnapshots.buildSnapshot(
            fixtureSnapshots.requestBuild(3, false), fixture.project);
        if (!(fixtureSnapshot.snapshot.fxSlots.size() == 3
                    && fixtureSnapshot.snapshot.fxSlots[1].effectVersion == 1
                    && fixtureSnapshot.snapshot.fxSlots[1].parameters[0].id == "future"
                    && fixtureSnapshot.snapshot.fxSlots[1].parameters[0].value == 0.5
                    && fixtureSnapshot.snapshot.fxSlots[1].unavailable
                    && fixtureSnapshot.snapshot.fxSlots[0].catalogResolved
                    && fixtureSnapshot.snapshot.fxSlots[0].cost.costUnits == 1
                    && fixtureSnapshot.snapshot.fxSlots[0].supportedScopes.size() == 3
                    && fixtureSnapshot.snapshot.routingBuses[2].chainBypassed == false))
        {
            std::string details = "An immutable snapshot must contain complete authored DSP version, parameter, bypass, and availability state.";
            for (const auto& finding : fixtureSnapshot.findings) details += " [" + finding.code + ": " + finding.message + "]";
            throw std::runtime_error(details);
        }
        const auto fixturePlan = drs::engine::compileDspGraphPlan(fixtureSnapshot.snapshot);
        require(fixturePlan.compiled && fixturePlan.plan.nodes.size() == 1
                    && fixturePlan.plan.nodes[0].ownerKind == drs::engine::DspGraphOwnerKind::zone
                    && fixturePlan.plan.nodes[0].ownerId == "example"
                    && fixturePlan.plan.nodes[0].outputDestinationId == "groups/example"
                    && !fixturePlan.plan.directFastPath,
                "The immutable graph plan must compile only executable zone/group/master nodes from the snapshot.");
        auto bypassedPlanSnapshot = fixtureSnapshot.snapshot;
        bypassedPlanSnapshot.fxSlots[0].bypassed = true;
        const auto bypassedPlan = drs::engine::compileDspGraphPlan(bypassedPlanSnapshot);
        require(bypassedPlan.compiled && bypassedPlan.plan.nodes.empty() && bypassedPlan.plan.directFastPath,
                "A no-active-DSP snapshot must collapse to the direct render fast path.");
        auto invalidBusInputSnapshot = fixtureSnapshot.snapshot;
        invalidBusInputSnapshot.routingBuses[0].inputSourceId = "buses/forbidden";
        const auto invalidBusInputPlan = drs::engine::compileDspGraphPlan(invalidBusInputSnapshot);
        require(!invalidBusInputPlan.compiled
                    && std::any_of(invalidBusInputPlan.findings.begin(), invalidBusInputPlan.findings.end(),
                                   [](const auto& finding) { return finding.code == "graph-invalid-owner-source"; }),
                "Bus-to-bus inputs and cycles must be rejected before a graph plan can reach audio.");
        auto overParameterPlanSnapshot = fixtureSnapshot.snapshot;
        overParameterPlanSnapshot.fxSlots[0].parameters.assign(1025, { "bounded", 0.0 });
        const auto overParameterPlan = drs::engine::compileDspGraphPlan(overParameterPlanSnapshot);
        require(!overParameterPlan.compiled
                    && std::any_of(overParameterPlan.findings.begin(), overParameterPlan.findings.end(),
                                   [](const auto& finding) { return finding.code == "graph-parameter-budget"; }),
                "Over-budget parameter plans must fail before activation with a slot path.");
        auto overScratchPlanSnapshot = fixtureSnapshot.snapshot;
        overScratchPlanSnapshot.fxSlots[0].cost.scratchBytes = 8u * 1024u * 1024u + 1u;
        const auto overScratchPlan = drs::engine::compileDspGraphPlan(overScratchPlanSnapshot);
        require(!overScratchPlan.compiled
                    && std::any_of(overScratchPlan.findings.begin(), overScratchPlan.findings.end(),
                                   [](const auto& finding) { return finding.code == "graph-scratch-budget"; }),
                "Over-budget scratch plans must fail before activation with a slot path.");
        const auto hasSnapshotFinding = [](const auto& snapshot, const std::string& code)
        {
            return std::any_of(snapshot.findings.begin(), snapshot.findings.end(),
                               [&](const auto& finding) { return finding.code == code; });
        };
        auto canonicalSourceProject = fixture.project;
        canonicalSourceProject.authoring.routingBuses[0].inputSourceId = "example";
        const auto canonicalSourceSnapshot = fixtureSnapshots.buildSnapshot(
            fixtureSnapshots.requestBuild(4, false), canonicalSourceProject);
        require(canonicalSourceSnapshot.snapshot.routingBuses[0].inputSourceId == "zones/example",
                "Snapshot build must canonicalize legacy bare zone owner sources.");
        auto duplicateParameterSnapshotProject = fixture.project;
        duplicateParameterSnapshotProject.authoring.fxSlots[0].parameters.push_back({ "gainDb", 1.0 });
        const auto duplicateParameterSnapshot = fixtureSnapshots.buildSnapshot(
            fixtureSnapshots.requestBuild(5, false), duplicateParameterSnapshotProject);
        require(hasSnapshotFinding(duplicateParameterSnapshot, "snapshot-dsp-duplicate-parameter"),
                "Invalid snapshot DSP parameters must produce a structured snapshot-dsp finding.");
        auto orphanSlotSnapshotProject = fixture.project;
        orphanSlotSnapshotProject.authoring.routingBuses[0].fxSlotIds.clear();
        const auto orphanSlotSnapshot = fixtureSnapshots.buildSnapshot(
            fixtureSnapshots.requestBuild(6, false), orphanSlotSnapshotProject);
        require(hasSnapshotFinding(orphanSlotSnapshot, "snapshot-dsp-slot-owner-count"),
                "Invalid snapshot DSP ownership must produce a structured snapshot-dsp finding.");
        auto unknownBypassedProject = fixture.project;
        unknownBypassedProject.authoring.fxSlots[1].bypassed = true;
        const auto unknownBypassedSnapshot = fixtureSnapshots.buildSnapshot(
            fixtureSnapshots.requestBuild(8, false), unknownBypassedProject);
        require(!hasSnapshotFinding(unknownBypassedSnapshot, "snapshot-dsp-unknown-catalog-version")
                    && unknownBypassedSnapshot.snapshot.fxSlots[1].unavailable
                    && unknownBypassedSnapshot.snapshot.fxSlots[1].bypassed,
                "Unknown unavailable nodes must be preserved as explicitly bypassed snapshot nodes.");
        const auto baseGraphDigest = drs::engine::computePlaybackSnapshotDspGraphDigest(fixtureSnapshot.snapshot);
        auto relabeledGraph = fixtureSnapshot.snapshot;
        relabeledGraph.fxSlots[0].displayName = "A UI-only label";
        relabeledGraph.routingBuses[0].displayName = "Another UI-only label";
        require(drs::engine::computePlaybackSnapshotDspGraphDigest(relabeledGraph) == baseGraphDigest,
                "DSP graph identity must not change for non-semantic display labels.");
        auto changedValueGraph = fixtureSnapshot.snapshot;
        changedValueGraph.fxSlots[0].parameters[0].value = 1.0;
        require(drs::engine::computePlaybackSnapshotDspGraphDigest(changedValueGraph) != baseGraphDigest,
                "DSP graph identity must change for an authored parameter value.");
        auto changedOrderGraph = fixtureSnapshot.snapshot;
        std::swap(changedOrderGraph.routingBuses[0].fxSlotIds[0], changedOrderGraph.routingBuses[1].fxSlotIds[0]);
        require(drs::engine::computePlaybackSnapshotDspGraphDigest(changedOrderGraph) != baseGraphDigest,
                "DSP graph identity must change for chain ownership/order changes.");
        auto changedBypassGraph = fixtureSnapshot.snapshot;
        changedBypassGraph.fxSlots[0].bypassed = true;
        require(drs::engine::computePlaybackSnapshotDspGraphDigest(changedBypassGraph) != baseGraphDigest,
                "DSP graph identity must change for bypass state changes.");
        auto orphan = fixture.project;
        orphan.authoring.routingBuses[0].fxSlotIds.clear();
        require(!drs::engine::validateRuntimeProjectModel(orphan).valid,
                "An orphaned FX slot must be rejected.");
        auto shared = fixture.project;
        shared.authoring.routingBuses[1].fxSlotIds = { "zone-gain" };
        require(!drs::engine::validateRuntimeProjectModel(shared).valid,
                "A slot shared by two chains must be rejected.");
        auto duplicateSource = fixture.project;
        duplicateSource.authoring.routingBuses[2].inputSourceId = "zones/example";
        require(!drs::engine::validateRuntimeProjectModel(duplicateSource).valid,
                "A source with multiple chain owners must be rejected.");
        auto overBudget = fixture.project;
        overBudget.authoring.fxSlots[0].parameters.assign(1025, { "p", 0.0 });
        require(!drs::engine::validateRuntimeProjectModel(overBudget).valid,
                "Over-budget DSP parameter state must be rejected.");

        const auto caseDefinitions = nlohmann::ordered_json::parse(readTextFile(
            std::string(DRS_CURATED_DSP_FIXTURE_ROOT) + "/negative-cases.json"));
        require(caseDefinitions.value("baseFixture", std::string {}) == "valid-all-scopes.json"
                    && caseDefinitions["cases"].is_array(),
                "The curated DSP negative fixture catalog must name the all-scopes base fixture.");
        for (const auto& fixtureCase : caseDefinitions["cases"])
        {
            auto candidate = fixture.project;
            const auto mutation = fixtureCase.at("mutation").get<std::string>();
            const auto slotId = fixtureCase.value("slotId", std::string {});
            const auto busId = fixtureCase.value("busId", std::string {});
            const auto findSlot = [&]() -> drs::engine::RuntimeProjectFxSlotDefinition&
            {
                for (auto& candidateSlot : candidate.authoring.fxSlots)
                    if (candidateSlot.id == slotId)
                        return candidateSlot;
                throw std::runtime_error("Negative DSP fixture references an unknown slot: " + slotId);
            };
            const auto findBus = [&]() -> drs::engine::RuntimeProjectRoutingBusDefinition&
            {
                for (auto& candidateBus : candidate.authoring.routingBuses)
                    if (candidateBus.id == busId)
                        return candidateBus;
                throw std::runtime_error("Negative DSP fixture references an unknown bus: " + busId);
            };

            if (mutation == "unknownEffectVersion")
                findSlot().effectVersion = fixtureCase.at("effectVersion").get<std::uint32_t>();
            else if (mutation == "missingEffectVersion")
                findSlot().effectVersion = 0;
            else if (mutation == "duplicateParameter")
                findSlot().parameters.push_back(findSlot().parameters.front());
            else if (mutation == "sharedSlot")
                findBus().fxSlotIds.push_back(slotId);
            else if (mutation == "orphanSlot")
                findBus().fxSlotIds.clear();
            else if (mutation == "duplicateSourceOwner")
                findBus().inputSourceId = fixtureCase.at("inputSourceId").get<std::string>();
            else if (mutation == "overBudgetParameters")
            {
                const auto parameterCount = fixtureCase.at("parameterCount").get<std::size_t>();
                auto& parameters = findSlot().parameters;
                parameters.clear();
                for (std::size_t index = 0; index < parameterCount; ++index)
                    parameters.push_back({ "parameter-" + std::to_string(index), 0.0 });
            }
            else
                throw std::runtime_error("Unknown curated DSP negative fixture mutation: " + mutation);

            const auto candidateParsed = drs::engine::parseRuntimeProjectManifest(
                drs::engine::serializeRuntimeProjectManifest(candidate, "negative-case.drsproj"),
                "negative-case.drsproj", false);
            const auto expectLoaded = fixtureCase.at("expectLoaded").get<bool>();
            require(candidateParsed.loaded == expectLoaded,
                    ("Curated DSP negative fixture case did not produce its expected loader result: "
                     + fixtureCase.at("id").get<std::string>()).c_str());
            if (mutation == "unknownEffectVersion")
                require(candidateParsed.loaded && candidateParsed.project.authoring.fxSlots[0].unavailable,
                        "Unknown catalog versions must load as unavailable rather than being discarded.");
        }

        drs::engine::AuthoringSession authoring(fixture.project);
        drs::engine::RuntimeProjectFxSlotDefinition createdSlot;
        createdSlot.id = "zone-saturator";
        createdSlot.displayName = "Zone Saturator";
        createdSlot.effectType = "drs.saturator";
        createdSlot.effectVersion = 1;
        createdSlot.parameters = { { "driveDb", 6.0 } };
        require(authoring.createFxSlot(createdSlot, "zone", "Create zone saturator").applied,
                "Creating a slot must add exactly one stable owner reference atomically.");
        require(authoring.duplicateFxSlot("zone-saturator", "zone-saturator-copy", "Duplicate zone saturator").applied,
                "Duplicating a slot must allocate its caller-supplied stable unique ID.");
        require(authoring.moveFxSlot("zone-saturator-copy", -1, "Move duplicated saturator").applied,
                "Reordering a slot must reorder its owning chain.");
        const auto& createdChain = authoring.getProject().authoring.routingBuses.front().fxSlotIds;
        require(createdChain.size() == 3 && createdChain[0] == "zone-gain"
                    && createdChain[1] == "zone-saturator-copy" && createdChain[2] == "zone-saturator",
                "Create, duplicate, and reorder must preserve deterministic chain ordering.");
        require(authoring.deleteFxSlot("zone-saturator-copy", "Delete duplicated saturator").applied,
                "Deleting a slot must remove it and exactly its one owner reference.");
        require(authoring.getDspSelection().fxSlotId == "zone-gain"
                    && authoring.getDspSelection().routingBusId == "zone",
                "Deleting the selected slot must deterministically recover to the first surviving owner slot.");
        require(authoring.undo().applied
                    && authoring.getProject().authoring.routingBuses.front().fxSlotIds[1] == "zone-saturator-copy",
                "Undo must restore a deleted slot ID and its exact chain position.");
        require(authoring.redo().applied
                    && std::find(authoring.getProject().authoring.routingBuses.front().fxSlotIds.begin(),
                                 authoring.getProject().authoring.routingBuses.front().fxSlotIds.end(),
                                 "zone-saturator-copy")
                           == authoring.getProject().authoring.routingBuses.front().fxSlotIds.end(),
                "Redo must remove only the duplicated slot and preserve prior chain state.");
        require(authoring.moveFxSlotToBus("zone-saturator", "group", "Move saturator to group").applied
                    && authoring.getProject().authoring.routingBuses[1].fxSlotIds.back() == "zone-saturator",
                "Cross-scope moves must transfer the sole owner reference atomically.");
        require(!authoring.moveFxSlotToBus("zone-saturator", "group", "Reject same-owner move").applied,
                "A same-owner move must be rejected without a partial topology mutation.");
        require(authoring.setRoutingBusChainBypassed("group", true, "Bypass group chain").applied
                    && authoring.getProject().authoring.routingBuses[1].chainBypassed,
                "Chain bypass must be an explicit undoable routing transaction.");
        require(authoring.selectDspSlot("zone-saturator").applied
                    && authoring.getDspSelection().routingBusId == "group",
                "DSP selection must be editor-only and resolve its owner from stable source topology.");
        const auto topologyBeforeInvalidEdit = drs::engine::serializeRuntimeProjectManifest(
            authoring.getProject(), "authoring-ownership.drsproj");
        auto duplicateSourceBus = authoring.getProject().authoring.routingBuses[2];
        duplicateSourceBus.inputSourceId = "zones/example";
        require(!authoring.updateRoutingBus(2, duplicateSourceBus, "Reject duplicate source").applied,
                "A second bus for one source must be rejected before document mutation.");
        require(topologyBeforeInvalidEdit == drs::engine::serializeRuntimeProjectManifest(
                    authoring.getProject(), "authoring-ownership.drsproj"),
                "Rejected cross-owner routing changes must leave the document byte-for-byte unchanged.");
        require(authoring.setFxSlotParameter("zone-saturator", "driveDb", 18.0, "Drive gesture").applied,
                "Known catalog parameters must edit through their stable slot and parameter IDs.");
        require(!authoring.setFxSlotParameter("zone-saturator", "driveDb", 37.0, "Reject invalid drive").applied,
                "Known catalog parameter ranges must reject invalid values without mutation.");
        require(authoring.resetFxSlotParameterToDefault("zone-saturator", "driveDb", "Reset drive").applied,
                "Known catalog parameters must reset through their catalog-versioned defaults.");
        const auto& resetParameters = authoring.getProject().authoring.fxSlots.back().parameters;
        require(std::any_of(resetParameters.begin(), resetParameters.end(),
                            [](const auto& parameter) { return parameter.id == "driveDb" && parameter.value == 6.0; }),
                "A reset must persist the descriptor's exact default value.");
        const auto historyBeforeGesture = authoring.getDocumentState().undoDepth;
        std::vector<double> liveGestureValues;
        authoring.setDspParameterGesturePreviewListener(
            [&](const std::string& slotId, const std::string& parameterId, const double value)
            {
                require(slotId == "zone-saturator" && parameterId == "driveDb",
                        "A live gesture must publish stable slot and parameter identities.");
                liveGestureValues.push_back(value);
            });
        require(authoring.beginFxSlotParameterGesture("zone-saturator", "driveDb").applied
                    && authoring.updateFxSlotParameterGesture(8.0).applied
                    && authoring.updateFxSlotParameterGesture(12.0).applied,
                "A parameter drag must collect validated live values before committing.");
        require(liveGestureValues == std::vector<double> { 8.0, 12.0 }
                    && authoring.getDocumentState().undoDepth == historyBeforeGesture,
                "Live gesture ticks must publish values without creating snapshot transactions.");
        const auto committedGesture = authoring.commitFxSlotParameterGesture("Coalesced drive drag");
        require(committedGesture.applied && committedGesture.requiresHostStateRebuild
                    && committedGesture.changedPaths == std::vector<std::string> {
                        "authoring.fxSlots[3].parameters.driveDb" },
                "A parameter drag must collect validated live values and commit successfully.");
        require(authoring.getDocumentState().undoDepth == historyBeforeGesture + 1u,
                "A coalesced parameter drag must create exactly one document-history entry.");
        require(authoring.deleteRoutingBus("group", "Delete selected group chain").applied
                    && authoring.getDspSelection().fxSlotId == "zone-gain"
                    && authoring.getDspSelection().routingBusId == "zone",
                "Deleting an owner chain must delete its owned slots and deterministically recover selection.");
        require(authoring.deleteFxSlot("zone-gain", "Empty zone chain").applied
                    && authoring.getDspSelection().fxSlotId == "master-delay"
                    && authoring.getDspSelection().routingBusId == "master",
                "Deleting the last slot in one chain must recover selection to the next available chain.");
        require(authoring.deleteRoutingBus("master", "Delete final non-empty chain").applied
                    && authoring.getDspSelection().fxSlotId.empty()
                    && authoring.getDspSelection().routingBusId == "zone",
                "With all chains empty, selection must retain a deterministic empty-chain owner and no slot ID.");

        std::cout << "Curated DSP model contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Curated DSP model contract tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
