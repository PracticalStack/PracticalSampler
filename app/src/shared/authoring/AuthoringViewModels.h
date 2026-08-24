#pragma once

#include "drs/engine/RuntimeModel.h"

#include <functional>
#include <string>
#include <vector>

namespace drs::app::authoring
{
enum class WorkbenchTab
{
    waveform,
    groups,
    macros,
    routing,
    performance,
    articulations,
    instrumentControls
};

struct WorkbenchState
{
    bool open = false;
    WorkbenchTab activeTab = WorkbenchTab::waveform;
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
    drs::engine::RegionLoopMode loopMode = drs::engine::RegionLoopMode::noLoop;
    std::uint64_t sampleEndFrame = 0;
    double releaseSeconds = drs::engine::nativeDefaultReleaseSeconds;
    double releaseShape = 0.0;
    bool roundRobinEnabled = false;
    std::string roundRobinPoolText;
    std::string roundRobinSlotText;
    std::string roundRobinModeText;
    std::string roundRobinHintText;
    bool canCreateRoundRobinPool = false;
    bool canAddCompatibleZonesToRoundRobinPool = false;
    bool canNormalizeRoundRobinPool = false;
    bool canRemoveZoneFromRoundRobinPool = false;
    bool previewAdvancesRoundRobin = false;
    drs::engine::ZoneTriggerMode triggerMode = drs::engine::ZoneTriggerMode::gated;
    drs::engine::PerformanceEventKind performanceEvent = drs::engine::PerformanceEventKind::noteOn;
    drs::engine::PerformanceSustainCondition performanceSustain = drs::engine::PerformanceSustainCondition::any;
    drs::engine::PerformancePitchSource performancePitchSource = drs::engine::PerformancePitchSource::eventNote;
    std::string exclusiveGroupId;
    std::string exclusiveTargetGroupId;
    std::vector<std::string> exclusiveGroupIds;
    double chokeReleaseSeconds = 0.0;
    std::string articulationId;
    std::vector<std::string> articulationIds;
    bool hasMultipleZoneSelection = false;
    bool crossfadeHasFadeIn = false;
    bool crossfadeHasFadeOut = false;
    bool crossfadeCanCreate = false;
    bool crossfadeCanEdit = false;
    bool crossfadeCanRemove = false;
    bool crossfadeCanCreateStack = false;
    bool crossfadeCanRemoveStack = false;
    bool crossfadeCanAudition = false;
    int crossfadeOverlapLow = 1;
    int crossfadeOverlapHigh = 2;
    std::string crossfadeLowerZoneId;
    std::string crossfadeUpperZoneId;
    std::string crossfadeFadeInLowerZoneId;
    std::string crossfadeFadeInUpperZoneId;
    std::string crossfadeFadeOutLowerZoneId;
    std::string crossfadeFadeOutUpperZoneId;
    std::string crossfadeFadeInText;
    std::string crossfadeFadeOutText;
    std::string crossfadeGuidanceText;
    std::vector<std::string> crossfadeStackZoneIds;
    std::string crossfadeStackPreviewText;
    std::vector<int> crossfadeAuditionVelocities;
    std::string crossfadeAuditionText;
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
    std::function<void(const std::string&)> onArticulationCommitRequested;
    std::function<void(const std::string&, const std::string&, int, int)> onCreateVelocityCrossfadeRequested;
    std::function<void(const std::string&, const std::string&, int, int)> onUpdateVelocityCrossfadeRequested;
    std::function<void(const std::string&, const std::string&)> onRemoveVelocityCrossfadeRequested;
    std::function<void(const std::vector<std::string>&, int)> onCreateVelocityCrossfadeStackRequested;
    std::function<void(const std::vector<std::string>&)> onRemoveVelocityCrossfadeStackRequested;
    std::function<void(const std::vector<int>&)> onAuditionVelocityCrossfadeRequested;
    std::function<void()> onCreateChokeGroupRequested;
    std::function<void()> onRestoreRootKeyRequested;
    std::function<void()> onRevealInStructureRequested;
    std::function<void()> onOpenWaveformRequested;
    std::function<void()> onPreviewRequested;
    std::function<void()> onCreateRoundRobinPoolRequested;
    std::function<void()> onAddCompatibleZonesToRoundRobinPoolRequested;
    std::function<void()> onNormalizeRoundRobinPoolRequested;
    std::function<void()> onRemoveSelectedZoneFromRoundRobinPoolRequested;
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
