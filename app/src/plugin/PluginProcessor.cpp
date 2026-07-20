#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

#include "drs/engine/AuthoringPreviewPreparation.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>

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

std::uint64_t monotonicMicros()
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

template <typename Finding>
drs::engine::AuthoringPreviewFailureFinding makePreviewFailureFinding(
    const Finding& finding)
{
    return drs::engine::classifyAuthoringPreviewFailure(
        finding.code, finding.path, finding.message);
}

drs::engine::AuthoringPreviewInvalidationCategory classifyPreviewInvalidation(
    const std::string& changeLabel,
    bool selectionChanged)
{
    using Category = drs::engine::AuthoringPreviewInvalidationCategory;
    if (selectionChanged)
        return Category::selection;

    auto normalized = changeLabel;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    if (normalized.find("gain") != std::string::npos)
        return Category::gain;
    if (normalized.find("pan") != std::string::npos)
        return Category::pan;
    if (normalized.find("root") != std::string::npos)
        return Category::rootKey;
    if (normalized.find("velocity") != std::string::npos)
        return Category::velocityRange;
    if (normalized.find("start") != std::string::npos)
        return Category::sampleStartOffset;
    if (normalized.find("loop") != std::string::npos)
        return Category::loop;
    if (normalized.find("source") != std::string::npos
        || normalized.find("sample") != std::string::npos)
        return Category::sourceAssignment;
    if (normalized.find("bound") != std::string::npos
        || normalized.find("key range") != std::string::npos)
        return Category::keyBounds;
    if (normalized.find("map") != std::string::npos
        || normalized.find("zone") != std::string::npos)
        return Category::mapping;
    return Category::authoredTopology;
}

std::optional<drs::engine::RuntimeProjectSampleSource> findProjectSampleSource(
    const drs::engine::RuntimeProjectModel& project,
    const std::string& sampleSourceId);

std::string buildSelectedZonePreviewFingerprint(
    const drs::engine::RuntimeProjectModel& project,
    const std::optional<drs::engine::RuntimeProjectZoneDefinition>& selectedZone)
{
    if (!selectedZone.has_value())
        return "no-selection";
    const auto& zone = *selectedZone;
    const auto sampleSource = findProjectSampleSource(project, zone.sampleSourceId);
    const auto sourcePath = sampleSource.has_value() ? sampleSource->path : std::string {};
    return zone.sampleSourceId + "|" + sourcePath + "|" + std::to_string(zone.rootKey)
        + "|" + std::to_string(zone.keyLow) + "|" + std::to_string(zone.keyHigh)
        + "|" + std::to_string(zone.velocityLow) + "|" + std::to_string(zone.velocityHigh)
        + "|" + std::to_string(zone.gainDb) + "|" + std::to_string(zone.pan)
        + "|" + std::to_string(zone.sampleStartFrame)
        + "|" + std::to_string(zone.loopEnabled)
        + "|" + std::to_string(zone.loopStartFrame)
        + "|" + std::to_string(zone.loopEndFrame);
}

std::string buildCurrentDraftPreviewFingerprint(const drs::engine::RuntimeProjectModel& project)
{
    constexpr std::uint64_t offsetBasis = 1469598103934665603ull;
    constexpr std::uint64_t prime = 1099511628211ull;
    auto hash = offsetBasis;
    for (const auto byte : drs::engine::serializeRuntimeProjectManifest(project, {}))
    {
        hash ^= static_cast<unsigned char>(byte);
        hash *= prime;
    }

    constexpr char digits[] = "0123456789abcdef";
    std::string result(16, '0');
    for (auto index = 0; index < 16; ++index)
    {
        result[15 - index] = digits[hash & 0x0f];
        hash >>= 4;
    }
    return result;
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
    if (failureState == "Sample missing"
        || failureState.find("missing-sample-source-asset") != std::string::npos
        || failureState.find("not found") != std::string::npos
        || failureState.find("does not exist") != std::string::npos)
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
    authoringPreviewCommandAdapter.clearOwnership();
    performancePlaybackContext.prepare(currentSampleRate);
    authoringPreviewPlaybackContext.prepare(currentSampleRate);
    primeRealtimeSafetyState(samplesPerBlock > 0 ? samplesPerBlock : 512);
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
}

void Processor::releaseResources()
{
    authoringPreviewCommandAdapter.clearOwnership();
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
    const auto frameCount = buffer.getNumSamples();
    if (authoringPreviewCloseRequested.exchange(false, std::memory_order_acq_rel))
        authoringPreviewPlaybackContext.closeAtBlockBoundary();
    drainRealtimeNoteEvents(performanceSurfaceNoteQueue, performanceEvents,
                            static_cast<std::uint32_t>(std::max(frameCount, 0)));
    drainRealtimeNoteEvents(authoringPreviewNoteQueue, authoringPreviewEvents,
                            static_cast<std::uint32_t>(std::max(frameCount, 0)));

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
        else if (command == 0xb0 && metadata.numBytes > 2 && eventData[1] == 64u)
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::sustainPedal,
                                     eventSample,
                                     0,
                                     static_cast<float>(eventData[2] & 0x7fu) / 127.0f });
        }
        else if (command == 0xb0 && metadata.numBytes > 1 && eventData[1] == 123u)
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::allNotesOff,
                                     eventSample,
                                     0,
                                     0.0f });
        }
        else if (command == 0xb0 && metadata.numBytes > 1 && eventData[1] == 120u)
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::reset,
                                     eventSample,
                                     0,
                                     0.0f });
        }
    }

    diagnosticsPerformanceDroppedEventCount.fetch_add(performanceEvents.droppedEventCount(),
                                                       std::memory_order_relaxed);
    diagnosticsAuthoringPreviewDroppedEventCount.fetch_add(authoringPreviewEvents.droppedEventCount(),
                                                            std::memory_order_relaxed);

    if (frameCount > 0 && buffer.getNumChannels() > 0)
    {
        drs::engine::SamplerAudioBufferView output {
            buffer.getArrayOfWritePointers(),
            static_cast<std::uint32_t>(buffer.getNumChannels()),
            static_cast<std::uint32_t>(frameCount)
        };
        const auto performanceRenderStart = std::chrono::steady_clock::now();
        const auto performanceResult = performancePlaybackContext.renderBlock(
            output, performanceEvents.view());
        const auto performanceRenderEnd = std::chrono::steady_clock::now();
        const auto previewResult = authoringPreviewPlaybackContext.renderBlock(
            output, authoringPreviewEvents.view());
        const auto previewRenderEnd = std::chrono::steady_clock::now();
        lastPerformanceRenderMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(performanceRenderEnd - performanceRenderStart).count());
        lastAuthoringPreviewRenderMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(previewRenderEnd - performanceRenderEnd).count());
        maxPerformanceRenderMicros = std::max(maxPerformanceRenderMicros, lastPerformanceRenderMicros);
        maxAuthoringPreviewRenderMicros = std::max(maxAuthoringPreviewRenderMicros,
                                                   lastAuthoringPreviewRenderMicros);
        const auto performanceSnapshot = performancePlaybackContext.getSnapshot();
        const auto previewSnapshot = authoringPreviewPlaybackContext.getSnapshot();
        performancePeakActiveVoiceCount = std::max(performancePeakActiveVoiceCount,
                                                   static_cast<std::size_t>(performanceSnapshot.activeVoiceCount));
        performancePeakReleasingVoiceCount = std::max(
            performancePeakReleasingVoiceCount,
            static_cast<std::size_t>(performanceSnapshot.releasingVoiceCount));
        authoringPreviewPeakActiveVoiceCount = std::max(
            authoringPreviewPeakActiveVoiceCount,
            static_cast<std::size_t>(previewSnapshot.activeVoiceCount));
        authoringPreviewPeakReleasingVoiceCount = std::max(
            authoringPreviewPeakReleasingVoiceCount,
            static_cast<std::size_t>(previewSnapshot.releasingVoiceCount));
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
    drs::engine::AuthoringPreviewCommand command;
    command.type = drs::engine::AuthoringPreviewCommandType::noteOn;
    command.source = drs::engine::AuthoringPreviewAuditionSource::authoringKeyboard;
    command.midiNote = midiNoteNumber;
    command.velocity = velocity;
    submitAuthoringPreviewCommand(command);
}

