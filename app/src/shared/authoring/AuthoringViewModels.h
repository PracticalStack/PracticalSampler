#pragma once

#include <functional>
#include <string>
#include <vector>

namespace drs::app::authoring
{
enum class DrawerTab
{
    waveform,
    macros,
    routing,
    performance
};

struct DrawerState
{
    bool open = false;
    DrawerTab activeTab = DrawerTab::waveform;
};

struct SelectionSummaryViewModel
{
    std::string title;
    std::string statusText;
    std::string sourceText;
    std::string articulationText;
    std::string playbackText;
    bool canPreview = false;
    bool canPrepareDraftPlayback = false;
    bool canPublishDraftPlayback = false;
    bool canRestoreRootKey = false;
    bool canUndo = false;
    bool canRedo = false;
    bool dirty = false;
};

struct ZoneFieldValuesViewModel
{
    bool hasSelection = false;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    double gainDb = 0.0;
    double pan = 0.0;
    bool loopEnabled = false;
    std::string emptyStateText;
};

struct RepeatedStructureRowViewModel
{
    std::string key;
    std::string title;
    std::string statusText;
    bool enabled = true;
};

struct RepeatedStructureListViewModel
{
    std::string emptyStateText;
    std::vector<RepeatedStructureRowViewModel> rows;
    int selectedIndex = -1;
};

struct RepeatedStructureSelectionPathViewModel
{
    std::string scopeLabel;
    std::string breadcrumbText;
};

struct RepeatedStructureDetailViewModel
{
    std::string title;
    std::string statusText;
    std::string bodyText;
};

struct RepeatedStructurePaneViewModel
{
    std::string title;
    RepeatedStructureSelectionPathViewModel selectionPath;
    RepeatedStructureListViewModel list;
    RepeatedStructureDetailViewModel detail;
};

struct RepeatedStructureEditIntent
{
    std::string rowKey;
    std::string actionId;
    std::string actionLabel;
};

struct SelectionSummaryCallbacks
{
    std::function<void()> onPreviewRequested;
    std::function<void()> onPrepareDraftPlaybackRequested;
    std::function<void()> onPublishDraftPlaybackRequested;
    std::function<void()> onUndoRequested;
    std::function<void()> onRedoRequested;
    std::function<void()> onMarkSavedRequested;
};

struct ZoneFieldCallbacks
{
    std::function<void(const ZoneFieldValuesViewModel&, const std::string&)> onCommitRequested;
    std::function<void()> onRestoreRootKeyRequested;
};

using RefreshEditIntent = std::function<void(const std::string&)>;
using RepeatedStructureSelectionCallback = std::function<void(int)>;
using RepeatedStructureEditIntentHandler = std::function<void(const RepeatedStructureEditIntent&)>;

struct RepeatedStructureAdapterCallbacks
{
    RepeatedStructureSelectionCallback onSelectionRequested;
    RepeatedStructureEditIntentHandler onEditIntentRequested;
};
} // namespace drs::app::authoring
