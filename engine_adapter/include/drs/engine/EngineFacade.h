#pragma once

#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageV2.h"
#include "drs/engine/SampleDataSource.h"
#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PlaybackSnapshotWorker.h"
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
#include <optional>
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
    CompiledControlLaw controlLaw;
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
    std::string contentRootPath;
    std::string backgroundArtworkSourceKey;
    std::shared_ptr<const std::vector<std::uint8_t>> backgroundArtworkJpgBytes;
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

struct EnginePerformancePackageActivationResult
{
    bool activated = false;
    PerformancePackageFailureCategory failureCategory = PerformancePackageFailureCategory::none;
    std::string state;
    std::vector<std::string> issues;
};

struct PerformancePackagePreparationTimings
{
    std::uint64_t packageLoadMicros = 0;
    std::uint64_t snapshotBuildMicros = 0;
    std::uint64_t preparedBuildMicros = 0;
    std::uint64_t activationPayloadMicros = 0;
    std::uint64_t renderModelBuildMicros = 0;
    std::uint64_t engineSessionActivationMicros = 0;
    std::uint64_t workspaceTransitionMicros = 0;
    std::uint64_t totalMicros = 0;
};

struct PreparedPerformancePackageActivationResult
{
    bool prepared = false;
    PerformancePackageFailureCategory failureCategory = PerformancePackageFailureCategory::none;
    std::string state;
    std::vector<std::string> issues;
    PerformancePackageLoadResult packageLoad;
    PlaybackSnapshotBuildResult snapshotResult;
    PreparedPlaybackBuildResult preparedResult;
    PlaybackActivationPayloadPtr activationPayload;
    SamplerRenderModelPtr renderModel;
    PerformancePackagePreparationTimings timings;
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
    std::uint64_t getPerformancePublishLifecycleRevision() const noexcept
    {
        return performancePublishLifecycleRevision;
    }
    std::uint64_t getPerformanceMacroTopologyRevision() const noexcept
    {
        return performanceMacroTopologyRevision;
    }
    std::uint64_t getPerformanceMacroValueRevision() const noexcept
    {
        return performanceMacroValueRevision;
    }
    std::uint64_t getPerformanceTelemetryRevision() const noexcept
    {
        return performanceTelemetryRevision;
    }
    EngineStatusSnapshot getStatusSnapshot() const;
    RuntimeManifestLoadResult loadPhase1ReferenceInstrument() const;
    RuntimeStreamLoadResult loadPhase1ReferenceStream() const;
    EnginePerformancePackageActivationResult activatePreparedPerformancePackageSession(
        PreparedPerformancePackageActivationResult preparedActivation);
    EnginePerformancePackageActivationResult openPerformancePackageSession(
        const PerformancePackageLoadResult& packageLoad);
    EnginePerformancePackageActivationResult activatePerformancePackageSession(
        const PerformancePackageLoadResult& packageLoad);
    void restoreBundledReferenceRuntimeSession();
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
    PlaybackActivationPayloadPtr getPerformancePackageActivationPayload() const
    {
        return packagePerformanceActivationPayload;
    }
    SamplerRenderModelPtr getPerformancePackageRenderModel() const
    {
        return packagePerformanceRenderModel;
    }
    std::shared_ptr<const std::string> getPerformancePackageLicenseText() const
    {
        return packageLicenseText;
    }
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
                                  : packagePublishedMacroBindings;
    }
    std::vector<EngineArticulationDescriptor> getArticulationDescriptors() const;
    std::vector<EngineMacroDescriptor> getMacroDescriptors() const;
    bool setSelectedArticulation(const std::string& articulationId);
    bool setMacroValue(const std::string& macroId, double value);
    bool stageDraftRevision(std::size_t revision);
    bool refreshPreviewToCurrentDraft();
    bool refreshPreviewForPreparationScope(const PlaybackPreparationScopeRequest& scopeRequest,
                                           bool forceRebuild = false);
    bool cancelPreviewPreparation(
        const std::string& reason = "Preview preparation superseded by a newer request");
    bool publishCurrentDraft();
    void closeDraftPlaybackProject(bool preservePublishedPerformance = false);
    bool reopenDraftPlaybackProject(std::size_t revision,
                                    bool preservePublishedPerformance = false);
    bool replaceDraftPlaybackAuthoringProject(RuntimeProjectModel project);
    std::shared_ptr<const RuntimeProjectModel> getDraftPlaybackAuthoringProjectPublication() const
    {
        return authoringProjectPublication;
    }
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
    RuntimePresetStateValidationResult validateProjectPresetState(
        const RuntimePresetState& presetState,
        const RuntimeProjectModel& project) const;
    EnginePresetStateRestoreResult restoreProjectPresetState(
        const RuntimePresetState& presetState,
        const RuntimeProjectModel& project);
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
        bool performancePackage = false;
        std::uint64_t contractRequestId = 0;
        PlaybackSnapshotBuildResult snapshotResult;
        PerformancePublishRequestIdentity publishIdentity;
    };

    struct ReusablePreviewPreparation
    {
        PlaybackSnapshotBuildResult snapshotResult;
        PreparedPlaybackBuildResult preparedResult;
    };

    struct PendingPreviewReusePublish
    {
        std::uint64_t contractRequestId = 0;
        PerformancePublishRequestIdentity publishIdentity;
    };

    PlaybackSnapshotBuildResult buildCurrentPlaybackSnapshot(bool activationRequested);
    PreparedPlaybackBuildResult buildRejectedPreparedPlayback(const PlaybackSnapshotBuildResult& snapshotResult);
    bool enqueuePreparedPlaybackBuild(std::uint64_t contractRequestId,
                                      const PlaybackSnapshotBuildResult& snapshotResult,
                                      PreparedPlaybackWorkLane lane,
                                      bool bootstrapPerformance = false,
                                      std::string precomputedMacroSchemaDigest = {});
    bool enqueuePerformancePackagePreparedBuild(const PlaybackSnapshotBuildResult& snapshotResult);
    bool pumpPlaybackSnapshotWorkerCompletions();
    bool pumpPreparedPlaybackWorkerCompletions();
    bool completePendingPreviewReusePublish();
    bool canReuseCurrentFullDraftPreview() const;
    void markStateChanged();
    void clearPendingPreparedCompletions();
    void discardSupersededPreviewPendingPreparedCompletions(std::uint64_t newestBuildId);
    void syncPreviewSnapshotFromDraftPlayback();
    void initializeDraftPlaybackContract(bool activatePerformanceRevision,
                                         bool bootstrapPreparedPlayback = true);
    void refreshDiagnosticsSnapshot();
    void refreshPerformanceMacroRevisions();

    RuntimeManifestLoadResult bundledReferenceManifest;
    RuntimeStreamLoadResult bundledReferenceStream;
    RuntimeManifestLoadResult referenceManifest;
    RuntimeStreamLoadResult referenceStream;
    RuntimeProjectLoadResult authoringProject;
    std::shared_ptr<const RuntimeProjectModel> authoringProjectPublication;
    bool referenceInstrumentActive = false;
    PlaybackActivationPayloadPtr packagePerformanceActivationPayload;
    SamplerRenderModelPtr packagePerformanceRenderModel;
    ImmutablePublishedMacroBindingTablePtr packagePublishedMacroBindings;
    std::string packageBackgroundArtworkPayloadId;
    std::shared_ptr<const std::vector<std::uint8_t>> packageBackgroundArtworkJpgBytes;
    std::shared_ptr<const std::string> packageLicenseText;
    RuntimeSessionStateSnapshot currentSessionState;
    EngineDiagnosticsSnapshot diagnosticsSnapshot;
    EnginePreviewPlaybackSnapshot previewPlaybackSnapshot;
    EngineContentFailureProbeResult lastContentFailureProbe;
    DraftPlaybackContract draftPlaybackContract;
    PerformancePublishController performancePublishController;
    PlaybackSnapshotBuilder playbackSnapshotBuilder;
    PlaybackSnapshotWorker playbackSnapshotWorker;
    PreparedPlaybackService preparedPlaybackService;
    std::unordered_map<std::uint64_t, PendingPreparedCompletion> pendingPreparedCompletions;
    std::optional<ReusablePreviewPreparation> reusablePreviewPreparation;
    std::optional<PendingPreviewReusePublish> pendingPreviewReusePublish;
    std::uint64_t stateRevision = 0;
    std::uint64_t nextPreviewVoiceId = 4000;
    std::uint64_t performancePublishProjectGeneration = 1;
    std::uint64_t nextPerformanceActivationToken = 1;
    std::shared_ptr<const PerformancePublishPresentationSnapshot> performancePublishPresentation;
    std::uint64_t nextPerformancePublishPresentationSequence = 1;
    std::uint64_t performancePublishLifecycleRevision = 0;
    std::uint64_t performanceMacroTopologyRevision = 0;
    std::uint64_t performanceMacroValueRevision = 0;
    std::uint64_t performanceTelemetryRevision = 0;
    std::vector<EngineMacroDescriptor> lastPerformanceMacroDescriptors;
};

PreparedPerformancePackageActivationResult preparePerformancePackageActivation(
    const PerformancePackageLoadResult& packageLoad,
    const PerformancePackagePreparationTimings& priorTimings = {});

PreparedPerformancePackageActivationResult preparePerformancePackageV2Activation(
    const PerformancePackageLoadResult& packageLoad,
    std::shared_ptr<const PackageV2OpenResult> package,
    const std::vector<SampleDataSourceDescriptor>& sampleDescriptors,
    const PerformancePackagePreparationTimings& priorTimings = {});
} // namespace drs::engine