void Processor::requestAuthoringPreview(drs::engine::AuthoringPreviewScope scope)
{
    drs::engine::AuthoringPreviewCommand command;
    command.type = scope == drs::engine::AuthoringPreviewScope::currentDraft
        ? drs::engine::AuthoringPreviewCommandType::auditionCurrentDraft
        : drs::engine::AuthoringPreviewCommandType::auditionSelectedZone;
    command.emitNote = false;
    submitAuthoringPreviewCommand(command);
}

void Processor::queueAuthoringPreviewNoteOff(int midiNoteNumber)
{
    drs::engine::AuthoringPreviewCommand command;
    command.type = drs::engine::AuthoringPreviewCommandType::noteOff;
    command.source = drs::engine::AuthoringPreviewAuditionSource::authoringKeyboard;
    command.midiNote = midiNoteNumber;
    submitAuthoringPreviewCommand(command);
}

bool Processor::submitAuthoringPreviewCommand(
    const drs::engine::AuthoringPreviewCommand& submittedCommand)
{
    auto command = submittedCommand;
    if (command.type == drs::engine::AuthoringPreviewCommandType::auditionSelectedZone
        && command.selectedZoneId.empty())
    {
        const auto selectedZone = authoringSession.getSelectedZone();
        if (selectedZone.has_value())
            command.selectedZoneId = selectedZone->id;
    }

    const auto dispatch = authoringPreviewCommandAdapter.dispatch(command);
    if (!dispatch.accepted)
        return false;

    if (dispatch.preparationRequested)
    {
        authoringPreviewRequestedScope = dispatch.requestedScope;
        authoringPreviewDirectAuditionRequested = true;
        serviceMessageThreadWork();
    }

    if (!dispatch.hasEvent)
    {
        publishAuthoringPreviewStatus();
        return true;
    }

    drs::engine::SamplerRenderEventType eventType;
    switch (dispatch.event.type)
    {
        case drs::engine::AuthoringPreviewEventType::noteOn:
            eventType = drs::engine::SamplerRenderEventType::noteOn;
            break;
        case drs::engine::AuthoringPreviewEventType::noteOff:
            eventType = drs::engine::SamplerRenderEventType::noteOff;
            break;
        case drs::engine::AuthoringPreviewEventType::allNotesOff:
            eventType = drs::engine::SamplerRenderEventType::allNotesOff;
            break;
        case drs::engine::AuthoringPreviewEventType::reset:
            eventType = drs::engine::SamplerRenderEventType::reset;
            break;
    }

    if (authoringPreviewNoteQueue.push({ eventType,
                                         clampMidiValue(dispatch.event.midiNote),
                                         std::clamp(dispatch.event.velocity, 0.0f, 1.0f),
                                         dispatch.event.sampleOffset }))
    {
        publishAuthoringPreviewStatus();
        return true;
    }

    diagnosticsAuthoringPreviewDroppedNoteCount.fetch_add(1, std::memory_order_relaxed);
    return false;
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
    const auto published = std::atomic_load_explicit(&authoringPreviewStatusPublication,
                                                     std::memory_order_acquire);
    return published != nullptr ? *published : drs::app::AuthoringPreviewStatusSnapshot {};
}

