#pragma once

#include "drs/engine/RuntimeModel.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
enum class PlaybackSnapshotLifecycleState
{
    idle,
    preparing,
    ready,
    activating,
    active,
    failed,
    superseded,
    canceled
};

enum class PlaybackSnapshotFindingSeverity
{
    warning,
    error
};

struct PlaybackSnapshotSampleIdentity
{
    std::string sampleSourceId;
    std::string sourcePath;
    std::string role;
};

struct PlaybackSnapshotMacroTarget
{
    std::string parameterId;
    std::string parameterPath;
    std::string role;
};

struct PlaybackSnapshotMacroDefault
{
    std::string id;
    std::string name;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    std::vector<PlaybackSnapshotMacroTarget> targets;
};

struct PlaybackSnapshotFxSlotReference
{
    std::string id;
    std::string displayName;
    std::string effectType;
    bool bypassed = false;
};

struct PlaybackSnapshotRoutingBusReference
{
    std::string id;
    std::string displayName;
    std::string inputSourceId;
    std::vector<std::string> fxSlotIds;
};

struct PlaybackSnapshotArticulationRoute
{
    std::string articulationId;
    std::vector<std::string> zoneIds;
};

struct PlaybackSnapshotGroupRoute
{
    std::string groupId;
    std::vector<std::string> articulationIds;
    std::vector<std::string> zoneIds;
};

struct PlaybackSnapshotZone
{
    std::string id;
    std::string sampleSourceId;
    std::string displayName;
    std::string groupId;
    std::string articulationId;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    double gainDb = 0.0;
    double pan = 0.0;
    std::uint64_t sampleStartFrame = 0;
    bool loopEnabled = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    ZoneTriggerMode triggerMode = ZoneTriggerMode::gated;
};

// S3.7-T5 deferral note: this remains a public aggregate for current builder, facade, and
// regression-test plumbing. Treat it as a write-once build product after snapshot creation;
// a dedicated encapsulation pass should replace direct field access with const views once
// Sprint 4's shared-renderer read API stabilizes.
struct ImmutablePlaybackSnapshot
{
    std::string schemaName;
    int schemaVersion = 0;
    std::string projectId;
    std::string displayName;
    std::string sourceProjectSchemaName;
    int sourceProjectSchemaVersion = 0;
    std::string sourceAuthoringSchemaName;
    int sourceAuthoringSchemaVersion = 0;
    std::size_t draftRevision = 0;
    std::string selectedZoneId;
    std::string selectedPerformanceBankId;
    std::string contentDigest;
    std::vector<PlaybackSnapshotSampleIdentity> sampleIdentities;
    std::vector<PlaybackSnapshotMacroDefault> macroDefaults;
    std::vector<PlaybackSnapshotFxSlotReference> fxSlots;
    std::vector<PlaybackSnapshotRoutingBusReference> routingBuses;
    std::vector<PlaybackSnapshotArticulationRoute> articulationRoutes;
    std::vector<PlaybackSnapshotGroupRoute> groupRoutes;
    std::vector<PlaybackSnapshotZone> zones;
    std::vector<std::string> notes;
};

struct PlaybackSnapshotFinding
{
    PlaybackSnapshotFindingSeverity severity = PlaybackSnapshotFindingSeverity::warning;
    std::string code;
    std::string path;
    std::string message;
};

struct PlaybackSnapshotBuildRequest
{
    bool accepted = false;
    std::uint64_t buildId = 0;
    std::uint64_t cancellationId = 0;
    std::size_t requestedDraftRevision = 0;
    bool activationRequested = false;
    PlaybackSnapshotLifecycleState lifecycleState = PlaybackSnapshotLifecycleState::idle;
    std::string state;
};

struct PlaybackSnapshotBuildResult
{
    bool built = false;
    bool activationEligible = false;
    std::uint64_t buildId = 0;
    std::uint64_t cancellationId = 0;
    std::size_t requestedDraftRevision = 0;
    bool activationRequested = false;
    PlaybackSnapshotLifecycleState lifecycleState = PlaybackSnapshotLifecycleState::idle;
    std::uint64_t buildDurationMicros = 0;
    std::string state;
    std::vector<PlaybackSnapshotFinding> findings;
    ImmutablePlaybackSnapshot snapshot;
};

class PlaybackSnapshotBuilder
{
public:
    PlaybackSnapshotBuildRequest requestBuild(std::size_t draftRevision, bool activationRequested);
    PlaybackSnapshotBuildResult buildSnapshot(const PlaybackSnapshotBuildRequest& request,
                                              const RuntimeProjectModel& project) const;
    PlaybackSnapshotBuildResult cancelBuild(const PlaybackSnapshotBuildRequest& request,
                                            const std::string& state = "Snapshot build canceled") const;
    PlaybackSnapshotBuildResult supersedeBuild(const PlaybackSnapshotBuildRequest& request,
                                               std::uint64_t replacementBuildId,
                                               const std::string& state = "Snapshot build superseded") const;

private:
    std::uint64_t nextBuildId = 1;
};

std::string toString(PlaybackSnapshotLifecycleState state);
std::string toString(PlaybackSnapshotFindingSeverity severity);
std::string serializeImmutablePlaybackSnapshot(const ImmutablePlaybackSnapshot& snapshot);
std::string computePlaybackSnapshotContentDigest(const ImmutablePlaybackSnapshot& snapshot);
} // namespace drs::engine
