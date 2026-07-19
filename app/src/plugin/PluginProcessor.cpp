#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeVoice.h"

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

struct RealtimeRenderRoute
{
    const drs::engine::RuntimeZoneDefinition* zone = nullptr;
    const drs::engine::RuntimeStreamSampleDefinition* sample = nullptr;
};

RealtimeRenderRoute resolveRealtimeRenderRoute(
    const drs::engine::RuntimeInstrumentModel& instrument,
    const drs::engine::RuntimeStreamContainerModel& stream,
    const std::string& selectedArticulationId,
    int midiNote,
    int velocity) noexcept
{
    const std::string* articulationId = &selectedArticulationId;
    if (articulationId->empty())
    {
        const auto articulation = std::find_if(instrument.articulations.begin(),
                                               instrument.articulations.end(),
                                               [](const auto& candidate) { return candidate.isDefault; });
        if (articulation == instrument.articulations.end())
            return {};
        articulationId = &articulation->id;
    }

    const drs::engine::RuntimeZoneDefinition* selectedZone = nullptr;
    for (const auto& zone : instrument.zones)
    {
        if (zone.articulationId != *articulationId
            || midiNote < zone.keyLow || midiNote > zone.keyHigh
            || velocity < zone.velocityLow || velocity > zone.velocityHigh)
        {
            continue;
        }

        if (selectedZone == nullptr)
        {
            selectedZone = &zone;
            continue;
        }

        const auto candidateRootDistance = std::abs(zone.rootKey - midiNote);
        const auto selectedRootDistance = std::abs(selectedZone->rootKey - midiNote);
        const auto candidateKeySpan = zone.keyHigh - zone.keyLow;
        const auto selectedKeySpan = selectedZone->keyHigh - selectedZone->keyLow;
        const auto candidateVelocitySpan = zone.velocityHigh - zone.velocityLow;
        const auto selectedVelocitySpan = selectedZone->velocityHigh - selectedZone->velocityLow;
        if (candidateRootDistance < selectedRootDistance
            || (candidateRootDistance == selectedRootDistance && candidateKeySpan < selectedKeySpan)
            || (candidateRootDistance == selectedRootDistance && candidateKeySpan == selectedKeySpan
                && candidateVelocitySpan < selectedVelocitySpan)
            || (candidateRootDistance == selectedRootDistance && candidateKeySpan == selectedKeySpan
                && candidateVelocitySpan == selectedVelocitySpan && zone.id < selectedZone->id))
        {
            selectedZone = &zone;
        }
    }

    if (selectedZone == nullptr)
        return {};

    const auto sample = std::find_if(stream.samples.begin(),
                                     stream.samples.end(),
                                     [&](const auto& candidate)
                                     {
                                         return candidate.sourcePath == selectedZone->samplePath
                                             || candidate.payloadOffsetBytes == selectedZone->streamOffsetBytes;
                                     });
    return sample != stream.samples.end()
        ? RealtimeRenderRoute { selectedZone, &(*sample) }
        : RealtimeRenderRoute {};
}

fs::path resolveSamplePath(const std::string& streamContainerPath, const std::string& samplePath)
{
    const fs::path containerPath(streamContainerPath);
    const fs::path candidate(samplePath);

    if (candidate.is_absolute())
        return candidate.lexically_normal();

    return (containerPath.parent_path() / candidate).lexically_normal();
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
    ensureReferencePlaybackAssetsLoaded(false);
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
}

Processor::~Processor()
{
    for (const auto& macro : engineFacade.getMacroDescriptors())
        parameterState.removeParameterListener(buildMacroParameterId(macro.id), this);

    clearVoices(VoiceSource::performance, false);
    clearVoices(VoiceSource::authoringPreview, false);
    drainRetiredPerformanceActivationSlots();
    drainRetiredAuthoringPreviewActivationSlots();
    delete[] realtimeGuardTestAllocation;
}

void Processor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    performanceSurfaceNoteQueue.reset();
    authoringPreviewNoteQueue.reset();
    clearVoices(VoiceSource::performance, false);
    clearVoices(VoiceSource::authoringPreview, false);
    primeRealtimeSafetyState(samplesPerBlock > 0 ? samplesPerBlock : 512);
    ensureReferencePlaybackAssetsLoaded(false);
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
}

