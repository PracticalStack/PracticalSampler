#pragma once

#include "drs/engine/AuthoringSession.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/SampleImport.h"
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
    std::size_t referenceSampleCountLoaded = 0;
    std::size_t referenceWarmupCount = 0;
    std::size_t samplePathResolutionsOnAudioThread = 0;
    std::size_t sampleDecodeEntriesOnAudioThread = 0;
    std::size_t referenceSampleLoadsOnAudioThread = 0;
    std::size_t authoringSampleLoadsOnAudioThread = 0;
    std::size_t performanceActiveVoiceCount = 0;
    std::size_t authoringPreviewActiveVoiceCount = 0;
    std::size_t activeVoiceCapacity = 0;
    std::size_t activeVoiceCapacityLimit = 0;
    std::size_t activeVoiceCapacityGrowthCount = 0;
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
            + referenceSampleLoadsOnAudioThread
            + authoringSampleLoadsOnAudioThread
            + largeResourceDestructionsOnAudioThread
            + finalSharedOwnershipReleasesOnAudioThread
            + activeVoiceCapacityGrowthCount;
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
    drs::app::AuthoringImportResponsivenessSnapshot getAuthoringImportResponsivenessSnapshot() const;
    void replaceAuthoringProject(drs::engine::RuntimeProjectModel project);
    const juce::File& getAuthoringProjectFile() const { return authoringProjectFile; }
    void setAuthoringProjectFile(juce::File file) { authoringProjectFile = std::move(file); }
    void setMacroValueFromShell(const std::string& macroId, double value);
    void queueAuthoringPreviewNoteOn(int midiNoteNumber, float velocity);
    void queueAuthoringPreviewNoteOff(int midiNoteNumber);
    void queuePerformanceSurfaceNoteOn(int midiNoteNumber, float velocity);
    void queuePerformanceSurfaceNoteOff(int midiNoteNumber);
    bool serviceMessageThreadWork();
    ProcessorRealtimeSafetySnapshot getRealtimeSafetySnapshot() const;
    void clearReferencePlaybackCacheForTests();
    void setRealtimeGuardTestInjection(RealtimeGuardOperation operation);
    static constexpr RealtimeCallbackBudgetProfile getRealtimeCallbackBudgetProfile() { return {}; }

