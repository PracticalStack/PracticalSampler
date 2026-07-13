#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

#include "drs/engine/RuntimeVoice.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
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
} // namespace

Processor::Processor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameterState(*this, nullptr, "macroParameters", buildParameterLayout(engineFacade))
{
    performanceSurfaceMidiCollector.reset(currentSampleRate);
    referenceManifest = engineFacade.loadPhase1ReferenceInstrument();
    referenceStream = engineFacade.loadPhase1ReferenceStream();
    initializeReferencePlaybackAssets();

    for (const auto& macro : engineFacade.getMacroDescriptors())
        parameterState.addParameterListener(buildMacroParameterId(macro.id), this);

    syncParametersFromEngine();
}

Processor::~Processor()
{
    for (const auto& macro : engineFacade.getMacroDescriptors())
        parameterState.removeParameterListener(buildMacroParameterId(macro.id), this);
}

void Processor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    performanceSurfaceMidiCollector.reset(currentSampleRate);
    activeVoices.clear();

    if (loadedSamples.empty())
        initializeReferencePlaybackAssets();
}

void Processor::releaseResources()
{
    activeVoices.clear();
}

bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    if (!referenceManifest.loaded || !referenceStream.loaded || loadedSamples.empty())
        return;

    juce::MidiBuffer combinedMidiMessages;
    combinedMidiMessages.addEvents(midiMessages, 0, buffer.getNumSamples(), 0);
    performanceSurfaceMidiCollector.removeNextBlockOfMessages(combinedMidiMessages, buffer.getNumSamples());

    int renderedSamples = 0;

    for (const auto metadata : combinedMidiMessages)
    {
        const auto eventSample = std::clamp(metadata.samplePosition, 0, buffer.getNumSamples());
        renderBlockRange(buffer, renderedSamples, eventSample - renderedSamples);

        const auto message = metadata.getMessage();
        if (message.isNoteOn())
            startVoiceForMidiMessage(message);
        else if (message.isNoteOff())
            releaseVoicesForMidiNote(message.getNoteNumber());
        else if (message.isAllNotesOff() || message.isAllSoundOff())
            activeVoices.clear();

        renderedSamples = eventSample;
    }

    renderBlockRange(buffer, renderedSamples, buffer.getNumSamples() - renderedSamples);
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

void Processor::initializeReferencePlaybackAssets()
{
    loadedSamples.clear();

    if (!referenceManifest.loaded || !referenceStream.loaded)
        return;

    for (const auto& sample : referenceStream.container.samples)
    {
        const auto samplePath = resolveSamplePath(referenceStream.containerPath, sample.sourcePath);
        const auto importResult = drs::engine::importSampleFile(samplePath.generic_string());
        if (!importResult.imported)
            continue;

        loadedSamples.emplace(sample.sampleId, LoadedReferenceSample { importResult.sample });
    }
}

void Processor::startVoiceForMidiMessage(const juce::MidiMessage& message)
{
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
    voice.zoneId = route.zoneId;
    voice.sampleId = route.sampleId;
    voice.loadedSample = &sampleIterator->second;
    voice.incrementFrames = std::pow(2.0, (effectiveMidiNote - zone->rootKey) / 12.0)
        * (sample.metadata.sampleRate / std::max(currentSampleRate, 1.0));
    voice.baseGain = 0.25f * (static_cast<float>(effectiveVelocity) / 127.0f);

    activeVoices.push_back(std::move(voice));

    constexpr std::size_t maxActiveVoices = 24;
    if (activeVoices.size() > maxActiveVoices)
    {
        activeVoices.erase(activeVoices.begin(),
                           activeVoices.begin() + static_cast<std::ptrdiff_t>(activeVoices.size() - maxActiveVoices));
    }
}

void Processor::releaseVoicesForMidiNote(int midiNoteNumber)
{
    for (auto& voice : activeVoices)
    {
        if (voice.sourceMidiNote != midiNoteNumber || voice.releasing)
            continue;

        voice.releasing = true;
        voice.releaseSamplesTotal = 2048;
        voice.releaseSamplesRemaining = voice.releaseSamplesTotal;
    }
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
}
} // namespace drs::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new drs::plugin::Processor();
}