void Processor::releaseResources()
{
    clearVoices(VoiceSource::performance, false);
    clearVoices(VoiceSource::authoringPreview, false);
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

    performanceMidiScratchBuffer.clear();
    performanceMidiScratchBuffer.addEvents(midiMessages, 0, buffer.getNumSamples(), 0);

    authoringPreviewMidiScratchBuffer.clear();

    applyPendingPerformanceActivationAtBlockBoundary();
    applyPendingAuthoringPreviewActivationAtBlockBoundary();
    drainRealtimeNoteEvents(performanceSurfaceNoteQueue, VoiceSource::performance);
    drainRealtimeNoteEvents(authoringPreviewNoteQueue, VoiceSource::authoringPreview);

    if (performanceMidiScratchBuffer.isEmpty()
        && authoringPreviewMidiScratchBuffer.isEmpty()
        && performanceActiveVoices.empty()
        && authoringPreviewActiveVoices.empty())
    {
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
        return;
    }

    int renderedSamples = 0;
    auto performanceIterator = performanceMidiScratchBuffer.begin();
    auto authoringIterator = authoringPreviewMidiScratchBuffer.begin();
    const auto performanceEnd = performanceMidiScratchBuffer.end();
    const auto authoringEnd = authoringPreviewMidiScratchBuffer.end();

    while (performanceIterator != performanceEnd || authoringIterator != authoringEnd)
    {
        const auto performanceSamplePosition = performanceIterator != performanceEnd
            ? (*performanceIterator).samplePosition
            : std::numeric_limits<int>::max();
        const auto authoringSamplePosition = authoringIterator != authoringEnd
            ? (*authoringIterator).samplePosition
            : std::numeric_limits<int>::max();
        const auto useAuthoringMessage = performanceIterator == performanceEnd
            || (authoringIterator != authoringEnd && authoringSamplePosition <= performanceSamplePosition);
        const auto metadata = useAuthoringMessage ? *authoringIterator++ : *performanceIterator++;
        const auto eventSample = std::clamp(metadata.samplePosition, 0, buffer.getNumSamples());
        renderBlockRange(buffer, renderedSamples, eventSample - renderedSamples);

        const auto voiceSource = useAuthoringMessage ? VoiceSource::authoringPreview : VoiceSource::performance;
        const auto* eventData = metadata.data;
        const auto command = metadata.numBytes > 0 ? static_cast<int>(eventData[0] & 0xf0u) : 0;
        const auto noteNumber = metadata.numBytes > 1 ? static_cast<int>(eventData[1] & 0x7fu) : 0;
        const auto velocity = metadata.numBytes > 2 ? static_cast<int>(eventData[2] & 0x7fu) : 0;
        if (command == 0x90 && velocity > 0)
        {
            if (useAuthoringMessage)
                startAuthoringVoiceForMidiMessage(noteNumber, static_cast<float>(velocity) / 127.0f);
            else
                startVoiceForMidiMessage(noteNumber);
        }
        else if (command == 0x80 || (command == 0x90 && velocity == 0))
        {
            releaseVoicesForMidiNote(noteNumber, voiceSource);
        }
        else if (command == 0xb0 && metadata.numBytes > 1
                 && (eventData[1] == 120u || eventData[1] == 123u))
        {
            clearVoices(voiceSource);
        }

        renderedSamples = eventSample;
    }

    renderBlockRange(buffer, renderedSamples, buffer.getNumSamples() - renderedSamples);

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

void Processor::clearReferencePlaybackCacheForTests()
{
    loadedSamples.clear();
    updateRealtimeSafetyState();
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
    drainRetiredAuthoringPreviewActivationSlots();
    drainRetiredPerformanceActivationSlots();

    auto synchronizedActivation = false;
    const auto stateRevision = engineFacade.getStateRevision();
    if (stateRevision != observedEngineStateRevision)
    {
        observedEngineStateRevision = stateRevision;
        synchronizedActivation = synchronizePerformanceActivation(
            activePerformanceActivationSlotIndex.load(std::memory_order_acquire) < 0);
    }

    auto synchronizedAuthoringPreview = false;
    if (authoringRevision != observedAuthoringPreviewRevision
        || activeAuthoringPreviewActivationSlotIndex.load(std::memory_order_acquire) < 0)
    {
        synchronizedAuthoringPreview = synchronizeAuthoringPreviewActivation(
            activeAuthoringPreviewActivationSlotIndex.load(std::memory_order_acquire) < 0);
        observedAuthoringPreviewRevision = authoringRevision;
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

bool Processor::ensureReferencePlaybackAssetsLoaded(bool invokedFromAudioThread)
{
    if (referenceManifest.loaded && referenceStream.loaded && !loadedSamples.empty())
        return true;

    if (!referenceManifest.loaded)
        referenceManifest = engineFacade.loadPhase1ReferenceInstrument();

    if (!referenceStream.loaded)
    {
        if (invokedFromAudioThread)
            recordRealtimeGuardOperation(RealtimeGuardOperation::streamDecode);
        referenceStream = engineFacade.loadPhase1ReferenceStream();
    }

    initializeReferencePlaybackAssets(invokedFromAudioThread);
    updateRealtimeSafetyState();
    return referenceManifest.loaded && referenceStream.loaded && !loadedSamples.empty();
}

void Processor::initializeReferencePlaybackAssets(bool invokedFromAudioThread)
{
    // Sprint 3 boundary note: this reference fallback path is compatibility scaffolding, not the intended playback
    // preparation service. Preview/Publish preparation belongs behind PreparedPlaybackService, and audio-thread
    // fallback here is tracked only so regressions stay visible until the reference-backed renderer is retired.
    if (invokedFromAudioThread && !loadedSamples.empty())
        recordRealtimeGuardOperation(RealtimeGuardOperation::largeResourceDestruction);

    loadedSamples.clear();

    if (!referenceManifest.loaded || !referenceStream.loaded)
    {
        diagnosticsReferenceSampleCountLoaded.store(0, std::memory_order_release);
        return;
    }

    loadedSamples.reserve(referenceStream.container.samples.size());

    for (const auto& sample : referenceStream.container.samples)
    {
        if (invokedFromAudioThread)
            recordRealtimeGuardOperation(RealtimeGuardOperation::pathResolution);
        const auto samplePath = resolveSamplePath(referenceStream.containerPath, sample.sourcePath);
        if (invokedFromAudioThread)
        {
            recordRealtimeGuardOperation(RealtimeGuardOperation::fileOpen);
            recordRealtimeGuardOperation(RealtimeGuardOperation::fileRead);
            recordRealtimeGuardOperation(RealtimeGuardOperation::sampleDecode);
        }
        const auto importResult = drs::engine::importSampleFile(samplePath.generic_string());
        if (!importResult.imported)
            continue;

        loadedSamples.emplace(sample.sampleId, LoadedReferenceSample { importResult.sample });
    }

    diagnosticsReferenceSampleCountLoaded.store(loadedSamples.size(), std::memory_order_release);
    if (!loadedSamples.empty())
    {
        if (invokedFromAudioThread)
            diagnosticsReferenceSampleLoadsOnAudioThread.fetch_add(loadedSamples.size(), std::memory_order_relaxed);
        else
            diagnosticsReferenceWarmupCount.fetch_add(1, std::memory_order_relaxed);
    }
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

    authoringLoadedSamples.emplace(projectSampleSource->id, LoadedReferenceSample { importResult.sample });
    lastAuthoringSampleLoadFailureState.clear();
    if (invokedFromAudioThread)
        diagnosticsAuthoringSampleLoadsOnAudioThread.fetch_add(1, std::memory_order_relaxed);
    updateRealtimeSafetyState();
    return true;
}

std::vector<Processor::ActiveRenderVoice>& Processor::getVoicePool(VoiceSource source)
{
    return source == VoiceSource::performance ? performanceActiveVoices : authoringPreviewActiveVoices;
}

const std::vector<Processor::ActiveRenderVoice>& Processor::getVoicePool(VoiceSource source) const
{
    return source == VoiceSource::performance ? performanceActiveVoices : authoringPreviewActiveVoices;
}

bool Processor::startAuthoringVoiceForMidiMessage(int midiNoteNumber, float velocity)
{
    const auto* activation = getActiveAuthoringPreviewActivation();
    if (activation == nullptr || !activation->ready || !activation->preparedSample)
        return false;

    const auto& sample = activation->preparedSample->sample;
    if (sample.normalizedChannels.empty() || sample.metadata.sampleRate <= 0.0)
        return false;

    ActiveRenderVoice voice;
    voice.renderVoiceId = nextRenderVoiceId++;
    voice.sourceMidiNote = midiNoteNumber;
    voice.effectiveMidiNote = midiNoteNumber;
    voice.effectiveVelocity = std::clamp(static_cast<int>(std::round(velocity * 127.0f)), 1, 127);
    voice.rootKey = activation->rootKey;
    voice.source = VoiceSource::authoringPreview;
    voice.retainedLoadedSample = activation->preparedSample;
    voice.loadedSample = voice.retainedLoadedSample.get();
    voice.incrementFrames = std::pow(2.0, (voice.effectiveMidiNote - activation->rootKey) / 12.0)
        * (sample.metadata.sampleRate / std::max(currentSampleRate, 1.0));
    voice.baseGain = 0.25f * (static_cast<float>(voice.effectiveVelocity) / 127.0f)
        * static_cast<float>(std::pow(10.0, activation->gainDb / 20.0));

    auto& activeVoices = authoringPreviewActiveVoices;
    if (activeVoices.size() >= maxRealtimeActiveVoices)
    {
        activeVoices.erase(activeVoices.begin());
    }
    else if (activeVoices.size() >= activeVoices.capacity())
        diagnosticsActiveVoiceCapacityGrowthCount.fetch_add(1, std::memory_order_relaxed);
    activeVoices.push_back(std::move(voice));
    return true;
}

void Processor::startVoiceForMidiMessage(int midiNoteNumber)
{
    const auto* activation = getActivePerformanceActivation();
    if (activation == nullptr || !activation->ready)
        return;

    if (!ensureReferencePlaybackAssetsLoaded(true))
        return;

    const auto& sessionState = activation->sessionState;
    const auto effectiveMidiNote = computeMotionRenderNote(sessionState, midiNoteNumber);
    const auto effectiveVelocity = computeToneRenderVelocity(sessionState);

    const auto route = resolveRealtimeRenderRoute(referenceManifest.instrument,
                                                  referenceStream.container,
                                                  sessionState.selectedArticulationId,
                                                  effectiveMidiNote,
                                                  effectiveVelocity);
    if (route.zone == nullptr || route.sample == nullptr)
        return;

    const auto sampleIterator = loadedSamples.find(route.sample->sampleId);
    if (sampleIterator == loadedSamples.end())
        return;

    const auto& sample = sampleIterator->second.sample;
    if (sample.normalizedChannels.empty() || sample.metadata.sampleRate <= 0.0)
        return;

    ActiveRenderVoice voice;
    voice.renderVoiceId = nextRenderVoiceId++;
    voice.sourceMidiNote = midiNoteNumber;
    voice.effectiveMidiNote = effectiveMidiNote;
    voice.effectiveVelocity = effectiveVelocity;
    voice.rootKey = route.zone->rootKey;
    voice.source = VoiceSource::performance;
    voice.loadedSample = &sampleIterator->second;
    voice.incrementFrames = std::pow(2.0, (effectiveMidiNote - route.zone->rootKey) / 12.0)
        * (sample.metadata.sampleRate / std::max(currentSampleRate, 1.0));
    voice.baseGain = 0.25f * (static_cast<float>(effectiveVelocity) / 127.0f);

    const auto retainedActivationSlotIndex = activePerformanceActivationSlotIndex.load(std::memory_order_acquire);
    if (retainedActivationSlotIndex >= 0)
    {
        performanceActivationVoiceLeaseCounts[static_cast<std::size_t>(retainedActivationSlotIndex)]
            .fetch_add(1, std::memory_order_acq_rel);
        voice.retainedPerformanceActivationSlotIndex = retainedActivationSlotIndex;
    }

    auto& activeVoices = performanceActiveVoices;
    if (activeVoices.size() >= maxRealtimeActiveVoices)
    {
        releasePerformanceActivationLease(activeVoices.front());
        activeVoices.erase(activeVoices.begin());
    }
    else if (activeVoices.size() >= activeVoices.capacity())
        diagnosticsActiveVoiceCapacityGrowthCount.fetch_add(1, std::memory_order_relaxed);
    activeVoices.push_back(std::move(voice));
}

void Processor::releaseVoicesForMidiNote(int midiNoteNumber, VoiceSource source)
{
    for (auto& voice : getVoicePool(source))
    {
        if (voice.source != source || voice.sourceMidiNote != midiNoteNumber || voice.releasing)
            continue;

        voice.releasing = true;
        voice.releaseSamplesTotal = 2048;
        voice.releaseSamplesRemaining = voice.releaseSamplesTotal;
    }

    updateRealtimeSafetyState();
}

void Processor::clearVoices(VoiceSource source)
{
    clearVoices(source, true);
}

void Processor::clearVoices(VoiceSource source, bool updateState)
{
    auto& activeVoices = getVoicePool(source);
    if (source == VoiceSource::performance)
    {
        for (auto& voice : activeVoices)
            if (voice.source == source)
                releasePerformanceActivationLease(voice);
    }
    activeVoices.erase(std::remove_if(activeVoices.begin(),
                                      activeVoices.end(),
                                      [source](const ActiveRenderVoice& voice)
                                      {
                                          return voice.source == source;
                                      }),
                       activeVoices.end());
    if (updateState)
        updateRealtimeSafetyState();
}

void Processor::releasePerformanceActivationLease(ActiveRenderVoice& voice)
{
    if (voice.retainedPerformanceActivationSlotIndex < 0)
        return;

    performanceActivationVoiceLeaseCounts[
        static_cast<std::size_t>(voice.retainedPerformanceActivationSlotIndex)]
        .fetch_sub(1, std::memory_order_acq_rel);
    voice.retainedPerformanceActivationSlotIndex = -1;
}

bool Processor::synchronizeAuthoringPreviewActivation(bool installImmediately)
{
    return stageAuthoringPreviewActivation(installImmediately);
}

bool Processor::stageAuthoringPreviewActivation(bool installImmediately)
{
    drainRetiredAuthoringPreviewActivationSlots();
    const auto currentRevision = authoringSession.getDocumentState().revision;
    const auto failPreviewActivation = [&](const std::string& state)
    {
        failedAuthoringPreviewRevision = currentRevision;
        failedAuthoringPreviewState = state.empty() ? "Authoring preview preparation failed." : state;
        return false;
    };

    const auto supersededPendingSlot = pendingAuthoringPreviewActivationSlotIndex.exchange(-1, std::memory_order_acq_rel);
    if (supersededPendingSlot >= 0)
        releaseAuthoringPreviewActivationSlot(supersededPendingSlot);

    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
    {
        failedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
        failedAuthoringPreviewState.clear();
        return false;
    }

    const auto projectSampleSource = findProjectSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (!projectSampleSource.has_value())
        return failPreviewActivation("Selected zone sample source is missing from the project.");

    if (!ensureSelectedAuthoringSampleLoaded(false))
        return failPreviewActivation(lastAuthoringSampleLoadFailureState);

    const auto sampleIterator = authoringLoadedSamples.find(projectSampleSource->id);
    if (sampleIterator == authoringLoadedSamples.end())
        return failPreviewActivation("Selected authoring sample was not cached after preparation.");

    const auto slotIndex = acquireAuthoringPreviewActivationSlot();
    if (slotIndex < 0)
        return failPreviewActivation("Authoring preview activation slots are exhausted.");

    auto& activation = authoringPreviewActivationSlots[static_cast<std::size_t>(slotIndex)];
    activation = {};
    activation.ready = true;
    activation.activationSerial = nextAuthoringPreviewActivationSerial++;
    activation.projectRevision = currentRevision;
    activation.zoneId = selectedZone->id;
    activation.sampleId = projectSampleSource->id;
    activation.rootKey = selectedZone->rootKey;
    activation.gainDb = selectedZone->gainDb;
    activation.preparedSample = std::make_shared<LoadedReferenceSample>(sampleIterator->second);
    activation.payload = engineFacade.getPreviewActivationPayload();
    if (activation.payload != nullptr && activation.payload->revision != currentRevision)
        activation.payload.reset();
    activation.payloadRetainedBytes = activation.payload != nullptr
        ? activation.payload->retainedPreparedBytes
        : 0;
    authoringPreviewDiagnosticRevisions[static_cast<std::size_t>(slotIndex)]
        .store(activation.projectRevision, std::memory_order_release);
    authoringPreviewDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)]
        .store(activation.payloadRetainedBytes, std::memory_order_release);
    failedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
    failedAuthoringPreviewState.clear();

    if (installImmediately && activeAuthoringPreviewActivationSlotIndex.load(std::memory_order_acquire) < 0)
    {
        activeAuthoringPreviewActivationSlotIndex.store(slotIndex, std::memory_order_release);
        diagnosticsAuthoringPreviewActivationCount.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    pendingAuthoringPreviewActivationSlotIndex.store(slotIndex, std::memory_order_release);
    return true;
}

const Processor::AuthoringPreviewRenderActivation* Processor::applyPendingAuthoringPreviewActivationAtBlockBoundary()
{
    const auto pendingSlotIndex = pendingAuthoringPreviewActivationSlotIndex.exchange(-1, std::memory_order_acq_rel);
    if (pendingSlotIndex >= 0)
    {
        clearVoices(VoiceSource::authoringPreview, false);
        const auto retiredSlotIndex = activeAuthoringPreviewActivationSlotIndex.exchange(
            pendingSlotIndex,
            std::memory_order_acq_rel);
        diagnosticsAuthoringPreviewActivationCount.fetch_add(1, std::memory_order_relaxed);

        if (retiredSlotIndex >= 0 && retiredSlotIndex != pendingSlotIndex)
            enqueueRetiredAuthoringPreviewActivationSlot(retiredSlotIndex);
    }

    return getActiveAuthoringPreviewActivation();
}

const Processor::AuthoringPreviewRenderActivation* Processor::getActiveAuthoringPreviewActivation() const
{
    const auto slotIndex = activeAuthoringPreviewActivationSlotIndex.load(std::memory_order_acquire);
    if (slotIndex < 0)
        return nullptr;

    return &authoringPreviewActivationSlots[static_cast<std::size_t>(slotIndex)];
}

bool Processor::enqueueRetiredAuthoringPreviewActivationSlot(int slotIndex)
{
    auto writeIndex = retiredAuthoringPreviewActivationWriteIndex.load(std::memory_order_relaxed);
    const auto readIndex = retiredAuthoringPreviewActivationReadIndex.load(std::memory_order_acquire);
    const auto nextWriteIndex = (writeIndex + 1u) % static_cast<std::uint32_t>(retiredActivationQueueCapacity);
    if (nextWriteIndex == readIndex)
        return false;

    retiredAuthoringPreviewActivationSlots[writeIndex] = slotIndex;
    queuedAuthoringPreviewRetirementBytes.fetch_add(
        authoringPreviewDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)].load(std::memory_order_acquire),
        std::memory_order_relaxed);
    retiredAuthoringPreviewActivationWriteIndex.store(nextWriteIndex, std::memory_order_release);
    return true;
}

