#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <limits>
#include <optional>
#include <string_view>

namespace drs::plugin
{
namespace
{
namespace fs = std::filesystem;

static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "Realtime diagnostics require lock-free 64-bit atomics.");
static_assert(std::atomic<std::size_t>::is_always_lock_free,
              "Realtime diagnostics require lock-free size atomics.");
static_assert(std::atomic<int>::is_always_lock_free,
              "Realtime activation handoff requires lock-free index atomics.");
static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
              "Realtime event queues require lock-free 32-bit atomics.");
static_assert(std::atomic<RealtimeGuardOperation>::is_always_lock_free,
              "Realtime test injection requires a lock-free operation atomic.");

void updateAtomicMaximum(std::atomic<std::uint64_t>& destination, std::uint64_t value)
{
    auto current = destination.load(std::memory_order_relaxed);
    while (current < value
           && !destination.compare_exchange_weak(current,
                                                 value,
                                                 std::memory_order_release,
                                                 std::memory_order_relaxed))
    {
    }
}

std::optional<double> findMacroValue(const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                     std::string_view macroId)
{
    const auto iterator = std::find_if(sessionState.macroValues.begin(),
                                       sessionState.macroValues.end(),
                                       [&](const auto& macroValue)
                                       {
                                           return macroValue.id.size() == macroId.size()
                                               && std::equal(macroValue.id.begin(), macroValue.id.end(), macroId.begin());
                                       });
    if (iterator == sessionState.macroValues.end())
        return std::nullopt;

    return iterator->value;
}

int clampMidiValue(int value)
{
    return std::clamp(value, 0, 127);
}

int computeToneRenderVelocity(const drs::engine::RuntimeSessionStateSnapshot& sessionState)
{
    const auto toneValue = findMacroValue(sessionState, "tone").value_or(0.35);
    const auto effectiveVelocity = static_cast<int>(std::lround(32.0 + toneValue * 95.0));
    return std::clamp(effectiveVelocity, 1, 127);
}

int computeMotionRenderNote(const drs::engine::RuntimeSessionStateSnapshot& sessionState, int playedNote)
{
    const auto motionValue = findMacroValue(sessionState, "motion").value_or(0.15);
    const auto semitoneOffset = static_cast<int>(std::lround((motionValue - 0.5) * 24.0));
    return clampMidiValue(playedNote + semitoneOffset);
}

drs::engine::RuntimeProjectModel buildInitialAuthoringProject()
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 2;
    project.displayName = "No Project Loaded";
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 1;
    project.authoring.notes = { "Open a project or create a new one to begin authoring." };
    project.notes = { "This session starts without loading the checked-in reference project." };
    return project;
}

std::optional<drs::engine::RuntimeProjectSampleSource> findProjectSampleSource(const drs::engine::RuntimeProjectModel& project,
                                                                               const std::string& sampleSourceId)
{
    const auto iterator = std::find_if(project.sampleSources.begin(),
                                       project.sampleSources.end(),
                                       [&](const drs::engine::RuntimeProjectSampleSource& sampleSource)
                                       {
                                           return sampleSource.id == sampleSourceId;
                                       });
    if (iterator == project.sampleSources.end())
        return std::nullopt;

    return *iterator;
}

struct AuthoringPreviewBlockingHint
{
    std::string prerequisite;
    std::string guidance;
};

std::string describeZoneLabel(const drs::engine::RuntimeProjectZoneDefinition& zone)
{
    return zone.displayName.empty() ? zone.id : zone.displayName;
}

std::string describeSampleFileLabel(const std::string& samplePath)
{
    if (samplePath.empty())
        return "the selected sample file";

    const auto fileName = fs::path(samplePath).filename().generic_string();
    return fileName.empty() ? samplePath : fileName;
}

AuthoringPreviewBlockingHint buildAuthoringPreviewBlockingHint(const drs::engine::AuthoringSession& authoringSession,
                                                              const std::string& failureState)
{
    if (failureState.empty())
        return {};

    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
    {
        return { "Select a zone to audition.",
                 "Choose a zone in the map or Selected Zone list before auditioning the authoring preview." };
    }

    const auto zoneLabel = describeZoneLabel(*selectedZone);
    const auto sampleSource = findProjectSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (!sampleSource.has_value())
    {
        return { "Assign a sample source to the selected zone.",
                 "Zone '" + zoneLabel + "' points to a sample source that is no longer in the project. Reassign it to an imported sample, then audition again." };
    }

    if (sampleSource->path.empty())
    {
        return { "Choose a sample file for the selected zone.",
                 "Sample source '" + sampleSource->id + "' does not have an audio file path yet. Import or relink a source file for zone '" + zoneLabel + "'." };
    }

    const auto sampleFileLabel = describeSampleFileLabel(sampleSource->path);
    if (failureState == "Sample missing" || failureState.find("not found") != std::string::npos)
    {
        return { "Relink or re-import the selected sample file.",
                 "Restore or replace '" + sampleFileLabel + "' for zone '" + zoneLabel + "', then prepare the authoring preview again." };
    }

    if (failureState == "Sample format unsupported"
        || failureState.find("supported audio format") != std::string::npos
        || failureState.find("WAV and FLAC") != std::string::npos)
    {
        return { "Convert the selected sample to a supported format.",
                 "Zone '" + zoneLabel + "' is pointing at '" + sampleFileLabel + "'. Convert it to a supported WAV, AIFF, or FLAC file, then audition again." };
    }

    if (failureState.find("44100 Hz and 48000 Hz") != std::string::npos)
    {
        return { "Use a 44.1 kHz or 48 kHz sample for this zone.",
                 "Resample '" + sampleFileLabel + "' to 44.1 kHz or 48 kHz so zone '" + zoneLabel + "' can be prepared for preview." };
    }

    if (failureState == "Sample channel count invalid"
        || failureState.find("mono and stereo") != std::string::npos)
    {
        return { "Use a mono or stereo sample for this zone.",
                 "Replace '" + sampleFileLabel + "' with a mono or stereo file so zone '" + zoneLabel + "' can be prepared for preview." };
    }

    if (failureState == "Sample too large")
    {
        return { "Replace the selected sample with a smaller source file.",
                 "The current file '" + sampleFileLabel + "' exceeds the importer limit. Choose a smaller source for zone '" + zoneLabel + "' and prepare preview again." };
    }

    if (failureState == "Sample read failed" || failureState == "Sample length invalid")
    {
        return { "Repair or replace the selected sample file.",
                 "The source file '" + sampleFileLabel + "' could not be read cleanly for zone '" + zoneLabel + "'. Repair the file or replace it before previewing again." };
    }

    return { "Repair the selected sample source and prepare preview again.",
             "Zone '" + zoneLabel + "' still has a blocking sample-source problem. Recheck '" + sampleFileLabel + "', then audition the preview again." };
}

const std::vector<float>* selectWaveformPreviewChannel(const drs::engine::ImportedSampleData& sample)
{
    const std::vector<float>* previewChannel = nullptr;
    auto previewChannelPeak = 0.0f;

    for (const auto& channel : sample.normalizedChannels)
    {
        if (channel.empty())
            continue;

        auto channelPeak = 0.0f;
        for (const auto value : channel)
            channelPeak = std::max(channelPeak, std::abs(value));

        if (previewChannel == nullptr || channelPeak > previewChannelPeak)
        {
            previewChannel = &channel;
            previewChannelPeak = channelPeak;
        }
    }

    return previewChannel;
}
} // namespace

bool Processor::RealtimeNoteEventQueue::push(QueuedRealtimeNoteEvent event) noexcept
{
    const auto write = writeIndex.load(std::memory_order_relaxed);
    const auto next = (write + 1u) % storageCapacity;
    if (next == readIndex.load(std::memory_order_acquire))
        return false;

    events[write] = event;
    writeIndex.store(next, std::memory_order_release);
    return true;
}

