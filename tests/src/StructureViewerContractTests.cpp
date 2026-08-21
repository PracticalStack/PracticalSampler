#include "shared/authoring/AuthoringStructureSelection.h"
#include "shared/authoring/StructureViewModels.h"
#include "shared/authoring/StructureViewState.h"
#include "shared/authoring/StructureOverlapPolicy.h"
#include "shared/authoring/StructureInspector.h"
#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "support/StructureViewerFixtures.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
using drs::app::authoring::AuthoringStructureSelection;
using drs::app::authoring::StructureSelectionKind;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    try
    {
        AuthoringStructureSelection selection;
        selection.replace(StructureSelectionKind::zone, { "zone-b", "", "zone-a", "zone-a" }, "zone-b");
        require(selection.getIds() == std::vector<std::string>({ "zone-a", "zone-b" }),
                "Selection should remove empty and duplicate IDs deterministically.");
        require(selection.getPrimaryId() == "zone-b", "Primary selection should remain selected by stable ID.");

        selection.toggle("zone-c", StructureSelectionKind::zone);
        require(selection.getIds().size() == 3 && selection.getPrimaryId() == "zone-c",
                "Toggling a new same-kind ID should add it and make it primary.");
        selection.toggle("zone-c", StructureSelectionKind::zone);
        require(!selection.contains("zone-c") && selection.getPrimaryId() == "zone-a",
                "Removing the primary should use the first deterministic fallback.");

        selection.selectRange({ "zone-a", "zone-b", "zone-c", "zone-d" },
                              "zone-c", StructureSelectionKind::zone);
        require(selection.getIds() == std::vector<std::string>({ "zone-a", "zone-b", "zone-c" }),
                "Range selection should use the visible order between anchor and target.");
        selection.toggle("group-a", StructureSelectionKind::group);
        require(selection.getKind() == StructureSelectionKind::group
                    && selection.getIds() == std::vector<std::string>({ "group-a" }),
                "Selecting a different entity kind should replace the prior selection.");
        selection.reconcile({ "group-a", "group-b" });
        require(selection.getPrimaryId() == "group-a", "Reconcile should retain a valid primary.");
        selection.reconcile({ "group-b" });
        require(selection.getIds() == std::vector<std::string>({}),
                "Reconcile should clear a selection when all selected IDs disappear.");
        require(selection.getKind() == StructureSelectionKind::none,
                "Cleared selection should return to none kind.");

        const auto project = drs::tests::makeStructureViewerFixture();
        AuthoringStructureSelection layerSelection;
        layerSelection.replace(StructureSelectionKind::layer, { "layer-piano" }, "layer-piano");
        const auto hierarchy = drs::app::authoring::buildStructureHierarchyViewModel(
            project, layerSelection, "layer-piano", "group-piano-sustain");
        require(hierarchy.layers.size() == 3, "Hierarchy projection should expose every authored layer.");
        require(hierarchy.groups.size() == 2, "Primary layer projection should expose only its groups.");
        require(hierarchy.zones.size() == 2, "Primary group projection should expose only its zones.");
        require(hierarchy.layers.front().groupCount == 2 && hierarchy.layers.front().zoneCount == 3,
                "Layer counts should be derived from group and zone parent relationships.");
        require(hierarchy.layers.front().keyLow == 36 && hierarchy.layers.front().keyHigh == 60,
                "Layer coverage should aggregate member zone key ranges.");

        const auto overlapInfo = drs::app::authoring::analyzeStructureOverlaps(project.authoring.zones);
        require(overlapInfo.size() == project.authoring.zones.size(),
                "Overlap analysis should return one diagnostic per zone.");
        require(overlapInfo[0].kind == drs::app::authoring::StructureOverlapKind::exactKeyStack,
                "Exact key and velocity stacks with the same trigger should be classified explicitly.");
        require(overlapInfo[3].kind == drs::app::authoring::StructureOverlapKind::keyRangeOverlap,
                "Round-robin siblings with distinct articulation should remain key-range diagnostics.");
        require(overlapInfo[3].overlapCount >= 1 && overlapInfo[4].overlapCount >= 1,
                "Overlap counts should identify each member of a sibling pair.");

        const auto zoneInspector = drs::app::authoring::buildStructureInspectorSnapshot(
            project,
            [&]
            {
                AuthoringStructureSelection value;
                value.replace(StructureSelectionKind::zone,
                              { "zone-piano-low", "zone-piano-high" },
                              "zone-piano-high");
                return value;
            }());
        require(zoneInspector.kind == StructureSelectionKind::zone
                    && zoneInspector.selectedCount == 2
                    && zoneInspector.title.find("Zone:") == 0,
                "Zone inspector should identify the active multi-selection.");
        const auto velocityField = std::find_if(zoneInspector.fields.begin(), zoneInspector.fields.end(),
                                                [](const auto& field) { return field.first == "Velocity"; });
        require(velocityField != zoneInspector.fields.end() && velocityField->second == "Mixed",
                "Multi-selection inspector should expose mixed compatible attributes.");

        drs::app::authoring::StructureViewProjectionOptions filterOptions;
        filterOptions.searchText = "high";
        filterOptions.sortMode = drs::app::authoring::StructureSortMode::name;
        const auto filteredHierarchy = drs::app::authoring::buildStructureHierarchyViewModel(
            project, layerSelection, "layer-piano", "group-piano-sustain", filterOptions);
        require(filteredHierarchy.zones.size() == 1 && filteredHierarchy.zones.front().id == "zone-piano-high",
                "Structure projection search should filter zone rows by display name.");
        require(filteredHierarchy.zones.front().keyStartNormalized >= 0.0f
                    && filteredHierarchy.zones.front().keyEndNormalized <= 1.0f
                    && filteredHierarchy.zones.front().keyStartNormalized < filteredHierarchy.zones.front().keyEndNormalized,
                "Structure zone rows should carry cached normalized key geometry for paint and ruler alignment.");

        drs::app::authoring::StructureViewState viewState;
        viewState.setMode(drs::app::authoring::StructureViewMode::structure);
        viewState.setLayerColumnWidth(80);
        viewState.setSearchText("Piano");
        viewState.setShowOverlapsOnly(true);
        viewState.setLayerColumnCollapsed(true);
        viewState.setGroupColumnCollapsed(true);
        viewState.setArticulationFilter("legato");
        viewState.setPerformanceEventFilter(drs::engine::PerformanceEventKind::noteOff);
        viewState.setLayerScrollAnchor(3);
        viewState.setGroupScrollAnchor(6);
        viewState.setZoneScrollAnchor(12);
        require(viewState.getMode() == drs::app::authoring::StructureViewMode::structure
                    && viewState.getLayerColumnWidth() == 96
                    && viewState.getSearchText() == "Piano"
                    && viewState.getShowOverlapsOnly()
                    && viewState.isLayerColumnCollapsed()
                    && viewState.isGroupColumnCollapsed()
                    && viewState.getArticulationFilter() == "legato"
                    && viewState.getPerformanceEventFilter().has_value()
                    && *viewState.getPerformanceEventFilter() == drs::engine::PerformanceEventKind::noteOff
                    && viewState.getLayerScrollAnchor() == 3
                    && viewState.getGroupScrollAnchor() == 6
                    && viewState.getZoneScrollAnchor() == 12,
                "Structure view presentation state should clamp and retain workspace-only values.");
        const auto workspaceSnapshot = viewState.exportWorkspaceSnapshot();
        drs::app::authoring::StructureViewState restoredViewState;
        restoredViewState.importWorkspaceSnapshot(workspaceSnapshot);
        require(restoredViewState.getMode() == drs::app::authoring::StructureViewMode::structure
                    && restoredViewState.getSearchText() == "Piano"
                    && restoredViewState.getShowOverlapsOnly()
                    && restoredViewState.isLayerColumnCollapsed()
                    && restoredViewState.isGroupColumnCollapsed()
                    && restoredViewState.getArticulationFilter() == "legato"
                    && restoredViewState.getPerformanceEventFilter().has_value()
                    && *restoredViewState.getPerformanceEventFilter() == drs::engine::PerformanceEventKind::noteOff
                    && restoredViewState.getLayerScrollAnchor() == 3
                    && restoredViewState.getGroupScrollAnchor() == 6
                    && restoredViewState.getZoneScrollAnchor() == 12,
                "Structure workspace state should round-trip independently of authored project data.");
        const auto serializedProject = drs::engine::serializeRuntimeProjectManifest(project, "structure-viewer-fixture.sfz");
        require(serializedProject.find("layerColumnWidth") == std::string::npos
                    && serializedProject.find("groupColumnWidth") == std::string::npos
                    && serializedProject.find("zoneScrollAnchor") == std::string::npos
                    && serializedProject.find("articulationFilter") == std::string::npos,
                "Workspace presentation state must not enter serialized project JSON.");

        drs::engine::AuthoringSession session(project);
        const auto before = session.getDocumentState();
        require(session.selectLayer("layer-strings").applied, "Layer selection should apply in the session.");
        require(session.selectGroup("group-strings-sustain").applied,
                "Group selection should apply in the session.");
        require(session.selectZone("zone-strings-a").applied,
                "Zone selection should apply in the session.");
        const auto after = session.getDocumentState();
        require(before.revision == after.revision && before.undoDepth == after.undoDepth && !after.dirty,
                "Hierarchy selection must not mutate authored document state.");
        require(!session.undo().applied && !session.redo().applied,
                "Selection-only navigation must not create undoable document actions.");

        drs::engine::AuthoringStructureBatchPatch zonePatch;
        zonePatch.releaseSeconds = 0.75;
        zonePatch.gainDb = -1.5;
        const auto zonePatchResult = session.applyStructureBatchPatch(
            drs::engine::AuthoringStructureEntityKind::zone,
            { "zone-strings-a", "zone-strings-b" }, zonePatch, "Set shared zone values");
        require(zonePatchResult.applied && session.getDocumentState().undoDepth == before.undoDepth + 1,
                "Compatible zone inspector edits should commit as one atomic transaction.");
        require(session.getProject().authoring.zones[3].releaseSeconds == 0.75
                    && session.getProject().authoring.zones[4].releaseSeconds == 0.75,
                "Atomic zone patch should update every selected zone.");
        drs::engine::AuthoringStructureBatchPatch invalidPatch;
        invalidPatch.keyLow = 120;
        invalidPatch.keyHigh = 10;
        const auto invalidResult = session.applyStructureBatchPatch(
            drs::engine::AuthoringStructureEntityKind::zone,
            { "zone-strings-a", "missing-zone" }, invalidPatch, "Reject invalid structure edit");
        require(!invalidResult.applied && session.getProject().authoring.zones[3].keyLow == 48,
                "Invalid or partially missing batch targets must reject without mutation.");
        require(session.undo().applied && session.getProject().authoring.zones[3].releaseSeconds != 0.75,
                "One-step undo should revert the complete structure batch transaction.");
        require(session.redo().applied && session.getProject().authoring.zones[3].releaseSeconds == 0.75,
                "Redo should restore the complete structure batch transaction.");

        drs::engine::AuthoringStructureBatchPatch groupPatch;
        groupPatch.layerId = "layer-strings";
        const auto groupPatchResult = session.applyStructureBatchPatch(
            drs::engine::AuthoringStructureEntityKind::group,
            { "group-piano-sustain", "group-piano-release" }, groupPatch, "Move selected groups");
        require(groupPatchResult.applied
                    && session.getProject().authoring.groups[0].layerId == "layer-strings"
                    && session.getProject().authoring.groups[1].layerId == "layer-strings",
                "Group parent reassignment should apply atomically to every selected group.");
        drs::engine::AuthoringStructureBatchPatch layerPatch;
        layerPatch.gainDb = -2.0;
        const auto layerPatchResult = session.applyStructureBatchPatch(
            drs::engine::AuthoringStructureEntityKind::layer,
            { "layer-piano", "layer-strings" }, layerPatch, "Set selected layer gain");
        require(layerPatchResult.applied
                    && session.getProject().authoring.layers[0].gainDb == -2.0
                    && session.getProject().authoring.layers[1].gainDb == -2.0,
                "Layer inspector edits should apply atomically to every selected layer.");
        drs::engine::AuthoringStructureBatchPatch nudgePatch;
        nudgePatch.gainDelta = 0.5;
        const auto nudgeResult = session.applyStructureBatchPatch(
            drs::engine::AuthoringStructureEntityKind::layer,
            { "layer-piano", "layer-strings" }, nudgePatch, "Nudge selected layer gain");
        require(nudgeResult.applied
                    && session.getProject().authoring.layers[0].gainDb == -1.5
                    && session.getProject().authoring.layers[1].gainDb == -1.5,
                "Relative gain nudges should apply to each selected entity without flattening mixed values.");

        std::cout << "Structure viewer contract tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Structure viewer contract tests failed: " << error.what() << "\n";
        return 1;
    }
}