void Processor::publishAuthoringPreviewStatus()
{
    using Presentation = drs::engine::AuthoringPreviewPresentationState;
    using Preparation = drs::engine::AuthoringPreviewPreparationState;
    using Activation = drs::engine::AuthoringPreviewActivationState;

    const auto diagnostics = getRealtimeSafetySnapshot();
    const auto controller = authoringPreviewController.getSnapshot();
    const auto command = authoringPreviewCommandAdapter.getSnapshot();
    const auto selectedZone = authoringSession.getSelectedZone();

    drs::app::AuthoringPreviewStatusSnapshot status;
    status.available = diagnostics.available;
    status.preparationState = controller.preparationState;
    status.activationState = controller.activationState;
    status.scope = controller.hasRequest
        ? controller.currentRequest.identity.scope : authoringPreviewRequestedScope;
    status.requestId = controller.hasRequest
        ? controller.currentRequest.identity.requestId : 0;
    status.cancellationGeneration = controller.hasRequest
        ? controller.currentRequest.identity.cancellationGeneration : 0;
    status.draftRevision = authoringSession.getDocumentState().revision;
    status.activeRevision = controller.hasActiveRequest
        ? controller.activeRequestIdentity.draftRevision : 0;
    status.pendingRevision = controller.activationState == Activation::pending
        && controller.hasRequest ? controller.currentRequest.identity.draftRevision : 0;
    status.requestedRevision = controller.hasRequest
        ? controller.currentRequest.identity.draftRevision : 0;
    status.failedRevision = controller.hasFailedRequest
        ? controller.failedRequestIdentity.draftRevision : 0;
    status.audibleRevision = status.activeRevision;
    status.selectedZoneId = selectedZone.has_value() ? selectedZone->id : std::string {};
    status.requestedPreparedBuildId = controller.acceptedPreparedBuildId;
    status.activePreparedBuildId = controller.activePreparedBuildId;
    status.requestedSnapshotDigest = controller.acceptedSnapshotDigest;
    status.requestedPreparedDigest = controller.acceptedPreparedDigest;
    status.activeSnapshotDigest = controller.activeSnapshotDigest;
    status.activePreparedDigest = controller.activePreparedDigest;
    status.auditionAvailable = engineFacade.getDraftPlaybackStatus().projectOpen
        && (status.scope == drs::engine::AuthoringPreviewScope::currentDraft
            || selectedZone.has_value());
    status.stopAvailable = command.ownedNoteCount > 0
        || diagnostics.authoringPreviewActiveVoiceCount > 0;
    status.usingLastKnownGood = controller.hasActiveRequest
        && (controller.hasFailedRequest
            || controller.activeRequestIdentity.draftRevision != status.draftRevision
            || (controller.hasRequest
                && controller.activeRequestIdentity != controller.currentRequest.identity));

    if (!controller.failureFinding.code.empty())
        status.findings.push_back(controller.failureFinding);
    status.failureState = controller.failureState;
    status.failureFamily = drs::engine::toString(controller.failureFinding.family);
    status.failureCode = controller.failureFinding.code;
    status.failurePath = controller.failureFinding.path;
    const auto blockingHint = buildAuthoringPreviewBlockingHint(authoringSession,
                                                                status.failureState);
    status.blockingPrerequisite = blockingHint.prerequisite;
    status.blockingGuidance = blockingHint.guidance;

    if (controller.preparationState == Preparation::failed)
        status.presentationState = Presentation::failed;
    else if (controller.preparationState == Preparation::canceled)
        status.presentationState = Presentation::canceled;
    else if (controller.preparationState == Preparation::superseded)
        status.presentationState = Presentation::superseded;
    else if (controller.activationState == Activation::pending)
        status.presentationState = Presentation::activating;
    else if (controller.preparationState == Preparation::queued)
        status.presentationState = Presentation::queued;
    else if (controller.preparationState == Preparation::preparing)
        status.presentationState = Presentation::preparing;
    else if (controller.hasActiveRequest
             && controller.activeRequestIdentity.draftRevision == status.draftRevision)
        status.presentationState = Presentation::active;
    else if (controller.hasActiveRequest)
        status.presentationState = Presentation::stale;
    else if (controller.preparationState == Preparation::ready)
        status.presentationState = Presentation::ready;
    else
        status.presentationState = Presentation::idle;

    if (status.scope == drs::engine::AuthoringPreviewScope::selectedZone
        && !selectedZone.has_value())
    {
        status.stateLabel = status.usingLastKnownGood
            ? "No Selection — Last Good Active" : "No Selection";
        status.creatorGuidance = "Select a zone to enable selected-zone Preview.";
    }
    else
    {
        switch (status.presentationState)
        {
            case Presentation::queued:
                status.stateLabel = "Preparing";
                status.creatorGuidance = "Preview is coalescing recent authored changes.";
                break;
            case Presentation::preparing:
                status.stateLabel = "Preparing";
                status.creatorGuidance = "Preview is building the current authored content.";
                break;
            case Presentation::ready:
                status.stateLabel = "Ready";
                status.creatorGuidance = "Preview is prepared and ready to activate.";
                break;
            case Presentation::activating:
                status.stateLabel = "Preparing";
                status.creatorGuidance = "Preview is waiting for the next audio block boundary.";
                break;
            case Presentation::active:
                status.stateLabel = "Ready";
                status.creatorGuidance = "Preview matches the current authored revision.";
                break;
            case Presentation::stale:
                status.stateLabel = "Stale — Last Good Active";
                status.creatorGuidance = "The last known good Preview remains audible while the current draft is prepared.";
                break;
            case Presentation::failed:
                status.stateLabel = status.usingLastKnownGood
                    ? "Failed — Last Good Active" : "Failed";
                status.creatorGuidance = status.blockingGuidance.empty()
                    ? "Repair the reported Preview finding and audition again."
                    : status.blockingGuidance;
                break;
            case Presentation::canceled:
                status.stateLabel = "Canceled";
                status.creatorGuidance = "The Preview request was canceled safely.";
                break;
            case Presentation::superseded:
                status.stateLabel = "Superseded";
                status.creatorGuidance = "A newer authored revision replaced this Preview request.";
                break;
            case Presentation::idle:
            default:
                status.stateLabel = "Ready";
                status.creatorGuidance = "Preview is available for the current authored selection.";
                break;
        }
    }

    status.lastRequestToLaunchMicros = controller.lastRequestToLaunchMicros;
    status.maxRequestToLaunchMicros = controller.maxRequestToLaunchMicros;
    status.lastPreparationMicros = controller.lastPreparationMicros;
    status.maxPreparationMicros = controller.maxPreparationMicros;
    status.lastReadyToActivationMicros = controller.lastReadyToActivationMicros;
    status.maxReadyToActivationMicros = controller.maxReadyToActivationMicros;
    status.lastRequestToAudibleMicros = controller.lastRequestToAudibleMicros;
    status.maxRequestToAudibleMicros = controller.maxRequestToAudibleMicros;
    status.lastCancellationMicros = controller.lastCancellationMicros;
    status.maxCancellationMicros = controller.maxCancellationMicros;
    status.coalescedCount = controller.coalescedCount;
    status.canceledCount = controller.canceledCount;
    status.pendingDepth = controller.pendingDepth;
    status.maximumPendingDepth = controller.maximumPendingDepth;

    std::shared_ptr<const drs::app::AuthoringPreviewStatusSnapshot> immutable
        = std::make_shared<const drs::app::AuthoringPreviewStatusSnapshot>(std::move(status));
    std::atomic_store_explicit(&authoringPreviewStatusPublication,
                               std::move(immutable),
                               std::memory_order_release);
}

drs::app::AuthoringImportResponsivenessSnapshot Processor::getAuthoringImportResponsivenessSnapshot() const
{
    return authoringImportResponsivenessSnapshot;
}

void Processor::replaceAuthoringProject(drs::engine::RuntimeProjectModel project)
{
    const auto& previousProject = authoringSession.getProject();
    const auto replacingDifferentProject = !previousProject.projectId.empty()
        && !project.projectId.empty()
        && previousProject.projectId != project.projectId;
    auto draftPlaybackProject = project;
    if (!engineFacade.replaceDraftPlaybackAuthoringProject(std::move(draftPlaybackProject)))
        return;

    authoringSession.replaceProject(std::move(project));
    performancePlaybackContext.cancelPendingActivation();
    pendingPerformanceActivation.reset();
    engineFacade.closeDraftPlaybackProject();
    engineFacade.reopenDraftPlaybackProject(authoringSession.getDocumentState().revision);
    authoringWaveformPreviewCache.clear();
    if (replacingDifferentProject)
    {
        authoringPreviewController.reset();
        authoringPreviewCommandAdapter.clearOwnership();
        authoringPreviewCloseRequested.store(true, std::memory_order_release);
    }
    authoringPreviewDirectAuditionRequested = false;
    authoringPreviewRequestedScope = drs::engine::AuthoringPreviewScope::selectedZone;
    observedDraftPlaybackProjectRevision = authoringSession.getDocumentState().revision;
    initializeAuthoringImportMetrics();
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
}