bool Processor::RealtimeNoteEventQueue::pop(QueuedRealtimeNoteEvent& event) noexcept
{
    const auto read = readIndex.load(std::memory_order_relaxed);
    if (read == writeIndex.load(std::memory_order_acquire))
        return false;

    event = events[read];
    readIndex.store((read + 1u) % storageCapacity, std::memory_order_release);
    return true;
}

void Processor::RealtimeNoteEventQueue::reset() noexcept
{
    readIndex.store(0, std::memory_order_release);
    writeIndex.store(0, std::memory_order_release);
}

Processor::Processor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      authoringSession(buildInitialAuthoringProject()),
      parameterState(*this, nullptr, "macroParameters", buildParameterLayout(engineFacade))
{
    primeRealtimeSafetyState(512);
    initializeAuthoringImportMetrics();

    for (const auto& macro : engineFacade.getMacroDescriptors())
        parameterState.addParameterListener(buildMacroParameterId(macro.id), this);

    syncParametersFromEngine();
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
}

Processor::~Processor()
{
    for (const auto& macro : engineFacade.getMacroDescriptors())
        parameterState.removeParameterListener(buildMacroParameterId(macro.id), this);

    performancePlaybackContext.closeAtBlockBoundary();
    authoringPreviewPlaybackContext.closeAtBlockBoundary();
    performancePlaybackContext.serviceRetirements();
    authoringPreviewPlaybackContext.serviceRetirements();
    delete[] realtimeGuardTestAllocation;
}

void Processor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    performanceSurfaceNoteQueue.reset();
    authoringPreviewNoteQueue.reset();
    performancePlaybackContext.prepare(currentSampleRate);
    authoringPreviewPlaybackContext.prepare(currentSampleRate);
    primeRealtimeSafetyState(samplesPerBlock > 0 ? samplesPerBlock : 512);
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
}

void Processor::releaseResources()
{
    performancePlaybackContext.resetAtBlockBoundary();
    authoringPreviewPlaybackContext.resetAtBlockBoundary();
    updateRealtimeSafetyState();
}

bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const auto blockStartTime = std::chrono::steady_clock::now();
    juce::ScopedNoDenormals noDenormals;
    const ScopedRealtimeAudioThread audioCallbackScope(realtimeGuardState);
    runRealtimeGuardTestInjection();

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    drs::engine::SamplerEventBlock performanceEvents;
    drs::engine::SamplerEventBlock authoringPreviewEvents;
    drainRealtimeNoteEvents(performanceSurfaceNoteQueue, performanceEvents);
    drainRealtimeNoteEvents(authoringPreviewNoteQueue, authoringPreviewEvents);

    const auto frameCount = buffer.getNumSamples();
    for (const auto metadata : midiMessages)
    {
        const auto* eventData = metadata.data;
        const auto command = metadata.numBytes > 0 ? static_cast<int>(eventData[0] & 0xf0u) : 0;
        const auto noteNumber = metadata.numBytes > 1 ? static_cast<int>(eventData[1] & 0x7fu) : 0;
        const auto velocity = metadata.numBytes > 2 ? static_cast<int>(eventData[2] & 0x7fu) : 0;
        const auto eventSample = frameCount > 0
            ? static_cast<std::uint32_t>(std::clamp(metadata.samplePosition, 0, frameCount - 1))
            : 0u;
        if (command == 0x90 && velocity > 0)
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::noteOn,
                                     eventSample,
                                     static_cast<std::uint8_t>(noteNumber),
                                     static_cast<float>(velocity) / 127.0f });
        }
        else if (command == 0x80 || (command == 0x90 && velocity == 0))
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::noteOff,
                                     eventSample,
                                     static_cast<std::uint8_t>(noteNumber),
                                     0.0f });
        }
        else if (command == 0xb0 && metadata.numBytes > 1
                 && (eventData[1] == 120u || eventData[1] == 123u))
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::reset,
                                     eventSample,
                                     0,
                                     0.0f });
        }
    }

    if (frameCount > 0 && buffer.getNumChannels() > 0)
    {
        drs::engine::SamplerAudioBufferView output {
            buffer.getArrayOfWritePointers(),
            static_cast<std::uint32_t>(buffer.getNumChannels()),
            static_cast<std::uint32_t>(frameCount)
        };
        const auto performanceResult = performancePlaybackContext.renderBlock(
            output, performanceEvents.view());
        const auto previewResult = authoringPreviewPlaybackContext.renderBlock(
            output, authoringPreviewEvents.view());
        if (performanceResult.activationApplied)
            diagnosticsPerformanceActivationCount.fetch_add(1, std::memory_order_relaxed);
        if (previewResult.activationApplied)
            diagnosticsAuthoringPreviewActivationCount.fetch_add(1, std::memory_order_relaxed);
    }

    diagnosticsProcessBlockCount.fetch_add(1, std::memory_order_relaxed);
    const auto callbackBudgetMicros = RealtimeCallbackBudgetProfile::deadlineMicros(
        currentSampleRate,
        static_cast<std::size_t>(buffer.getNumSamples()));
    diagnosticsCallbackBudgetMicros.store(callbackBudgetMicros, std::memory_order_relaxed);
    const auto elapsedMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - blockStartTime).count());
    diagnosticsLastProcessBlockMicros.store(elapsedMicros, std::memory_order_relaxed);
    updateAtomicMaximum(diagnosticsMaxProcessBlockMicros, elapsedMicros);
    if (elapsedMicros > callbackBudgetMicros)
    {
        diagnosticsOverBudgetCallbackCount.fetch_add(1, std::memory_order_relaxed);
        recordRealtimeGuardOperation(RealtimeGuardOperation::overBudget);
    }
    updateRealtimeSafetyState();
}

juce::AudioProcessorEditor* Processor::createEditor()
{
    return new Editor(*this);
}

const juce::String Processor::getName() const
{
    return "DecentRhapsodyStudioPlugin";
}

void Processor::getStateInformation(juce::MemoryBlock& destinationData)
{
    syncEngineFromParameters();
    const auto presetStateJson = engineFacade.exportPresetStateJson();
    destinationData.replaceWith(presetStateJson.data(), presetStateJson.size());
}

void Processor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    const auto* bytes = static_cast<const char*>(data);
    engineFacade.restorePresetStateJson(std::string(bytes, bytes + sizeInBytes));
    syncParametersFromEngine();
    serviceMessageThreadWork();
}

void Processor::setMacroValueFromShell(const std::string& macroId, double value)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(
            parameterState.getParameter(buildMacroParameterId(macroId))))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(value)));
        parameter->endChangeGesture();
    }
}

void Processor::queueAuthoringPreviewNoteOn(int midiNoteNumber, float velocity)
{
    serviceMessageThreadWork();
    authoringPreviewNoteQueue.push(
        { clampMidiValue(midiNoteNumber), std::clamp(velocity, 0.0f, 1.0f), true });
}

void Processor::queueAuthoringPreviewNoteOff(int midiNoteNumber)
{
    authoringPreviewNoteQueue.push({ clampMidiValue(midiNoteNumber), 0.0f, false });
}

