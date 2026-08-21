#pragma once

#include <algorithm>
#include <string>
#include <optional>
#include <utility>
#include "drs/engine/RuntimeModel.h"

namespace drs::app::authoring
{
enum class StructureViewMode
{
    map,
    structure
};

enum class StructureSortMode
{
    authoredOrder,
    name,
    keyLow,
    rootKey,
    velocityLow,
    diagnostic
};

// Session-local presentation state. It intentionally has no project/document
// serialization API: the Structure viewer must not dirty an authored project.
class StructureViewState
{
public:
    struct Snapshot
    {
        StructureViewMode mode = StructureViewMode::map;
        int layerColumnWidth = 188;
        int groupColumnWidth = 220;
        int zoneColumnWidth = 520;
        bool layerColumnCollapsed = false;
        bool groupColumnCollapsed = false;
        std::string searchText;
        StructureSortMode sortMode = StructureSortMode::authoredOrder;
        bool showOverlapsOnly = false;
        bool showPotentialCollisionsOnly = false;
        bool showExactStacksOnly = false;
        bool visibleOnly = false;
        std::string articulationFilter;
        std::optional<drs::engine::PerformanceEventKind> performanceEventFilter;
        int layerScrollAnchor = 0;
        int groupScrollAnchor = 0;
        int zoneScrollAnchor = 0;
    };

    Snapshot exportWorkspaceSnapshot() const
    {
        return { mode, layerColumnWidth, groupColumnWidth, zoneColumnWidth,
                 layerColumnCollapsed, groupColumnCollapsed, searchText, sortMode,
                 showOverlapsOnly, showPotentialCollisionsOnly, showExactStacksOnly, visibleOnly,
                 articulationFilter, performanceEventFilter,
                 layerScrollAnchor, groupScrollAnchor, zoneScrollAnchor };
    }

    void importWorkspaceSnapshot(const Snapshot& snapshot)
    {
        mode = snapshot.mode;
        setLayerColumnWidth(snapshot.layerColumnWidth);
        setGroupColumnWidth(snapshot.groupColumnWidth);
        setZoneColumnWidth(snapshot.zoneColumnWidth);
        layerColumnCollapsed = snapshot.layerColumnCollapsed;
        groupColumnCollapsed = snapshot.groupColumnCollapsed;
        searchText = snapshot.searchText;
        sortMode = snapshot.sortMode;
        showOverlapsOnly = snapshot.showOverlapsOnly;
        showPotentialCollisionsOnly = snapshot.showPotentialCollisionsOnly;
        showExactStacksOnly = snapshot.showExactStacksOnly;
        visibleOnly = snapshot.visibleOnly;
        articulationFilter = snapshot.articulationFilter;
        performanceEventFilter = snapshot.performanceEventFilter;
        layerScrollAnchor = std::max(0, snapshot.layerScrollAnchor);
        groupScrollAnchor = std::max(0, snapshot.groupScrollAnchor);
        zoneScrollAnchor = std::max(0, snapshot.zoneScrollAnchor);
    }
    void setMode(const StructureViewMode nextMode) noexcept { mode = nextMode; }
    StructureViewMode getMode() const noexcept { return mode; }

    void setLayerColumnWidth(const int width) noexcept { layerColumnWidth = clampWidth(width); }
    void setGroupColumnWidth(const int width) noexcept { groupColumnWidth = clampWidth(width); }
    void setZoneColumnWidth(const int width) noexcept { zoneColumnWidth = clampWidth(width); }
    int getLayerColumnWidth() const noexcept { return layerColumnWidth; }
    int getGroupColumnWidth() const noexcept { return groupColumnWidth; }
    int getZoneColumnWidth() const noexcept { return zoneColumnWidth; }

    void setLayerColumnCollapsed(const bool collapsed) noexcept { layerColumnCollapsed = collapsed; }
    void setGroupColumnCollapsed(const bool collapsed) noexcept { groupColumnCollapsed = collapsed; }
    bool isLayerColumnCollapsed() const noexcept { return layerColumnCollapsed; }
    bool isGroupColumnCollapsed() const noexcept { return groupColumnCollapsed; }

    void setSearchText(std::string text) { searchText = std::move(text); }
    const std::string& getSearchText() const noexcept { return searchText; }
    void setShowOverlapsOnly(const bool value) noexcept { showOverlapsOnly = value; }
    void setShowPotentialCollisionsOnly(const bool value) noexcept { showPotentialCollisionsOnly = value; }
    void setShowExactStacksOnly(const bool value) noexcept { showExactStacksOnly = value; }
    void setVisibleOnly(const bool value) noexcept { visibleOnly = value; }
    bool getShowOverlapsOnly() const noexcept { return showOverlapsOnly; }
    bool getShowPotentialCollisionsOnly() const noexcept { return showPotentialCollisionsOnly; }
    bool getShowExactStacksOnly() const noexcept { return showExactStacksOnly; }
    bool getVisibleOnly() const noexcept { return visibleOnly; }
    void setArticulationFilter(std::string value) { articulationFilter = std::move(value); }
    void setPerformanceEventFilter(std::optional<drs::engine::PerformanceEventKind> value) { performanceEventFilter = value; }
    const std::string& getArticulationFilter() const noexcept { return articulationFilter; }
    const auto& getPerformanceEventFilter() const noexcept { return performanceEventFilter; }

    void setSortMode(const StructureSortMode nextSortMode) noexcept { sortMode = nextSortMode; }
    StructureSortMode getSortMode() const noexcept { return sortMode; }

    void setLayerScrollAnchor(const int row) noexcept { layerScrollAnchor = std::max(0, row); }
    void setGroupScrollAnchor(const int row) noexcept { groupScrollAnchor = std::max(0, row); }
    void setZoneScrollAnchor(const int row) noexcept { zoneScrollAnchor = std::max(0, row); }
    int getLayerScrollAnchor() const noexcept { return layerScrollAnchor; }
    int getGroupScrollAnchor() const noexcept { return groupScrollAnchor; }
    int getZoneScrollAnchor() const noexcept { return zoneScrollAnchor; }

private:
    static int clampWidth(const int width) noexcept
    {
        return std::clamp(width, 96, 1200);
    }

    StructureViewMode mode = StructureViewMode::map;
    int layerColumnWidth = 188;
    int groupColumnWidth = 220;
    int zoneColumnWidth = 520;
    bool layerColumnCollapsed = false;
    bool groupColumnCollapsed = false;
    std::string searchText;
    bool showOverlapsOnly = false;
    bool showPotentialCollisionsOnly = false;
    bool showExactStacksOnly = false;
    bool visibleOnly = false;
    std::string articulationFilter;
    std::optional<drs::engine::PerformanceEventKind> performanceEventFilter;
    StructureSortMode sortMode = StructureSortMode::authoredOrder;
    int layerScrollAnchor = 0;
    int groupScrollAnchor = 0;
    int zoneScrollAnchor = 0;
};
} // namespace drs::app::authoring
