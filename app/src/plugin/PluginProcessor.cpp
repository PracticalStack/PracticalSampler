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

namespace drs::plugin
{
namespace
{
namespace fs = std::filesystem;

std::optional<double> findMacroValue(const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                     const std::string& macroId)
{
    const auto iterator = std::find_if(sessionState.macroValues.begin(),
                                       sessionState.macroValues.end(),
                                       [&](const auto& macroValue)
                                       {
                                           return macroValue.id == macroId;
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

const drs::engine::RuntimeZoneDefinition* findZone(const drs::engine::RuntimeInstrumentModel& instrument,
                                                   const std::string& zoneId)
{
    const auto iterator = std::find_if(instrument.zones.begin(),
                                       instrument.zones.end(),
                                       [&](const auto& zone)
                                       {
                                           return zone.id == zoneId;
                                       });

    return iterator != instrument.zones.end() ? &(*iterator) : nullptr;
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

Processor::Processor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      authoringSession(buildInitialAuthoringProject()),
      parameterState(*this, nullptr, "macroParameters", buildParameterLayout(engineFacade))
{
    realtimeSafetySnapshot.available = true;
    performanceSurfaceMidiCollector.reset(currentSampleRate);
    authoringPreviewMidiCollector.reset(currentSampleRate);
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
}

void Processor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    performanceSurfaceMidiCollector.reset(currentSampleRate);
    authoringPreviewMidiCollector.reset(currentSampleRate);
    performanceActiveVoices.clear();
    authoringPreviewActiveVoices.clear();
    primeRealtimeSafetyState(samplesPerBlock > 0 ? samplesPerBlock : 512);
    ensureReferencePlaybackAssetsLoaded(false);
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
}

void Processor::releaseResources()
{
    performanceActiveVoices.clear();
    authoringPreviewActiveVoices.clear();
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
    const juce::ScopedValueSetter<bool> audioCallbackScope(processingAudioCallback, true);

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    performanceMidiScratchBuffer.clear();
    performanceMidiScratchBuffer.addEvents(midiMessages, 0, buffer.getNumSamples(), 0);
    performanceSurfaceMidiCollector.removeNextBlockOfMessages(performanceMidiScratchBuffer, buffer.getNumSamples());

    authoringPreviewMidiScratchBuffer.clear();
    authoringPreviewMidiCollector.removeNextBlockOfMessages(authoringPreviewMidiScratchBuffer, buffer.getNumSamples());

    applyPendingPerformanceActivationAtBlockBoundary();
    applyPendingAuthoringPreviewActivationAtBlockBoundary();

    if (performanceMidiScratchBuffer.isEmpty()
        && authoringPreviewMidiScratchBuffer.isEmpty()
        && performanceActiveVoices.empty()
        && authoringPreviewActiveVoices.empty())
    {
        ++realtimeSafetySnapshot.processBlockCount;
        realtimeSafetySnapshot.callbackBudgetMicros = static_cast<std::uint64_t>(
            std::llround(static_cast<double>(buffer.getNumSamples()) * 1000000.0 / std::max(currentSampleRate, 1.0)));
        const auto elapsedMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - blockStartTime).count());
        realtimeSafetySnapshot.lastProcessBlockMicros = elapsedMicros;
        realtimeSafetySnapshot.maxProcessBlockMicros = std::max(realtimeSafetySnapshot.maxProcessBlockMicros, elapsedMicros);
        if (elapsedMicros > realtimeSafetySnapshot.callbackBudgetMicros)
            ++realtimeSafetySnapshot.overBudgetCallbackCount;
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

        const auto message = metadata.getMessage();
        const auto voiceSource = useAuthoringMessage ? VoiceSource::authoringPreview : VoiceSource::performance;
        if (message.isNoteOn())
        {
            if (useAuthoringMessage)
                startAuthoringVoiceForMidiMessage(message);
            else
                startVoiceForMidiMessage(message);
        }
        else if (message.isNoteOff())
        {
            releaseVoicesForMidiNote(message.getNoteNumber(), voiceSource);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            clearVoices(voiceSource);
        }

        renderedSamples = eventSample;
    }

    renderBlockRange(buffer, renderedSamples, buffer.getNumSamples() - renderedSamples);

    ++realtimeSafetySnapshot.processBlockCount;
    realtimeSafetySnapshot.callbackBudgetMicros = static_cast<std::uint64_t>(
        std::llround(static_cast<double>(buffer.getNumSamples()) * 1000000.0 / std::max(currentSampleRate, 1.0)));
    const auto elapsedMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - blockStartTime).count());
    realtimeSafetySnapshot.lastProcessBlockMicros = elapsedMicros;
    realtimeSafetySnapshot.maxProcessBlockMicros = std::max(realtimeSafetySnapshot.maxProcessBlockMicros, elapsedMicros);
    if (elapsedMicros > realtimeSafetySnapshot.callbackBudgetMicros)
        ++realtimeSafetySnapshot.overBudgetCallbackCount;
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