void Processor::drainRetiredAuthoringPreviewActivationSlots()
{
    auto readIndex = retiredAuthoringPreviewActivationReadIndex.load(std::memory_order_relaxed);
    const auto writeIndex = retiredAuthoringPreviewActivationWriteIndex.load(std::memory_order_acquire);
    while (readIndex != writeIndex)
    {
        const auto slotIndex = retiredAuthoringPreviewActivationSlots[readIndex];
        queuedAuthoringPreviewRetirementBytes.fetch_sub(
            authoringPreviewDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)].load(std::memory_order_acquire),
            std::memory_order_relaxed);
        if (authoringPreviewActivationSlots[static_cast<std::size_t>(slotIndex)].payload != nullptr)
            diagnosticsReclaimedActivationPayloadCount.fetch_add(1, std::memory_order_relaxed);
        releaseAuthoringPreviewActivationSlot(slotIndex);
        readIndex = (readIndex + 1u) % static_cast<std::uint32_t>(retiredActivationQueueCapacity);
    }

    retiredAuthoringPreviewActivationReadIndex.store(readIndex, std::memory_order_release);
}

int Processor::acquireAuthoringPreviewActivationSlot()
{
    if (freeAuthoringPreviewActivationSlotCount == 0)
        return -1;

    return freeAuthoringPreviewActivationSlots[--freeAuthoringPreviewActivationSlotCount];
}

