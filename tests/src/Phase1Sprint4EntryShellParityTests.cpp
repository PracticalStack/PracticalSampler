#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct ShellGateResult
{
    std::size_t revision = 0;
    std::uint64_t previewBuildId = 0;
    std::uint64_t performanceBuildId = 0;
    std::string previewDigest;
    std::string performanceDigest;
    std::size_t previewSampleCount = 0;
    std::size_t performanceSampleCount = 0;
    std::uint64_t previewRetainedBytes = 0;
    std::uint64_t performanceRetainedBytes = 0;
    float previewMagnitude = 0.0f;
    float performanceMagnitude = 0.0f;
};

float renderPreview(drs::plugin::Processor& processor, int midiNote)
{
    processor.queueAuthoringPreviewNoteOn(midiNote, 0.75f);
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
    processor.queueAuthoringPreviewNoteOff(midiNote);
    return buffer.getMagnitude(0, buffer.getNumSamples());
}

float renderPerformance(drs::plugin::Processor& processor, int midiNote)
{
    processor.queuePerformanceSurfaceNoteOn(midiNote, 0.8f);
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
    processor.queuePerformanceSurfaceNoteOff(midiNote);
    return buffer.getMagnitude(0, buffer.getNumSamples());
}

ShellGateResult runPreviewPublishScenario(drs::plugin::Processor& processor,
                                          const drs::engine::RuntimeProjectModel& project,
                                          const std::string& shellLabel)
{
    processor.prepareToPlay(48000.0, 512);
    processor.replaceAuthoringProject(project);
    const auto selection = processor.getAuthoringSession().selectZone("pad-a3-high");
    require(selection.applied, shellLabel + " should select the authored preview zone.");
    require(processor.serviceMessageThreadWork(),
            shellLabel + " should synchronize the selected authored revision before preparation.");
    const auto selectedZone = processor.getAuthoringSession().getSelectedZone();
    require(selectedZone.has_value(), shellLabel + " should retain the selected authored zone.");
    const auto revision = processor.getAuthoringSession().getDocumentState().revision;

    require(processor.getEngineFacade().refreshPreviewToCurrentDraft(),
            shellLabel + " should accept Preview preparation.");
    require(processor.getEngineFacade().waitForPreparedPlaybackIdle(),
            shellLabel + " should settle Preview preparation on the worker.");
    require(processor.serviceMessageThreadWork(),
            shellLabel + " should apply the Preview completion and stage activation.");

    const auto previewPayload = processor.getEngineFacade().getPreviewActivationPayload();
    require(previewPayload != nullptr && previewPayload->activationEligible,
            shellLabel + " should retain an activation-eligible Preview payload.");
    require(previewPayload->lane == drs::engine::PlaybackActivationLane::preview
                && previewPayload->revision == revision,
            shellLabel + " Preview payload should identify the current authored revision.");
    require(previewPayload->snapshot != nullptr && previewPayload->prepared != nullptr,
            shellLabel + " Preview payload should retain immutable snapshot and prepared handles.");

    const auto previewMagnitude = renderPreview(processor, selectedZone->rootKey);
    require(previewMagnitude > 0.0001f,
            shellLabel + " should render Preview audio through the retained payload activation.");
    const auto retainedPreviewPayload = processor.getEngineFacade().getPreviewActivationPayload();
    require(retainedPreviewPayload == previewPayload,
            shellLabel + " Preview payload should survive completion-queue drain and playback.");

    require(processor.getEngineFacade().publishCurrentDraft(),
            shellLabel + " should accept Publish preparation.");
    require(processor.getEngineFacade().waitForPreparedPlaybackIdle(),
            shellLabel + " should settle Publish preparation on the worker.");
    require(processor.serviceMessageThreadWork(),
            shellLabel + " should apply the Publish completion and stage activation.");

    const auto performancePayload = processor.getEngineFacade().getPerformanceActivationPayload();
    require(performancePayload != nullptr && performancePayload->activationEligible,
            shellLabel + " should retain an activation-eligible Performance payload.");
    require(performancePayload->lane == drs::engine::PlaybackActivationLane::performance
                && performancePayload->revision == revision,
            shellLabel + " Performance payload should identify the current authored revision.");
    require(performancePayload->snapshot != nullptr && performancePayload->prepared != nullptr,
            shellLabel + " Performance payload should retain immutable snapshot and prepared handles.");

    const auto performanceMagnitude = renderPerformance(processor, 57);
    require(performanceMagnitude > 0.0001f,
            shellLabel + " should render editor-independent Performance audio.");
    require(processor.getEngineFacade().getPerformanceActivationPayload() == performancePayload,
            shellLabel + " Performance payload should survive playback and queue drain.");

    const auto diagnostics = processor.getRealtimeSafetySnapshot();
    require(diagnostics.getRealtimeGuardFailureCount() == 0,
            shellLabel + " should complete Preview/Publish playback without realtime guard failures.");
    require(diagnostics.activePreparedBuildId == performancePayload->preparedBuildId,
            shellLabel + " processor activation should expose the retained Performance build identity.");

    return {
        revision,
        previewPayload->preparedBuildId,
        performancePayload->preparedBuildId,
        previewPayload->preparedContentDigest,
        performancePayload->preparedContentDigest,
        previewPayload->prepared->samples.size(),
        performancePayload->prepared->samples.size(),
        previewPayload->retainedPreparedBytes,
        performancePayload->retainedPreparedBytes,
        previewMagnitude,
        performanceMagnitude
    };
}

void requireParity(const ShellGateResult& standalone, const ShellGateResult& plugin)
{
    require(standalone.revision == plugin.revision,
            "Standalone and plugin shells should publish the same authored revision.");
    require(standalone.previewBuildId == plugin.previewBuildId
                && standalone.performanceBuildId == plugin.performanceBuildId,
            "Standalone and plugin shells should report matching prepared build identities.");
    require(standalone.previewDigest == plugin.previewDigest
                && standalone.performanceDigest == plugin.performanceDigest,
            "Standalone and plugin shells should report matching prepared content digests.");
    require(standalone.previewSampleCount == plugin.previewSampleCount
                && standalone.performanceSampleCount == plugin.performanceSampleCount,
            "Standalone and plugin shells should retain matching prepared sample counts.");
    require(standalone.previewRetainedBytes == plugin.previewRetainedBytes
                && standalone.performanceRetainedBytes == plugin.performanceRetainedBytes,
            "Standalone and plugin shells should retain matching prepared byte totals.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "Shell parity should load the authored project fixture.");

        drs::standalone::MainComponent standaloneShell(false);
        require(!standaloneShell.isAudioOutputEnabled(),
                "Standalone parity must remain headless and avoid a physical audio device.");
        const auto standaloneResult = runPreviewPublishScenario(
            standaloneShell.getProcessor(),
            projectLoad.project,
            "Standalone shell");

        // Deliberately never call createEditor(): this is the editor-closed plugin playback path.
        drs::plugin::Processor editorClosedPluginProcessor;
        const auto pluginResult = runPreviewPublishScenario(
            editorClosedPluginProcessor,
            projectLoad.project,
            "Editor-closed plugin shell");

        requireParity(standaloneResult, pluginResult);
        std::cout << "Sprint 4 Entry Gate shell parity passed: standalone and editor-closed plugin Preview/Publish metrics match."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 4 Entry Gate shell parity failed: " << exception.what() << std::endl;
        return 1;
    }
}