private:
    static constexpr std::size_t maxRealtimeActiveVoices = 24;
    static constexpr std::size_t performanceActivationSlotCount = 4;
    static constexpr std::size_t retiredActivationQueueCapacity = 8;

    enum class VoiceSource
    {
        performance,
        authoringPreview
    };

    struct QueuedRealtimeNoteEvent
    {
        int midiNoteNumber = 0;
        float velocity = 0.0f;
        bool noteOn = false;
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

    struct LoadedReferenceSample
    {
        drs::engine::ImportedSampleData sample;
    };

    struct ActiveRenderVoice
    {
        std::uint64_t renderVoiceId = 0;
        int sourceMidiNote = 0;
        int effectiveMidiNote = 0;
        int effectiveVelocity = 0;
        int rootKey = 60;
        VoiceSource source = VoiceSource::performance;
        const LoadedReferenceSample* loadedSample = nullptr;
        std::shared_ptr<const LoadedReferenceSample> retainedLoadedSample;
        double positionFrames = 0.0;
        double incrementFrames = 1.0;
        float baseGain = 0.0f;
        bool releasing = false;
        int releaseSamplesRemaining = 0;
        int releaseSamplesTotal = 0;
        int retainedPerformanceActivationSlotIndex = -1;
    };

    struct PerformanceRenderActivation
    {
        bool ready = false;
        std::uint64_t activationSerial = 0;
        std::size_t publishedRevision = 0;
        std::uint64_t publishedBuildId = 0;
        std::uint64_t preparedBuildId = 0;
        std::string publishedContentDigest;
        std::string preparedContentDigest;
        drs::engine::PlaybackActivationPayloadPtr payload;
        std::uint64_t payloadRetainedBytes = 0;
        drs::engine::RuntimeSessionStateSnapshot sessionState;
    };

    struct AuthoringPreviewRenderActivation
    {
        bool ready = false;
        std::uint64_t activationSerial = 0;
        std::size_t projectRevision = 0;
        std::string zoneId;
        std::string sampleId;
        int rootKey = 60;
        double gainDb = 0.0;
        std::shared_ptr<const LoadedReferenceSample> preparedSample;
        drs::engine::PlaybackActivationPayloadPtr payload;
        std::uint64_t payloadRetainedBytes = 0;
    };

    static juce::String buildMacroParameterId(const std::string& macroId);
    static juce::AudioProcessorValueTreeState::ParameterLayout buildParameterLayout(
        const drs::engine::EngineFacade& engineFacade);
    bool ensureReferencePlaybackAssetsLoaded(bool invokedFromAudioThread = false);
    void initializeReferencePlaybackAssets(bool invokedFromAudioThread);
    bool ensureSelectedAuthoringSampleLoaded(bool invokedFromAudioThread);
    bool startAuthoringVoiceForMidiMessage(int midiNoteNumber, float velocity);
    void startVoiceForMidiMessage(int midiNoteNumber);
    std::vector<ActiveRenderVoice>& getVoicePool(VoiceSource source);
    const std::vector<ActiveRenderVoice>& getVoicePool(VoiceSource source) const;
    void releaseVoicesForMidiNote(int midiNoteNumber, VoiceSource source);
    void clearVoices(VoiceSource source);
    void clearVoices(VoiceSource source, bool updateState);
    void releasePerformanceActivationLease(ActiveRenderVoice& voice);
    void renderBlockRange(juce::AudioBuffer<float>& buffer, int startSample, int sampleCount);
    void drainRealtimeNoteEvents(RealtimeNoteEventQueue& queue, VoiceSource source);
    bool synchronizeAuthoringPreviewActivation(bool installImmediately);
    bool stageAuthoringPreviewActivation(bool installImmediately);
    const AuthoringPreviewRenderActivation* applyPendingAuthoringPreviewActivationAtBlockBoundary();
    const AuthoringPreviewRenderActivation* getActiveAuthoringPreviewActivation() const;
    bool enqueueRetiredAuthoringPreviewActivationSlot(int slotIndex);
    void drainRetiredAuthoringPreviewActivationSlots();
    int acquireAuthoringPreviewActivationSlot();
    void releaseAuthoringPreviewActivationSlot(int slotIndex);
    bool synchronizePerformanceActivation(bool installImmediately);
    bool stagePerformanceActivation(const drs::engine::EnginePerformanceSnapshot& performanceSnapshot,
                                    const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                    bool installImmediately);
    const PerformanceRenderActivation* applyPendingPerformanceActivationAtBlockBoundary();
    const PerformanceRenderActivation* getActivePerformanceActivation() const;
    bool enqueueRetiredPerformanceActivationSlot(int slotIndex);
    void drainRetiredPerformanceActivationSlots();
    int acquirePerformanceActivationSlot();
    void releasePerformanceActivationSlot(int slotIndex);
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void syncEngineFromParameters();
    void syncParametersFromEngine();
    drs::app::AuthoringWaveformPreview buildAuthoringWaveformPreview(const drs::engine::ImportedSampleData& sample,
                                                                     bool loopEnabled,
                                                                     std::uint64_t loopStartFrame,
                                                                     std::uint64_t loopEndFrame) const;
    void initializeAuthoringImportMetrics();
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
    drs::engine::EngineFacade engineFacade;
    drs::engine::RuntimeManifestLoadResult referenceManifest;
    drs::engine::RuntimeStreamLoadResult referenceStream;
    std::unordered_map<std::string, LoadedReferenceSample> loadedSamples;
    std::unordered_map<std::string, LoadedReferenceSample> authoringLoadedSamples;
    std::unordered_map<std::string, drs::app::AuthoringWaveformPreview> authoringWaveformPreviewCache;
    std::vector<ActiveRenderVoice> performanceActiveVoices;
    std::vector<ActiveRenderVoice> authoringPreviewActiveVoices;
    RealtimeNoteEventQueue authoringPreviewNoteQueue;
    RealtimeNoteEventQueue performanceSurfaceNoteQueue;
    juce::MidiBuffer performanceMidiScratchBuffer;
    juce::MidiBuffer authoringPreviewMidiScratchBuffer;
    juce::AudioProcessorValueTreeState parameterState;
    drs::app::AuthoringImportResponsivenessSnapshot authoringImportResponsivenessSnapshot;
    juce::File authoringProjectFile;
    std::shared_ptr<const ProcessorRealtimeSafetySnapshot> publishedRealtimeSafetySnapshot;
    AudioDiagnosticsPublication audioDiagnosticsPublication;
    std::atomic<std::size_t> diagnosticsProcessBlockCount { 0 };
    std::atomic<std::size_t> diagnosticsPreparedBlockSize { 0 };
    std::atomic<std::size_t> diagnosticsReferenceSampleCountLoaded { 0 };
    std::atomic<std::size_t> diagnosticsReferenceWarmupCount { 0 };
    std::atomic<std::size_t> diagnosticsReferenceSampleLoadsOnAudioThread { 0 };
    std::atomic<std::size_t> diagnosticsAuthoringSampleLoadsOnAudioThread { 0 };
    std::atomic<std::size_t> diagnosticsActiveVoiceCapacityLimit { 0 };
    std::atomic<std::size_t> diagnosticsPrimedActiveVoiceCapacity { 0 };
    std::atomic<std::size_t> diagnosticsActiveVoiceCapacityGrowthCount { 0 };
    std::atomic<std::size_t> diagnosticsAuthoringPreviewActivationCount { 0 };
    std::atomic<std::size_t> diagnosticsPerformanceActivationCount { 0 };
    std::atomic<std::size_t> diagnosticsRetiredActivationCount { 0 };
    std::atomic<std::size_t> diagnosticsReclaimedActivationPayloadCount { 0 };
    std::atomic<std::uint64_t> diagnosticsCallbackBudgetMicros { 0 };
    std::atomic<std::uint64_t> diagnosticsLastProcessBlockMicros { 0 };
    std::atomic<std::uint64_t> diagnosticsMaxProcessBlockMicros { 0 };
    std::atomic<std::size_t> diagnosticsOverBudgetCallbackCount { 0 };
    std::atomic<std::size_t> diagnosticsCurrentAuthoringPreviewDraftRevision { 0 };
    std::array<AuthoringPreviewRenderActivation, performanceActivationSlotCount> authoringPreviewActivationSlots {};
    std::array<std::atomic<std::size_t>, performanceActivationSlotCount> authoringPreviewDiagnosticRevisions {};
    std::array<std::atomic<std::uint64_t>, performanceActivationSlotCount> authoringPreviewDiagnosticPayloadBytes {};
    std::array<int, performanceActivationSlotCount> freeAuthoringPreviewActivationSlots { 0, 1, 2, 3 };
    std::size_t freeAuthoringPreviewActivationSlotCount = performanceActivationSlotCount;
    std::array<int, retiredActivationQueueCapacity> retiredAuthoringPreviewActivationSlots {};
    std::atomic<std::uint32_t> retiredAuthoringPreviewActivationWriteIndex { 0 };
    std::atomic<std::uint32_t> retiredAuthoringPreviewActivationReadIndex { 0 };
    std::atomic<int> pendingAuthoringPreviewActivationSlotIndex { -1 };
    std::atomic<int> activeAuthoringPreviewActivationSlotIndex { -1 };
    std::uint64_t nextAuthoringPreviewActivationSerial = 1;
    std::array<PerformanceRenderActivation, performanceActivationSlotCount> performanceActivationSlots {};
    std::array<std::atomic<std::size_t>, performanceActivationSlotCount> performanceDiagnosticRevisions {};
    std::array<std::atomic<std::uint64_t>, performanceActivationSlotCount> performanceDiagnosticPreparedBuildIds {};
    std::array<std::atomic<std::uint64_t>, performanceActivationSlotCount> performanceDiagnosticPayloadBytes {};
    std::array<std::atomic<std::uint32_t>, performanceActivationSlotCount> performanceActivationVoiceLeaseCounts {};
    std::array<int, performanceActivationSlotCount> freePerformanceActivationSlots { 0, 1, 2, 3 };
    std::size_t freePerformanceActivationSlotCount = performanceActivationSlotCount;
    std::array<int, retiredActivationQueueCapacity> retiredPerformanceActivationSlots {};
    std::atomic<std::uint32_t> retiredPerformanceActivationWriteIndex { 0 };
    std::atomic<std::uint32_t> retiredPerformanceActivationReadIndex { 0 };
    std::array<int, performanceActivationSlotCount> deferredPerformanceRetirementSlots {};
    std::size_t deferredPerformanceRetirementSlotCount = 0;
    std::atomic<std::size_t> deferredPerformanceRetirementBacklog { 0 };
    std::atomic<std::uint64_t> deferredPerformanceRetirementBytes { 0 };
    std::atomic<std::uint64_t> queuedAuthoringPreviewRetirementBytes { 0 };
    std::atomic<std::uint64_t> queuedPerformanceRetirementBytes { 0 };
    std::atomic<int> pendingPerformanceActivationSlotIndex { -1 };
    std::atomic<int> activePerformanceActivationSlotIndex { -1 };
    std::uint64_t nextPerformanceActivationSerial = 1;
    std::size_t failedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
    std::string failedAuthoringPreviewState;
    std::string lastAuthoringSampleLoadFailureState;
    std::size_t observedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
    std::size_t observedDraftPlaybackProjectRevision = std::numeric_limits<std::size_t>::max();
    std::uint64_t observedEngineStateRevision = 0;
    double currentSampleRate = 44100.0;
    std::uint64_t nextRenderVoiceId = 1;
    bool isSynchronizingParameterState = false;
    RealtimeGuardState realtimeGuardState;
    std::atomic<RealtimeGuardOperation> realtimeGuardTestInjection { RealtimeGuardOperation::none };
    std::byte* realtimeGuardTestAllocation = nullptr;
    std::mutex realtimeGuardTestMutex;
};
} // namespace drs::plugin