void Processor::releaseAuthoringPreviewActivationSlot(int slotIndex)
{
    if (slotIndex < 0)
        return;

    if (isCurrentThreadRealtimeAudio())
        recordRealtimeGuardOperation(RealtimeGuardOperation::finalSharedOwnershipRelease);

    authoringPreviewActivationSlots[static_cast<std::size_t>(slotIndex)] = {};
    authoringPreviewDiagnosticRevisions[static_cast<std::size_t>(slotIndex)].store(0, std::memory_order_release);
    authoringPreviewDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)].store(0, std::memory_order_release);
    if (freeAuthoringPreviewActivationSlotCount < freeAuthoringPreviewActivationSlots.size())
        freeAuthoringPreviewActivationSlots[freeAuthoringPreviewActivationSlotCount++] = slotIndex;
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
    drainRetiredPerformanceActivationSlots();

    const auto supersededPendingSlot = pendingPerformanceActivationSlotIndex.exchange(-1, std::memory_order_acq_rel);
    if (supersededPendingSlot >= 0)
        releasePerformanceActivationSlot(supersededPendingSlot);

    const auto slotIndex = acquirePerformanceActivationSlot();
    if (slotIndex < 0)
        return false;

    const auto manifestLoaded = engineFacade.loadPhase1ReferenceInstrument().loaded;
    const auto streamLoaded = engineFacade.loadPhase1ReferenceStream().loaded;
    const auto fallbackReady = manifestLoaded && streamLoaded;
    const auto publishedReady = performanceSnapshot.loaded;
    auto& activation = performanceActivationSlots[static_cast<std::size_t>(slotIndex)];
    activation.ready = publishedReady || fallbackReady;
    activation.activationSerial = nextPerformanceActivationSerial++;
    activation.publishedRevision = publishedReady
        ? performanceSnapshot.publishedRevision
        : performanceSnapshot.previewRevision;
    activation.publishedBuildId = publishedReady
        ? performanceSnapshot.publishedBuildId
        : performanceSnapshot.previewBuildId;
    activation.preparedBuildId = publishedReady
        ? performanceSnapshot.publishedPreparedBuildId
        : performanceSnapshot.previewPreparedBuildId;
    activation.publishedContentDigest = publishedReady
        ? performanceSnapshot.publishedContentDigest
        : performanceSnapshot.previewContentDigest;
    activation.preparedContentDigest = publishedReady
        ? performanceSnapshot.publishedPreparedContentDigest
        : performanceSnapshot.previewPreparedContentDigest;
    activation.payload = publishedReady
        ? engineFacade.getPerformanceActivationPayload()
        : engineFacade.getPreviewActivationPayload();
    activation.payloadRetainedBytes = activation.payload != nullptr
        ? activation.payload->retainedPreparedBytes
        : 0;
    activation.sessionState = sessionState;
    performanceDiagnosticRevisions[static_cast<std::size_t>(slotIndex)]
        .store(activation.publishedRevision, std::memory_order_release);
    performanceDiagnosticPreparedBuildIds[static_cast<std::size_t>(slotIndex)]
        .store(activation.preparedBuildId, std::memory_order_release);
    performanceDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)]
        .store(activation.payloadRetainedBytes, std::memory_order_release);

    if (installImmediately && activePerformanceActivationSlotIndex.load(std::memory_order_acquire) < 0)
    {
        activePerformanceActivationSlotIndex.store(slotIndex, std::memory_order_release);
        diagnosticsPerformanceActivationCount.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    pendingPerformanceActivationSlotIndex.store(slotIndex, std::memory_order_release);
    return true;
}

