#pragma once

#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PerformancePublishController.h"
#include "drs/engine/PerformancePublishPresentation.h"
#include "drs/engine/PublishedMacroBinding.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/RuntimePresetState.h"
#include "drs/engine/RuntimeModel.h"
#include "drs/engine/SfzImportProjection.h"
#include "drs/engine/SfzImportReport.h"

#include <chrono>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace drs::engine
{
enum class HiseFrontendTargetKind
{
    plugin,
    standalone
};

struct HiseFrontendExportProfile
{
    std::string name;
    HiseFrontendTargetKind targetKind;
    bool useFrontend = false;
    bool isStandaloneApp = false;
    bool frontendIsPlugin = false;
    bool isStandaloneFrontend = false;
    bool requiresAsioSdk = false;
    bool requiresVst3Sdk = false;
    std::string sourceTemplate;
    std::string summary;
};

struct EngineMacroDescriptor
{
    std::string id;
    std::string name;
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;
    double currentValue = 0.0;
    std::string ownershipKey;
    std::string soundIntent;
    std::string currentEffect;
    bool publishedControl = false;
    bool exposedInPerformance = true;
    std::string sectionLabel;
    std::string parameterLabel;
    std::string valueUnit;
    PublishedMacroControlKind controlKind = PublishedMacroControlKind::knob;
    std::size_t authoredOrder = 0;
    std::string accessibilityDescription;
    double displayMinimum = 0.0;
    double displayMaximum = 1.0;
    std::string authoredId;
};

struct EngineArticulationDescriptor
{
    std::string id;
    std::string name;
    bool isDefault = false;
    bool selected = false;
};

struct EnginePreviewPlaybackSnapshot
{
    bool ready = false;
    bool attempted = false;
    bool succeeded = false;
    std::size_t draftRevision = 0;
    std::size_t preparedRevision = 0;
    int midiNote = 0;
    int velocity = 0;
    int effectiveMidiNote = 0;
    int effectiveVelocity = 0;
    std::string articulationId;
    std::string zoneId;
    bool pendingBuild = false;
    bool waitedForPage = false;
    bool acquiredPageLease = false;
    bool voiceFinished = false;
    std::string revisionState;
    std::string state;
    std::string errorMessage;
    std::string appliedMacroSummary;
};

struct EnginePerformanceSnapshot
{
    PreparedPlaybackWorkerStatus preparedScheduler;
    bool loaded = false;
    std::size_t draftRevision = 0;
    std::size_t previewRevision = 0;
    std::size_t publishedRevision = 0;
    std::uint64_t previewBuildId = 0;
    std::uint64_t publishedBuildId = 0;
    std::uint64_t previewPreparedBuildId = 0;
    std::uint64_t publishedPreparedBuildId = 0;
    std::string instrumentDisplayName;
    std::string presetId;
    std::string loadProfileId;
    std::string selectedArticulationId;
    std::string selectedArticulationName;
    bool previewPending = false;
    bool publishedPending = false;
    bool previewActivationEligible = false;
    bool publishedActivationEligible = false;
    std::string previewRevisionState;
    PerformancePublishPresentationState publishedPresentationState
        = PerformancePublishPresentationState::idle;
    std::string previewContentDigest;
    std::string publishedContentDigest;
    std::string previewPreparedContentDigest;
    std::string publishedPreparedContentDigest;
    std::string publishedRouteDigest;
    std::string publishedSourceProvenanceDigest;
    std::string publishedMacroSchemaDigest;
    bool playableRangeAvailable = false;
    int lowestPlayableNote = 36;
    int highestPlayableNote = 96;
    std::string playableRangeSource;
    std::string surfaceStateSource;
    std::string rendererMode;
    std::string draftPlaybackEvent;
    std::string loadIndicator;
    std::size_t preparedWorkerPendingCount = 0;
    std::size_t preparedWorkerConfiguredMaxPendingCount = 0;
    std::size_t preparedWorkerConfiguredMaxInFlightCount = 0;
    std::size_t preparedWorkerCancellationCount = 0;
    std::size_t preparedWorkerSupersededCount = 0;
    std::size_t preparedWorkerFailureCount = 0;
    std::uint64_t preparedWorkerActiveOwnershipBytes = 0;
    std::uint64_t preparedWorkerRetiredBytes = 0;
    std::string preparedWorkerEvent;
    std::string preparedWorkerLastCancellationLane;
    std::string preparedWorkerLastCancellationReason;
    std::string preparedWorkerLastSupersededLane;
    std::string preparedWorkerLastSupersededReason;
    std::size_t previewPreparedSampleCount = 0;
    std::size_t previewPreparedStreamCount = 0;
    std::size_t previewPreparedOwnershipRecordCount = 0;
    std::size_t publishedPreparedSampleCount = 0;
    std::size_t publishedPreparedStreamCount = 0;
    std::size_t publishedPreparedOwnershipRecordCount = 0;
    std::uint64_t previewPreparedBytes = 0;
    std::uint64_t publishedPreparedBytes = 0;
    std::uint64_t previewPreparedOwnershipBytes = 0;
    std::uint64_t publishedPreparedOwnershipBytes = 0;
    std::uint64_t previewPreparedBuildMicros = 0;
    std::uint64_t publishedPreparedBuildMicros = 0;
    std::uint64_t previewPreparedDecodedBytes = 0;
    std::uint64_t publishedPreparedDecodedBytes = 0;
    std::uint64_t previewPreparedSampleDataBytes = 0;
    std::uint64_t publishedPreparedSampleDataBytes = 0;
    std::uint64_t previewActivationPayloadBytes = 0;
    std::uint64_t publishedActivationPayloadBytes = 0;
    std::uint64_t retainedActivationPayloadBytes = 0;
    std::size_t previewPreparationCacheHits = 0;
    std::size_t previewPreparationCacheMisses = 0;
    std::size_t publishedPreparationCacheHits = 0;
    std::size_t publishedPreparationCacheMisses = 0;
    double previewPreparationCacheHitRate = 0.0;
    double publishedPreparationCacheHitRate = 0.0;
    std::size_t preparedWorkerActiveOwnershipRecordCount = 0;
    std::size_t preparedWorkerRetiredOwnershipRecordCount = 0;
    std::size_t preparedCacheRetentionWorkingSetCount = 0;
    std::uint64_t preparedCacheWorkingSetBytes = 0;
    std::uint64_t preparedCacheByteBudget = 0;
    std::uint64_t preparedCacheResidentBytes = 0;
    std::uint64_t preparedCacheHeadroomBytes = 0;
    std::string preparedCachePressureState;
    std::vector<PlaybackSnapshotFinding> previewFindings;
    std::vector<PlaybackSnapshotFinding> publishedFindings;
    EnginePreviewPlaybackSnapshot previewPlayback;
};

struct EngineDiagnosticsSnapshot
{
    PreparedPlaybackWorkerStatus preparedScheduler;
    bool available = false;
    bool hasFailure = false;
    std::size_t draftRevision = 0;
    std::size_t previewRevision = 0;
    std::size_t publishedRevision = 0;
    std::uint64_t previewBuildId = 0;
    std::uint64_t publishedBuildId = 0;
    std::uint64_t previewPreparedBuildId = 0;
    std::uint64_t publishedPreparedBuildId = 0;
    std::string headline;
    std::string presetId;
    std::string loadProfileId;
    std::string selectedArticulationId;
    bool previewPending = false;
    bool publishedPending = false;
    bool previewActivationEligible = false;
    bool publishedActivationEligible = false;
    std::string previewRevisionState;
    PerformancePublishPresentationState publishedPresentationState
        = PerformancePublishPresentationState::idle;
    std::string previewContentDigest;
    std::string publishedContentDigest;
    std::string previewPreparedContentDigest;
    std::string publishedPreparedContentDigest;
    std::string publishedRouteDigest;
    std::string publishedSourceProvenanceDigest;
    std::string publishedMacroSchemaDigest;
    bool playableRangeAvailable = false;
    int lowestPlayableNote = 36;
    int highestPlayableNote = 96;
    std::string playableRangeSource;
    std::string surfaceStateSource;
    std::string rendererMode;
    std::string draftPlaybackEvent;
    std::size_t preparedWorkerPendingCount = 0;
    std::size_t preparedWorkerConfiguredMaxPendingCount = 0;
    std::size_t preparedWorkerConfiguredMaxInFlightCount = 0;
    std::size_t preparedWorkerCancellationCount = 0;
    std::size_t preparedWorkerSupersededCount = 0;
    std::size_t preparedWorkerFailureCount = 0;
    std::size_t preparedWorkerMaxPendingCount = 0;
    std::uint64_t preparedWorkerActiveOwnershipBytes = 0;
    std::uint64_t preparedWorkerRetiredBytes = 0;
    std::string preparedWorkerEvent;
    std::string preparedWorkerLastCancellationLane;
    std::string preparedWorkerLastCancellationReason;
    std::string preparedWorkerLastSupersededLane;
    std::string preparedWorkerLastSupersededReason;
    std::size_t previewPreparedSampleCount = 0;
    std::size_t previewPreparedStreamCount = 0;
    std::size_t previewPreparedZoneCount = 0;
    std::size_t previewPreparedOwnershipRecordCount = 0;
    std::size_t publishedPreparedSampleCount = 0;
    std::size_t publishedPreparedStreamCount = 0;
    std::size_t publishedPreparedZoneCount = 0;
    std::size_t publishedPreparedOwnershipRecordCount = 0;
    std::uint64_t previewPreparedBytes = 0;
    std::uint64_t publishedPreparedBytes = 0;
    std::uint64_t previewPreparedOwnershipBytes = 0;
    std::uint64_t publishedPreparedOwnershipBytes = 0;
    std::uint64_t previewPreparedBuildMicros = 0;
    std::uint64_t publishedPreparedBuildMicros = 0;
    std::uint64_t previewPreparedDecodedBytes = 0;
    std::uint64_t publishedPreparedDecodedBytes = 0;
    std::uint64_t previewPreparedSampleDataBytes = 0;
    std::uint64_t publishedPreparedSampleDataBytes = 0;
    std::uint64_t previewActivationPayloadBytes = 0;
    std::uint64_t publishedActivationPayloadBytes = 0;
    std::uint64_t retainedActivationPayloadBytes = 0;
    std::size_t previewPreparationCacheHits = 0;
    std::size_t previewPreparationCacheMisses = 0;
    std::size_t publishedPreparationCacheHits = 0;
    std::size_t publishedPreparationCacheMisses = 0;
    double previewPreparationCacheHitRate = 0.0;
    double publishedPreparationCacheHitRate = 0.0;
    std::size_t preparedWorkerActiveOwnershipRecordCount = 0;
    std::size_t preparedWorkerRetiredOwnershipRecordCount = 0;
    std::size_t preparedCacheRetentionWorkingSetCount = 0;
    std::uint64_t preparedCacheWorkingSetBytes = 0;
    std::uint64_t preparedCacheByteBudget = 0;
    std::uint64_t preparedCacheResidentBytes = 0;
    std::uint64_t preparedCacheHeadroomBytes = 0;
    std::string preparedCachePressureState;
    std::size_t configuredMaxCachedPages = 0;
    std::uint64_t maxPrefetchBytesPerVoice = 0;
    std::size_t cacheHitCount = 0;
    std::size_t cacheMissCount = 0;
    std::size_t pageMissCount = 0;
    std::size_t backgroundReadCount = 0;
    std::size_t residentPageCount = 0;
    std::size_t pendingPageCount = 0;
    std::size_t activeVoiceCount = 0;
    std::size_t peakActiveVoiceCount = 0;
    std::size_t purgePassCount = 0;
    std::size_t dormantPurgeCount = 0;
    std::size_t evictedPageCount = 0;
    std::size_t lastPurgeEvictedPageCount = 0;
    std::uint64_t averageReadLatencyMicros = 0;
    std::uint64_t maxReadLatencyMicros = 0;
    std::uint64_t headFramesRead = 0;
    std::uint64_t headBytesRead = 0;
    std::vector<std::string> routedZones;
    std::string failureState;
    std::string lastContentProbeCategory;
    bool lastContentProbeFailedGracefully = false;
    std::string lastContentProbeState;
    std::vector<std::string> lastContentProbeIssues;
    std::vector<PlaybackSnapshotFinding> previewFindings;
    std::vector<PlaybackSnapshotFinding> publishedFindings;
    std::vector<std::string> issues;
};

struct EngineStatusSnapshot
{
    std::string mode;
    std::string integrationState;
    EngineDiagnosticsSnapshot diagnostics;
    std::string detail;
    std::vector<std::string> nextSteps;
};

struct EnginePresetStateRestoreResult
{
    bool restored = false;
    std::string state;
    std::vector<std::string> issues;
};

enum class EngineContentFailureCategory
{
    missingContent,
    badChecksum,
    schemaMismatch,
    partialCompiledArtifact
};

struct EngineContentFailureProbeResult
{
    bool attempted = false;
    bool failedGracefully = false;
    std::string categoryId;
    std::string state;
    std::vector<std::string> issues;
};

class EngineFacade
{
public:
    EngineFacade();

    std::vector<HiseFrontendExportProfile> getFrontendExportProfiles() const;
    bool serviceBackgroundWork();
    std::uint64_t getStateRevision() const { return stateRevision; }
    EngineStatusSnapshot getStatusSnapshot() const;
    RuntimeManifestLoadResult loadPhase1ReferenceInstrument() const;
    RuntimeStreamLoadResult loadPhase1ReferenceStream() const;
    EngineDiagnosticsSnapshot getDiagnosticsSnapshot() const;
    EnginePerformanceSnapshot getPerformanceSnapshot() const;
    const DraftPlaybackStatus& getDraftPlaybackStatus() const { return draftPlaybackContract.getStatus(); }
    PlaybackActivationPayloadPtr getPreviewActivationPayload() const
    {
        return draftPlaybackContract.getStatus().preview.activationPayload;
    }
    PlaybackActivationPayloadPtr getPerformanceActivationPayload() const
    {
        return draftPlaybackContract.getStatus().performance.activationPayload;
    }
    PlaybackActivationPayloadPtr getBootstrapPerformanceActivationPayload() const;
    PerformancePublishActivationPayloadPtr authorizePerformanceActivation(
        std::uint64_t nowMicros = 0);
    bool rejectPerformanceActivationStaging(
        const PerformancePublishActivationPayloadPtr& payload,
        PerformancePublishFinding finding);
    bool acknowledgePerformanceActivation(
        const PerformancePublishActivationPayloadPtr& payload,
        std::uint64_t nowMicros = 0);
    PreparedPlaybackWorkerStatus getPreparedPlaybackWorkerStatus() const
    {
        return preparedPlaybackService.getWorkerStatus();
    }
    PerformancePublishControllerSnapshot getPerformancePublishControllerSnapshot() const
    {
        return performancePublishController.getSnapshot();
    }
    std::shared_ptr<const PerformancePublishPresentationSnapshot>
        getPerformancePublishPresentationSnapshot() const
    {
        return std::atomic_load_explicit(&performancePublishPresentation,
                                         std::memory_order_acquire);
    }
    ImmutablePublishedMacroBindingTablePtr getActivePublishedMacroBindings() const
    {
        const auto payload = performancePublishController.getActiveActivationPayload();
        return payload != nullptr ? payload->macroBindings
                                  : ImmutablePublishedMacroBindingTablePtr {};
    }
    std::vector<EngineArticulationDescriptor> getArticulationDescriptors() const;
    std::vector<EngineMacroDescriptor> getMacroDescriptors() const;
    bool setSelectedArticulation(const std::string& articulationId);
    bool setMacroValue(const std::string& macroId, double value);
    bool stageDraftRevision(std::size_t revision);
    bool refreshPreviewToCurrentDraft();
    bool cancelPreviewPreparation(
        const std::string& reason = "Preview preparation superseded by a newer request");
    bool publishCurrentDraft();
    void closeDraftPlaybackProject(bool preservePublishedPerformance = false);
    bool reopenDraftPlaybackProject(std::size_t revision,
                                    bool preservePublishedPerformance = false);
    bool replaceDraftPlaybackAuthoringProject(RuntimeProjectModel project);
    bool restorePerformancePublishProjectGeneration(std::uint64_t projectGeneration);
    std::uint64_t getPerformancePublishProjectGeneration() const noexcept
    {
        return performancePublishProjectGeneration;
    }
    bool beginDraftPlaybackDeviceRestart();
    bool completeDraftPlaybackDeviceRestart(bool restored);
    bool waitForPreparedPlaybackIdle(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    EnginePreviewPlaybackSnapshot auditionPreviewNote(int midiNote, int velocity);
    SfzImportAnalysisResult analyzeSfzImportDocument(const std::string& sfzPath) const;
    SfzImportProjectionResult projectSfzImportDocument(const RuntimeProjectModel& baseProject,
                                                       const std::string& sfzPath) const;
    std::string exportPresetStateJson() const;
    EnginePresetStateRestoreResult restorePresetStateJson(const std::string& presetStateJson);
    EnginePresetStateRestoreResult restorePresetStateFile(const std::string& presetStatePath);
    EngineContentFailureProbeResult probeContentFailure(EngineContentFailureCategory category);
    void clearContentFailureProbe();
    void resetSessionStateToDefault();
    const RuntimeSessionStateSnapshot& getCurrentSessionState() const { return currentSessionState; }
    const EngineContentFailureProbeResult& getLastContentFailureProbe() const { return lastContentFailureProbe; }

private:
    struct PendingPreparedCompletion
    {
        PreparedPlaybackWorkLane lane = PreparedPlaybackWorkLane::preview;
        std::uint64_t contractRequestId = 0;
        PlaybackSnapshotBuildResult snapshotResult;
        PerformancePublishRequestIdentity publishIdentity;
    };

    PlaybackSnapshotBuildResult buildCurrentPlaybackSnapshot(bool activationRequested);
    PreparedPlaybackBuildResult buildRejectedPreparedPlayback(const PlaybackSnapshotBuildResult& snapshotResult);
    bool enqueuePreparedPlaybackBuild(std::uint64_t contractRequestId,
                                      const PlaybackSnapshotBuildResult& snapshotResult,
                                      PreparedPlaybackWorkLane lane,
                                      bool bootstrapPerformance = false);
    bool pumpPreparedPlaybackWorkerCompletions();
    void markStateChanged();
    void clearPendingPreparedCompletions();
    void discardSupersededPreviewPendingPreparedCompletions(std::uint64_t newestBuildId);
    void syncPreviewSnapshotFromDraftPlayback();
    void initializeDraftPlaybackContract(bool activatePerformanceRevision,
                                         bool bootstrapPreparedPlayback = true);
    void refreshDiagnosticsSnapshot();

    RuntimeManifestLoadResult referenceManifest;
    RuntimeStreamLoadResult referenceStream;
    RuntimeProjectLoadResult authoringProject;
    bool referenceInstrumentActive = false;
    RuntimeSessionStateSnapshot currentSessionState;
    EngineDiagnosticsSnapshot diagnosticsSnapshot;
    EnginePreviewPlaybackSnapshot previewPlaybackSnapshot;
    EngineContentFailureProbeResult lastContentFailureProbe;
    DraftPlaybackContract draftPlaybackContract;
    PerformancePublishController performancePublishController;
    PlaybackSnapshotBuilder playbackSnapshotBuilder;
    PreparedPlaybackService preparedPlaybackService;
    std::unordered_map<std::uint64_t, PendingPreparedCompletion> pendingPreparedCompletions;
    std::uint64_t stateRevision = 0;
    std::uint64_t nextPreviewVoiceId = 4000;
    std::uint64_t performancePublishProjectGeneration = 1;
    std::uint64_t nextPerformanceActivationToken = 1;
    std::shared_ptr<const PerformancePublishPresentationSnapshot> performancePublishPresentation;
    std::uint64_t nextPerformancePublishPresentationSequence = 1;
};
} // namespace drs::engine
