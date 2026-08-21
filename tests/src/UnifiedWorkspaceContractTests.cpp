#include "shared/authoring/AuthoringStructureSelection.h"
#include "shared/authoring/InstrumentStructureBrowser.h"
#include "shared/authoring/ScopedZoneProjection.h"
#include "shared/authoring/StructureScope.h"
#include "shared/authoring/ZoneMapOverlapIndex.h"
#include "shared/authoring/StructureInspector.h"
#include "shared/authoring/StructureViewState.h"
#include "shared/authoring/ZoneMapCanvas.h"
#include "drs/engine/AuthoringSession.h"
#include "support/StructureViewerFixtures.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::vector<std::string> idsOf(const std::vector<drs::engine::AuthoringZoneSummary>& summaries)
{
    std::vector<std::string> ids;
    ids.reserve(summaries.size());
    for (const auto& summary : summaries)
        ids.push_back(summary.id);
    return ids;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI juceInitialiser;
        const auto project = drs::tests::makeStructureViewerFixture();
        drs::engine::AuthoringSession session(project);

        drs::app::authoring::ZoneMapCanvas map;
        map.setZoneSummaries(session.getZoneSummaries());
        map.setSelectionState({ { "zone-piano-low" }, "zone-piano-low" });

        drs::app::authoring::StructureViewState viewState;
        const auto viewBefore = viewState.exportWorkspaceSnapshot();
        const auto mapBefore = idsOf(map.getZoneSummaries());
        const auto documentBefore = session.getDocumentState();

        drs::app::authoring::AuthoringStructureSelection selection;
        selection.replace(drs::app::authoring::StructureSelectionKind::layer,
                          { "layer-piano" },
                          "layer-piano");
        require(session.selectLayer("layer-piano").applied,
                "Layer selection should apply through the session.");
        selection.replace(drs::app::authoring::StructureSelectionKind::group,
                          { "group-piano-sustain" },
                          "group-piano-sustain");
        require(session.selectGroup("group-piano-sustain").applied,
                "Group selection should apply through the session.");
        selection.replace(drs::app::authoring::StructureSelectionKind::zone,
                          { "zone-piano-high" },
                          "zone-piano-high");
        require(session.selectZone("zone-piano-high").applied,
                "Zone selection should apply through the session.");

        const auto viewAfter = viewState.exportWorkspaceSnapshot();
        const auto mapAfter = idsOf(map.getZoneSummaries());
        const auto documentAfter = session.getDocumentState();

        require(mapBefore == mapAfter,
                "Selection-only navigation must not change the current Zone Map input.");
        require(viewBefore.mode == viewAfter.mode
                    && viewBefore.searchText == viewAfter.searchText
                    && viewBefore.visibleOnly == viewAfter.visibleOnly
                    && viewBefore.layerScrollAnchor == viewAfter.layerScrollAnchor
                    && viewBefore.groupScrollAnchor == viewAfter.groupScrollAnchor
                    && viewBefore.zoneScrollAnchor == viewAfter.zoneScrollAnchor,
                "Selection-only navigation must not change Map visibility or workspace presentation state.");
        require(documentBefore.revision == documentAfter.revision
                    && documentBefore.undoDepth == documentAfter.undoDepth
                    && documentBefore.dirty == documentAfter.dirty,
                "Selection-only navigation must not change document revision, undo depth, or dirty state.");
        require(!session.undo().applied && !session.redo().applied,
                "Selection-only navigation must not create undoable document actions.");

        using namespace drs::app::authoring;
        require(deriveStructureScope(project, selection)
                    == StructureScope { StructureScopeKind::group, "group-piano-sustain" },
                "A same-group zone selection should derive the group scope.");
        selection.replace(StructureSelectionKind::zone, { "zone-piano-low" }, "zone-piano-low");
        require(deriveStructureScope(project, selection)
                    == StructureScope { StructureScopeKind::group, "group-piano-sustain" },
                "A single zone selection should derive its parent group scope.");
        selection.replace(StructureSelectionKind::zone,
                          { "zone-piano-low", "zone-piano-release" },
                          "zone-piano-low");
        require(deriveStructureScope(project, selection)
                    == StructureScope { StructureScopeKind::layer, "layer-piano" },
                "A cross-group same-layer selection should derive the layer scope.");
        selection.replace(StructureSelectionKind::zone,
                          { "zone-piano-low", "zone-strings-a" },
                          "zone-piano-low");
        require(deriveStructureScope(project, selection).kind == StructureScopeKind::instrument,
                "A cross-layer selection should derive the instrument scope.");

        const auto groupProjection = buildScopedZoneProjection(
            project, StructureScope { StructureScopeKind::group, "group-piano-sustain" }, selection);
        require(groupProjection.totalInScope == 2 && groupProjection.zones.size() == 2,
                "Group projection should contain exactly the group's authored zones.");
        const auto layerProjection = buildScopedZoneProjection(
            project, StructureScope { StructureScopeKind::layer, "layer-piano" }, selection);
        require(layerProjection.totalInScope == 3 && layerProjection.zones.size() == 3,
                "Layer projection should include all groups in the layer.");

        const auto rows = buildInstrumentStructureRows(project, selection,
                                                        { kInstrumentStructureId, "layer-piano", "group-piano-sustain" });
        require(!rows.empty() && rows.front().kind == InstrumentStructureRowKind::instrument,
                "Structure browser must always expose a stable instrument root row.");
        require(std::any_of(rows.begin(), rows.end(), [](const auto& row)
                            { return row.id == "zone-piano-low" && row.depth == 3; }),
                "Structure browser must flatten visible zone descendants with hierarchy depth.");
        drs::app::authoring::InstrumentStructureBrowserOptions browserFilter;
        browserFilter.searchText = "Piano High";
        const auto filteredRows = buildInstrumentStructureRows(project, selection, {}, {}, browserFilter);
        require(std::any_of(filteredRows.begin(), filteredRows.end(), [](const auto& row)
                            { return row.id == "layer-piano"; })
                    && std::any_of(filteredRows.begin(), filteredRows.end(), [](const auto& row)
                                   { return row.id == "zone-piano-high"; })
                    && std::none_of(filteredRows.begin(), filteredRows.end(), [](const auto& row)
                                    { return row.id == "zone-strings-a"; }),
                "Browser search must retain matching ancestors without leaking unrelated branches.");

        const auto snapshotBefore = viewState.exportWorkspaceSnapshot();
        viewState.setMapPaneVisible(false);
        viewState.setScope({ StructureScopeKind::group, "group-piano-sustain" });
        viewState.setTreeWidth(410);
        viewState.setDisclosed("layer-piano", true);
        viewState.setCollapsed("group-piano-release", true);
        const auto snapshot = viewState.exportWorkspaceSnapshot();
        StructureViewState restored;
        restored.importWorkspaceSnapshot(snapshot);
        require(!restored.isMapPaneVisible()
                    && restored.getScope() == StructureScope { StructureScopeKind::group, "group-piano-sustain" }
                    && restored.getTreeWidth() == 410
                    && restored.isDisclosed("layer-piano")
                    && restored.isCollapsed("group-piano-release"),
                "Unified workspace state must round-trip scope, Map visibility, width, and disclosure.");
        require(snapshotBefore.mode == viewState.exportWorkspaceSnapshot().mode,
                "Workspace-only state changes must not repurpose the legacy Map/Structure mode value.");

        drs::app::authoring::ZoneMapOverlapIndex overlapIndex;
        overlapIndex.rebuild(map.getZoneSummaries());
        const auto candidates = overlapIndex.query(48, 96, "zone-piano-low");
        require(std::any_of(candidates.begin(), candidates.end(), [](const auto& candidate)
                            { return candidate.zoneId == "zone-piano-high"; }),
                "Overlap index should make coincident velocity/key zones reachable.");
        require(overlapIndex.query(1, 1, "zone-piano-low").empty(),
                "Overlap index should exclude zones outside the hit coordinate.");

        const auto edgeProject = drs::tests::makeUnifiedWorkspaceEdgeCaseFixture();
        const auto edgeRows = buildInstrumentStructureRows(edgeProject, {}, {});
        require(std::any_of(edgeRows.begin(), edgeRows.end(), [](const auto& row)
                            { return row.id == "layer-hidden" && !row.workspaceVisible; }),
                "Edge fixture must preserve hidden containers in the hierarchy model.");
        const auto hiddenFiltered = buildScopedZoneProjection(
            edgeProject, { StructureScopeKind::instrument, kInstrumentStructureId }, {},
            { false, false, {}, {} });
        require(std::none_of(hiddenFiltered.zones.begin(), hiddenFiltered.zones.end(),
                             [](const auto& zone) { return zone.id == "zone-hidden"; }),
                "Hidden-container policy must remove hidden zones when selected-hidden inclusion is disabled.");
        require(reconcileStructureScope(edgeProject,
                                        { StructureScopeKind::group, "group-missing-parent" }).kind
                    == StructureScopeKind::instrument,
                "Missing-parent scope reconciliation must use deterministic instrument fallback.");
        const auto emptyRows = buildInstrumentStructureRows(drs::tests::makeEmptyInstrumentFixture(), {}, {});
        require(emptyRows.size() == 1 && emptyRows.front().kind == InstrumentStructureRowKind::instrument,
                "Empty instrument must still render a selectable hierarchy root.");

        StructureInspector instrumentInspector;
        AuthoringStructureSelection instrumentSelection;
        instrumentSelection.replace(StructureSelectionKind::instrument, { kInstrumentStructureId }, kInstrumentStructureId);
        instrumentInspector.setSnapshot(buildStructureInspectorSnapshot(project, instrumentSelection));
        require(instrumentInspector.findChildWithID("authoringStructureInspectorPrimaryAction") != nullptr
                    && instrumentInspector.findChildWithID("authoringStructureInspectorPrimaryAction")->isVisible(),
                "Instrument context must expose the primary Show Zones action.");

        std::cout << "Unified workspace contract tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Unified workspace contract tests failed: " << error.what() << "\n";
        return 1;
    }
}
