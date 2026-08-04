#pragma once

#include "drs/engine/AuthoringSession.h"
#include "drs/engine/AuthoringPreviewCommandAdapter.h"
#include "drs/engine/AuthoringPreviewController.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/HostSessionState.h"
#include "drs/engine/PerformancePackage.h"
#include "drs/engine/PerformancePublishCommandAdapter.h"
#include "drs/engine/ProjectRestoreCoordinator.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/SamplerPlaybackContext.h"
#include "plugin/RealtimeGuard.h"
#include "shared/AuthoringPreviewModel.h"
#include "shared/ProjectSourceValidationService.h"
#include "shared/SfzImportReviewService.h"
#include "shared/WaveformPreviewService.h"
#include "shared/WavImportService.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace drs::plugin
{
struct PerformancePackageWorkspaceLoadResult
{
    bool loaded = false;
    drs::engine::PerformancePackageFailureCategory failureCategory
        = drs::engine::PerformancePackageFailureCategory::none;
    std::string state;
    std::vector<std::string> issues;
};

struct PerformancePackageExportResult
{
    bool exported = false;
    std::string state;
    std::vector<std::string> issues;
    std::string packagePath;
    std::uint64_t packageBytes = 0;
    std::uint32_t payloadCount = 0;
};

struct ProcessorRealtimeSafetySnapshot
{
    bool available = false;
    std::size_t processBlockCount = 0;
    std::size_t preparedBlockSize = 0;
    std::size_t samplePathResolutionsOnAudioThread = 0;
    std::size_t sampleDecodeEntriesOnAudioThread = 0;
    std::size_t authoringSampleLoadsOnAudioThread = 0;
    std::size_t performanceActiveVoiceCount = 0;
    std::size_t authoringPreviewActiveVoiceCount = 0;
    std::size_t activeVoiceCapacity = 0;
    std::size_t activeVoiceCapacityLimit = 0;
    std::uint32_t performanceContextIdentity = 0;
    std::uint32_t authoringPreviewContextIdentity = 0;
    std::uint64_t lastPerformanceRenderMicros = 0;
    std::uint64_t maxPerformanceRenderMicros = 0;
    std::uint64_t lastAuthoringPreviewRenderMicros = 0;
    std::uint64_t maxAuthoringPreviewRenderMicros = 0;
    std::size_t performancePeakActiveVoiceCount = 0;
    std::size_t performancePeakReleasingVoiceCount = 0;
    std::size_t authoringPreviewPeakActiveVoiceCount = 0;
    std::size_t authoringPreviewPeakReleasingVoiceCount = 0;
    std::uint64_t performanceVoiceStealCount = 0;
    std::uint64_t performanceGenerationStealCount = 0;
    std::uint64_t performanceReleasingVoiceStealCount = 0;
    std::uint64_t performanceActiveGeneration = 0;
    std::size_t performanceActiveGenerationVoiceCount = 0;
    std::size_t performanceRetiredGenerationVoiceCount = 0;
    std::size_t performanceSustainDeferredVoiceCount = 0;
    std::uint64_t authoringPreviewVoiceStealCount = 0;
    std::uint64_t performanceDroppedEventCount = 0;
    std::uint64_t authoringPreviewDroppedEventCount = 0;
    std::uint64_t performanceDroppedNoteCount = 0;
    std::uint64_t authoringPreviewDroppedNoteCount = 0;
    std::size_t authoringPreviewActivationCount = 0;
    std::size_t performanceActivationCount = 0;
    std::size_t retiredActivationCount = 0;
    std::size_t retiredActivationBacklog = 0;
    std::size_t reclaimedActivationPayloadCount = 0;
    std::uint64_t activeActivationPayloadBytes = 0;
    std::uint64_t pendingActivationPayloadBytes = 0;
    std::uint64_t retiredActivationPayloadBytes = 0;
    std::uint64_t lastActivationReclamationLatencyBlocks = 0;
    std::uint64_t maxActivationReclamationLatencyBlocks = 0;
    std::size_t largeResourceReleasesOnAudioThread = 0;
    std::size_t allocationsOnAudioThread = 0;
    std::size_t deallocationsOnAudioThread = 0;
    std::size_t blockingLockAttemptsOnAudioThread = 0;
    std::size_t waitsOnAudioThread = 0;
    std::size_t fileOpenEntriesOnAudioThread = 0;
    std::size_t fileReadEntriesOnAudioThread = 0;
    std::size_t streamDecodeEntriesOnAudioThread = 0;
    std::size_t largeResourceDestructionsOnAudioThread = 0;
    std::size_t finalSharedOwnershipReleasesOnAudioThread = 0;
    std::uint64_t callbackBudgetMicros = 0;
    std::uint64_t lastProcessBlockMicros = 0;
    std::uint64_t maxProcessBlockMicros = 0;
    std::size_t overBudgetCallbackCount = 0;
    std::size_t currentAuthoringPreviewDraftRevision = 0;
    std::size_t activeAuthoringPreviewRevision = 0;
    std::size_t pendingAuthoringPreviewRevision = 0;
    std::size_t activePublishedRevision = 0;
    std::size_t pendingPublishedRevision = 0;
    std::size_t activePublishedMacroRevision = 0;
    int activePublishedMacroFixedVelocity = 0;
    int activePublishedMacroMidiNoteOffset = 0;
    std::uint64_t activePreparedBuildId = 0;
    std::uint64_t pendingPreparedBuildId = 0;
    std::string authoringPreviewRevisionState;
    std::string authoringPreviewFailureState;
    std::string state;
    std::uint64_t publicationSequence = 0;

    std::size_t getAudioThreadViolationCount() const
    {
        return allocationsOnAudioThread
            + deallocationsOnAudioThread
            + blockingLockAttemptsOnAudioThread
            + waitsOnAudioThread
            + fileOpenEntriesOnAudioThread
            + fileReadEntriesOnAudioThread
            + samplePathResolutionsOnAudioThread
            + sampleDecodeEntriesOnAudioThread
            + streamDecodeEntriesOnAudioThread
            + authoringSampleLoadsOnAudioThread
            + largeResourceDestructionsOnAudioThread
            + finalSharedOwnershipReleasesOnAudioThread;
    }

    std::size_t getRealtimeGuardFailureCount() const
    {
        return getAudioThreadViolationCount() + overBudgetCallbackCount;
    }
};

class Processor final : public juce::AudioProcessor,
                        private juce::AudioProcessorValueTreeState::Listener,
                        private juce::Timer
{
public:
    Processor();
    explicit Processor(drs::app::WaveformPreviewServiceOptions waveformPreviewServiceOptions);
    ~Processor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    drs::engine::EngineFacade& getEngineFacade() { return engineFacade; }
    const drs::engine::EngineFacade& getEngineFacade() const { return engineFacade; }
    drs::engine::AuthoringSession& getAuthoringSession() { return authoringSession; }
    const drs::engine::AuthoringSession& getAuthoringSession() const { return authoringSession; }
    drs::app::SfzImportReviewService& getSfzImportReviewService() { return sfzImportReviewService; }
    const drs::app::SfzImportReviewService& getSfzImportReviewService() const { return sfzImportReviewService; }
    drs::app::WavImportService& getWavImportService() { return wavImportService; }
    const drs::app::WavImportService& getWavImportService() const { return wavImportService; }
    bool requestAuthoringSourceValidation();
    bool cancelAuthoringSourceValidation();
    drs::app::AuthoringSourceValidationSnapshot getAuthoringSourceValidationSnapshot() const;
    void authorizeAuthoringWaveformPreviewLoad();
    juce::AudioProcessorValueTreeState& getParameterState() { return parameterState; }
    const juce::AudioProcessorValueTreeState& getParameterState() const { return parameterState; }
    drs::app::AuthoringWaveformPreview getAuthoringWaveformPreview();
    drs::app::AuthoringPreviewStatusSnapshot getAuthoringPreviewStatusSnapshot() const;
    drs::engine::AuthoringPreviewControllerSnapshot getAuthoringPreviewControllerSnapshot() const
    {
        return authoringPreviewController.getSnapshot();
    }
    drs::engine::PerformancePublishControllerSnapshot getPerformancePublishControllerSnapshot() const
    {
        return engineFacade.getPerformancePublishControllerSnapshot();
    }
    drs::app::AuthoringImportResponsivenessSnapshot getAuthoringImportResponsivenessSnapshot() const;
    bool replaceAuthoringProject(drs::engine::RuntimeProjectModel project,
                                 juce::File resolvedProjectFile = {});
    bool applyAuthoringProjectMigration(drs::engine::RuntimeProjectModel migratedProject);
    void closeAuthoringProject(drs::engine::RuntimeProjectModel unloadedProject);
    bool bindAuthoringProjectFile(const juce::File& resolvedProjectFile);
    void clearAuthoringProjectFileBinding();
    juce::File getAuthoringProjectFile() const;
    const drs::engine::WorkspaceDocumentState& getWorkspaceDocumentState() const noexcept
    {
        return workspaceDocumentState;
    }
    bool activatePerformancePackageWorkspace(
        const drs::engine::PerformancePackageManifest& package,
        juce::File resolvedPackageFile = {});
    PerformancePackageWorkspaceLoadResult loadPerformancePackageWorkspace(
        const juce::File& resolvedPackageFile);
    PerformancePackageExportResult exportPerformancePackage(
        const juce::File& resolvedPackageFile);
    void closePerformancePackageWorkspace(drs::engine::RuntimeProjectModel unloadedProject);
    const drs::engine::HostProjectBinding& getAuthoringProjectBinding() const
    {
        return authoringProjectBinding;
    }
    std::shared_ptr<const drs::engine::ProjectRestoreSnapshot> getProjectRestoreSnapshot() const
    {
        return projectRestoreCoordinator.getSnapshot();
    }
    bool retryProjectRestore();
    bool retryProjectRestoreWithFile(const juce::File& locatedProjectFile);
    void setMacroValueFromShell(const std::string& macroId, double value);
    void requestAuthoringPreview(drs::engine::AuthoringPreviewScope scope);
    bool submitAuthoringPreviewCommand(const drs::engine::AuthoringPreviewCommand& command);
    bool submitPerformancePublishCommand(
        const drs::engine::PerformancePublishCommand& command = {},
        drs::engine::PerformancePublishCommandSource source
            = drs::engine::PerformancePublishCommandSource::externalApi);
    drs::engine::PerformancePublishCommandAdapterSnapshot
        getPerformancePublishCommandSnapshot() const noexcept
    {
        return performancePublishCommandAdapter.getSnapshot();
    }
    std::shared_ptr<const drs::engine::PerformancePublishPresentationSnapshot>
        getPerformancePublishPresentationSnapshot() const
    {
        return engineFacade.getPerformancePublishPresentationSnapshot();
    }
    drs::engine::AuthoringPreviewCommandAdapterSnapshot getAuthoringPreviewCommandSnapshot() const
    {
        return authoringPreviewCommandAdapter.getSnapshot();
    }
    void queueAuthoringPreviewNoteOn(int midiNoteNumber, float velocity);
    void queueAuthoringPreviewNoteOff(int midiNoteNumber);
    void queuePerformanceSurfaceNoteOn(int midiNoteNumber, float velocity);
    void queuePerformanceSurfaceNoteOff(int midiNoteNumber);
    bool hasRecentAudioCallback(std::uint64_t maximumAgeMicros = 750000) const noexcept;
    bool serviceMessageThreadWork();
    ProcessorRealtimeSafetySnapshot getRealtimeSafetySnapshot() const;
    void setRealtimeGuardTestInjection(RealtimeGuardOperation operation);
    static constexpr RealtimeCallbackBudgetProfile getRealtimeCallbackBudgetProfile() { return {}; }

private:
    static constexpr std::size_t maxRealtimeActiveVoices = drs::engine::SamplerVoicePool::capacity;
    static constexpr std::size_t maxPublishedMacroSlots
        = drs::engine::maximumPublishedMacroHostSlots;

    struct QueuedRealtimeNoteEvent
    {
        drs::engine::SamplerRenderEventType type = drs::engine::SamplerRenderEventType::noteOn;
        int midiNoteNumber = 0;
        float velocity = 0.0f;
        std::uint32_t sampleOffset = 0;
    };

    class RealtimeNoteEventQueue
    {
    public:
        bool push(QueuedRealtimeNoteEvent event) noexcept;
        bool pop(QueuedRealtimeNoteEvent& event) noexcept;
        void reset() noexcept;

    private:
        static constexpr std::uint32_t storageCapacity = 257;
        std::array<QueuedRealtimeNoteEvent, storageCapacity> events {};
        std::atomic<std::uint32_t> writeIndex { 0 };
        std::atomic<std::uint32_t> readIndex { 0 };
    };

    static juce::String buildMacroParameterId(const std::string& macroId);
    static juce::AudioProcessorValueTreeState::ParameterLayout buildParameterLayout();
    void resetAuthoringPreviewPreparationAuthorization() noexcept;
    void resetAuthoringWaveformPreviewAuthorization() noexcept;
    void initializePublishedMacroRealtimeState();
    void installPublishedMacroBindings(
        const drs::engine::ImmutablePublishedMacroBindingTable& bindings) noexcept;
    drs::engine::SamplerRenderControlValues buildPublishedMacroRenderControls() noexcept;
    void drainRealtimeNoteEvents(RealtimeNoteEventQueue& queue,
                                 drs::engine::SamplerEventBlock& destination,
                                 std::uint32_t frameCount) noexcept;
    bool stageAuthoringPreviewActivation(const drs::engine::AuthoringPreviewRequest& request,
                                         bool installImmediately);
    bool synchronizePerformanceActivation(bool installImmediately);
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void syncEngineFromParameters();
    void syncParametersFromEngine();
    struct WaveformPreviewCacheEntry
    {
        std::string sampleSourceId;
        std::string sourcePath;
        std::uint64_t sourceFileSizeBytes = 0;
        std::int64_t sourceModificationTicks = 0;
        std::string fingerprintHex;
        std::size_t displayPointCount = 0;
        drs::engine::WaveformPeakChannelReduction channelReduction
            = drs::engine::WaveformPeakChannelReduction::channelExtrema;
        std::string requestStamp;
        drs::app::AuthoringWaveformPreview preview;
    };

    static constexpr std::size_t authoringWaveformPreviewPointCount = 192;
    static constexpr std::uint64_t authoringWaveformPreviewChunkFrameCount = 4096;
    static constexpr auto authoringWaveformPreviewChannelReduction
        = drs::engine::WaveformPeakChannelReduction::channelExtrema;

    drs::app::AuthoringWaveformPreview buildAuthoringWaveformPreview(
        const drs::engine::WaveformPeakBuildResult& waveform,
        bool loopEnabled,
        std::uint64_t loopStartFrame,
        std::uint64_t loopEndFrame) const;
    void consumeAuthoringWaveformPreviewSnapshot();
    bool describeAuthoringWaveformPreviewSource(const drs::engine::RuntimeProjectSampleSource& sampleSource,
                                                std::uint64_t& fileSizeBytes,
                                                std::int64_t& modificationTicks) const;
    std::string buildAuthoringWaveformPreviewRequestStamp(
        const drs::engine::RuntimeProjectSampleSource& sampleSource,
        std::uint64_t fileSizeBytes,
        std::int64_t modificationTicks) const;
    const WaveformPreviewCacheEntry* findAuthoringWaveformPreviewCacheEntryForStamp(
        const std::string& requestStamp) const;
    const WaveformPreviewCacheEntry* findLatestAuthoringWaveformPreviewCacheEntryForSource(
        const std::string& sampleSourceId) const;
    void clearAuthoringWaveformPreviewCache();
    void initializeAuthoringImportMetrics();
    void initializeAuthoringSourceValidationSnapshot();
    std::optional<drs::engine::HostProjectBinding> buildValidatedAuthoringProjectBinding(
        const juce::File& resolvedProjectFile,
        const drs::engine::RuntimeProjectModel& project) const;
    void refreshWorkspaceDocumentStateFromAuthoringProject();
    struct ProjectRestoreApplicationOutcome
    {
        bool applied = false;
        drs::engine::ProjectRestoreFinding finding
            = drs::engine::ProjectRestoreFinding::checkpointInvalid;
        std::string message;
    };

    ProjectRestoreApplicationOutcome applyValidatedProjectRestore(
        const drs::engine::ProjectRestoreSnapshot& restore);
    bool serviceProjectRestore();
    void refreshSerializedHostStatePublication(bool force = false);
    std::string buildHostStatePublicationKey() const;
    void setPendingRestoreAudioPolicy(bool pending) noexcept;
    void supersedeFailedProjectRestoreForManualAction();
    bool restorePublishIdentityMatches(
        const drs::engine::PerformancePublishControllerSnapshot& published) const;
    void publishAuthoringPreviewStatus();
    void primeRealtimeSafetyState(int samplesPerBlock);
    void updateRealtimeSafetyState();
    void publishAudioDiagnostics();
    void publishMessageDiagnostics();
    void runRealtimeGuardTestInjection();

    struct AudioDiagnosticsValues
    {
        std::size_t performanceActiveVoiceCount = 0;
        std::size_t authoringPreviewActiveVoiceCount = 0;
        std::size_t activeVoiceCapacity = 0;
        std::uint32_t performanceContextIdentity = 0;
        std::uint32_t authoringPreviewContextIdentity = 0;
        std::uint64_t lastPerformanceRenderMicros = 0;
        std::uint64_t maxPerformanceRenderMicros = 0;
        std::uint64_t lastAuthoringPreviewRenderMicros = 0;
        std::uint64_t maxAuthoringPreviewRenderMicros = 0;
        std::size_t performancePeakActiveVoiceCount = 0;
        std::size_t performancePeakReleasingVoiceCount = 0;
        std::size_t authoringPreviewPeakActiveVoiceCount = 0;
        std::size_t authoringPreviewPeakReleasingVoiceCount = 0;
        std::uint64_t performanceVoiceStealCount = 0;
        std::uint64_t performanceGenerationStealCount = 0;
        std::uint64_t performanceReleasingVoiceStealCount = 0;
        std::uint64_t performanceActiveGeneration = 0;
        std::size_t performanceActiveGenerationVoiceCount = 0;
        std::size_t performanceRetiredGenerationVoiceCount = 0;
        std::size_t performanceSustainDeferredVoiceCount = 0;
        std::uint64_t authoringPreviewVoiceStealCount = 0;
        std::uint64_t performanceDroppedEventCount = 0;
        std::uint64_t authoringPreviewDroppedEventCount = 0;
        std::uint64_t performanceDroppedNoteCount = 0;
        std::uint64_t authoringPreviewDroppedNoteCount = 0;
        std::size_t activeAuthoringPreviewRevision = 0;
        std::size_t pendingAuthoringPreviewRevision = 0;
        std::size_t activePublishedRevision = 0;
        std::size_t pendingPublishedRevision = 0;
        std::uint64_t activePreparedBuildId = 0;
        std::uint64_t pendingPreparedBuildId = 0;
        std::size_t retiredActivationBacklog = 0;
        std::uint64_t activeActivationPayloadBytes = 0;
        std::uint64_t pendingActivationPayloadBytes = 0;
        std::uint64_t retiredActivationPayloadBytes = 0;
        std::uint64_t lastActivationReclamationLatencyBlocks = 0;
        std::uint64_t maxActivationReclamationLatencyBlocks = 0;
        bool hasActiveAuthoringPreviewActivation = false;
        bool hasPendingAuthoringPreviewActivation = false;
        bool hasActivePerformanceActivation = false;
        bool hasPendingPerformanceActivation = false;
    };

    struct AudioDiagnosticsPublication
    {
        std::atomic<std::uint64_t> sequence { 0 };
        std::atomic<std::size_t> performanceActiveVoiceCount { 0 };
        std::atomic<std::size_t> authoringPreviewActiveVoiceCount { 0 };
        std::atomic<std::size_t> activeVoiceCapacity { 0 };
        std::atomic<std::uint32_t> performanceContextIdentity { 0 };
        std::atomic<std::uint32_t> authoringPreviewContextIdentity { 0 };
        std::atomic<std::uint64_t> lastPerformanceRenderMicros { 0 };
        std::atomic<std::uint64_t> maxPerformanceRenderMicros { 0 };
        std::atomic<std::uint64_t> lastAuthoringPreviewRenderMicros { 0 };
        std::atomic<std::uint64_t> maxAuthoringPreviewRenderMicros { 0 };
        std::atomic<std::size_t> performancePeakActiveVoiceCount { 0 };
        std::atomic<std::size_t> performancePeakReleasingVoiceCount { 0 };
        std::atomic<std::size_t> authoringPreviewPeakActiveVoiceCount { 0 };
        std::atomic<std::size_t> authoringPreviewPeakReleasingVoiceCount { 0 };
        std::atomic<std::uint64_t> performanceVoiceStealCount { 0 };
        std::atomic<std::uint64_t> performanceGenerationStealCount { 0 };
        std::atomic<std::uint64_t> performanceReleasingVoiceStealCount { 0 };
        std::atomic<std::uint64_t> performanceActiveGeneration { 0 };
        std::atomic<std::size_t> performanceActiveGenerationVoiceCount { 0 };
        std::atomic<std::size_t> performanceRetiredGenerationVoiceCount { 0 };
        std::atomic<std::size_t> performanceSustainDeferredVoiceCount { 0 };
        std::atomic<std::uint64_t> authoringPreviewVoiceStealCount { 0 };
        std::atomic<std::uint64_t> performanceDroppedEventCount { 0 };
        std::atomic<std::uint64_t> authoringPreviewDroppedEventCount { 0 };
        std::atomic<std::uint64_t> performanceDroppedNoteCount { 0 };
        std::atomic<std::uint64_t> authoringPreviewDroppedNoteCount { 0 };
        std::atomic<std::size_t> activeAuthoringPreviewRevision { 0 };
        std::atomic<std::size_t> pendingAuthoringPreviewRevision { 0 };
        std::atomic<std::size_t> activePublishedRevision { 0 };
        std::atomic<std::size_t> pendingPublishedRevision { 0 };
        std::atomic<std::uint64_t> activePreparedBuildId { 0 };
        std::atomic<std::uint64_t> pendingPreparedBuildId { 0 };
        std::atomic<std::size_t> retiredActivationBacklog { 0 };
        std::atomic<std::uint64_t> activeActivationPayloadBytes { 0 };
        std::atomic<std::uint64_t> pendingActivationPayloadBytes { 0 };
        std::atomic<std::uint64_t> retiredActivationPayloadBytes { 0 };
        std::atomic<std::uint64_t> lastActivationReclamationLatencyBlocks { 0 };
        std::atomic<std::uint64_t> maxActivationReclamationLatencyBlocks { 0 };
        std::atomic<bool> hasActiveAuthoringPreviewActivation { false };
        std::atomic<bool> hasPendingAuthoringPreviewActivation { false };
        std::atomic<bool> hasActivePerformanceActivation { false };
        std::atomic<bool> hasPendingPerformanceActivation { false };
    };

    AudioDiagnosticsValues captureActivationDiagnostics() const;
    AudioDiagnosticsValues readAudioDiagnostics(std::uint64_t& sequence) const;
    ProcessorRealtimeSafetySnapshot composeDiagnosticsSnapshot(const AudioDiagnosticsValues& audioValues,
                                                               std::uint64_t publicationSequence) const;
    void applyRealtimeGuardDiagnostics(ProcessorRealtimeSafetySnapshot& snapshot) const;

    drs::engine::AuthoringSession authoringSession;
    drs::engine::AuthoringPreviewCommandAdapter authoringPreviewCommandAdapter;
    drs::engine::PerformancePublishCommandAdapter performancePublishCommandAdapter;
    drs::engine::AuthoringPreviewController authoringPreviewController;
    std::shared_ptr<const drs::app::AuthoringPreviewStatusSnapshot> authoringPreviewStatusPublication;
    drs::engine::EngineFacade engineFacade;
    std::unordered_map<std::string, WaveformPreviewCacheEntry> authoringWaveformPreviewCache;
    std::unordered_map<std::string, std::string> authoringWaveformPreviewLatestStampBySourceId;
    std::unordered_map<std::string, std::string> authoringWaveformPreviewCurrentStampBySourceId;
    std::uint64_t authoringWaveformPreviewConsumedGeneration = 0;
    bool authoringWaveformPreviewLoadAuthorized = false;
    bool authoringPreviewPreparationAuthorized = false;
    drs::engine::SamplerPlaybackContext performancePlaybackContext {
        drs::engine::PlaybackActivationLane::performance
    };
    drs::engine::PerformancePublishActivationPayloadPtr pendingPerformanceActivation;
    drs::engine::SamplerPlaybackContext authoringPreviewPlaybackContext {
        drs::engine::PlaybackActivationLane::preview
    };
    RealtimeNoteEventQueue authoringPreviewNoteQueue;
    RealtimeNoteEventQueue performanceSurfaceNoteQueue;
    juce::AudioProcessorValueTreeState parameterState;
    std::vector<juce::String> hostMacroParameterIds;
    std::vector<std::string> hostMacroStableIds;
    std::array<std::atomic<float>, maxPublishedMacroSlots> hostMacroValues {};
    std::array<std::atomic<std::uint64_t>, maxPublishedMacroSlots> hostMacroValueSequences {};
    drs::engine::PublishedMacroCallbackView activePublishedMacroCallbackView;
    std::array<std::uint64_t, maxPublishedMacroSlots> activePublishedMacroBaselines {};
    std::atomic<std::size_t> diagnosticActivePublishedMacroRevision { 0 };
    std::atomic<int> diagnosticActivePublishedMacroFixedVelocity { 0 };
    std::atomic<int> diagnosticActivePublishedMacroMidiNoteOffset { 0 };
    drs::app::AuthoringImportResponsivenessSnapshot authoringImportResponsivenessSnapshot;
    drs::app::AuthoringSourceValidationSnapshot authoringSourceValidationSnapshot;
    drs::engine::HostProjectBinding authoringProjectBinding;
    drs::engine::WorkspaceDocumentState workspaceDocumentState;
    drs::engine::ProjectRestoreCoordinator projectRestoreCoordinator;
    drs::app::SfzImportReviewService sfzImportReviewService;
    drs::app::WavImportService wavImportService;
    drs::app::ProjectSourceValidationService projectSourceValidationService;
    drs::app::WaveformPreviewService waveformPreviewService;
    std::shared_ptr<const std::string> serializedHostStatePublication;
    std::shared_ptr<const std::string> latestSubmittedHostState;
    std::string hostStatePublicationKey;
    std::uint64_t handledRestoreGeneration = 0;
    std::uint64_t awaitingRestoreActivationGeneration = 0;
    std::optional<drs::engine::HostPublishedCheckpoint> expectedRestoredPublishedState;
    std::atomic<bool> pendingRestoreAudioSilence { false };
    std::atomic<bool> restoreAudioSilenceApplied { false };
    std::shared_ptr<const ProcessorRealtimeSafetySnapshot> publishedRealtimeSafetySnapshot;
    AudioDiagnosticsPublication audioDiagnosticsPublication;
    std::atomic<std::size_t> diagnosticsProcessBlockCount { 0 };
    std::atomic<std::uint64_t> diagnosticsLastProcessBlockAtMicros { 0 };
    std::atomic<std::size_t> diagnosticsPreparedBlockSize { 0 };
    std::atomic<std::size_t> diagnosticsAuthoringSampleLoadsOnAudioThread { 0 };
    std::atomic<std::size_t> diagnosticsActiveVoiceCapacityLimit { 0 };
    std::atomic<std::size_t> diagnosticsPrimedActiveVoiceCapacity { 0 };
    std::atomic<std::uint64_t> diagnosticsPerformanceDroppedNoteCount { 0 };
    std::atomic<std::uint64_t> diagnosticsAuthoringPreviewDroppedNoteCount { 0 };
    std::atomic<std::uint64_t> diagnosticsPerformanceDroppedEventCount { 0 };
    std::atomic<std::uint64_t> diagnosticsAuthoringPreviewDroppedEventCount { 0 };
    std::atomic<std::size_t> diagnosticsAuthoringPreviewActivationCount { 0 };
    std::atomic<std::size_t> diagnosticsPerformanceActivationCount { 0 };
    std::atomic<std::size_t> diagnosticsRetiredActivationCount { 0 };
    std::atomic<std::size_t> diagnosticsReclaimedActivationPayloadCount { 0 };
    std::atomic<std::uint64_t> diagnosticsCallbackBudgetMicros { 0 };
    std::atomic<std::uint64_t> diagnosticsLastProcessBlockMicros { 0 };
    std::atomic<std::uint64_t> diagnosticsMaxProcessBlockMicros { 0 };
    std::atomic<std::size_t> diagnosticsOverBudgetCallbackCount { 0 };
    std::atomic<std::size_t> diagnosticsCurrentAuthoringPreviewDraftRevision { 0 };
    std::atomic<bool> authoringPreviewCloseRequested { false };
    std::uint64_t lastPerformanceRenderMicros = 0;
    std::uint64_t maxPerformanceRenderMicros = 0;
    std::uint64_t lastAuthoringPreviewRenderMicros = 0;
    std::uint64_t maxAuthoringPreviewRenderMicros = 0;
    std::size_t performancePeakActiveVoiceCount = 0;
    std::size_t performancePeakReleasingVoiceCount = 0;
    std::size_t authoringPreviewPeakActiveVoiceCount = 0;
    std::size_t authoringPreviewPeakReleasingVoiceCount = 0;
    bool hasPreviousHostTransportObservation = false;
    bool previousHostTransportWasPlaying = false;
    bool previousHostTransportHadTimeInSamples = false;
    std::int64_t previousHostTransportTimeInSamples = 0;
    int previousHostTransportBlockSize = 0;
    std::size_t observedDraftPlaybackProjectRevision = std::numeric_limits<std::size_t>::max();
    std::uint64_t observedEngineStateRevision = 0;
    bool authoringPreviewDirectAuditionRequested = false;
    drs::engine::AuthoringPreviewScope authoringPreviewRequestedScope
        = drs::engine::AuthoringPreviewScope::selectedZone;
    std::string pendingAuthoringPreviewZoneId;
    std::string pendingAuthoringPreviewGroupId;
    double currentSampleRate = 44100.0;
    bool isSynchronizingParameterState = false;
    RealtimeGuardState realtimeGuardState;
    std::atomic<RealtimeGuardOperation> realtimeGuardTestInjection { RealtimeGuardOperation::none };
    std::byte* realtimeGuardTestAllocation = nullptr;
    std::mutex realtimeGuardTestMutex;
};
} // namespace drs::plugin
