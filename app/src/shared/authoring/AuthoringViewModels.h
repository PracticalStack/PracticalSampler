#pragma once

#include <functional>
#include <string>

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
    bool canPreview = false;
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

struct SelectionSummaryCallbacks
{
    std::function<void()> onPreviewRequested;
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
} // namespace drs::app::authoring