void Processor::closeAuthoringProject(drs::engine::RuntimeProjectModel unloadedProject)
{
    engineFacade.cancelPreviewPreparation("Authoring project closed");
    performancePlaybackContext.cancelPendingActivation();
    pendingPerformanceActivation.reset();
    engineFacade.closeDraftPlaybackProject();
    authoringSession.replaceProject(std::move(unloadedProject));
    authoringWaveformPreviewCache.clear();
    authoringPreviewController.reset();
    authoringPreviewCommandAdapter.clearOwnership();
    authoringPreviewCloseRequested.store(true, std::memory_order_release);
    authoringPreviewDirectAuditionRequested = false;
    authoringPreviewRequestedScope = drs::engine::AuthoringPreviewScope::selectedZone;
    observedDraftPlaybackProjectRevision = authoringSession.getDocumentState().revision;
    initializeAuthoringImportMetrics();
    updateRealtimeSafetyState();
    publishAuthoringPreviewStatus();
}

void Processor::queuePerformanceSurfaceNoteOn(int midiNoteNumber, float velocity)
{
    if (!performanceSurfaceNoteQueue.push(
            { drs::engine::SamplerRenderEventType::noteOn,
              clampMidiValue(midiNoteNumber), std::clamp(velocity, 0.0f, 1.0f), 0 }))
        diagnosticsPerformanceDroppedNoteCount.fetch_add(1, std::memory_order_relaxed);
}

