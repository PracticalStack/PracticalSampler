#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
class TestPlayHead final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        return position;
    }

    void setPosition(bool playing, std::int64_t timeInSamples)
    {
        position = PositionInfo {};
        position.setIsPlaying(playing);
        position.setTimeInSamples(timeInSamples);
    }

private:
    PositionInfo position;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void settleWorker(drs::plugin::Processor& processor, const std::string& shellLabel)
{
    require(processor.getEngineFacade().waitForPreparedPlaybackIdle(),
            shellLabel + " should settle prepared playback work off the audio thread.");
    processor.serviceMessageThreadWork();
}

void settleAuthoringPreview(drs::plugin::Processor& processor,
                            const std::string& shellLabel)
{
    processor.requestAuthoringPreview(drs::engine::AuthoringPreviewScope::selectedZone);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processor.getEngineFacade().waitForPreparedPlaybackIdle(std::chrono::milliseconds(50));

        auto controller = processor.getAuthoringPreviewControllerSnapshot();
        if (controller.activationState == drs::engine::AuthoringPreviewActivationState::pending)
        {
            juce::AudioBuffer<float> activationBuffer(2, 64);
            juce::MidiBuffer emptyMidi;
            processor.processBlock(activationBuffer, emptyMidi);
            processor.serviceMessageThreadWork();
            controller = processor.getAuthoringPreviewControllerSnapshot();
        }

        if (controller.activationState == drs::engine::AuthoringPreviewActivationState::active)
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    require(false, shellLabel + " should activate the requested authored Preview.");
}

float magnitude(const juce::AudioBuffer<float>& buffer, int start, int length)
{
    auto result = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        result = std::max(result, buffer.getMagnitude(channel, start, length));
    return result;
}

void drainPreviewRelease(drs::plugin::Processor& processor, int note)
{
    processor.queueAuthoringPreviewNoteOff(note);
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> buffer(2, 256);
    for (int block = 0; block < 12; ++block)
        processor.processBlock(buffer, midi);
}

struct ShellResult
{
    std::size_t firstRevision = 0;
    std::size_t replacementRevision = 0;
    std::uint64_t firstPreparedBuildId = 0;
    std::uint64_t replacementPreparedBuildId = 0;
    std::string firstDigest;
    std::string replacementDigest;
    float previewMagnitude = 0.0f;
    float hostMagnitude = 0.0f;
    std::uint64_t activePayloadBytes = 0;
    std::size_t activeVoiceCapacity = 0;
};