drs::app::AuthoringWaveformPreview Processor::getAuthoringWaveformPreview()
{
    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
        return { false, "No zone selected" };

    const auto projectSampleSource = findProjectSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (!projectSampleSource.has_value())
        return { false, "Selected zone sample source is missing from the project." };

    if (const auto iterator = authoringWaveformPreviewCache.find(projectSampleSource->id);
        iterator != authoringWaveformPreviewCache.end())
    {
        auto preview = iterator->second;
        preview.loopEnabled = selectedZone->loopEnabled;
        preview.loopStartFrame = selectedZone->loopStartFrame;
        preview.loopEndFrame = selectedZone->loopEndFrame;
        return preview;
    }

    // Sprint 3 boundary note: waveform preview is a shell-only helper seam.
    // It may decode for authoring UI today, but it must not become the Preview/Publish playback-preparation path.
    const auto importResult = drs::engine::importSampleFile(projectSampleSource->path);
    if (!importResult.imported)
    {
        drs::app::AuthoringWaveformPreview preview;
        preview.state = importResult.state.empty() ? "Waveform preview unavailable" : importResult.state;
        return preview;
    }

    auto preview = buildAuthoringWaveformPreview(importResult.sample,
                                                 selectedZone->loopEnabled,
                                                 selectedZone->loopStartFrame,
                                                 selectedZone->loopEndFrame);
    authoringWaveformPreviewCache.emplace(projectSampleSource->id, preview);
    return preview;
}

drs::app::AuthoringPreviewStatusSnapshot Processor::getAuthoringPreviewStatusSnapshot() const
{
    const auto diagnostics = getRealtimeSafetySnapshot();
    drs::app::AuthoringPreviewStatusSnapshot status;
    status.available = diagnostics.available;
    status.draftRevision = diagnostics.currentAuthoringPreviewDraftRevision;
    status.activeRevision = diagnostics.activeAuthoringPreviewRevision;
    status.pendingRevision = diagnostics.pendingAuthoringPreviewRevision;
    status.revisionState = diagnostics.authoringPreviewRevisionState;
    status.failureState = diagnostics.authoringPreviewFailureState;
    const auto blockingHint = buildAuthoringPreviewBlockingHint(authoringSession, status.failureState);
    status.blockingPrerequisite = blockingHint.prerequisite;
    status.blockingGuidance = blockingHint.guidance;
    return status;
}

drs::app::AuthoringImportResponsivenessSnapshot Processor::getAuthoringImportResponsivenessSnapshot() const
{
    return authoringImportResponsivenessSnapshot;
}

void Processor::replaceAuthoringProject(drs::engine::RuntimeProjectModel project)
{
    auto draftPlaybackProject = project;
    if (!engineFacade.replaceDraftPlaybackAuthoringProject(std::move(draftPlaybackProject)))
        return;

    authoringSession.replaceProject(std::move(project));
    engineFacade.closeDraftPlaybackProject();
    engineFacade.reopenDraftPlaybackProject(authoringSession.getDocumentState().revision);
    authoringLoadedSamples.clear();
    authoringWaveformPreviewCache.clear();
    lastAuthoringSampleLoadFailureState.clear();
    failedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
    failedAuthoringPreviewState.clear();
    observedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
    observedAuthoringPreviewZoneId.clear();
    observedDraftPlaybackProjectRevision = authoringSession.getDocumentState().revision;
    initializeAuthoringImportMetrics();
    ensureSelectedAuthoringSampleLoaded(false);
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
}

void Processor::queuePerformanceSurfaceNoteOn(int midiNoteNumber, float velocity)
{
    performanceSurfaceNoteQueue.push(
        { clampMidiValue(midiNoteNumber), std::clamp(velocity, 0.0f, 1.0f), true });
}

void Processor::queuePerformanceSurfaceNoteOff(int midiNoteNumber)
{
    performanceSurfaceNoteQueue.push({ clampMidiValue(midiNoteNumber), 0.0f, false });
}

void Processor::setRealtimeGuardTestInjection(RealtimeGuardOperation operation)
{
    if (realtimeGuardTestAllocation != nullptr)
    {
        delete[] realtimeGuardTestAllocation;
        realtimeGuardTestAllocation = nullptr;
    }

    if (operation == RealtimeGuardOperation::deallocation)
        realtimeGuardTestAllocation = new std::byte[64];

    realtimeGuardTestInjection.store(operation, std::memory_order_release);
}

void Processor::runRealtimeGuardTestInjection()
{
    const auto operation = realtimeGuardTestInjection.exchange(RealtimeGuardOperation::none,
                                                               std::memory_order_acq_rel);
    switch (operation)
    {
        case RealtimeGuardOperation::none:
            return;
        case RealtimeGuardOperation::allocation:
            realtimeGuardTestAllocation = new std::byte[64];
            return;
        case RealtimeGuardOperation::deallocation:
            delete[] realtimeGuardTestAllocation;
            realtimeGuardTestAllocation = nullptr;
            return;
        case RealtimeGuardOperation::blockingLock:
        {
            recordRealtimeGuardOperation(operation);
            const std::lock_guard<std::mutex> lock(realtimeGuardTestMutex);
            return;
        }
        case RealtimeGuardOperation::overBudget:
            recordRealtimeGuardOperation(operation);
            diagnosticsOverBudgetCallbackCount.fetch_add(1, std::memory_order_relaxed);
            return;
        case RealtimeGuardOperation::count:
            return;
        default:
            recordRealtimeGuardOperation(operation);
            return;
    }
}

bool Processor::serviceMessageThreadWork()
{
    const auto authoringRevision = authoringSession.getDocumentState().revision;
    auto synchronizedDraftPlaybackProject = false;
    if (authoringRevision != observedDraftPlaybackProjectRevision)
    {
        auto draftPlaybackProject = authoringSession.getProject();
        if (engineFacade.replaceDraftPlaybackAuthoringProject(std::move(draftPlaybackProject)))
        {
            synchronizedDraftPlaybackProject = engineFacade.stageDraftRevision(authoringRevision);
            observedDraftPlaybackProjectRevision = authoringRevision;
        }
    }

    const auto servicedBackgroundWork = engineFacade.serviceBackgroundWork();
    const auto retiredCountBefore = diagnosticsRetiredActivationCount.load(std::memory_order_acquire);
    const auto reclaimedPreview = authoringPreviewPlaybackContext.serviceRetirements();
    const auto reclaimedPerformance = performancePlaybackContext.serviceRetirements();
    const auto reclaimed = reclaimedPreview + reclaimedPerformance;
    diagnosticsRetiredActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    diagnosticsReclaimedActivationPayloadCount.fetch_add(reclaimed, std::memory_order_relaxed);

    auto synchronizedActivation = false;
    const auto stateRevision = engineFacade.getStateRevision();
    if (stateRevision != observedEngineStateRevision)
    {
        observedEngineStateRevision = stateRevision;
        const auto performanceSnapshot = performancePlaybackContext.getSnapshot();
        synchronizedActivation = synchronizePerformanceActivation(
            !performanceSnapshot.hasActiveActivation);
    }

    auto synchronizedAuthoringPreview = false;
    const auto previewPayload = engineFacade.getPreviewActivationPayload();
    const auto previewPreparedBuildId = previewPayload != nullptr
            && previewPayload->revision == authoringRevision
        ? previewPayload->preparedBuildId
        : 0;
    const auto previewContextSnapshot = authoringPreviewPlaybackContext.getSnapshot();
    const auto selectedZone = authoringSession.getSelectedZone();
    const auto selectedZoneId = selectedZone.has_value() ? selectedZone->id : std::string {};
    if (authoringRevision != observedAuthoringPreviewRevision
        || selectedZoneId != observedAuthoringPreviewZoneId
        || !previewContextSnapshot.hasActiveActivation
        || (previewPreparedBuildId != 0 && previewPreparedBuildId != observedPreviewPreparedBuildId))
    {
        synchronizedAuthoringPreview = synchronizeAuthoringPreviewActivation(
            !previewContextSnapshot.hasActiveActivation);
        observedAuthoringPreviewRevision = authoringRevision;
        observedAuthoringPreviewZoneId = selectedZoneId;
        if (synchronizedAuthoringPreview && previewPreparedBuildId != 0)
            observedPreviewPreparedBuildId = previewPreparedBuildId;
    }

    updateRealtimeSafetyState();
    return servicedBackgroundWork
        || synchronizedDraftPlaybackProject
        || synchronizedAuthoringPreview
        || synchronizedActivation
        || diagnosticsRetiredActivationCount.load(std::memory_order_acquire) != retiredCountBefore;
}