const Processor::PerformanceRenderActivation* Processor::applyPendingPerformanceActivationAtBlockBoundary()
{
    const auto pendingSlotIndex = pendingPerformanceActivationSlotIndex.exchange(-1, std::memory_order_acq_rel);
    if (pendingSlotIndex >= 0)
    {
        const auto retiredSlotIndex = activePerformanceActivationSlotIndex.exchange(
            pendingSlotIndex,
            std::memory_order_acq_rel);
        diagnosticsPerformanceActivationCount.fetch_add(1, std::memory_order_relaxed);

        if (retiredSlotIndex >= 0 && retiredSlotIndex != pendingSlotIndex)
            enqueueRetiredPerformanceActivationSlot(retiredSlotIndex);
    }

    return getActivePerformanceActivation();
}

const Processor::PerformanceRenderActivation* Processor::getActivePerformanceActivation() const
{
    const auto slotIndex = activePerformanceActivationSlotIndex.load(std::memory_order_acquire);
    if (slotIndex < 0)
        return nullptr;

    return &performanceActivationSlots[static_cast<std::size_t>(slotIndex)];
}

bool Processor::enqueueRetiredPerformanceActivationSlot(int slotIndex)
{
    auto writeIndex = retiredPerformanceActivationWriteIndex.load(std::memory_order_relaxed);
    const auto readIndex = retiredPerformanceActivationReadIndex.load(std::memory_order_acquire);
    const auto nextWriteIndex = (writeIndex + 1u) % static_cast<std::uint32_t>(retiredActivationQueueCapacity);
    if (nextWriteIndex == readIndex)
        return false;

    retiredPerformanceActivationSlots[writeIndex] = slotIndex;
    queuedPerformanceRetirementBytes.fetch_add(
        performanceDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)].load(std::memory_order_acquire),
        std::memory_order_relaxed);
    retiredPerformanceActivationWriteIndex.store(nextWriteIndex, std::memory_order_release);
    return true;
}

