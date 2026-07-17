#pragma once

#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/RuntimePresetState.h"
#include "drs/engine/RuntimeModel.h"

#include <string>
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
    bool loaded = false;
    std::size_t draftRevision = 0;
    std::size_t previewRevision = 0;
    std::size_t publishedRevision = 0;
    std::string instrumentDisplayName;
    std::string presetId;
    std::string loadProfileId;
    std::string selectedArticulationId;
    std::string selectedArticulationName;
    bool previewPending = false;
    bool publishedPending = false;
    std::string previewRevisionState;
    std::string publishedRevisionState;
    std::string draftPlaybackEvent;
    std::string loadIndicator;
    EnginePreviewPlaybackSnapshot previewPlayback;
};

struct EngineDiagnosticsSnapshot
{
    bool available = false;
    bool hasFailure = false;
    std::size_t draftRevision = 0;
    std::size_t previewRevision = 0;
    std::size_t publishedRevision = 0;
    std::string headline;
    std::string presetId;
    std::string loadProfileId;
    std::string selectedArticulationId;
    bool previewPending = false;
    bool publishedPending = false;
    std::string previewRevisionState;
    std::string publishedRevisionState;
    std::string draftPlaybackEvent;
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
    EngineStatusSnapshot getStatusSnapshot() const;
    RuntimeManifestLoadResult loadPhase1ReferenceInstrument() const;
    RuntimeStreamLoadResult loadPhase1ReferenceStream() const;
    EngineDiagnosticsSnapshot getDiagnosticsSnapshot() const { return diagnosticsSnapshot; }
    EnginePerformanceSnapshot getPerformanceSnapshot() const;
    const DraftPlaybackStatus& getDraftPlaybackStatus() const { return draftPlaybackContract.getStatus(); }
    std::vector<EngineArticulationDescriptor> getArticulationDescriptors() const;
    std::vector<EngineMacroDescriptor> getMacroDescriptors() const;
    bool setSelectedArticulation(const std::string& articulationId);
    bool setMacroValue(const std::string& macroId, double value);
    bool stageDraftRevision(std::size_t revision);
    bool refreshPreviewToCurrentDraft();
    bool publishCurrentDraft();
    void closeDraftPlaybackProject();
    bool reopenDraftPlaybackProject(std::size_t revision);
    bool beginDraftPlaybackDeviceRestart();
    bool completeDraftPlaybackDeviceRestart(bool restored);
    EnginePreviewPlaybackSnapshot auditionPreviewNote(int midiNote, int velocity);
    std::string exportPresetStateJson() const;
    EnginePresetStateRestoreResult restorePresetStateJson(const std::string& presetStateJson);
    EnginePresetStateRestoreResult restorePresetStateFile(const std::string& presetStatePath);
    EngineContentFailureProbeResult probeContentFailure(EngineContentFailureCategory category);
    void clearContentFailureProbe();
    void resetSessionStateToDefault();
    const RuntimeSessionStateSnapshot& getCurrentSessionState() const { return currentSessionState; }
    const EngineContentFailureProbeResult& getLastContentFailureProbe() const { return lastContentFailureProbe; }

private:
    void syncPreviewSnapshotFromDraftPlayback();
    void initializeDraftPlaybackContract(bool activatePerformanceRevision);
    void refreshDiagnosticsSnapshot();

    RuntimeManifestLoadResult referenceManifest;
    RuntimeStreamLoadResult referenceStream;
    bool referenceInstrumentActive = false;
    RuntimeSessionStateSnapshot currentSessionState;
    EngineDiagnosticsSnapshot diagnosticsSnapshot;
    EnginePreviewPlaybackSnapshot previewPlaybackSnapshot;
    EngineContentFailureProbeResult lastContentFailureProbe;
    DraftPlaybackContract draftPlaybackContract;
    std::uint64_t nextPreviewVoiceId = 4000;
};
} // namespace drs::engine