juce::String Processor::buildMacroParameterId(const std::string& macroId)
{
    return "macro." + juce::String::fromUTF8(macroId.c_str());
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::buildParameterLayout(
    const drs::engine::EngineFacade& facade)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& macro : facade.getMacroDescriptors())
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(buildMacroParameterId(macro.id), 1),
            juce::String::fromUTF8(macro.name.c_str()),
            juce::NormalisableRange<float>(static_cast<float>(macro.minValue),
                                           static_cast<float>(macro.maxValue)),
            static_cast<float>(macro.defaultValue)));
    }

    return layout;
}

void Processor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (isSynchronizingParameterState)
        return;

    const auto parameterIdText = parameterID.toStdString();
    const auto macroPrefix = std::string("macro.");
    if (parameterIdText.rfind(macroPrefix, 0) != 0)
        return;

    engineFacade.setMacroValue(parameterIdText.substr(macroPrefix.size()), static_cast<double>(newValue));
    synchronizePerformanceActivation(false);
}

void Processor::syncEngineFromParameters()
{
    for (const auto& macro : engineFacade.getMacroDescriptors())
    {
        if (auto* rawValue = parameterState.getRawParameterValue(buildMacroParameterId(macro.id)))
            engineFacade.setMacroValue(macro.id, static_cast<double>(rawValue->load()));
    }
}

void Processor::syncParametersFromEngine()
{
    const juce::ScopedValueSetter<bool> syncGuard(isSynchronizingParameterState, true);

    for (const auto& macro : engineFacade.getMacroDescriptors())
    {
        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(
                parameterState.getParameter(buildMacroParameterId(macro.id))))
        {
            parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(macro.currentValue)));
        }
    }
}

drs::app::AuthoringWaveformPreview Processor::buildAuthoringWaveformPreview(const drs::engine::ImportedSampleData& sample,
                                                                            bool loopEnabled,
                                                                            std::uint64_t loopStartFrame,
                                                                            std::uint64_t loopEndFrame) const
{
    drs::app::AuthoringWaveformPreview preview;
    preview.available = true;
    preview.state = "Waveform preview ready";
    preview.sourcePath = sample.metadata.sourcePath;
    preview.formatName = sample.metadata.formatName;
    preview.durationSeconds = sample.metadata.durationSeconds;
    preview.sampleRate = sample.metadata.sampleRate;
    preview.frameCount = sample.metadata.frameCount;
    preview.channelCount = sample.metadata.channelCount;
    preview.loopEnabled = loopEnabled;
    preview.loopStartFrame = loopStartFrame;
    preview.loopEndFrame = loopEndFrame;

    const auto* previewChannel = selectWaveformPreviewChannel(sample);
    if (previewChannel == nullptr)
        return preview;

    const auto& monoView = *previewChannel;
    constexpr std::size_t pointCount = 192;
    const auto bucketSize = std::max<std::size_t>(1, monoView.size() / pointCount);

    preview.points.reserve(pointCount);
    for (std::size_t index = 0; index < monoView.size(); index += bucketSize)
    {
        const auto bucketEnd = std::min(monoView.size(), index + bucketSize);
        auto minValue = monoView[index];
        auto maxValue = monoView[index];

        for (std::size_t sampleIndex = index + 1; sampleIndex < bucketEnd; ++sampleIndex)
        {
            minValue = std::min(minValue, monoView[sampleIndex]);
            maxValue = std::max(maxValue, monoView[sampleIndex]);
        }

        preview.points.push_back({ minValue, maxValue });
    }

    return preview;
}

void Processor::initializeAuthoringImportMetrics()
{
    const auto& project = authoringSession.getProject();
    std::vector<std::string> samplePaths;
    samplePaths.reserve(project.sampleSources.size());

    for (const auto& sampleSource : project.sampleSources)
        samplePaths.push_back(sampleSource.path);

    auto queue = drs::engine::createAuthoringImportQueue(samplePaths, project.contentRootPath);
    while (true)
    {
        const auto processResult = drs::engine::processNextAuthoringImportQueueItem(queue);
        if (!processResult.processed)
            break;
    }

    authoringImportResponsivenessSnapshot.available = true;
    authoringImportResponsivenessSnapshot.state = queue.metrics.state;
    authoringImportResponsivenessSnapshot.totalItemCount = queue.metrics.totalItemCount;
    authoringImportResponsivenessSnapshot.pendingCount = queue.metrics.pendingCount;
    authoringImportResponsivenessSnapshot.processedCount = queue.metrics.processedCount;
    authoringImportResponsivenessSnapshot.warningItemCount = queue.metrics.warningItemCount;
    authoringImportResponsivenessSnapshot.failedItemCount = queue.metrics.failedItemCount;
    authoringImportResponsivenessSnapshot.canceledItemCount = queue.metrics.canceledItemCount;
    authoringImportResponsivenessSnapshot.acceptedItemCount = queue.metrics.acceptedItemCount;
    authoringImportResponsivenessSnapshot.lastProcessDurationMicros = queue.metrics.lastProcessDurationMicros;
    authoringImportResponsivenessSnapshot.averageProcessDurationMicros = queue.metrics.averageProcessDurationMicros;
    authoringImportResponsivenessSnapshot.maxProcessDurationMicros = queue.metrics.maxProcessDurationMicros;
    authoringImportResponsivenessSnapshot.lastProcessedItemId = queue.metrics.lastProcessedItemId;
}

bool Processor::ensureSelectedAuthoringSampleLoaded(bool invokedFromAudioThread)
{
    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
    {
        lastAuthoringSampleLoadFailureState = "No zone selected.";
        return false;
    }

    const auto projectSampleSource = findProjectSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (!projectSampleSource.has_value())
    {
        lastAuthoringSampleLoadFailureState = "Selected zone sample source is missing from the project.";
        return false;
    }

    if (authoringLoadedSamples.find(projectSampleSource->id) != authoringLoadedSamples.end())
    {
        lastAuthoringSampleLoadFailureState.clear();
        return true;
    }

    // Sprint 3 boundary note: selected-zone preview loading is still a shell-side helper seam.
    // It must not be expanded into the product-owned Preview/Publish preparation boundary.
    if (invokedFromAudioThread)
    {
        recordRealtimeGuardOperation(RealtimeGuardOperation::pathResolution);
        recordRealtimeGuardOperation(RealtimeGuardOperation::fileOpen);
        recordRealtimeGuardOperation(RealtimeGuardOperation::fileRead);
        recordRealtimeGuardOperation(RealtimeGuardOperation::sampleDecode);
    }

    const auto importResult = drs::engine::importSampleFile(projectSampleSource->path);
    if (!importResult.imported)
    {
        lastAuthoringSampleLoadFailureState = !importResult.issues.empty()
            ? importResult.issues.front()
            : (importResult.state.empty() ? "Selected authoring sample could not be prepared."
                                          : importResult.state);
        return false;
    }

    authoringLoadedSamples.emplace(projectSampleSource->id, LoadedAuthoringSample { importResult.sample });
    lastAuthoringSampleLoadFailureState.clear();
    if (invokedFromAudioThread)
        diagnosticsAuthoringSampleLoadsOnAudioThread.fetch_add(1, std::memory_order_relaxed);
    updateRealtimeSafetyState();
    return true;
}

bool Processor::synchronizeAuthoringPreviewActivation(bool installImmediately)
{
    return stageAuthoringPreviewActivation(installImmediately);
}