void Processor::queuePerformanceSurfaceNoteOff(int midiNoteNumber)
{
    if (!performanceSurfaceNoteQueue.push({ drs::engine::SamplerRenderEventType::noteOff,
                                            clampMidiValue(midiNoteNumber), 0.0f, 0 }))
        diagnosticsPerformanceDroppedNoteCount.fetch_add(1, std::memory_order_relaxed);
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
    const auto serviceTimeMicros = monotonicMicros();
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

    auto synchronizedPerformancePublish = false;
    const auto publishControllerBeforeSynchronization
        = engineFacade.getPerformancePublishControllerSnapshot();
    if (pendingPerformanceActivation != nullptr
        && (!publishControllerBeforeSynchronization.hasRequest
            || publishControllerBeforeSynchronization.currentRequest.identity
                != pendingPerformanceActivation->requestIdentity
            || publishControllerBeforeSynchronization.activationState
                != drs::engine::PerformancePublishActivationState::pending
            || publishControllerBeforeSynchronization.pendingActivationToken
                != pendingPerformanceActivation->activationToken))
    {
        performancePlaybackContext.cancelPendingActivation();
        pendingPerformanceActivation.reset();
        synchronizedPerformancePublish = true;
    }
    const auto performanceContextBeforeSynchronization = performancePlaybackContext.getSnapshot();
    if (pendingPerformanceActivation != nullptr
        && performanceContextBeforeSynchronization.hasActiveActivation
        && !performanceContextBeforeSynchronization.hasPendingActivation
        && performanceContextBeforeSynchronization.activeRevision
            == pendingPerformanceActivation->revision
        && performanceContextBeforeSynchronization.activePreparedBuildId
            == pendingPerformanceActivation->preparedBuildId)
    {
        synchronizedPerformancePublish = engineFacade.acknowledgePerformanceActivation(
            pendingPerformanceActivation, serviceTimeMicros);
        if (synchronizedPerformancePublish)
            pendingPerformanceActivation.reset();
    }
    else if (pendingPerformanceActivation != nullptr
             && !performanceContextBeforeSynchronization.hasPendingActivation)
    {
        synchronizedPerformancePublish = engineFacade.rejectPerformanceActivationStaging(
            pendingPerformanceActivation,
            { drs::engine::PerformancePublishFindingSeverity::error,
              "performance-activation-apply-rejected",
              "performance.activationSlot",
              "The audio boundary rejected the authorized Performance activation payload." });
        pendingPerformanceActivation.reset();
    }

    auto synchronizedActivation = false;
    const auto stateRevision = engineFacade.getStateRevision();
    if (stateRevision != observedEngineStateRevision)
    {
        observedEngineStateRevision = stateRevision;
        const auto performanceSnapshot = performancePlaybackContext.getSnapshot();
        synchronizedActivation = synchronizePerformanceActivation(
            !performanceSnapshot.hasActiveActivation);
    }

    const auto previewContextSnapshot = authoringPreviewPlaybackContext.getSnapshot();
    const auto& authoredProject = authoringSession.getProject();
    const auto selectedZone = authoringSession.getSelectedZone();
    const auto selectedZoneId = selectedZone.has_value() ? selectedZone->id : std::string {};
    const auto requestedScope = authoringPreviewRequestedScope;
    const auto requestSelectedZoneId = requestedScope
            == drs::engine::AuthoringPreviewScope::selectedZone
        ? selectedZoneId
        : std::string {};
    const auto controllerBeforeRequest = authoringPreviewController.getSnapshot();
    const auto scopeChanged = controllerBeforeRequest.hasRequest
        && controllerBeforeRequest.currentRequest.identity.scope != requestedScope;
    const auto selectionChanged = requestedScope == drs::engine::AuthoringPreviewScope::selectedZone
        && controllerBeforeRequest.hasRequest
        && controllerBeforeRequest.currentRequest.identity.selectedZoneId != selectedZoneId;
    const auto requestReason = authoringPreviewDirectAuditionRequested
        ? (requestedScope == drs::engine::AuthoringPreviewScope::currentDraft
               ? drs::engine::AuthoringPreviewRequestReason::explicitCurrentDraftAudition
               : drs::engine::AuthoringPreviewRequestReason::explicitSelectedZoneAudition)
        : (!controllerBeforeRequest.hasRequest
        ? drs::engine::AuthoringPreviewRequestReason::projectOpened
        : (selectionChanged
               ? drs::engine::AuthoringPreviewRequestReason::selectionChanged
               : drs::engine::AuthoringPreviewRequestReason::authoringChanged));
    const auto observesNewRevision = !controllerBeforeRequest.hasRequest
        || controllerBeforeRequest.currentRequest.identity.draftRevision != authoringRevision;
    const auto invalidationCategory = scopeChanged
        ? drs::engine::AuthoringPreviewInvalidationCategory::previewScope
        : (observesNewRevision
        ? classifyPreviewInvalidation(authoringSession.getDocumentState().lastChangeLabel,
                                      selectionChanged)
        : controllerBeforeRequest.currentRequest.invalidationCategory);
    const auto authoredContentFingerprint
        = requestedScope == drs::engine::AuthoringPreviewScope::currentDraft
        ? buildCurrentDraftPreviewFingerprint(authoredProject)
        : buildSelectedZonePreviewFingerprint(authoredProject, selectedZone);
    const auto requestSignature = drs::engine::buildAuthoringPreviewRequestSignature(
        requestedScope,
        requestSelectedZoneId,
        invalidationCategory,
        authoredContentFingerprint);
    const auto requestResult = authoringPreviewController.request(
        requestedScope,
        authoringRevision,
        requestSelectedZoneId,
        requestReason,
        invalidationCategory,
        requestSignature,
        serviceTimeMicros);
    authoringPreviewDirectAuditionRequested = false;
    const auto canceledSupersededWork = requestResult.supersededPrevious
        && engineFacade.cancelPreviewPreparation();
    if (canceledSupersededWork && !requestResult.cancellationRequested)
        authoringPreviewController.recordWorkerCancellation();

    auto synchronizedAuthoringPreview = false;
    auto controllerSnapshot = authoringPreviewController.getSnapshot();
    if (controllerSnapshot.hasRequest
        && controllerSnapshot.activationState == drs::engine::AuthoringPreviewActivationState::pending
        && previewContextSnapshot.hasActiveActivation
        && !previewContextSnapshot.hasPendingActivation
        && previewContextSnapshot.activeRevision
            == controllerSnapshot.currentRequest.identity.draftRevision)
    {
        synchronizedAuthoringPreview = authoringPreviewController.markActive(
            controllerSnapshot.currentRequest.identity, serviceTimeMicros);
        controllerSnapshot = authoringPreviewController.getSnapshot();
    }
    const auto preparedPayload = engineFacade.getPreviewActivationPayload();
    const auto directAuditionContentPrepared = preparedPayload != nullptr
        && preparedPayload->revision == authoringRevision
        && preparedPayload->preparedBuildId != 0;
    const auto launch = authoringPreviewController.launchIfEligible(
        serviceTimeMicros, directAuditionContentPrepared);
    if (launch.launched)
    {
        if (!directAuditionContentPrepared && !engineFacade.refreshPreviewToCurrentDraft())
        {
            const auto& draftStatus = engineFacade.getDraftPlaybackStatus();
            if (!draftStatus.preview.findings.empty())
            {
                const auto& finding = draftStatus.preview.findings.front();
                const auto previewFinding = makePreviewFailureFinding(finding);
                authoringPreviewController.fail(launch.request.identity, previewFinding);
            }
            else
            {
                const auto previewFinding = drs::engine::classifyAuthoringPreviewFailure(
                    "preview-worker-request-rejected", "worker",
                    draftStatus.lastEvent.empty()
                        ? std::string("Preview worker request was rejected.")
                        : draftStatus.lastEvent);
                authoringPreviewController.fail(launch.request.identity, previewFinding);
            }
            synchronizedAuthoringPreview = true;
        }
        controllerSnapshot = authoringPreviewController.getSnapshot();
    }

    const auto preparedAfterLaunch = engineFacade.getPreviewActivationPayload();
    if (controllerSnapshot.hasRequest
        && controllerSnapshot.preparationState
            == drs::engine::AuthoringPreviewPreparationState::preparing
        && preparedAfterLaunch != nullptr
        && preparedAfterLaunch->revision == authoringRevision)
    {
        synchronizedAuthoringPreview = stageAuthoringPreviewActivation(
            controllerSnapshot.currentRequest,
            !previewContextSnapshot.hasActiveActivation) || synchronizedAuthoringPreview;
    }
    else if (controllerSnapshot.hasRequest
             && controllerSnapshot.preparationState
                 == drs::engine::AuthoringPreviewPreparationState::preparing)
    {
        const auto& draftStatus = engineFacade.getDraftPlaybackStatus();
        if (!draftStatus.pendingPreview.active && !draftStatus.preview.findings.empty())
        {
            const auto& finding = draftStatus.preview.findings.front();
            const auto previewFinding = makePreviewFailureFinding(finding);
            authoringPreviewController.fail(controllerSnapshot.currentRequest.identity,
                                             previewFinding);
            synchronizedAuthoringPreview = true;
        }
    }

    updateRealtimeSafetyState();
    publishAuthoringPreviewStatus();
    return servicedBackgroundWork
        || synchronizedDraftPlaybackProject
        || requestResult.accepted
        || requestResult.expeditedCurrent
        || canceledSupersededWork
        || synchronizedPerformancePublish
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

bool Processor::stageAuthoringPreviewActivation(const drs::engine::AuthoringPreviewRequest& request,
                                                bool installImmediately)
{
    const auto reclaimed = authoringPreviewPlaybackContext.serviceRetirements();
    diagnosticsRetiredActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    diagnosticsReclaimedActivationPayloadCount.fetch_add(reclaimed, std::memory_order_relaxed);
    const auto currentRevision = authoringSession.getDocumentState().revision;
    const auto failPreviewActivation = [&](drs::engine::AuthoringPreviewFailureFinding finding)
    {
        authoringPreviewController.fail(request.identity, std::move(finding));
        return false;
    };

    if (!authoringPreviewController.isCurrent(request.identity)
        || request.identity.draftRevision != currentRevision)
        return false;

    const auto payload = engineFacade.getPreviewActivationPayload();
    const auto preparation = drs::engine::prepareAuthoringPreviewRenderModel(payload, request);
    if (!preparation.prepared || preparation.model == nullptr)
    {
        if (preparation.findings.empty())
            return failPreviewActivation(drs::engine::classifyAuthoringPreviewFailure(
                "preview-preparation-finding-missing", "preparation",
                "Authoring Preview preparation failed without a finding."));
        return failPreviewActivation(makePreviewFailureFinding(preparation.findings.front()));
    }
    if (!authoringPreviewController.acceptPrepared(request.identity,
                                                    preparation.scopedPayload->preparedBuildId,
                                                    monotonicMicros(),
                                                    preparation.scopedPayload->snapshotContentDigest,
                                                    preparation.scopedPayload->preparedContentDigest))
        return false;
    if (!authoringPreviewPlaybackContext.stageActivation(preparation.model))
        return failPreviewActivation(drs::engine::classifyAuthoringPreviewFailure(
            "preview-activation-slot-exhausted", "preview.activationSlots",
            "Authoring Preview activation slots are exhausted."));
    if (!authoringPreviewController.markActivationPending(request.identity, monotonicMicros()))
        return false;

    if (installImmediately && authoringPreviewPlaybackContext.activatePendingForPreparation())
    {
        diagnosticsAuthoringPreviewActivationCount.fetch_add(1, std::memory_order_relaxed);
        authoringPreviewController.markActive(request.identity, monotonicMicros());
    }
    return true;
}

bool Processor::synchronizePerformanceActivation(bool installImmediately)
{
    const auto reclaimed = performancePlaybackContext.serviceRetirements();
    diagnosticsRetiredActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    diagnosticsReclaimedActivationPayloadCount.fetch_add(reclaimed, std::memory_order_relaxed);
    if (pendingPerformanceActivation != nullptr)
        return false;

    const auto performanceSnapshot = performancePlaybackContext.getSnapshot();
    auto authorized = engineFacade.authorizePerformanceActivation(monotonicMicros());
    auto payload = authorized != nullptr
        ? authorized->playbackPayload
        : drs::engine::PlaybackActivationPayloadPtr {};
    const auto bootstrap = !performanceSnapshot.hasActiveActivation
        && (authorized == nullptr
            || authorized->requestIdentity.origin
                == drs::engine::PerformancePublishRequestOrigin::bootstrap);
    if (bootstrap && authorized == nullptr)
        payload = engineFacade.getBootstrapPerformanceActivationPayload();
    if (payload == nullptr)
        return false;

    const auto& sessionState = engineFacade.getCurrentSessionState();

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
    if (!modelResult.built || modelResult.model == nullptr)
    {
        if (authorized != nullptr)
            engineFacade.rejectPerformanceActivationStaging(
                authorized,
                { drs::engine::PerformancePublishFindingSeverity::error,
                  "performance-render-model-rejected",
                  "performance.renderModel",
                  "The controller-authorized Performance payload could not build a render model." });
        return false;
    }
    if (!performancePlaybackContext.stageActivation(modelResult.model))
    {
        if (authorized != nullptr)
            engineFacade.rejectPerformanceActivationStaging(
                authorized,
                { drs::engine::PerformancePublishFindingSeverity::error,
                  "performance-activation-slot-rejected",
                  "performance.activationSlots",
                  "The bounded Performance activation slots rejected the authorized payload." });
        return false;
    }

    pendingPerformanceActivation = authorized;

    if (installImmediately && bootstrap
        && performancePlaybackContext.activatePendingForPreparation())
    {
        diagnosticsPerformanceActivationCount.fetch_add(1, std::memory_order_relaxed);
        if (pendingPerformanceActivation != nullptr)
        {
            engineFacade.acknowledgePerformanceActivation(
                pendingPerformanceActivation, monotonicMicros());
            pendingPerformanceActivation.reset();
        }
    }
    return true;
}

void Processor::drainRealtimeNoteEvents(RealtimeNoteEventQueue& queue,
                                        drs::engine::SamplerEventBlock& destination,
                                        std::uint32_t frameCount) noexcept
{
    QueuedRealtimeNoteEvent event;
    while (queue.pop(event))
    {
        const auto sampleOffset = frameCount == 0
            ? 0u
            : std::min(event.sampleOffset, frameCount - 1u);
        destination.push({ event.type,
                           sampleOffset,
                           static_cast<std::uint8_t>(clampMidiValue(event.midiNoteNumber)),
                           event.type == drs::engine::SamplerRenderEventType::noteOn
                               ? std::max(event.velocity, 1.0f / 127.0f)
                               : 0.0f });
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
    values.lastActivationReclamationLatencyBlocks = std::max(
        preview.counters.lastReclamationLatencyBlocks,
        performance.counters.lastReclamationLatencyBlocks);
    values.maxActivationReclamationLatencyBlocks = std::max(
        preview.counters.maxReclamationLatencyBlocks,
        performance.counters.maxReclamationLatencyBlocks);
    values.performanceContextIdentity = static_cast<std::uint32_t>(performance.lane) + 1u;
    values.authoringPreviewContextIdentity = static_cast<std::uint32_t>(preview.lane) + 1u;
    values.performanceVoiceStealCount = performance.counters.stolenVoiceCount;
    values.performanceGenerationStealCount = performance.counters.generationStealCount;
    values.performanceReleasingVoiceStealCount = performance.counters.releasingVoiceStealCount;
    values.performanceActiveGeneration = performance.activeActivationGeneration;
    values.performanceActiveGenerationVoiceCount = performance.activeGenerationVoiceCount;
    values.performanceRetiredGenerationVoiceCount = performance.retiredGenerationVoiceCount;
    values.performanceSustainDeferredVoiceCount = performance.sustainDeferredVoiceCount;
    values.authoringPreviewVoiceStealCount = preview.counters.stolenVoiceCount;
    values.performanceDroppedEventCount = performance.counters.droppedEventCount
        + diagnosticsPerformanceDroppedEventCount.load(std::memory_order_relaxed);
    values.authoringPreviewDroppedEventCount = preview.counters.droppedEventCount
        + diagnosticsAuthoringPreviewDroppedEventCount.load(std::memory_order_relaxed);
    values.performanceDroppedNoteCount = diagnosticsPerformanceDroppedNoteCount.load(std::memory_order_relaxed);
    values.authoringPreviewDroppedNoteCount = diagnosticsAuthoringPreviewDroppedNoteCount.load(std::memory_order_relaxed);

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
    values.lastPerformanceRenderMicros = lastPerformanceRenderMicros;
    values.maxPerformanceRenderMicros = maxPerformanceRenderMicros;
    values.lastAuthoringPreviewRenderMicros = lastAuthoringPreviewRenderMicros;
    values.maxAuthoringPreviewRenderMicros = maxAuthoringPreviewRenderMicros;
    values.performancePeakActiveVoiceCount = performancePeakActiveVoiceCount;
    values.performancePeakReleasingVoiceCount = performancePeakReleasingVoiceCount;
    values.authoringPreviewPeakActiveVoiceCount = authoringPreviewPeakActiveVoiceCount;
    values.authoringPreviewPeakReleasingVoiceCount = authoringPreviewPeakReleasingVoiceCount;

    auto sequence = audioDiagnosticsPublication.sequence.fetch_add(1, std::memory_order_acq_rel) + 1;
    audioDiagnosticsPublication.performanceActiveVoiceCount.store(values.performanceActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewActiveVoiceCount.store(values.authoringPreviewActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.activeVoiceCapacity.store(values.activeVoiceCapacity, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceContextIdentity.store(values.performanceContextIdentity, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewContextIdentity.store(values.authoringPreviewContextIdentity, std::memory_order_relaxed);
    audioDiagnosticsPublication.lastPerformanceRenderMicros.store(values.lastPerformanceRenderMicros, std::memory_order_relaxed);
    audioDiagnosticsPublication.maxPerformanceRenderMicros.store(values.maxPerformanceRenderMicros, std::memory_order_relaxed);
    audioDiagnosticsPublication.lastAuthoringPreviewRenderMicros.store(values.lastAuthoringPreviewRenderMicros, std::memory_order_relaxed);
    audioDiagnosticsPublication.maxAuthoringPreviewRenderMicros.store(values.maxAuthoringPreviewRenderMicros, std::memory_order_relaxed);
    audioDiagnosticsPublication.performancePeakActiveVoiceCount.store(values.performancePeakActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performancePeakReleasingVoiceCount.store(values.performancePeakReleasingVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewPeakActiveVoiceCount.store(values.authoringPreviewPeakActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewPeakReleasingVoiceCount.store(values.authoringPreviewPeakReleasingVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceVoiceStealCount.store(values.performanceVoiceStealCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceGenerationStealCount.store(values.performanceGenerationStealCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceReleasingVoiceStealCount.store(values.performanceReleasingVoiceStealCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceActiveGeneration.store(values.performanceActiveGeneration, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceActiveGenerationVoiceCount.store(values.performanceActiveGenerationVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceRetiredGenerationVoiceCount.store(values.performanceRetiredGenerationVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceSustainDeferredVoiceCount.store(values.performanceSustainDeferredVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewVoiceStealCount.store(values.authoringPreviewVoiceStealCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceDroppedEventCount.store(values.performanceDroppedEventCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewDroppedEventCount.store(values.authoringPreviewDroppedEventCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceDroppedNoteCount.store(values.performanceDroppedNoteCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewDroppedNoteCount.store(values.authoringPreviewDroppedNoteCount, std::memory_order_relaxed);
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
    audioDiagnosticsPublication.lastActivationReclamationLatencyBlocks.store(
        values.lastActivationReclamationLatencyBlocks, std::memory_order_relaxed);
    audioDiagnosticsPublication.maxActivationReclamationLatencyBlocks.store(
        values.maxActivationReclamationLatencyBlocks, std::memory_order_relaxed);
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
        values.performanceContextIdentity = audioDiagnosticsPublication.performanceContextIdentity.load(std::memory_order_relaxed);
        values.authoringPreviewContextIdentity = audioDiagnosticsPublication.authoringPreviewContextIdentity.load(std::memory_order_relaxed);
        values.lastPerformanceRenderMicros = audioDiagnosticsPublication.lastPerformanceRenderMicros.load(std::memory_order_relaxed);
        values.maxPerformanceRenderMicros = audioDiagnosticsPublication.maxPerformanceRenderMicros.load(std::memory_order_relaxed);
        values.lastAuthoringPreviewRenderMicros = audioDiagnosticsPublication.lastAuthoringPreviewRenderMicros.load(std::memory_order_relaxed);
        values.maxAuthoringPreviewRenderMicros = audioDiagnosticsPublication.maxAuthoringPreviewRenderMicros.load(std::memory_order_relaxed);
        values.performancePeakActiveVoiceCount = audioDiagnosticsPublication.performancePeakActiveVoiceCount.load(std::memory_order_relaxed);
        values.performancePeakReleasingVoiceCount = audioDiagnosticsPublication.performancePeakReleasingVoiceCount.load(std::memory_order_relaxed);
        values.authoringPreviewPeakActiveVoiceCount = audioDiagnosticsPublication.authoringPreviewPeakActiveVoiceCount.load(std::memory_order_relaxed);
        values.authoringPreviewPeakReleasingVoiceCount = audioDiagnosticsPublication.authoringPreviewPeakReleasingVoiceCount.load(std::memory_order_relaxed);
        values.performanceVoiceStealCount = audioDiagnosticsPublication.performanceVoiceStealCount.load(std::memory_order_relaxed);
        values.performanceGenerationStealCount = audioDiagnosticsPublication.performanceGenerationStealCount.load(std::memory_order_relaxed);
        values.performanceReleasingVoiceStealCount = audioDiagnosticsPublication.performanceReleasingVoiceStealCount.load(std::memory_order_relaxed);
        values.performanceActiveGeneration = audioDiagnosticsPublication.performanceActiveGeneration.load(std::memory_order_relaxed);
        values.performanceActiveGenerationVoiceCount = audioDiagnosticsPublication.performanceActiveGenerationVoiceCount.load(std::memory_order_relaxed);
        values.performanceRetiredGenerationVoiceCount = audioDiagnosticsPublication.performanceRetiredGenerationVoiceCount.load(std::memory_order_relaxed);
        values.performanceSustainDeferredVoiceCount = audioDiagnosticsPublication.performanceSustainDeferredVoiceCount.load(std::memory_order_relaxed);
        values.authoringPreviewVoiceStealCount = audioDiagnosticsPublication.authoringPreviewVoiceStealCount.load(std::memory_order_relaxed);
        values.performanceDroppedEventCount = audioDiagnosticsPublication.performanceDroppedEventCount.load(std::memory_order_relaxed);
        values.authoringPreviewDroppedEventCount = audioDiagnosticsPublication.authoringPreviewDroppedEventCount.load(std::memory_order_relaxed);
        values.performanceDroppedNoteCount = audioDiagnosticsPublication.performanceDroppedNoteCount.load(std::memory_order_relaxed);
        values.authoringPreviewDroppedNoteCount = audioDiagnosticsPublication.authoringPreviewDroppedNoteCount.load(std::memory_order_relaxed);
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
        values.lastActivationReclamationLatencyBlocks =
            audioDiagnosticsPublication.lastActivationReclamationLatencyBlocks.load(std::memory_order_relaxed);
        values.maxActivationReclamationLatencyBlocks =
            audioDiagnosticsPublication.maxActivationReclamationLatencyBlocks.load(std::memory_order_relaxed);
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
    snapshot.authoringSampleLoadsOnAudioThread = diagnosticsAuthoringSampleLoadsOnAudioThread.load(std::memory_order_acquire);
    snapshot.performanceActiveVoiceCount = audioValues.performanceActiveVoiceCount;
    snapshot.authoringPreviewActiveVoiceCount = audioValues.authoringPreviewActiveVoiceCount;
    snapshot.activeVoiceCapacity = std::max(audioValues.activeVoiceCapacity,
                                            diagnosticsPrimedActiveVoiceCapacity.load(std::memory_order_acquire));
    snapshot.activeVoiceCapacityLimit = diagnosticsActiveVoiceCapacityLimit.load(std::memory_order_acquire);
    snapshot.performanceContextIdentity = audioValues.performanceContextIdentity;
    snapshot.authoringPreviewContextIdentity = audioValues.authoringPreviewContextIdentity;
    snapshot.lastPerformanceRenderMicros = audioValues.lastPerformanceRenderMicros;
    snapshot.maxPerformanceRenderMicros = audioValues.maxPerformanceRenderMicros;
    snapshot.lastAuthoringPreviewRenderMicros = audioValues.lastAuthoringPreviewRenderMicros;
    snapshot.maxAuthoringPreviewRenderMicros = audioValues.maxAuthoringPreviewRenderMicros;
    snapshot.performancePeakActiveVoiceCount = audioValues.performancePeakActiveVoiceCount;
    snapshot.performancePeakReleasingVoiceCount = audioValues.performancePeakReleasingVoiceCount;
    snapshot.authoringPreviewPeakActiveVoiceCount = audioValues.authoringPreviewPeakActiveVoiceCount;
    snapshot.authoringPreviewPeakReleasingVoiceCount = audioValues.authoringPreviewPeakReleasingVoiceCount;
    snapshot.performanceVoiceStealCount = audioValues.performanceVoiceStealCount;
    snapshot.performanceGenerationStealCount = audioValues.performanceGenerationStealCount;
    snapshot.performanceReleasingVoiceStealCount = audioValues.performanceReleasingVoiceStealCount;
    snapshot.performanceActiveGeneration = audioValues.performanceActiveGeneration;
    snapshot.performanceActiveGenerationVoiceCount = audioValues.performanceActiveGenerationVoiceCount;
    snapshot.performanceRetiredGenerationVoiceCount = audioValues.performanceRetiredGenerationVoiceCount;
    snapshot.performanceSustainDeferredVoiceCount = audioValues.performanceSustainDeferredVoiceCount;
    snapshot.authoringPreviewVoiceStealCount = audioValues.authoringPreviewVoiceStealCount;
    snapshot.performanceDroppedEventCount = audioValues.performanceDroppedEventCount;
    snapshot.authoringPreviewDroppedEventCount = audioValues.authoringPreviewDroppedEventCount;
    snapshot.performanceDroppedNoteCount = audioValues.performanceDroppedNoteCount;
    snapshot.authoringPreviewDroppedNoteCount = audioValues.authoringPreviewDroppedNoteCount;
    snapshot.authoringPreviewActivationCount = diagnosticsAuthoringPreviewActivationCount.load(std::memory_order_acquire);
    snapshot.performanceActivationCount = diagnosticsPerformanceActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationCount = diagnosticsRetiredActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationBacklog = audioValues.retiredActivationBacklog;
    snapshot.reclaimedActivationPayloadCount = diagnosticsReclaimedActivationPayloadCount.load(std::memory_order_acquire);
    snapshot.activeActivationPayloadBytes = audioValues.activeActivationPayloadBytes;
    snapshot.pendingActivationPayloadBytes = audioValues.pendingActivationPayloadBytes;
    snapshot.retiredActivationPayloadBytes = audioValues.retiredActivationPayloadBytes;
    snapshot.lastActivationReclamationLatencyBlocks =
        audioValues.lastActivationReclamationLatencyBlocks;
    snapshot.maxActivationReclamationLatencyBlocks =
        audioValues.maxActivationReclamationLatencyBlocks;
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
    const auto controller = authoringPreviewController.getSnapshot();
    snapshot.authoringPreviewFailureState = controller.hasFailedRequest
        && controller.failedRequestIdentity.draftRevision
            == snapshot.currentAuthoringPreviewDraftRevision
        ? controller.failureState : std::string {};

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
    audioValues.lastActivationReclamationLatencyBlocks =
        activationValues.lastActivationReclamationLatencyBlocks;
    audioValues.maxActivationReclamationLatencyBlocks =
        activationValues.maxActivationReclamationLatencyBlocks;
    audioValues.performanceContextIdentity = activationValues.performanceContextIdentity;
    audioValues.authoringPreviewContextIdentity = activationValues.authoringPreviewContextIdentity;
    audioValues.performanceVoiceStealCount = activationValues.performanceVoiceStealCount;
    audioValues.performanceGenerationStealCount = activationValues.performanceGenerationStealCount;
    audioValues.performanceReleasingVoiceStealCount = activationValues.performanceReleasingVoiceStealCount;
    audioValues.performanceActiveGeneration = activationValues.performanceActiveGeneration;
    audioValues.performanceActiveGenerationVoiceCount = activationValues.performanceActiveGenerationVoiceCount;
    audioValues.performanceRetiredGenerationVoiceCount = activationValues.performanceRetiredGenerationVoiceCount;
    audioValues.performanceSustainDeferredVoiceCount = activationValues.performanceSustainDeferredVoiceCount;
    audioValues.authoringPreviewVoiceStealCount = activationValues.authoringPreviewVoiceStealCount;
    audioValues.performanceDroppedEventCount = activationValues.performanceDroppedEventCount;
    audioValues.authoringPreviewDroppedEventCount = activationValues.authoringPreviewDroppedEventCount;
    audioValues.performanceDroppedNoteCount = activationValues.performanceDroppedNoteCount;
    audioValues.authoringPreviewDroppedNoteCount = activationValues.authoringPreviewDroppedNoteCount;
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
    snapshot.authoringSampleLoadsOnAudioThread = diagnosticsAuthoringSampleLoadsOnAudioThread.load(std::memory_order_acquire);
    snapshot.performanceActiveVoiceCount = audioValues.performanceActiveVoiceCount;
    snapshot.authoringPreviewActiveVoiceCount = audioValues.authoringPreviewActiveVoiceCount;
    snapshot.activeVoiceCapacity = std::max(audioValues.activeVoiceCapacity,
                                            diagnosticsPrimedActiveVoiceCapacity.load(std::memory_order_acquire));
    snapshot.performanceContextIdentity = audioValues.performanceContextIdentity;
    snapshot.authoringPreviewContextIdentity = audioValues.authoringPreviewContextIdentity;
    snapshot.lastPerformanceRenderMicros = audioValues.lastPerformanceRenderMicros;
    snapshot.maxPerformanceRenderMicros = audioValues.maxPerformanceRenderMicros;
    snapshot.lastAuthoringPreviewRenderMicros = audioValues.lastAuthoringPreviewRenderMicros;
    snapshot.maxAuthoringPreviewRenderMicros = audioValues.maxAuthoringPreviewRenderMicros;
    snapshot.performancePeakActiveVoiceCount = audioValues.performancePeakActiveVoiceCount;
    snapshot.performancePeakReleasingVoiceCount = audioValues.performancePeakReleasingVoiceCount;
    snapshot.authoringPreviewPeakActiveVoiceCount = audioValues.authoringPreviewPeakActiveVoiceCount;
    snapshot.authoringPreviewPeakReleasingVoiceCount = audioValues.authoringPreviewPeakReleasingVoiceCount;
    snapshot.performanceVoiceStealCount = audioValues.performanceVoiceStealCount;
    snapshot.performanceGenerationStealCount = audioValues.performanceGenerationStealCount;
    snapshot.performanceReleasingVoiceStealCount = audioValues.performanceReleasingVoiceStealCount;
    snapshot.performanceActiveGeneration = audioValues.performanceActiveGeneration;
    snapshot.performanceActiveGenerationVoiceCount = audioValues.performanceActiveGenerationVoiceCount;
    snapshot.performanceRetiredGenerationVoiceCount = audioValues.performanceRetiredGenerationVoiceCount;
    snapshot.performanceSustainDeferredVoiceCount = audioValues.performanceSustainDeferredVoiceCount;
    snapshot.authoringPreviewVoiceStealCount = audioValues.authoringPreviewVoiceStealCount;
    snapshot.performanceDroppedEventCount = audioValues.performanceDroppedEventCount;
    snapshot.authoringPreviewDroppedEventCount = audioValues.authoringPreviewDroppedEventCount;
    snapshot.performanceDroppedNoteCount = audioValues.performanceDroppedNoteCount;
    snapshot.authoringPreviewDroppedNoteCount = audioValues.authoringPreviewDroppedNoteCount;
    snapshot.authoringPreviewActivationCount = diagnosticsAuthoringPreviewActivationCount.load(std::memory_order_acquire);
    snapshot.performanceActivationCount = diagnosticsPerformanceActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationCount = diagnosticsRetiredActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationBacklog = audioValues.retiredActivationBacklog;
    snapshot.reclaimedActivationPayloadCount = diagnosticsReclaimedActivationPayloadCount.load(std::memory_order_acquire);
    snapshot.activeActivationPayloadBytes = audioValues.activeActivationPayloadBytes;
    snapshot.pendingActivationPayloadBytes = audioValues.pendingActivationPayloadBytes;
    snapshot.retiredActivationPayloadBytes = audioValues.retiredActivationPayloadBytes;
    snapshot.lastActivationReclamationLatencyBlocks =
        audioValues.lastActivationReclamationLatencyBlocks;
    snapshot.maxActivationReclamationLatencyBlocks =
        audioValues.maxActivationReclamationLatencyBlocks;
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
