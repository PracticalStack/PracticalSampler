#pragma once

#include "drs/engine/AuthoringSession.h"
#include "drs/engine/AuthoringPreviewCommandAdapter.h"
#include "drs/engine/AuthoringPreviewController.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/SamplerPlaybackContext.h"
#include "plugin/RealtimeGuard.h"
#include "shared/AuthoringPreviewModel.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace drs::plugin
{
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
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    Processor();
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
    juce::AudioProcessorValueTreeState& getParameterState() { return parameterState; }
    const juce::AudioProcessorValueTreeState& getParameterState() const { return parameterState; }
    drs::app::AuthoringWaveformPreview getAuthoringWaveformPreview();
    drs::app::AuthoringPreviewStatusSnapshot getAuthoringPreviewStatusSnapshot() const;
    drs::engine::AuthoringPreviewControllerSnapshot getAuthoringPreviewControllerSnapshot() const
    {
        return authoringPreviewController.getSnapshot();
    }
    drs::app::AuthoringImportResponsivenessSnapshot getAuthoringImportResponsivenessSnapshot() const;
    void replaceAuthoringProject(drs::engine::RuntimeProjectModel project);
    void closeAuthoringProject(drs::engine::RuntimeProjectModel unloadedProject);
    const juce::File& getAuthoringProjectFile() const { return authoringProjectFile; }
    void setAuthoringProjectFile(juce::File file) { authoringProjectFile = std::move(file); }
    void setMacroValueFromShell(const std::string& macroId, double value);
    void requestAuthoringPreview(drs::engine::AuthoringPreviewScope scope);
    bool submitAuthoringPreviewCommand(const drs::engine::AuthoringPreviewCommand& command);
    drs::engine::AuthoringPreviewCommandAdapterSnapshot getAuthoringPreviewCommandSnapshot() const
    {
        return authoringPreviewCommandAdapter.getSnapshot();
    }
    void queueAuthoringPreviewNoteOn(int midiNoteNumber, float velocity);
    void queueAuthoringPreviewNoteOff(int midiNoteNumber);
    void queuePerformanceSurfaceNoteOn(int midiNoteNumber, float velocity);
    void queuePerformanceSurfaceNoteOff(int midiNoteNumber);
    bool serviceMessageThreadWork();
    ProcessorRealtimeSafetySnapshot getRealtimeSafetySnapshot() const;
    void setRealtimeGuardTestInjection(RealtimeGuardOperation operation);
    static constexpr RealtimeCallbackBudgetProfile getRealtimeCallbackBudgetProfile() { return {}; }

private:
    static constexpr std::size_t maxRealtimeActiveVoices = drs::engine::SamplerVoicePool::capacity;

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
    static juce::AudioProcessorValueTreeState::ParameterLayout buildParameterLayout(
        const drs::engine::EngineFacade& engineFacade);
    void drainRealtimeNoteEvents(RealtimeNoteEventQueue& queue,
                                 drs::engine::SamplerEventBlock& destination,
                                 std::uint32_t frameCount) noexcept;
    bool stageAuthoringPreviewActivation(const drs::engine::AuthoringPreviewRequest& request,
                                         bool installImmediately);
    bool synchronizePerformanceActivation(bool installImmediately);
    bool stagePerformanceActivation(const drs::engine::EnginePerformanceSnapshot& performanceSnapshot,
                                    const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                    bool installImmediately);
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void syncEngineFromParameters();
    void syncParametersFromEngine();
    drs::app::AuthoringWaveformPreview buildAuthoringWaveformPreview(const drs::engine::ImportedSampleData& sample,
                                                                     bool loopEnabled,
                                                                     std::uint64_t loopStartFrame,
                                                                     std::uint64_t loopEndFrame) const;
    void initializeAuthoringImportMetrics();
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
    drs::engine::AuthoringPreviewController authoringPreviewController;
    std::shared_ptr<const drs::app::AuthoringPreviewStatusSnapshot> authoringPreviewStatusPublication;
    drs::engine::EngineFacade engineFacade;
    std::unordered_map<std::string, drs::app::AuthoringWaveformPreview> authoringWaveformPreviewCache;
    drs::engine::SamplerPlaybackContext performancePlaybackContext {
        drs::engine::PlaybackActivationLane::performance
    };
    drs::engine::SamplerPlaybackContext authoringPreviewPlaybackContext {
        drs::engine::PlaybackActivationLane::preview
    };
    RealtimeNoteEventQueue authoringPreviewNoteQueue;
    RealtimeNoteEventQueue performanceSurfaceNoteQueue;
    juce::AudioProcessorValueTreeState parameterState;
    drs::app::AuthoringImportResponsivenessSnapshot authoringImportResponsivenessSnapshot;
    juce::File authoringProjectFile;
    std::shared_ptr<const ProcessorRealtimeSafetySnapshot> publishedRealtimeSafetySnapshot;
    AudioDiagnosticsPublication audioDiagnosticsPublication;
    std::atomic<std::size_t> diagnosticsProcessBlockCount { 0 };
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
    std::size_t failedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
    std::string failedAuthoringPreviewState;
    std::size_t observedDraftPlaybackProjectRevision = std::numeric_limits<std::size_t>::max();
    std::uint64_t observedEngineStateRevision = 0;
    bool authoringPreviewDirectAuditionRequested = false;
    drs::engine::AuthoringPreviewScope authoringPreviewRequestedScope
        = drs::engine::AuthoringPreviewScope::selectedZone;
    double currentSampleRate = 44100.0;
    bool isSynchronizingParameterState = false;
    RealtimeGuardState realtimeGuardState;
    std::atomic<RealtimeGuardOperation> realtimeGuardTestInjection { RealtimeGuardOperation::none };
    std::byte* realtimeGuardTestAllocation = nullptr;
    std::mutex realtimeGuardTestMutex;
};
} // namespace drs::plugin
