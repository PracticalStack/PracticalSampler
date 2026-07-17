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
    activeVoices.clear();
    primeRealtimeSafetyState(samplesPerBlock > 0 ? samplesPerBlock : 512);
    ensureReferencePlaybackAssetsLoaded(false);
    updateRealtimeSafetyState();
}

void Processor::releaseResources()
{
    activeVoices.clear();
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

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    performanceMidiScratchBuffer.clear();
    performanceMidiScratchBuffer.addEvents(midiMessages, 0, buffer.getNumSamples(), 0);
    performanceSurfaceMidiCollector.removeNextBlockOfMessages(performanceMidiScratchBuffer, buffer.getNumSamples());

    authoringPreviewMidiScratchBuffer.clear();
    authoringPreviewMidiCollector.removeNextBlockOfMessages(authoringPreviewMidiScratchBuffer, buffer.getNumSamples());

    if (performanceMidiScratchBuffer.isEmpty() && authoringPreviewMidiScratchBuffer.isEmpty() && activeVoices.empty())
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

drs::app::AuthoringImportResponsivenessSnapshot Processor::getAuthoringImportResponsivenessSnapshot() const
{
    return authoringImportResponsivenessSnapshot;
}

void Processor::replaceAuthoringProject(drs::engine::RuntimeProjectModel project)
{
    authoringSession.replaceProject(std::move(project));
    authoringLoadedSamples.clear();
    authoringWaveformPreviewCache.clear();
    activeVoices.clear();
    initializeAuthoringImportMetrics();
    ensureSelectedAuthoringSampleLoaded(false);
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
    loadedSamples.clear();

    if (!referenceManifest.loaded || !referenceStream.loaded)
    {
        realtimeSafetySnapshot.referenceSampleCountLoaded = 0;
        return;
    }

    loadedSamples.reserve(referenceStream.container.samples.size());

    for (const auto& sample : referenceStream.container.samples)
    {
        const auto samplePath = resolveSamplePath(referenceStream.containerPath, sample.sourcePath);
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
        return false;

    const auto projectSampleSource = findProjectSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (!projectSampleSource.has_value())
        return false;

    if (authoringLoadedSamples.find(projectSampleSource->id) != authoringLoadedSamples.end())
        return true;

    const auto importResult = drs::engine::importSampleFile(projectSampleSource->path);
    if (!importResult.imported)
        return false;

    authoringLoadedSamples.emplace(projectSampleSource->id, LoadedReferenceSample { importResult.sample });
    if (invokedFromAudioThread)
        ++realtimeSafetySnapshot.authoringSampleLoadsOnAudioThread;
    updateRealtimeSafetyState();
    return true;
}

bool Processor::startAuthoringVoiceForMidiMessage(const juce::MidiMessage& message)
{
    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
        return false;

    const auto projectSampleSource = findProjectSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (!projectSampleSource.has_value())
        return false;

    if (!ensureSelectedAuthoringSampleLoaded(true))
        return false;

    auto sampleIterator = authoringLoadedSamples.find(projectSampleSource->id);
    if (sampleIterator == authoringLoadedSamples.end())
        return false;

    const auto& sample = sampleIterator->second.sample;
    if (sample.normalizedChannels.empty() || sample.metadata.sampleRate <= 0.0)
        return false;

    ActiveRenderVoice voice;
    voice.renderVoiceId = nextRenderVoiceId++;
    voice.sourceMidiNote = message.getNoteNumber();
    voice.effectiveMidiNote = message.getNoteNumber();
    voice.effectiveVelocity = std::clamp(static_cast<int>(std::round(message.getFloatVelocity() * 127.0f)), 1, 127);
    voice.rootKey = selectedZone->rootKey;
    voice.source = VoiceSource::authoringPreview;
    voice.zoneId = selectedZone->id;
    voice.sampleId = projectSampleSource->id;
    voice.loadedSample = &sampleIterator->second;
    voice.incrementFrames = std::pow(2.0, (voice.effectiveMidiNote - selectedZone->rootKey) / 12.0)
        * (sample.metadata.sampleRate / std::max(currentSampleRate, 1.0));
    voice.baseGain = 0.25f * (static_cast<float>(voice.effectiveVelocity) / 127.0f)
        * static_cast<float>(std::pow(10.0, selectedZone->gainDb / 20.0));

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
    if (!ensureReferencePlaybackAssetsLoaded(true))
        return;

    const auto sessionState = engineFacade.getCurrentSessionState();
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
    for (auto& voice : activeVoices)
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
    activeVoices.erase(std::remove_if(activeVoices.begin(),
                                      activeVoices.end(),
                                      [source](const ActiveRenderVoice& voice)
                                      {
                                          return voice.source == source;
                                      }),
                       activeVoices.end());
    updateRealtimeSafetyState();
}

void Processor::renderBlockRange(juce::AudioBuffer<float>& buffer, int startSample, int sampleCount)
{
    if (sampleCount <= 0)
        return;

    auto* left = buffer.getNumChannels() > 0 ? buffer.getWritePointer(0, startSample) : nullptr;
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1, startSample) : nullptr;

    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        float mixedLeft = 0.0f;
        float mixedRight = 0.0f;

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

        if (left != nullptr)
            left[sampleIndex] += mixedLeft;
        if (right != nullptr)
            right[sampleIndex] += mixedRight;
    }

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
    updateRealtimeSafetyState();
}

void Processor::primeRealtimeSafetyState(int samplesPerBlock)
{
    activeVoices.reserve(maxRealtimeActiveVoices);
    performanceMidiScratchBuffer.ensureSize(std::max<std::size_t>(1024, static_cast<std::size_t>(samplesPerBlock) * 16));
    authoringPreviewMidiScratchBuffer.ensureSize(std::max<std::size_t>(1024, static_cast<std::size_t>(samplesPerBlock) * 16));
    realtimeSafetySnapshot.preparedBlockSize = static_cast<std::size_t>(samplesPerBlock);
    realtimeSafetySnapshot.activeVoiceCapacityLimit = maxRealtimeActiveVoices;
    updateRealtimeSafetyState();
}

void Processor::updateRealtimeSafetyState()
{
    realtimeSafetySnapshot.available = true;
    realtimeSafetySnapshot.activeVoiceCapacity = activeVoices.capacity();
    realtimeSafetySnapshot.referenceSampleCountLoaded = loadedSamples.size();

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

    realtimeSafetySnapshot.state = "Realtime callback primed";
}
} // namespace drs::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new drs::plugin::Processor();
}