void Processor::drainRetiredPerformanceActivationSlots()
{
    std::size_t retainedDeferredCount = 0;
    std::uint64_t retainedDeferredBytes = 0;
    for (std::size_t index = 0; index < deferredPerformanceRetirementSlotCount; ++index)
    {
        const auto slotIndex = deferredPerformanceRetirementSlots[index];
        if (performanceActivationVoiceLeaseCounts[static_cast<std::size_t>(slotIndex)]
                .load(std::memory_order_acquire) != 0)
        {
            deferredPerformanceRetirementSlots[retainedDeferredCount++] = slotIndex;
            retainedDeferredBytes += performanceDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)]
                .load(std::memory_order_acquire);
            continue;
        }

        if (performanceActivationSlots[static_cast<std::size_t>(slotIndex)].payload != nullptr)
            diagnosticsReclaimedActivationPayloadCount.fetch_add(1, std::memory_order_relaxed);
        releasePerformanceActivationSlot(slotIndex);
        diagnosticsRetiredActivationCount.fetch_add(1, std::memory_order_relaxed);
    }
    deferredPerformanceRetirementSlotCount = retainedDeferredCount;

    auto readIndex = retiredPerformanceActivationReadIndex.load(std::memory_order_relaxed);
    const auto writeIndex = retiredPerformanceActivationWriteIndex.load(std::memory_order_acquire);
    while (readIndex != writeIndex)
    {
        const auto slotIndex = retiredPerformanceActivationSlots[readIndex];
        queuedPerformanceRetirementBytes.fetch_sub(
            performanceDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)].load(std::memory_order_acquire),
            std::memory_order_relaxed);
        if (performanceActivationVoiceLeaseCounts[static_cast<std::size_t>(slotIndex)]
                .load(std::memory_order_acquire) != 0)
        {
            if (deferredPerformanceRetirementSlotCount < deferredPerformanceRetirementSlots.size())
            {
                deferredPerformanceRetirementSlots[deferredPerformanceRetirementSlotCount++] = slotIndex;
                retainedDeferredBytes += performanceDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)]
                    .load(std::memory_order_acquire);
            }
        }
        else
        {
            if (performanceActivationSlots[static_cast<std::size_t>(slotIndex)].payload != nullptr)
                diagnosticsReclaimedActivationPayloadCount.fetch_add(1, std::memory_order_relaxed);
            releasePerformanceActivationSlot(slotIndex);
            diagnosticsRetiredActivationCount.fetch_add(1, std::memory_order_relaxed);
        }
        readIndex = (readIndex + 1u) % static_cast<std::uint32_t>(retiredActivationQueueCapacity);
    }

    retiredPerformanceActivationReadIndex.store(readIndex, std::memory_order_release);
    deferredPerformanceRetirementBacklog.store(deferredPerformanceRetirementSlotCount, std::memory_order_release);
    deferredPerformanceRetirementBytes.store(retainedDeferredBytes, std::memory_order_release);
}

int Processor::acquirePerformanceActivationSlot()
{
    if (freePerformanceActivationSlotCount == 0)
        return -1;

    return freePerformanceActivationSlots[--freePerformanceActivationSlotCount];
}

