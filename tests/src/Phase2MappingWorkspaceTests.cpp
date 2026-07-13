#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"

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

        std::cout << "Phase 2 mapping workspace tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 mapping workspace tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