bool Processor::stageAuthoringPreviewActivation(bool installImmediately)
{
    const auto reclaimed = authoringPreviewPlaybackContext.serviceRetirements();
    diagnosticsRetiredActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    diagnosticsReclaimedActivationPayloadCount.fetch_add(reclaimed, std::memory_order_relaxed);
    const auto currentRevision = authoringSession.getDocumentState().revision;
    const auto failPreviewActivation = [&](const std::string& state)
    {
        failedAuthoringPreviewRevision = currentRevision;
        failedAuthoringPreviewState = state.empty() ? "Authoring preview preparation failed." : state;
        return false;
    };

    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
    {
        failedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
        failedAuthoringPreviewState.clear();
        return false;
    }

    auto payload = engineFacade.getPreviewActivationPayload();
    if (payload == nullptr || payload->revision != currentRevision)
    {
        const auto projectSampleSource = findProjectSampleSource(authoringSession.getProject(),
                                                                 selectedZone->sampleSourceId);
        if (!projectSampleSource.has_value())
            return failPreviewActivation("Selected zone sample source is missing from the project.");
        if (!ensureSelectedAuthoringSampleLoaded(false))
            return failPreviewActivation(lastAuthoringSampleLoadFailureState);
        const auto sampleIterator = authoringLoadedSamples.find(projectSampleSource->id);
        if (sampleIterator == authoringLoadedSamples.end())
            return failPreviewActivation("Selected authoring sample was not cached after preparation.");

        const auto& imported = sampleIterator->second.sample;
        auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = imported.normalizedChannels;
        const auto snapshotBuildId = 0x8000000000000000ull
            | static_cast<std::uint64_t>(currentRevision + 1);
        const auto preparedBuildId = 0x4000000000000000ull
            | static_cast<std::uint64_t>(currentRevision + 1);
        const auto digestSuffix = std::to_string(currentRevision) + "-" + selectedZone->id
            + "-" + projectSampleSource->id;

        drs::engine::ImmutablePlaybackSnapshot snapshot;
        snapshot.draftRevision = currentRevision;
        snapshot.selectedZoneId = selectedZone->id;
        snapshot.contentDigest = "processor-preview-snapshot-" + digestSuffix;
        snapshot.zones.push_back({ selectedZone->id,
                                   projectSampleSource->id,
                                   selectedZone->displayName,
                                   selectedZone->groupId,
                                   selectedZone->articulationId,
                                   selectedZone->rootKey,
                                   selectedZone->keyLow,
                                   selectedZone->keyHigh,
                                   selectedZone->velocityLow,
                                   selectedZone->velocityHigh,
                                   selectedZone->gainDb,
                                   selectedZone->pan,
                                   selectedZone->sampleStartFrame,
                                   selectedZone->loopEnabled,
                                   selectedZone->loopStartFrame,
                                   selectedZone->loopEndFrame });

        drs::engine::PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = projectSampleSource->id;
        sample.streamSampleId = "processor-preview-stream-" + projectSampleSource->id;
        sample.sampleRate = imported.metadata.sampleRate;
        sample.frameCount = imported.metadata.frameCount;
        sample.channelCount = imported.metadata.channelCount;
        sample.decodedSampleData = std::move(decoded);

        drs::engine::ImmutablePreparedPlayback prepared;
        prepared.snapshotBuildId = snapshotBuildId;
        prepared.snapshotContentDigest = snapshot.contentDigest;
        prepared.draftRevision = currentRevision;
        prepared.preparedContentDigest = "processor-preview-prepared-" + digestSuffix;
        prepared.samples.push_back(std::move(sample));
        prepared.zones.push_back({ selectedZone->id,
                                   projectSampleSource->id,
                                   "processor-preview-stream-" + projectSampleSource->id,
                                   0,
                                   0,
                                   selectedZone->rootKey,
                                   selectedZone->keyLow,
                                   selectedZone->keyHigh,
                                   selectedZone->velocityLow,
                                   selectedZone->velocityHigh,
                                   selectedZone->gainDb,
                                   selectedZone->pan,
                                   selectedZone->sampleStartFrame,
                                   selectedZone->loopEnabled,
                                   selectedZone->loopStartFrame,
                                   selectedZone->loopEndFrame });

        auto immediatePayload = std::make_shared<drs::engine::PlaybackActivationPayload>();
        immediatePayload->lane = drs::engine::PlaybackActivationLane::preview;
        immediatePayload->revision = currentRevision;
        immediatePayload->snapshotBuildId = snapshotBuildId;
        immediatePayload->preparedBuildId = preparedBuildId;
        immediatePayload->lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::ready;
        immediatePayload->activationEligible = true;
        immediatePayload->snapshotContentDigest = snapshot.contentDigest;
        immediatePayload->preparedContentDigest = prepared.preparedContentDigest;
        immediatePayload->retainedPreparedBytes = imported.metadata.frameCount
            * imported.metadata.channelCount * sizeof(float);
        immediatePayload->snapshot
            = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(std::move(snapshot));
        immediatePayload->prepared
            = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(std::move(prepared));
        payload = std::move(immediatePayload);
    }

    drs::engine::SamplerRenderModelBuildOptions options;
    options.selectedZoneId = selectedZone->id;
    options.auditionSelectedZone = true;
    const auto modelResult = drs::engine::buildSamplerRenderModel(payload, options);
    if (!modelResult.built || modelResult.model == nullptr)
        return failPreviewActivation(modelResult.findings.empty()
                                         ? "Authoring preview route normalization failed."
                                         : modelResult.findings.front().message);
    if (!authoringPreviewPlaybackContext.stageActivation(modelResult.model))
        return failPreviewActivation("Authoring preview activation slots are exhausted.");

    failedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
    failedAuthoringPreviewState.clear();
    if (installImmediately && authoringPreviewPlaybackContext.activatePendingForPreparation())
    {
        diagnosticsAuthoringPreviewActivationCount.fetch_add(1, std::memory_order_relaxed);
    }
    return true;
}

bool Processor::synchronizePerformanceActivation(bool installImmediately)
{
    const auto performanceSnapshot = engineFacade.getPerformanceSnapshot();
    const auto& sessionState = engineFacade.getCurrentSessionState();
    return stagePerformanceActivation(performanceSnapshot, sessionState, installImmediately);
}

bool Processor::stagePerformanceActivation(const drs::engine::EnginePerformanceSnapshot& performanceSnapshot,
                                           const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                           bool installImmediately)
{
    const auto reclaimed = performancePlaybackContext.serviceRetirements();
    diagnosticsRetiredActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    diagnosticsReclaimedActivationPayloadCount.fetch_add(reclaimed, std::memory_order_relaxed);
    const auto payload = engineFacade.getPerformanceActivationPayload();
    if (!performanceSnapshot.loaded || payload == nullptr
        || payload->revision != performanceSnapshot.publishedRevision)
    {
        return false;
    }

    drs::engine::SamplerRenderModelBuildOptions options;
    options.selectedArticulationId = sessionState.selectedArticulationId;
    if (options.selectedArticulationId.empty())
    {
        const auto articulations = engineFacade.getArticulationDescriptors();
        const auto defaultArticulation = std::find_if(articulations.begin(),
                                                      articulations.end(),
                                                      [](const auto& articulation)
                                                      {
                                                          return articulation.isDefault;
                                                      });
        if (defaultArticulation != articulations.end())
            options.selectedArticulationId = defaultArticulation->id;
    }
    options.midiNoteOffset = computeMotionRenderNote(sessionState, 60) - 60;
    options.fixedVelocity = computeToneRenderVelocity(sessionState);
    const auto modelResult = drs::engine::buildSamplerRenderModel(payload, options);
    if (!modelResult.built || modelResult.model == nullptr
        || !performancePlaybackContext.stageActivation(modelResult.model))
    {
        return false;
    }

    if (installImmediately && performancePlaybackContext.activatePendingForPreparation())
    {
        diagnosticsPerformanceActivationCount.fetch_add(1, std::memory_order_relaxed);
    }
    return true;
}