ShellResult runShell(drs::plugin::Processor& processor,
                     const drs::engine::RuntimeProjectModel& project,
                     const std::string& shellLabel)
{
    processor.prepareToPlay(48000.0, 256);

    // No immutable Performance route means silence. The callback must never reconstruct one.
    juce::AudioBuffer<float> noRouteBuffer(2, 256);
    juce::MidiBuffer noRouteMidi;
    noRouteMidi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 31);
    processor.processBlock(noRouteBuffer, noRouteMidi);
    require(magnitude(noRouteBuffer, 0, noRouteBuffer.getNumSamples()) <= 0.000001f,
            shellLabel + " should render silence when no Performance activation exists.");
    auto diagnostics = processor.getRealtimeSafetySnapshot();
    require(diagnostics.getAudioThreadViolationCount() == 0
                && diagnostics.samplePathResolutionsOnAudioThread == 0
                && diagnostics.sampleDecodeEntriesOnAudioThread == 0,
            shellLabel + " no-route callback must not enter fixture lookup, path, or decode work.");

    processor.replaceAuthoringProject(project);
    require(processor.getAuthoringSession().selectZone("pad-a3-high").applied,
            shellLabel + " should select the authored Preview route.");
    require(processor.serviceMessageThreadWork(),
            shellLabel + " should normalize the authored project on the message/worker side.");
    const auto selectedZone = processor.getAuthoringSession().getSelectedZone();
    require(selectedZone.has_value(), shellLabel + " should retain the selected authored zone.");

    settleAuthoringPreview(processor, shellLabel);
    const auto previewPayload = processor.getEngineFacade().getPreviewActivationPayload();
    require(previewPayload != nullptr
                && previewPayload->lane == drs::engine::PlaybackActivationLane::preview
                && previewPayload->snapshot != nullptr
                && previewPayload->prepared != nullptr,
            shellLabel + " should expose a normalized immutable Preview activation.");

    processor.queueAuthoringPreviewNoteOn(selectedZone->rootKey, 0.75f);
    juce::AudioBuffer<float> previewBuffer(2, 256);
    juce::MidiBuffer emptyMidi;
    processor.processBlock(previewBuffer, emptyMidi);
    const auto previewMagnitude = magnitude(previewBuffer, 0, previewBuffer.getNumSamples());
    require(previewMagnitude > 0.0001f,
            shellLabel + " should render authored Preview through the product renderer.");
    diagnostics = processor.getRealtimeSafetySnapshot();
    require(diagnostics.authoringPreviewActiveVoiceCount > 0
                && diagnostics.performanceActiveVoiceCount == 0,
            shellLabel + " Preview events must remain isolated from Performance voice ownership.");
    drainPreviewRelease(processor, selectedZone->rootKey);

    require(processor.getEngineFacade().publishCurrentDraft(),
            shellLabel + " should accept Performance preparation.");
    settleWorker(processor, shellLabel);
    const auto firstPayload = processor.getEngineFacade().getPerformanceActivationPayload();
    require(firstPayload != nullptr
                && firstPayload->lane == drs::engine::PlaybackActivationLane::performance
                && firstPayload->snapshot != nullptr
                && firstPayload->prepared != nullptr,
            shellLabel + " should expose a normalized immutable Performance activation.");

    // Host MIDI must retain its exact sample offset through shell-to-core translation.
    constexpr int eventOffset = 73;
    juce::AudioBuffer<float> hostBuffer(2, 256);
    juce::MidiBuffer hostMidi;
    hostMidi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), eventOffset);
    processor.processBlock(hostBuffer, hostMidi);
    require(magnitude(hostBuffer, 0, eventOffset) <= 0.000001f,
            shellLabel + " should remain silent before the host MIDI sample offset.");
    const auto hostMagnitude = magnitude(hostBuffer, eventOffset,
                                         hostBuffer.getNumSamples() - eventOffset);
    require(hostMagnitude > 0.0001f,
            shellLabel + " should render host MIDI after its exact sample offset.");

    TestPlayHead playHead;
    processor.setPlayHead(&playHead);
    playHead.setPosition(true, 0);
    juce::AudioBuffer<float> transportStartBuffer(2, 256);
    juce::MidiBuffer transportStartMidi;
    transportStartMidi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 0);
    processor.processBlock(transportStartBuffer, transportStartMidi);
    diagnostics = processor.getRealtimeSafetySnapshot();
    require(diagnostics.performanceActiveVoiceCount > 0,
            shellLabel + " should keep a host-started looped voice active before a transport seek.");

    playHead.setPosition(true, 256);
    juce::AudioBuffer<float> transportAdvanceBuffer(2, 256);
    processor.processBlock(transportAdvanceBuffer, emptyMidi);
    diagnostics = processor.getRealtimeSafetySnapshot();
    require(diagnostics.performanceActiveVoiceCount > 0,
            shellLabel + " should keep the sustained Performance voice active during contiguous transport.");

    playHead.setPosition(true, 0);
    juce::AudioBuffer<float> transportLoopBuffer(2, 256);
    processor.processBlock(transportLoopBuffer, emptyMidi);
    diagnostics = processor.getRealtimeSafetySnapshot();
    require(diagnostics.performanceActiveVoiceCount == 0,
            shellLabel + " should clear stale Performance voices when the host loops or seeks backward.");

    juce::AudioBuffer<float> transportRestartBuffer(2, 256);
    juce::MidiBuffer transportRestartMidi;
    transportRestartMidi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 0);
    processor.processBlock(transportRestartBuffer, transportRestartMidi);
    require(magnitude(transportRestartBuffer, 0, transportRestartBuffer.getNumSamples()) > 0.0001f,
            shellLabel + " should still render the restarted note immediately after a host loop reset.");
    processor.setPlayHead(nullptr);

    const auto replacementRevision = firstPayload->revision + 1;
    require(processor.getEngineFacade().stageDraftRevision(replacementRevision)
                && processor.getEngineFacade().refreshPreviewToCurrentDraft(),
            shellLabel + " should stage replacement authored content.");
    settleWorker(processor, shellLabel);
    require(processor.getEngineFacade().publishCurrentDraft(),
            shellLabel + " should accept replacement Performance preparation.");
    settleWorker(processor, shellLabel);
    const auto replacementPayload = processor.getEngineFacade().getPerformanceActivationPayload();
    require(replacementPayload != nullptr
                && replacementPayload.get() != firstPayload.get()
                && replacementPayload->revision == replacementRevision
                && replacementPayload->preparedBuildId != firstPayload->preparedBuildId,
            shellLabel + " should replace the Performance payload by immutable identity.");

    juce::AudioBuffer<float> activationBuffer(2, 256);
    processor.processBlock(activationBuffer, emptyMidi);
    diagnostics = processor.getRealtimeSafetySnapshot();
    require(diagnostics.activePublishedRevision == replacementRevision
                && diagnostics.pendingPublishedRevision == 0
                && diagnostics.activePreparedBuildId == replacementPayload->preparedBuildId
                && diagnostics.activeActivationPayloadBytes >= replacementPayload->retainedPreparedBytes,
            shellLabel + " should apply payload replacement only at the block boundary.");
    require(diagnostics.getRealtimeGuardFailureCount() == 0,
            shellLabel + " cutover path should preserve realtime diagnostics parity.");

    return { firstPayload->revision,
             replacementRevision,
             firstPayload->preparedBuildId,
             replacementPayload->preparedBuildId,
             firstPayload->preparedContentDigest,
             replacementPayload->preparedContentDigest,
             previewMagnitude,
             hostMagnitude,
             diagnostics.activeActivationPayloadBytes,
             diagnostics.activeVoiceCapacity };
}

