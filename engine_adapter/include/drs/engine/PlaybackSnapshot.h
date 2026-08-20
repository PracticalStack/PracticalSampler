#pragma once

#include "drs/engine/RuntimeModel.h"
#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/PerformanceProgram.h"

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

enum class PlaybackPreparationScope
{
    currentDraft,
    selectedZone,
    selectedGroup
};

struct PlaybackPreparationScopeRequest
{
    PlaybackPreparationScope scope = PlaybackPreparationScope::currentDraft;
    std::string selectedZoneId;
    std::string selectedGroupId;
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
    std::string dspSlotId;
    std::string dspParameterId;
    double sourceMinimum = 0.0;
    double sourceMaximum = 1.0;
    double destinationMinimum = 0.0;
    double destinationMaximum = 1.0;
    std::string curve = "linear";
    RuntimeProjectMacroTargetControlLaw controlLaw;
};

struct PlaybackSnapshotMacroDefault
{
    std::string id;
    std::string name;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    bool exposedInPerformance = false;
    std::vector<PlaybackSnapshotMacroTarget> targets;
};

struct PlaybackSnapshotFxSlotReference
{
    struct ParameterValue
    {
        std::string id;
        double value = 0.0;
    };

    std::string id;
    std::string displayName;
    std::string effectType;
    bool bypassed = false;
    std::uint32_t effectVersion = 0;
    std::vector<ParameterValue> parameters;
    bool unavailable = false;
    bool legacyInert = false;
    bool catalogResolved = false;
    std::vector<CuratedDspScope> supportedScopes;
    CuratedDspStateClass stateClass = CuratedDspStateClass::stateless;
    CuratedDspCostMetadata cost;
};

struct PlaybackSnapshotRoutingBusReference
{
    std::string id;
    std::string displayName;
    std::string inputSourceId;
    std::vector<std::string> fxSlotIds;
    bool chainBypassed = false;
};

struct PlaybackSnapshotArticulationRoute
{
    std::string articulationId;
    std::vector<std::string> zoneIds;
};

struct PlaybackSnapshotArticulationDefinition
{
    std::string id;
    std::string displayName;
    bool isDefault = false;
    int displayOrder = 0;
    std::optional<RuntimeProjectArticulationActivationDefinition> activation;
};

struct PlaybackSnapshotLayerRoute
{
    std::string layerId;
    std::vector<std::string> groupIds;
    std::vector<std::string> zoneIds;
    std::string displayName;
    int displayOrder = 0;
    std::string routingSourceId;
    bool workspaceVisible = true;
    double gainDb = 0.0;
    double pan = 0.0;
    std::string routingBusId;
    std::string auditionAnchorGroupId;
    RuntimeProjectLayerCrossfadeDefinition crossfade;
};

struct PlaybackSnapshotGroupRoute
{
    std::string groupId;
    std::vector<std::string> articulationIds;
    std::vector<std::string> zoneIds;
    std::string displayName;
    int displayOrder = 0;
    std::string routingSourceId;
    bool workspaceVisible = true;
    double gainDb = 0.0;
    double pan = 0.0;
    std::string routingBusId;
    std::string auditionAnchorZoneId;
    std::string layerId;
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
    VelocityCrossfadeDescriptor velocityCrossfade;
    VelocityCrossfadeRuntimeDescriptor velocityCrossfadeRuntime;
    double gainDb = 0.0;
    double pan = 0.0;
    std::uint64_t sampleStartFrame = 0;
    bool loopEnabled = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    double releaseSeconds = 0.0;
    double releaseShape = 0.0;
    std::optional<RoundRobinDescriptor> roundRobin;
    int roundRobinLength = 0;
    int roundRobinPosition = 0;
    ZoneTriggerMode triggerMode = ZoneTriggerMode::gated;
    RuntimeProjectZonePerformanceDefinition performance;
    std::string exclusiveGroupId;
    std::vector<std::string> exclusiveTargetGroupIds;
    std::optional<double> chokeReleaseSeconds;
    double fineTuneCents = 0.0;
    double amplitudeVelocityTracking = 100.0;
    std::vector<RuntimeControllerCondition> controllerConditions;
    ContinuousDamperDefinition damper;
    RegionLoopMode loopMode = RegionLoopMode::noLoop;
    std::uint64_t sampleEndFrame = 0;
    std::uint64_t loopCrossfadeFrames = 0;
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
    std::string selectedGroupId;
    std::string selectedLayerId;
    std::string selectedPerformanceBankId;
    double masterGainDb = 0.0;
    std::string contentDigest;
    std::string dspGraphDigest;
    std::vector<PlaybackSnapshotSampleIdentity> sampleIdentities;
    std::vector<PlaybackSnapshotMacroDefault> macroDefaults;
    std::vector<PlaybackSnapshotFxSlotReference> fxSlots;
    std::vector<PlaybackSnapshotRoutingBusReference> routingBuses;
    std::vector<PlaybackSnapshotArticulationRoute> articulationRoutes;
    std::vector<PlaybackSnapshotArticulationDefinition> articulationDefinitions;
    std::vector<PlaybackSnapshotLayerRoute> layerRoutes;
    std::vector<PlaybackSnapshotGroupRoute> groupRoutes;
    std::vector<PlaybackSnapshotZone> zones;
    std::vector<RuntimeControllerDefault> controllerDefaults;
    std::vector<RuntimeProjectRoundRobinResetRuleDefinition> roundRobinResetRules;
    CompiledPerformanceProgram performanceProgram;
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
    PlaybackPreparationScope preparationScope = PlaybackPreparationScope::currentDraft;
    std::string preparationSelectedZoneId;
    std::string preparationSelectedGroupId;
    std::size_t unscopedZoneCount = 0;
    std::size_t retainedZoneCount = 0;
    std::size_t unscopedSampleCount = 0;
    std::size_t retainedSampleCount = 0;
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
std::string computePlaybackSnapshotDspGraphDigest(const ImmutablePlaybackSnapshot& snapshot);
PlaybackSnapshotBuildResult scopePlaybackSnapshotForPreparation(
    const PlaybackSnapshotBuildResult& source,
    const PlaybackPreparationScopeRequest& request);
std::string toString(PlaybackPreparationScope scope);
} // namespace drs::engine