void Processor::drainRealtimeNoteEvents(RealtimeNoteEventQueue& queue,
                                        drs::engine::SamplerEventBlock& destination) noexcept
{
    QueuedRealtimeNoteEvent event;
    while (queue.pop(event))
    {
        destination.push({ event.noteOn
                               ? drs::engine::SamplerRenderEventType::noteOn
                               : drs::engine::SamplerRenderEventType::noteOff,
                           0,
                           static_cast<std::uint8_t>(clampMidiValue(event.midiNoteNumber)),
                           event.noteOn ? std::max(event.velocity, 1.0f / 127.0f) : 0.0f });
    }
}

void Processor::primeRealtimeSafetyState(int samplesPerBlock)
{
    if (!performancePlaybackContext.getSnapshot().prepared)
        performancePlaybackContext.prepare(currentSampleRate);
    if (!authoringPreviewPlaybackContext.getSnapshot().prepared)
        authoringPreviewPlaybackContext.prepare(currentSampleRate);
    diagnosticsPreparedBlockSize.store(static_cast<std::size_t>(samplesPerBlock), std::memory_order_release);
    diagnosticsActiveVoiceCapacityLimit.store(maxRealtimeActiveVoices * 2, std::memory_order_release);
    diagnosticsPrimedActiveVoiceCapacity.store(maxRealtimeActiveVoices * 2, std::memory_order_release);
    updateRealtimeSafetyState();
}

void Processor::updateRealtimeSafetyState()
{
    if (isCurrentThreadRealtimeAudio())
    {
        publishAudioDiagnostics();
        return;
    }

    publishMessageDiagnostics();
}

Processor::AudioDiagnosticsValues Processor::captureActivationDiagnostics() const
{
    AudioDiagnosticsValues values;
    const auto preview = authoringPreviewPlaybackContext.getSnapshot();
    const auto performance = performancePlaybackContext.getSnapshot();
    values.hasActiveAuthoringPreviewActivation = preview.hasActiveActivation;
    values.hasPendingAuthoringPreviewActivation = preview.hasPendingActivation;
    values.hasActivePerformanceActivation = performance.hasActiveActivation;
    values.hasPendingPerformanceActivation = performance.hasPendingActivation;
    values.activeAuthoringPreviewRevision = preview.activeRevision;
    values.pendingAuthoringPreviewRevision = preview.pendingRevision;
    values.activePublishedRevision = performance.activeRevision;
    values.pendingPublishedRevision = performance.pendingRevision;
    values.activePreparedBuildId = performance.activePreparedBuildId;
    values.pendingPreparedBuildId = performance.pendingPreparedBuildId;
    values.retiredActivationBacklog = preview.retiredActivationBacklog
        + performance.retiredActivationBacklog;
    values.activeActivationPayloadBytes = preview.activeActivationPayloadBytes
        + performance.activeActivationPayloadBytes;
    values.pendingActivationPayloadBytes = preview.pendingActivationPayloadBytes
        + performance.pendingActivationPayloadBytes;
    values.retiredActivationPayloadBytes = preview.retiredActivationPayloadBytes
        + performance.retiredActivationPayloadBytes;

    return values;
}