    auto message = juce::MidiMessage::noteOn(1,
                                             clampMidiValue(midiNoteNumber),
                                             std::clamp(velocity, 0.0f, 1.0f));
    message.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    authoringPreviewMidiCollector.addMessageToQueue(message);
}

void Processor::queueAuthoringPreviewNoteOff(int midiNoteNumber)
{
    auto message = juce::MidiMessage::noteOff(1, clampMidiValue(midiNoteNumber));
    message.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    authoringPreviewMidiCollector.addMessageToQueue(message);
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
    drs::app::AuthoringPreviewStatusSnapshot status;
    status.available = realtimeSafetySnapshot.available;
    status.draftRevision = realtimeSafetySnapshot.currentAuthoringPreviewDraftRevision;
    status.activeRevision = realtimeSafetySnapshot.activeAuthoringPreviewRevision;
    status.pendingRevision = realtimeSafetySnapshot.pendingAuthoringPreviewRevision;
    status.revisionState = realtimeSafetySnapshot.authoringPreviewRevisionState;
    status.failureState = realtimeSafetySnapshot.authoringPreviewFailureState;
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
    auto message = juce::MidiMessage::noteOn(1,
                                             clampMidiValue(midiNoteNumber),
                                             std::clamp(velocity, 0.0f, 1.0f));
    message.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    performanceSurfaceMidiCollector.addMessageToQueue(message);
}

void Processor::queuePerformanceSurfaceNoteOff(int midiNoteNumber)
{
    auto message = juce::MidiMessage::noteOff(1, clampMidiValue(midiNoteNumber));
    message.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    performanceSurfaceMidiCollector.addMessageToQueue(message);
}

void Processor::clearReferencePlaybackCacheForTests()
{
    loadedSamples.clear();
    updateRealtimeSafetyState();
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
    const auto retiredCountBefore = realtimeSafetySnapshot.retiredActivationCount;
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
        || realtimeSafetySnapshot.retiredActivationCount != retiredCountBefore;
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
        referenceStream = engineFacade.loadPhase1ReferenceStream();

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
        ++realtimeSafetySnapshot.largeResourceReleasesOnAudioThread;

    loadedSamples.clear();

    if (!referenceManifest.loaded || !referenceStream.loaded)
    {
        realtimeSafetySnapshot.referenceSampleCountLoaded = 0;
        return;
    }

    loadedSamples.reserve(referenceStream.container.samples.size());

    for (const auto& sample : referenceStream.container.samples)
    {
        if (invokedFromAudioThread)
            ++realtimeSafetySnapshot.samplePathResolutionsOnAudioThread;
        const auto samplePath = resolveSamplePath(referenceStream.containerPath, sample.sourcePath);
        if (invokedFromAudioThread)
            ++realtimeSafetySnapshot.sampleDecodeEntriesOnAudioThread;
        const auto importResult = drs::engine::importSampleFile(samplePath.generic_string());
        if (!importResult.imported)
            continue;

        loadedSamples.emplace(sample.sampleId, LoadedReferenceSample { importResult.sample });
    }

    realtimeSafetySnapshot.referenceSampleCountLoaded = loadedSamples.size();
    if (!loadedSamples.empty())
    {
        if (invokedFromAudioThread)
            realtimeSafetySnapshot.referenceSampleLoadsOnAudioThread += loadedSamples.size();
        else
            ++realtimeSafetySnapshot.referenceWarmupCount;
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
        ++realtimeSafetySnapshot.sampleDecodeEntriesOnAudioThread;

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
        ++realtimeSafetySnapshot.authoringSampleLoadsOnAudioThread;
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

bool Processor::startAuthoringVoiceForMidiMessage(const juce::MidiMessage& message)
{
    const auto* activation = getActiveAuthoringPreviewActivation();
    if (activation == nullptr || !activation->ready || !activation->preparedSample)
        return false;

    const auto& sample = activation->preparedSample->sample;
    if (sample.normalizedChannels.empty() || sample.metadata.sampleRate <= 0.0)
        return false;

    ActiveRenderVoice voice;
    voice.renderVoiceId = nextRenderVoiceId++;
    voice.sourceMidiNote = message.getNoteNumber();
    voice.effectiveMidiNote = message.getNoteNumber();
    voice.effectiveVelocity = std::clamp(static_cast<int>(std::round(message.getFloatVelocity() * 127.0f)), 1, 127);
    voice.rootKey = activation->rootKey;
    voice.source = VoiceSource::authoringPreview;
    voice.zoneId = activation->zoneId;
    voice.sampleId = activation->sampleId;
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
        ++realtimeSafetySnapshot.activeVoiceCapacityGrowthCount;
    activeVoices.push_back(std::move(voice));

    updateRealtimeSafetyState();
    return true;
}

void Processor::startVoiceForMidiMessage(const juce::MidiMessage& message)
{
    const auto* activation = getActivePerformanceActivation();
    if (activation == nullptr || !activation->ready)
        return;

    if (!ensureReferencePlaybackAssetsLoaded(true))
        return;

    const auto& sessionState = activation->sessionState;
    const auto effectiveMidiNote = computeMotionRenderNote(sessionState, message.getNoteNumber());
    const auto effectiveVelocity = computeToneRenderVelocity(sessionState);

    drs::engine::RuntimeVoiceAllocationRequest request;
    request.voiceId = nextRenderVoiceId++;
    request.midiNote = effectiveMidiNote;
    request.velocity = effectiveVelocity;
    request.articulationId = sessionState.selectedArticulationId;

    const auto route = drs::engine::resolveRuntimeVoiceRoute(referenceManifest.instrument,
                                                             referenceStream.container,
                                                             request);
    if (!route.resolved)
        return;

    const auto* zone = findZone(referenceManifest.instrument, route.zoneId);
    if (zone == nullptr)
        return;

    const auto sampleIterator = loadedSamples.find(route.sampleId);
    if (sampleIterator == loadedSamples.end())
        return;

    const auto& sample = sampleIterator->second.sample;
    if (sample.normalizedChannels.empty() || sample.metadata.sampleRate <= 0.0)
        return;

    ActiveRenderVoice voice;
    voice.renderVoiceId = request.voiceId;
    voice.sourceMidiNote = message.getNoteNumber();
    voice.effectiveMidiNote = effectiveMidiNote;
    voice.effectiveVelocity = effectiveVelocity;
    voice.rootKey = zone->rootKey;
    voice.source = VoiceSource::performance;
    voice.zoneId = route.zoneId;
    voice.sampleId = route.sampleId;
    voice.loadedSample = &sampleIterator->second;
    voice.incrementFrames = std::pow(2.0, (effectiveMidiNote - zone->rootKey) / 12.0)
        * (sample.metadata.sampleRate / std::max(currentSampleRate, 1.0));
    voice.baseGain = 0.25f * (static_cast<float>(effectiveVelocity) / 127.0f);

    auto& activeVoices = performanceActiveVoices;
    if (activeVoices.size() >= maxRealtimeActiveVoices)
    {
        activeVoices.erase(activeVoices.begin());
    }
    else if (activeVoices.size() >= activeVoices.capacity())
        ++realtimeSafetySnapshot.activeVoiceCapacityGrowthCount;
    activeVoices.push_back(std::move(voice));

    updateRealtimeSafetyState();
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
    failedAuthoringPreviewRevision = std::numeric_limits<std::size_t>::max();
    failedAuthoringPreviewState.clear();

    if (installImmediately && activeAuthoringPreviewActivationSlotIndex.load(std::memory_order_acquire) < 0)
    {
        activeAuthoringPreviewActivationSlotIndex.store(slotIndex, std::memory_order_release);
        ++realtimeSafetySnapshot.authoringPreviewActivationCount;
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
        ++realtimeSafetySnapshot.authoringPreviewActivationCount;

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
    retiredAuthoringPreviewActivationWriteIndex.store(nextWriteIndex, std::memory_order_release);
    return true;
}

void Processor::drainRetiredAuthoringPreviewActivationSlots()
{
    auto readIndex = retiredAuthoringPreviewActivationReadIndex.load(std::memory_order_relaxed);
    const auto writeIndex = retiredAuthoringPreviewActivationWriteIndex.load(std::memory_order_acquire);
    while (readIndex != writeIndex)
    {
        releaseAuthoringPreviewActivationSlot(retiredAuthoringPreviewActivationSlots[readIndex]);
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

    if (processingAudioCallback)
        ++realtimeSafetySnapshot.largeResourceReleasesOnAudioThread;

    authoringPreviewActivationSlots[static_cast<std::size_t>(slotIndex)] = {};
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
    activation.sessionState = sessionState;

    if (installImmediately && activePerformanceActivationSlotIndex.load(std::memory_order_acquire) < 0)
    {
        activePerformanceActivationSlotIndex.store(slotIndex, std::memory_order_release);
        ++realtimeSafetySnapshot.performanceActivationCount;
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
        clearVoices(VoiceSource::performance, false);
        const auto retiredSlotIndex = activePerformanceActivationSlotIndex.exchange(
            pendingSlotIndex,
            std::memory_order_acq_rel);
        ++realtimeSafetySnapshot.performanceActivationCount;

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
    retiredPerformanceActivationWriteIndex.store(nextWriteIndex, std::memory_order_release);
    return true;
}

void Processor::drainRetiredPerformanceActivationSlots()
{
    auto readIndex = retiredPerformanceActivationReadIndex.load(std::memory_order_relaxed);
    const auto writeIndex = retiredPerformanceActivationWriteIndex.load(std::memory_order_acquire);
    while (readIndex != writeIndex)
    {
        releasePerformanceActivationSlot(retiredPerformanceActivationSlots[readIndex]);
        ++realtimeSafetySnapshot.retiredActivationCount;
        readIndex = (readIndex + 1u) % static_cast<std::uint32_t>(retiredActivationQueueCapacity);
    }

    retiredPerformanceActivationReadIndex.store(readIndex, std::memory_order_release);
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

    if (processingAudioCallback)
        ++realtimeSafetySnapshot.largeResourceReleasesOnAudioThread;

    performanceActivationSlots[static_cast<std::size_t>(slotIndex)] = {};
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

    auto trimFinishedVoices = [](std::vector<ActiveRenderVoice>& activeVoices)
    {
        activeVoices.erase(std::remove_if(activeVoices.begin(),
                                          activeVoices.end(),
                                          [](const auto& voice)
                                          {
                                              if (voice.loadedSample == nullptr)
                                                  return true;

                                              const auto finishedRelease = voice.releasing && voice.releaseSamplesRemaining <= 0;
                                              const auto frameCount = static_cast<double>(voice.loadedSample->sample.metadata.frameCount);
                                              return finishedRelease || voice.positionFrames >= frameCount;
                                          }),
                          activeVoices.end());
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

void Processor::primeRealtimeSafetyState(int samplesPerBlock)
{
    performanceActiveVoices.reserve(maxRealtimeActiveVoices);
    authoringPreviewActiveVoices.reserve(maxRealtimeActiveVoices);
    performanceMidiScratchBuffer.ensureSize(std::max<std::size_t>(1024, static_cast<std::size_t>(samplesPerBlock) * 16));
    authoringPreviewMidiScratchBuffer.ensureSize(std::max<std::size_t>(1024, static_cast<std::size_t>(samplesPerBlock) * 16));
    realtimeSafetySnapshot.preparedBlockSize = static_cast<std::size_t>(samplesPerBlock);
    realtimeSafetySnapshot.activeVoiceCapacityLimit = maxRealtimeActiveVoices * 2;
    updateRealtimeSafetyState();
}

void Processor::updateRealtimeSafetyState()
{
    realtimeSafetySnapshot.available = true;
    realtimeSafetySnapshot.performanceActiveVoiceCount = performanceActiveVoices.size();
    realtimeSafetySnapshot.authoringPreviewActiveVoiceCount = authoringPreviewActiveVoices.size();
    realtimeSafetySnapshot.activeVoiceCapacity =
        performanceActiveVoices.capacity() + authoringPreviewActiveVoices.capacity();
    realtimeSafetySnapshot.referenceSampleCountLoaded = loadedSamples.size();
    realtimeSafetySnapshot.currentAuthoringPreviewDraftRevision = authoringSession.getDocumentState().revision;
    const auto activeAuthoringPreviewSlotIndex = activeAuthoringPreviewActivationSlotIndex.load(std::memory_order_acquire);
    const auto pendingAuthoringPreviewSlotIndex = pendingAuthoringPreviewActivationSlotIndex.load(std::memory_order_acquire);
    const auto activeSlotIndex = activePerformanceActivationSlotIndex.load(std::memory_order_acquire);
    const auto pendingSlotIndex = pendingPerformanceActivationSlotIndex.load(std::memory_order_acquire);
    const auto hasActiveAuthoringPreviewActivation = activeAuthoringPreviewSlotIndex >= 0
        && authoringPreviewActivationSlots[static_cast<std::size_t>(activeAuthoringPreviewSlotIndex)].ready;
    const auto hasPendingAuthoringPreviewActivation = pendingAuthoringPreviewSlotIndex >= 0
        && authoringPreviewActivationSlots[static_cast<std::size_t>(pendingAuthoringPreviewSlotIndex)].ready;
    const auto hasActiveActivation = activeSlotIndex >= 0
        && performanceActivationSlots[static_cast<std::size_t>(activeSlotIndex)].ready;
    const auto hasPendingActivation = pendingSlotIndex >= 0
        && performanceActivationSlots[static_cast<std::size_t>(pendingSlotIndex)].ready;
    realtimeSafetySnapshot.activeAuthoringPreviewRevision = activeAuthoringPreviewSlotIndex >= 0
        ? authoringPreviewActivationSlots[static_cast<std::size_t>(activeAuthoringPreviewSlotIndex)].projectRevision
        : 0;
    realtimeSafetySnapshot.pendingAuthoringPreviewRevision = pendingAuthoringPreviewSlotIndex >= 0
        ? authoringPreviewActivationSlots[static_cast<std::size_t>(pendingAuthoringPreviewSlotIndex)].projectRevision
        : 0;
    realtimeSafetySnapshot.activePublishedRevision = activeSlotIndex >= 0
        ? performanceActivationSlots[static_cast<std::size_t>(activeSlotIndex)].publishedRevision
        : 0;
    realtimeSafetySnapshot.pendingPublishedRevision = pendingSlotIndex >= 0
        ? performanceActivationSlots[static_cast<std::size_t>(pendingSlotIndex)].publishedRevision
        : 0;
    realtimeSafetySnapshot.activePreparedBuildId = activeSlotIndex >= 0
        ? performanceActivationSlots[static_cast<std::size_t>(activeSlotIndex)].preparedBuildId
        : 0;
    realtimeSafetySnapshot.pendingPreparedBuildId = pendingSlotIndex >= 0
        ? performanceActivationSlots[static_cast<std::size_t>(pendingSlotIndex)].preparedBuildId
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
    realtimeSafetySnapshot.retiredActivationBacklog = retiredPerformanceBacklog + retiredPreviewBacklog;
    realtimeSafetySnapshot.authoringPreviewFailureState =
        failedAuthoringPreviewRevision == realtimeSafetySnapshot.currentAuthoringPreviewDraftRevision
            ? failedAuthoringPreviewState
            : std::string {};

    if (pendingAuthoringPreviewSlotIndex >= 0
        && realtimeSafetySnapshot.pendingAuthoringPreviewRevision == realtimeSafetySnapshot.currentAuthoringPreviewDraftRevision)
    {
        realtimeSafetySnapshot.authoringPreviewRevisionState = "Preparing";
    }
    else if (failedAuthoringPreviewRevision == realtimeSafetySnapshot.currentAuthoringPreviewDraftRevision)
    {
        realtimeSafetySnapshot.authoringPreviewRevisionState = "Failed";
    }
    else if (hasActiveAuthoringPreviewActivation
             && realtimeSafetySnapshot.activeAuthoringPreviewRevision == realtimeSafetySnapshot.currentAuthoringPreviewDraftRevision)
    {
        realtimeSafetySnapshot.authoringPreviewRevisionState = "Ready";
    }
    else if (hasActiveAuthoringPreviewActivation)
    {
        realtimeSafetySnapshot.authoringPreviewRevisionState = "Stale";
    }
    else
    {
        realtimeSafetySnapshot.authoringPreviewRevisionState = "Idle";
    }

    if (realtimeSafetySnapshot.getAudioThreadViolationCount() > 0)
    {
        realtimeSafetySnapshot.state = "Realtime callback violations recorded";
        return;
    }

    if (realtimeSafetySnapshot.referenceSampleCountLoaded == 0)
    {
        realtimeSafetySnapshot.state = "Reference playback cache unavailable";
        return;
    }

    if (!hasActiveActivation && hasPendingActivation)
    {
        realtimeSafetySnapshot.state = "Published activation pending";
        return;
    }

    if (!hasActiveAuthoringPreviewActivation && hasPendingAuthoringPreviewActivation)
    {
        realtimeSafetySnapshot.state = "Authoring preview activation pending";
        return;
    }

    if (!hasActiveActivation)
    {
        realtimeSafetySnapshot.state = "Published activation unavailable";
        return;
    }

    realtimeSafetySnapshot.state = "Realtime callback primed";
}
} // namespace drs::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new drs::plugin::Processor();
}