void requireParity(const ShellResult& standalone, const ShellResult& plugin)
{
    require(standalone.firstRevision == plugin.firstRevision
                && standalone.replacementRevision == plugin.replacementRevision,
            "Standalone and editor-closed plugin shells should activate identical revisions.");
    require(standalone.firstPreparedBuildId == plugin.firstPreparedBuildId
                && standalone.replacementPreparedBuildId == plugin.replacementPreparedBuildId,
            "Standalone and editor-closed plugin shells should activate identical prepared builds.");
    require(standalone.firstDigest == plugin.firstDigest
                && standalone.replacementDigest == plugin.replacementDigest,
            "Standalone and editor-closed plugin shells should consume identical immutable content.");
    require(std::abs(standalone.previewMagnitude - plugin.previewMagnitude) <= 0.000001f
                && std::abs(standalone.hostMagnitude - plugin.hostMagnitude) <= 0.000001f,
            "Standalone and editor-closed plugin shells should render the same Preview and Performance output.");
    require(standalone.activePayloadBytes == plugin.activePayloadBytes
                && standalone.activeVoiceCapacity == plugin.activeVoiceCapacity,
            "Standalone and editor-closed plugin shells should report matching activation diagnostics.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        const auto project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(project.loaded, "Sprint 4.6 shell cutover requires the authored project fixture.");

        drs::standalone::MainComponent standalone(false);
        require(!standalone.isAudioOutputEnabled(),
                "Sprint 4.6 standalone coverage must remain headless.");
        const auto standaloneResult = runShell(standalone.getProcessor(), project.project, "Standalone shell");

        // Deliberately leave the editor closed: plugin playback must not depend on UI lifetime.
        drs::plugin::Processor editorClosedPlugin;
        const auto pluginResult = runShell(editorClosedPlugin, project.project, "Editor-closed plugin shell");

        requireParity(standaloneResult, pluginResult);
        std::cout << "Sprint 4.6 route normalization and shell cutover matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 4.6 shell cutover failed: " << exception.what() << std::endl;
        return 1;
    }
}