void Processor::publishAudioDiagnostics()
{
    auto values = captureActivationDiagnostics();
    const auto performance = performancePlaybackContext.getSnapshot();
    const auto preview = authoringPreviewPlaybackContext.getSnapshot();
    values.performanceActiveVoiceCount = performance.activeVoiceCount + performance.releasingVoiceCount;
    values.authoringPreviewActiveVoiceCount = preview.activeVoiceCount + preview.releasingVoiceCount;
    values.activeVoiceCapacity = maxRealtimeActiveVoices * 2;

    auto sequence = audioDiagnosticsPublication.sequence.fetch_add(1, std::memory_order_acq_rel) + 1;
    audioDiagnosticsPublication.performanceActiveVoiceCount.store(values.performanceActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewActiveVoiceCount.store(values.authoringPreviewActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.activeVoiceCapacity.store(values.activeVoiceCapacity, std::memory_order_relaxed);
    audioDiagnosticsPublication.activeAuthoringPreviewRevision.store(values.activeAuthoringPreviewRevision, std::memory_order_relaxed);
    audioDiagnosticsPublication.pendingAuthoringPreviewRevision.store(values.pendingAuthoringPreviewRevision, std::memory_order_relaxed);
    audioDiagnosticsPublication.activePublishedRevision.store(values.activePublishedRevision, std::memory_order_relaxed);
    audioDiagnosticsPublication.pendingPublishedRevision.store(values.pendingPublishedRevision, std::memory_order_relaxed);
    audioDiagnosticsPublication.activePreparedBuildId.store(values.activePreparedBuildId, std::memory_order_relaxed);
    audioDiagnosticsPublication.pendingPreparedBuildId.store(values.pendingPreparedBuildId, std::memory_order_relaxed);
    audioDiagnosticsPublication.retiredActivationBacklog.store(values.retiredActivationBacklog, std::memory_order_relaxed);
    audioDiagnosticsPublication.activeActivationPayloadBytes.store(values.activeActivationPayloadBytes, std::memory_order_relaxed);
    audioDiagnosticsPublication.pendingActivationPayloadBytes.store(values.pendingActivationPayloadBytes, std::memory_order_relaxed);
    audioDiagnosticsPublication.retiredActivationPayloadBytes.store(values.retiredActivationPayloadBytes, std::memory_order_relaxed);
    audioDiagnosticsPublication.hasActiveAuthoringPreviewActivation.store(values.hasActiveAuthoringPreviewActivation, std::memory_order_relaxed);
    audioDiagnosticsPublication.hasPendingAuthoringPreviewActivation.store(values.hasPendingAuthoringPreviewActivation, std::memory_order_relaxed);
    audioDiagnosticsPublication.hasActivePerformanceActivation.store(values.hasActivePerformanceActivation, std::memory_order_relaxed);
    audioDiagnosticsPublication.hasPendingPerformanceActivation.store(values.hasPendingPerformanceActivation, std::memory_order_relaxed);
    audioDiagnosticsPublication.sequence.store(sequence + 1, std::memory_order_release);
}

Processor::AudioDiagnosticsValues Processor::readAudioDiagnostics(std::uint64_t& sequence) const
{
    AudioDiagnosticsValues values;
    for (;;)
    {
        const auto before = audioDiagnosticsPublication.sequence.load(std::memory_order_acquire);
        if ((before & 1u) != 0)
            continue;

        values.performanceActiveVoiceCount = audioDiagnosticsPublication.performanceActiveVoiceCount.load(std::memory_order_relaxed);
        values.authoringPreviewActiveVoiceCount = audioDiagnosticsPublication.authoringPreviewActiveVoiceCount.load(std::memory_order_relaxed);
        values.activeVoiceCapacity = audioDiagnosticsPublication.activeVoiceCapacity.load(std::memory_order_relaxed);
        values.activeAuthoringPreviewRevision = audioDiagnosticsPublication.activeAuthoringPreviewRevision.load(std::memory_order_relaxed);
        values.pendingAuthoringPreviewRevision = audioDiagnosticsPublication.pendingAuthoringPreviewRevision.load(std::memory_order_relaxed);
        values.activePublishedRevision = audioDiagnosticsPublication.activePublishedRevision.load(std::memory_order_relaxed);
        values.pendingPublishedRevision = audioDiagnosticsPublication.pendingPublishedRevision.load(std::memory_order_relaxed);
        values.activePreparedBuildId = audioDiagnosticsPublication.activePreparedBuildId.load(std::memory_order_relaxed);
        values.pendingPreparedBuildId = audioDiagnosticsPublication.pendingPreparedBuildId.load(std::memory_order_relaxed);
        values.retiredActivationBacklog = audioDiagnosticsPublication.retiredActivationBacklog.load(std::memory_order_relaxed);
        values.activeActivationPayloadBytes = audioDiagnosticsPublication.activeActivationPayloadBytes.load(std::memory_order_relaxed);
        values.pendingActivationPayloadBytes = audioDiagnosticsPublication.pendingActivationPayloadBytes.load(std::memory_order_relaxed);
        values.retiredActivationPayloadBytes = audioDiagnosticsPublication.retiredActivationPayloadBytes.load(std::memory_order_relaxed);
        values.hasActiveAuthoringPreviewActivation = audioDiagnosticsPublication.hasActiveAuthoringPreviewActivation.load(std::memory_order_relaxed);
        values.hasPendingAuthoringPreviewActivation = audioDiagnosticsPublication.hasPendingAuthoringPreviewActivation.load(std::memory_order_relaxed);
        values.hasActivePerformanceActivation = audioDiagnosticsPublication.hasActivePerformanceActivation.load(std::memory_order_relaxed);
        values.hasPendingPerformanceActivation = audioDiagnosticsPublication.hasPendingPerformanceActivation.load(std::memory_order_relaxed);

        const auto after = audioDiagnosticsPublication.sequence.load(std::memory_order_acquire);
        if (before == after)
        {
            sequence = after;
            return values;
        }
    }
}

ProcessorRealtimeSafetySnapshot Processor::composeDiagnosticsSnapshot(
    const AudioDiagnosticsValues& audioValues,
    std::uint64_t publicationSequence) const
{
    ProcessorRealtimeSafetySnapshot snapshot;
    snapshot.available = true;
    snapshot.publicationSequence = publicationSequence;
    snapshot.processBlockCount = diagnosticsProcessBlockCount.load(std::memory_order_acquire);
    snapshot.preparedBlockSize = diagnosticsPreparedBlockSize.load(std::memory_order_acquire);
    snapshot.referenceSampleCountLoaded = diagnosticsReferenceSampleCountLoaded.load(std::memory_order_acquire);
    snapshot.referenceWarmupCount = diagnosticsReferenceWarmupCount.load(std::memory_order_acquire);
    snapshot.referenceSampleLoadsOnAudioThread = diagnosticsReferenceSampleLoadsOnAudioThread.load(std::memory_order_acquire);
    snapshot.authoringSampleLoadsOnAudioThread = diagnosticsAuthoringSampleLoadsOnAudioThread.load(std::memory_order_acquire);
    snapshot.performanceActiveVoiceCount = audioValues.performanceActiveVoiceCount;
    snapshot.authoringPreviewActiveVoiceCount = audioValues.authoringPreviewActiveVoiceCount;
    snapshot.activeVoiceCapacity = std::max(audioValues.activeVoiceCapacity,
                                            diagnosticsPrimedActiveVoiceCapacity.load(std::memory_order_acquire));
    snapshot.activeVoiceCapacityLimit = diagnosticsActiveVoiceCapacityLimit.load(std::memory_order_acquire);
    snapshot.activeVoiceCapacityGrowthCount = diagnosticsActiveVoiceCapacityGrowthCount.load(std::memory_order_acquire);
    snapshot.authoringPreviewActivationCount = diagnosticsAuthoringPreviewActivationCount.load(std::memory_order_acquire);
    snapshot.performanceActivationCount = diagnosticsPerformanceActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationCount = diagnosticsRetiredActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationBacklog = audioValues.retiredActivationBacklog;
    snapshot.reclaimedActivationPayloadCount = diagnosticsReclaimedActivationPayloadCount.load(std::memory_order_acquire);
    snapshot.activeActivationPayloadBytes = audioValues.activeActivationPayloadBytes;
    snapshot.pendingActivationPayloadBytes = audioValues.pendingActivationPayloadBytes;
    snapshot.retiredActivationPayloadBytes = audioValues.retiredActivationPayloadBytes;
    applyRealtimeGuardDiagnostics(snapshot);
    snapshot.callbackBudgetMicros = diagnosticsCallbackBudgetMicros.load(std::memory_order_acquire);
    snapshot.lastProcessBlockMicros = diagnosticsLastProcessBlockMicros.load(std::memory_order_acquire);
    snapshot.maxProcessBlockMicros = diagnosticsMaxProcessBlockMicros.load(std::memory_order_acquire);
    snapshot.overBudgetCallbackCount = diagnosticsOverBudgetCallbackCount.load(std::memory_order_acquire);
    snapshot.currentAuthoringPreviewDraftRevision = diagnosticsCurrentAuthoringPreviewDraftRevision.load(std::memory_order_acquire);
    snapshot.activeAuthoringPreviewRevision = audioValues.activeAuthoringPreviewRevision;
    snapshot.pendingAuthoringPreviewRevision = audioValues.pendingAuthoringPreviewRevision;
    snapshot.activePublishedRevision = audioValues.activePublishedRevision;
    snapshot.pendingPublishedRevision = audioValues.pendingPublishedRevision;
    snapshot.activePreparedBuildId = audioValues.activePreparedBuildId;
    snapshot.pendingPreparedBuildId = audioValues.pendingPreparedBuildId;
    snapshot.authoringPreviewFailureState =
        failedAuthoringPreviewRevision == snapshot.currentAuthoringPreviewDraftRevision
            ? failedAuthoringPreviewState
            : std::string {};

    if (audioValues.hasPendingAuthoringPreviewActivation
        && snapshot.pendingAuthoringPreviewRevision == snapshot.currentAuthoringPreviewDraftRevision)
        snapshot.authoringPreviewRevisionState = "Preparing";
    else if (!snapshot.authoringPreviewFailureState.empty())
        snapshot.authoringPreviewRevisionState = "Failed";
    else if (audioValues.hasActiveAuthoringPreviewActivation
             && snapshot.activeAuthoringPreviewRevision == snapshot.currentAuthoringPreviewDraftRevision)
        snapshot.authoringPreviewRevisionState = "Ready";
    else if (audioValues.hasActiveAuthoringPreviewActivation)
        snapshot.authoringPreviewRevisionState = "Stale";
    else
        snapshot.authoringPreviewRevisionState = "Idle";

    if (snapshot.getAudioThreadViolationCount() > 0)
        snapshot.state = "Realtime callback violations recorded";
    else if (snapshot.referenceSampleCountLoaded == 0)
        snapshot.state = "Reference playback cache unavailable";
    else if (!audioValues.hasActivePerformanceActivation && audioValues.hasPendingPerformanceActivation)
        snapshot.state = "Published activation pending";
    else if (!audioValues.hasActiveAuthoringPreviewActivation && audioValues.hasPendingAuthoringPreviewActivation)
        snapshot.state = "Authoring preview activation pending";
    else if (!audioValues.hasActivePerformanceActivation)
        snapshot.state = "Published activation unavailable";
    else
        snapshot.state = "Realtime callback primed";
    return snapshot;
}

void Processor::applyRealtimeGuardDiagnostics(ProcessorRealtimeSafetySnapshot& snapshot) const
{
    const auto guard = realtimeGuardState.snapshot();
    snapshot.allocationsOnAudioThread = guard.allocationCount;
    snapshot.deallocationsOnAudioThread = guard.deallocationCount;
    snapshot.blockingLockAttemptsOnAudioThread = guard.blockingLockCount;
    snapshot.waitsOnAudioThread = guard.waitCount;
    snapshot.fileOpenEntriesOnAudioThread = guard.fileOpenCount;
    snapshot.fileReadEntriesOnAudioThread = guard.fileReadCount;
    snapshot.samplePathResolutionsOnAudioThread = guard.pathResolutionCount;
    snapshot.sampleDecodeEntriesOnAudioThread = guard.sampleDecodeCount;
    snapshot.streamDecodeEntriesOnAudioThread = guard.streamDecodeCount;
    snapshot.largeResourceDestructionsOnAudioThread = guard.largeResourceDestructionCount;
    snapshot.finalSharedOwnershipReleasesOnAudioThread = guard.finalSharedOwnershipReleaseCount;
    snapshot.largeResourceReleasesOnAudioThread = guard.largeResourceDestructionCount
        + guard.finalSharedOwnershipReleaseCount;
}

void Processor::publishMessageDiagnostics()
{
    diagnosticsCurrentAuthoringPreviewDraftRevision.store(authoringSession.getDocumentState().revision,
                                                          std::memory_order_release);
    std::uint64_t audioSequence = 0;
    auto audioValues = readAudioDiagnostics(audioSequence);
    const auto activationValues = captureActivationDiagnostics();
    audioValues.activeAuthoringPreviewRevision = activationValues.activeAuthoringPreviewRevision;
    audioValues.pendingAuthoringPreviewRevision = activationValues.pendingAuthoringPreviewRevision;
    audioValues.activePublishedRevision = activationValues.activePublishedRevision;
    audioValues.pendingPublishedRevision = activationValues.pendingPublishedRevision;
    audioValues.activePreparedBuildId = activationValues.activePreparedBuildId;
    audioValues.pendingPreparedBuildId = activationValues.pendingPreparedBuildId;
    audioValues.retiredActivationBacklog = activationValues.retiredActivationBacklog;
    audioValues.activeActivationPayloadBytes = activationValues.activeActivationPayloadBytes;
    audioValues.pendingActivationPayloadBytes = activationValues.pendingActivationPayloadBytes;
    audioValues.retiredActivationPayloadBytes = activationValues.retiredActivationPayloadBytes;
    audioValues.hasActiveAuthoringPreviewActivation = activationValues.hasActiveAuthoringPreviewActivation;
    audioValues.hasPendingAuthoringPreviewActivation = activationValues.hasPendingAuthoringPreviewActivation;
    audioValues.hasActivePerformanceActivation = activationValues.hasActivePerformanceActivation;
    audioValues.hasPendingPerformanceActivation = activationValues.hasPendingPerformanceActivation;
    auto snapshot = std::make_shared<const ProcessorRealtimeSafetySnapshot>(
        composeDiagnosticsSnapshot(audioValues, audioSequence));
    std::atomic_store_explicit(&publishedRealtimeSafetySnapshot, std::move(snapshot), std::memory_order_release);
}

ProcessorRealtimeSafetySnapshot Processor::getRealtimeSafetySnapshot() const
{
    auto published = std::atomic_load_explicit(&publishedRealtimeSafetySnapshot, std::memory_order_acquire);
    auto snapshot = published != nullptr ? *published : ProcessorRealtimeSafetySnapshot {};

    std::uint64_t audioSequence = 0;
    const auto audioValues = readAudioDiagnostics(audioSequence);
    if (audioSequence <= snapshot.publicationSequence)
        return snapshot;

    // Readers may need callback counters before the next message-service tick. Overlay only
    // primitives onto this private value copy; formatted strings remain message-owned.
    snapshot.publicationSequence = audioSequence;
    snapshot.processBlockCount = diagnosticsProcessBlockCount.load(std::memory_order_acquire);
    snapshot.referenceSampleCountLoaded = diagnosticsReferenceSampleCountLoaded.load(std::memory_order_acquire);
    snapshot.referenceWarmupCount = diagnosticsReferenceWarmupCount.load(std::memory_order_acquire);
    snapshot.referenceSampleLoadsOnAudioThread = diagnosticsReferenceSampleLoadsOnAudioThread.load(std::memory_order_acquire);
    snapshot.authoringSampleLoadsOnAudioThread = diagnosticsAuthoringSampleLoadsOnAudioThread.load(std::memory_order_acquire);
    snapshot.performanceActiveVoiceCount = audioValues.performanceActiveVoiceCount;
    snapshot.authoringPreviewActiveVoiceCount = audioValues.authoringPreviewActiveVoiceCount;
    snapshot.activeVoiceCapacity = std::max(audioValues.activeVoiceCapacity,
                                            diagnosticsPrimedActiveVoiceCapacity.load(std::memory_order_acquire));
    snapshot.activeVoiceCapacityGrowthCount = diagnosticsActiveVoiceCapacityGrowthCount.load(std::memory_order_acquire);
    snapshot.authoringPreviewActivationCount = diagnosticsAuthoringPreviewActivationCount.load(std::memory_order_acquire);
    snapshot.performanceActivationCount = diagnosticsPerformanceActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationCount = diagnosticsRetiredActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationBacklog = audioValues.retiredActivationBacklog;
    snapshot.reclaimedActivationPayloadCount = diagnosticsReclaimedActivationPayloadCount.load(std::memory_order_acquire);
    snapshot.activeActivationPayloadBytes = audioValues.activeActivationPayloadBytes;
    snapshot.pendingActivationPayloadBytes = audioValues.pendingActivationPayloadBytes;
    snapshot.retiredActivationPayloadBytes = audioValues.retiredActivationPayloadBytes;
    applyRealtimeGuardDiagnostics(snapshot);
    snapshot.callbackBudgetMicros = diagnosticsCallbackBudgetMicros.load(std::memory_order_acquire);
    snapshot.lastProcessBlockMicros = diagnosticsLastProcessBlockMicros.load(std::memory_order_acquire);
    snapshot.maxProcessBlockMicros = diagnosticsMaxProcessBlockMicros.load(std::memory_order_acquire);
    snapshot.overBudgetCallbackCount = diagnosticsOverBudgetCallbackCount.load(std::memory_order_acquire);
    snapshot.activeAuthoringPreviewRevision = audioValues.activeAuthoringPreviewRevision;
    snapshot.pendingAuthoringPreviewRevision = audioValues.pendingAuthoringPreviewRevision;
    snapshot.activePublishedRevision = audioValues.activePublishedRevision;
    snapshot.pendingPublishedRevision = audioValues.pendingPublishedRevision;
    snapshot.activePreparedBuildId = audioValues.activePreparedBuildId;
    snapshot.pendingPreparedBuildId = audioValues.pendingPreparedBuildId;

    // These strings belong only to the returned copy. Re-evaluate them off audio so the
    // public snapshot remains behaviorally current after a block-boundary activation.
    if (audioValues.hasPendingAuthoringPreviewActivation
        && snapshot.pendingAuthoringPreviewRevision == snapshot.currentAuthoringPreviewDraftRevision)
        snapshot.authoringPreviewRevisionState = "Preparing";
    else if (!snapshot.authoringPreviewFailureState.empty())
        snapshot.authoringPreviewRevisionState = "Failed";
    else if (audioValues.hasActiveAuthoringPreviewActivation
             && snapshot.activeAuthoringPreviewRevision == snapshot.currentAuthoringPreviewDraftRevision)
        snapshot.authoringPreviewRevisionState = "Ready";
    else if (audioValues.hasActiveAuthoringPreviewActivation)
        snapshot.authoringPreviewRevisionState = "Stale";
    else
        snapshot.authoringPreviewRevisionState = "Idle";

    if (snapshot.getAudioThreadViolationCount() > 0)
        snapshot.state = "Realtime callback violations recorded";
    else if (snapshot.referenceSampleCountLoaded == 0)
        snapshot.state = "Reference playback cache unavailable";
    else if (!audioValues.hasActivePerformanceActivation && audioValues.hasPendingPerformanceActivation)
        snapshot.state = "Published activation pending";
    else if (!audioValues.hasActiveAuthoringPreviewActivation && audioValues.hasPendingAuthoringPreviewActivation)
        snapshot.state = "Authoring preview activation pending";
    else if (!audioValues.hasActivePerformanceActivation)
        snapshot.state = "Published activation unavailable";
    else
        snapshot.state = "Realtime callback primed";
    return snapshot;
}
} // namespace drs::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new drs::plugin::Processor();
}