void Processor::releasePerformanceActivationSlot(int slotIndex)
{
    if (slotIndex < 0)
        return;

    if (isCurrentThreadRealtimeAudio())
        recordRealtimeGuardOperation(RealtimeGuardOperation::finalSharedOwnershipRelease);

    if (performanceActivationVoiceLeaseCounts[static_cast<std::size_t>(slotIndex)]
            .load(std::memory_order_acquire) != 0)
    {
        return;
    }

    performanceActivationSlots[static_cast<std::size_t>(slotIndex)] = {};
    performanceDiagnosticRevisions[static_cast<std::size_t>(slotIndex)].store(0, std::memory_order_release);
    performanceDiagnosticPreparedBuildIds[static_cast<std::size_t>(slotIndex)].store(0, std::memory_order_release);
    performanceDiagnosticPayloadBytes[static_cast<std::size_t>(slotIndex)].store(0, std::memory_order_release);
    if (freePerformanceActivationSlotCount < freePerformanceActivationSlots.size())
        freePerformanceActivationSlots[freePerformanceActivationSlotCount++] = slotIndex;
}

void Processor::renderBlockRange(juce::AudioBuffer<float>& buffer, int startSample, int sampleCount)
{
    if (sampleCount <= 0)
        return;

    auto* left = buffer.getNumChannels() > 0 ? buffer.getWritePointer(0, startSample) : nullptr;
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1, startSample) : nullptr;
    auto renderVoicePool = [&](std::vector<ActiveRenderVoice>& activeVoices, float& mixedLeft, float& mixedRight)
    {
        for (auto& voice : activeVoices)
        {
            if (voice.loadedSample == nullptr)
                continue;

            const auto& sample = voice.loadedSample->sample;
            const auto frameCount = static_cast<double>(sample.metadata.frameCount);
            if (frameCount <= 1.0 || voice.positionFrames >= frameCount)
            {
                voice.releaseSamplesRemaining = 0;
                voice.positionFrames = frameCount;
                continue;
            }

            const auto frameIndex = static_cast<std::size_t>(voice.positionFrames);
            const auto nextFrameIndex = std::min(frameIndex + 1,
                                                 static_cast<std::size_t>(sample.metadata.frameCount - 1));
            const auto fraction = static_cast<float>(voice.positionFrames - static_cast<double>(frameIndex));
            const auto sampleChannelCount = sample.normalizedChannels.size();

            const auto readInterpolated = [&](std::size_t channelIndex)
            {
                const auto resolvedChannel = std::min(channelIndex, sampleChannelCount - 1);
                const auto& channel = sample.normalizedChannels[resolvedChannel];
                const auto current = channel[frameIndex];
                const auto next = channel[nextFrameIndex];
                return current + (next - current) * fraction;
            };

            float envelope = 1.0f;
            if (voice.releasing)
            {
                envelope = voice.releaseSamplesTotal > 0
                    ? static_cast<float>(voice.releaseSamplesRemaining) / static_cast<float>(voice.releaseSamplesTotal)
                    : 0.0f;
            }

            const auto voiceGain = voice.baseGain * envelope;
            const auto sampleLeft = readInterpolated(0) * voiceGain;
            const auto sampleRight = sampleChannelCount > 1 ? readInterpolated(1) * voiceGain : sampleLeft;
            mixedLeft += sampleLeft;
            mixedRight += sampleRight;

            voice.positionFrames += voice.incrementFrames;

            if (voice.releasing && voice.releaseSamplesRemaining > 0)
                --voice.releaseSamplesRemaining;
        }
    };

    auto trimFinishedVoices = [this](std::vector<ActiveRenderVoice>& activeVoices)
    {
        for (auto iterator = activeVoices.begin(); iterator != activeVoices.end();)
        {
            const auto missingSample = iterator->loadedSample == nullptr;
            const auto finishedRelease = iterator->releasing && iterator->releaseSamplesRemaining <= 0;
            const auto frameCount = missingSample
                ? 0.0
                : static_cast<double>(iterator->loadedSample->sample.metadata.frameCount);
            if (missingSample || finishedRelease || iterator->positionFrames >= frameCount)
            {
                releasePerformanceActivationLease(*iterator);
                iterator = activeVoices.erase(iterator);
            }
            else
            {
                ++iterator;
            }
        }
    };

    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        float mixedLeft = 0.0f;
        float mixedRight = 0.0f;

        renderVoicePool(performanceActiveVoices, mixedLeft, mixedRight);
        renderVoicePool(authoringPreviewActiveVoices, mixedLeft, mixedRight);

        if (left != nullptr)
            left[sampleIndex] += mixedLeft;
        if (right != nullptr)
            right[sampleIndex] += mixedRight;
    }

    trimFinishedVoices(performanceActiveVoices);
    trimFinishedVoices(authoringPreviewActiveVoices);
    updateRealtimeSafetyState();
}

void Processor::drainRealtimeNoteEvents(RealtimeNoteEventQueue& queue, VoiceSource source)
{
    QueuedRealtimeNoteEvent event;
    while (queue.pop(event))
    {
        if (!event.noteOn)
        {
            releaseVoicesForMidiNote(event.midiNoteNumber, source);
            continue;
        }

        if (source == VoiceSource::authoringPreview)
            startAuthoringVoiceForMidiMessage(event.midiNoteNumber, event.velocity);
        else
            startVoiceForMidiMessage(event.midiNoteNumber);
    }
}

