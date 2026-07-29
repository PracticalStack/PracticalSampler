#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string joinIssues(const std::vector<std::string>& issues)
{
    std::string joined;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index != 0)
            joined += " | ";
        joined += issues[index];
    }
    return joined;
}

drs::engine::RuntimeProjectModel makeRoundRobinAuthoringFixture()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Round Robin authoring fixture requires the Phase 2 reference project.");

    auto project = loaded.project;
    require(project.authoring.zones.size() >= 3,
            "Round Robin authoring fixture requires at least three authored zones.");

    for (auto& zone : project.authoring.zones)
    {
        zone.groupId = "rr-main";
        zone.articulationId = "main";
        zone.rootKey = 60;
        zone.keyLow = 60;
        zone.keyHigh = 60;
        zone.velocityLow = 1;
        zone.velocityHigh = 127;
        zone.velocityCrossfade = {};
        zone.roundRobin.reset();
        zone.roundRobinLength = 0;
        zone.roundRobinPosition = 0;
        zone.triggerMode = drs::engine::ZoneTriggerMode::gated;
    }

    project.authoring.selectedZoneId = project.authoring.zones.front().id;
    return project;
}
} // namespace

int main()
{
    try
    {
        const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "Phase 2 reference authoring project must load before mapping workspace tests run.");

        drs::engine::AuthoringSession session(projectLoad.project);
        const auto initialSummaries = session.getZoneSummaries();
        require(initialSummaries.size() == 3, "Phase 2 mapping workspace zone count changed unexpectedly.");
        require(initialSummaries[2].selected, "Phase 2 mapping workspace should begin with the lead zone selected.");

        const auto initialPreview = session.buildSelectedZonePreviewRequest();
        require(initialPreview.available, "Selected Phase 2 zone should produce a preview request.");
        require(initialPreview.zoneId == "lead-a4-sustain", "Initial selected-zone preview target changed unexpectedly.");
        require(initialPreview.midiNote == 69, "Initial selected-zone preview note changed unexpectedly.");

        const auto selectionResult = session.selectZone("pad-a3-high");
        require(selectionResult.applied, "Selecting a different zone should create an undoable project transaction.");
        require(session.getDocumentState().revision == 1, "Zone selection should increment project revision.");
        require(session.getZoneSummaries()[1].selected, "Zone selection should move to the requested zone.");

        auto editedZone = session.getSelectedZone().value();
        editedZone.rootKey = 58;
        editedZone.keyLow = 48;
        editedZone.keyHigh = 79;
        editedZone.velocityLow = 12;
        editedZone.velocityHigh = 118;
        editedZone.gainDb = 2.25;
        editedZone.pan = -0.22;
        editedZone.loopEnabled = false;

        const auto editResult = session.updateSelectedZone(editedZone, "Refine selected zone mapping");
        require(editResult.applied, "Editing the selected zone should commit successfully.");
        require(session.getDocumentState().revision == 2, "Zone edit should increment project revision.");

        const auto selectedZone = session.getSelectedZone().value();
        require(selectedZone.rootKey == 58, "Selected zone root key edit did not persist.");
        require(selectedZone.keyLow == 48 && selectedZone.keyHigh == 79,
                "Selected zone key-range edit did not persist.");
        require(selectedZone.velocityLow == 12 && selectedZone.velocityHigh == 118,
                "Selected zone velocity-range edit did not persist.");
        require(selectedZone.gainDb == 2.25, "Selected zone gain edit did not persist.");
        require(selectedZone.pan == -0.22, "Selected zone pan edit did not persist.");
        require(!selectedZone.loopEnabled, "Selected zone loop toggle edit did not persist.");

        const auto updatedPreview = session.buildSelectedZonePreviewRequest();
        require(updatedPreview.available && updatedPreview.midiNote == 58,
                "Preview request should track the edited root key.");
        require(updatedPreview.velocity == 65,
                "Preview request should derive velocity from the edited range midpoint.");

        session.markSaved();
        require(!session.getDocumentState().dirty, "Marking a save checkpoint should clear the dirty flag.");

        const auto undoResult = session.undo();
        require(undoResult.applied, "Undo should restore the previous zone snapshot.");
        require(session.getSelectedZone()->rootKey == 57, "Undo should restore the previous root key.");
        require(session.getDocumentState().dirty, "Undo away from the save checkpoint should mark the project dirty.");

        const auto redoResult = session.redo();
        require(redoResult.applied, "Redo should reapply the edited zone snapshot.");
        require(session.getSelectedZone()->rootKey == 58, "Redo should restore the edited root key.");

        const auto badSelection = session.selectZone("missing-zone-id");
        require(!badSelection.applied, "Unknown zone selection should be rejected.");
        require(session.getDocumentState().revision == 2, "Rejected selection should not change project revision.");

        drs::engine::AuthoringSession uniqueSourceDeletionSession(projectLoad.project);
        const auto uniqueSourceDeletion = uniqueSourceDeletionSession.deleteSelectedSample();
        require(uniqueSourceDeletion.applied,
                "Deleting the selected sample should create an undoable project transaction. Issues: "
                    + joinIssues(uniqueSourceDeletion.issues));
        require(uniqueSourceDeletionSession.getProject().authoring.zones.size() == 2,
                "Deleting a selected sample should remove its zone mapping.");
        require(uniqueSourceDeletionSession.getProject().sampleSources.size() == 1,
                "Deleting the last zone for a sample source should remove the unused source record.");
        require(uniqueSourceDeletionSession.getProject().authoring.selectedZoneId == "pad-a3-high",
                "Deleting the final zone should select the previous neighboring zone.");
        require(uniqueSourceDeletionSession.undo().applied
                    && uniqueSourceDeletionSession.getProject().authoring.zones.size() == 3
                    && uniqueSourceDeletionSession.getProject().sampleSources.size() == 2
                    && uniqueSourceDeletionSession.getProject().authoring.selectedZoneId == "lead-a4-sustain",
                "Undo should restore the deleted zone, source record, and selection.");

        drs::engine::AuthoringSession sharedSourceDeletionSession(projectLoad.project);
        require(sharedSourceDeletionSession.selectZone("pad-a3-high").applied,
                "Shared-source deletion fixture should select the high pad zone.");
        require(sharedSourceDeletionSession.deleteSelectedSample().applied,
                "Deleting a zone that shares a source should succeed.");
        require(sharedSourceDeletionSession.getProject().authoring.zones.size() == 2
                    && sharedSourceDeletionSession.getProject().sampleSources.size() == 2,
                "Deleting one shared-source zone must preserve its source for remaining zones.");
        require(sharedSourceDeletionSession.getProject().authoring.selectedZoneId == "lead-a4-sustain",
                "Deletion should select the zone now occupying the deleted zone's position.");

        drs::engine::AuthoringSession roundRobinSession(makeRoundRobinAuthoringFixture());
        const auto createPoolResult =
            roundRobinSession.createRoundRobinPoolForSelectedZone("Create Round Robin pool");
        require(createPoolResult.applied,
                "Creating a Round Robin pool should produce an undoable authoring edit. Issues: "
                    + joinIssues(createPoolResult.issues));
        const auto firstPoolZone = roundRobinSession.getSelectedZone().value();
        require(firstPoolZone.roundRobin.has_value()
                    && firstPoolZone.roundRobin->slotCount == 1
                    && firstPoolZone.roundRobin->slotIndex == 1,
                "Creating a Round Robin pool should assign the selected zone to a solo slot.");
        const auto firstPoolId = firstPoolZone.roundRobin->poolId;

        require(roundRobinSession.addCompatibleZonesToSelectedRoundRobinPool(
                    "Add compatible zones to Round Robin pool").applied,
                "Adding compatible zones should extend the selected Round Robin pool.");
        const auto& groupedZones = roundRobinSession.getProject().authoring.zones;
        require(groupedZones.size() >= 3,
                "Grouped Round Robin fixture should still expose its authored zones.");
        std::vector<int> groupedSlotIndices;
        for (const auto& zone : groupedZones)
        {
            require(zone.roundRobin.has_value()
                        && zone.roundRobin->poolId == firstPoolId
                        && zone.roundRobin->slotCount == 3,
                    "Adding compatible zones should keep every authored zone in one shared pool.");
            groupedSlotIndices.push_back(zone.roundRobin->slotIndex);
        }
        std::sort(groupedSlotIndices.begin(), groupedSlotIndices.end());
        require(groupedSlotIndices == std::vector<int> {1, 2, 3},
                "Adding compatible zones should create one dense sequential pool.");

        const auto splitCandidate = std::find_if(groupedZones.begin(),
                                                 groupedZones.end(),
                                                 [](const auto& zone)
                                                 {
                                                     return zone.roundRobin.has_value()
                                                         && zone.roundRobin->slotIndex == 2;
                                                 });
        require(splitCandidate != groupedZones.end(),
                "Round Robin split coverage requires a stable slot-2 candidate.");
        require(roundRobinSession.selectZone(splitCandidate->id).applied,
                "Round Robin split coverage should select the middle grouped zone.");
        require(roundRobinSession.createRoundRobinPoolForSelectedZone("Split to new Round Robin pool").applied,
                "Creating a pool for a grouped zone should split it into a new explicit pool.");
        const auto splitZone = roundRobinSession.getSelectedZone().value();
        require(splitZone.roundRobin.has_value()
                    && splitZone.roundRobin->poolId != firstPoolId
                    && splitZone.roundRobin->slotCount == 1
                    && splitZone.roundRobin->slotIndex == 1,
                "Splitting a grouped Round Robin zone should create a new solo pool for that zone.");
        const auto& splitZones = roundRobinSession.getProject().authoring.zones;
        std::vector<int> remainingPoolSlots;
        for (const auto& zone : splitZones)
        {
            if (!zone.roundRobin.has_value() || zone.roundRobin->poolId != firstPoolId)
                continue;

            require(zone.roundRobin->slotCount == 2,
                    "Splitting a grouped zone should explicitly resize the remaining pool.");
            remainingPoolSlots.push_back(zone.roundRobin->slotIndex);
        }
        std::sort(remainingPoolSlots.begin(), remainingPoolSlots.end());
        require(remainingPoolSlots == std::vector<int> {1, 2},
                "Splitting a grouped zone should explicitly reindex the remaining pool.");

        require(roundRobinSession.removeSelectedZoneFromRoundRobinPool(
                    "Remove selected zone from Round Robin pool").applied,
                "Removing a solo Round Robin zone should clear its explicit pool assignment.");
        require(!roundRobinSession.getSelectedZone()->roundRobin.has_value(),
                "Removing a solo Round Robin zone should restore standalone playback.");
        require(roundRobinSession.undo().applied
                    && roundRobinSession.getSelectedZone()->roundRobin.has_value(),
                "Undo should restore the removed Round Robin assignment.");
        require(roundRobinSession.redo().applied
                    && !roundRobinSession.getSelectedZone()->roundRobin.has_value(),
                "Redo should reapply the Round Robin removal.");

        const auto normalizedCandidate = std::find_if(splitZones.begin(),
                                                      splitZones.end(),
                                                      [&](const auto& zone)
                                                      {
                                                          return zone.roundRobin.has_value()
                                                              && zone.roundRobin->poolId == firstPoolId;
                                                      });
        require(normalizedCandidate != splitZones.end(),
                "Round Robin normalization coverage requires a remaining pooled zone.");
        require(roundRobinSession.selectZone(normalizedCandidate->id).applied,
                "Round Robin normalization coverage should select a remaining pooled zone.");
        require(roundRobinSession.normalizeSelectedRoundRobinPool(
                    "Normalize Round Robin slot numbering").applied,
                "Normalizing a Round Robin pool should remain an undoable authoring action.");
        const auto& normalizedZones = roundRobinSession.getProject().authoring.zones;
        std::vector<int> normalizedSlots;
        for (const auto& zone : normalizedZones)
        {
            if (!zone.roundRobin.has_value() || zone.roundRobin->poolId != firstPoolId)
                continue;

            require(zone.roundRobin->slotCount == 2,
                    "Normalizing a Round Robin pool should preserve the surviving member count.");
            normalizedSlots.push_back(zone.roundRobin->slotIndex);
        }
        std::sort(normalizedSlots.begin(), normalizedSlots.end());
        require(normalizedSlots == std::vector<int> {1, 2},
                "Normalizing a Round Robin pool should keep the surviving pool dense and ordered.");

        std::cout << "Phase 2 mapping workspace tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 mapping workspace tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