void Processor::primeRealtimeSafetyState(int samplesPerBlock)
{
    performanceActiveVoices.reserve(maxRealtimeActiveVoices);
    authoringPreviewActiveVoices.reserve(maxRealtimeActiveVoices);
    performanceMidiScratchBuffer.ensureSize(std::max<std::size_t>(1024, static_cast<std::size_t>(samplesPerBlock) * 16));
    authoringPreviewMidiScratchBuffer.ensureSize(std::max<std::size_t>(1024, static_cast<std::size_t>(samplesPerBlock) * 16));
    diagnosticsPreparedBlockSize.store(static_cast<std::size_t>(samplesPerBlock), std::memory_order_release);
    diagnosticsActiveVoiceCapacityLimit.store(maxRealtimeActiveVoices * 2, std::memory_order_release);
    diagnosticsPrimedActiveVoiceCapacity.store(performanceActiveVoices.capacity()
                                                    + authoringPreviewActiveVoices.capacity(),
                                                std::memory_order_release);
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
    const auto activeAuthoringPreviewSlotIndex = activeAuthoringPreviewActivationSlotIndex.load(std::memory_order_acquire);
    const auto pendingAuthoringPreviewSlotIndex = pendingAuthoringPreviewActivationSlotIndex.load(std::memory_order_acquire);
    const auto activeSlotIndex = activePerformanceActivationSlotIndex.load(std::memory_order_acquire);
    const auto pendingSlotIndex = pendingPerformanceActivationSlotIndex.load(std::memory_order_acquire);
    values.hasActiveAuthoringPreviewActivation = activeAuthoringPreviewSlotIndex >= 0;
    values.hasPendingAuthoringPreviewActivation = pendingAuthoringPreviewSlotIndex >= 0;
    values.hasActivePerformanceActivation = activeSlotIndex >= 0;
    values.hasPendingPerformanceActivation = pendingSlotIndex >= 0;
    values.activeAuthoringPreviewRevision = activeAuthoringPreviewSlotIndex >= 0
        ? authoringPreviewDiagnosticRevisions[static_cast<std::size_t>(activeAuthoringPreviewSlotIndex)]
              .load(std::memory_order_acquire)
        : 0;
    values.pendingAuthoringPreviewRevision = pendingAuthoringPreviewSlotIndex >= 0
        ? authoringPreviewDiagnosticRevisions[static_cast<std::size_t>(pendingAuthoringPreviewSlotIndex)]
              .load(std::memory_order_acquire)
        : 0;
    values.activePublishedRevision = activeSlotIndex >= 0
        ? performanceDiagnosticRevisions[static_cast<std::size_t>(activeSlotIndex)].load(std::memory_order_acquire)
        : 0;
    values.pendingPublishedRevision = pendingSlotIndex >= 0
        ? performanceDiagnosticRevisions[static_cast<std::size_t>(pendingSlotIndex)].load(std::memory_order_acquire)
        : 0;
    values.activePreparedBuildId = activeSlotIndex >= 0
        ? performanceDiagnosticPreparedBuildIds[static_cast<std::size_t>(activeSlotIndex)]
              .load(std::memory_order_acquire)
        : 0;
    values.pendingPreparedBuildId = pendingSlotIndex >= 0
        ? performanceDiagnosticPreparedBuildIds[static_cast<std::size_t>(pendingSlotIndex)]
              .load(std::memory_order_acquire)
        : 0;
    const auto retiredWriteIndex = retiredPerformanceActivationWriteIndex.load(std::memory_order_acquire);
    const auto retiredReadIndex = retiredPerformanceActivationReadIndex.load(std::memory_order_acquire);
    const auto retiredPreviewWriteIndex = retiredAuthoringPreviewActivationWriteIndex.load(std::memory_order_acquire);
    const auto retiredPreviewReadIndex = retiredAuthoringPreviewActivationReadIndex.load(std::memory_order_acquire);
    const auto retiredPerformanceBacklog =
        retiredWriteIndex >= retiredReadIndex
            ? static_cast<std::size_t>(retiredWriteIndex - retiredReadIndex)
            : static_cast<std::size_t>(retiredActivationQueueCapacity - (retiredReadIndex - retiredWriteIndex));
    const auto retiredPreviewBacklog =
        retiredPreviewWriteIndex >= retiredPreviewReadIndex
            ? static_cast<std::size_t>(retiredPreviewWriteIndex - retiredPreviewReadIndex)
            : static_cast<std::size_t>(retiredActivationQueueCapacity - (retiredPreviewReadIndex - retiredPreviewWriteIndex));
    values.retiredActivationBacklog = retiredPerformanceBacklog + retiredPreviewBacklog
        + deferredPerformanceRetirementBacklog.load(std::memory_order_acquire);
    values.retiredActivationPayloadBytes = deferredPerformanceRetirementBytes.load(std::memory_order_acquire)
        + queuedAuthoringPreviewRetirementBytes.load(std::memory_order_acquire)
        + queuedPerformanceRetirementBytes.load(std::memory_order_acquire);

    if (activeSlotIndex >= 0)
        values.activeActivationPayloadBytes +=
            performanceDiagnosticPayloadBytes[static_cast<std::size_t>(activeSlotIndex)].load(std::memory_order_acquire);
    if (activeAuthoringPreviewSlotIndex >= 0)
        values.activeActivationPayloadBytes +=
            authoringPreviewDiagnosticPayloadBytes[static_cast<std::size_t>(activeAuthoringPreviewSlotIndex)]
                .load(std::memory_order_acquire);
    if (pendingSlotIndex >= 0)
        values.pendingActivationPayloadBytes +=
            performanceDiagnosticPayloadBytes[static_cast<std::size_t>(pendingSlotIndex)].load(std::memory_order_acquire);
    if (pendingAuthoringPreviewSlotIndex >= 0)
        values.pendingActivationPayloadBytes +=
            authoringPreviewDiagnosticPayloadBytes[static_cast<std::size_t>(pendingAuthoringPreviewSlotIndex)]
                .load(std::memory_order_acquire);

    return values;
}

void Processor::publishAudioDiagnostics()
{
    auto values = captureActivationDiagnostics();
    values.performanceActiveVoiceCount = performanceActiveVoices.size();
    values.authoringPreviewActiveVoiceCount = authoringPreviewActiveVoices.size();
    values.activeVoiceCapacity = performanceActiveVoices.capacity() + authoringPreviewActiveVoices.capacity();

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
    diagnosticsReferenceSampleCountLoaded.store(loadedSamples.size(), std::memory_order_release);
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
