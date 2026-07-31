#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void runMessageLoopFor(const int milliseconds)
{
    auto* messageManager = juce::MessageManager::getInstanceWithoutCreating();
    require(messageManager != nullptr, "A JUCE message manager is required for lifecycle IO audit.");
#if JUCE_MODAL_LOOPS_PERMITTED
    messageManager->runDispatchLoopUntil(milliseconds);
#else
    juce::Thread::sleep(milliseconds);
#endif
}

void processBlock(drs::plugin::Processor& processor)
{
    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
}

drs::app::AuthoringSourceValidationSnapshot waitForAuthoringSourceValidation(
    drs::plugin::Processor& processor)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processBlock(processor);
        processor.serviceMessageThreadWork();
        const auto snapshot = processor.getAuthoringSourceValidationSnapshot();
        if (snapshot.available
            && (snapshot.state == "completed"
                || snapshot.state == "failed"
                || snapshot.state == "canceled"))
        {
            return snapshot;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    throw std::runtime_error("Lifecycle IO audit source validation timed out.");
}

std::shared_ptr<const drs::engine::ProjectRestoreSnapshot> waitForRestore(
    drs::plugin::Processor& processor)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processBlock(processor);
        processor.serviceMessageThreadWork();
        const auto snapshot = processor.getProjectRestoreSnapshot();
        if (snapshot != nullptr
            && (snapshot->state == drs::engine::ProjectRestoreState::active
                || snapshot->state == drs::engine::ProjectRestoreState::ready
                || snapshot->state == drs::engine::ProjectRestoreState::failed
                || snapshot->state == drs::engine::ProjectRestoreState::needsLocation))
        {
            return snapshot;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    throw std::runtime_error("Lifecycle IO audit restore timed out.");
}

drs::app::AuthoringWaveformPreview waitForWaveformPreviewReady(drs::plugin::Processor& processor)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        auto preview = processor.getAuthoringWaveformPreview();
        if (preview.available && !preview.points.empty() && preview.state == "Ready")
            return preview;
        processor.serviceMessageThreadWork();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    throw std::runtime_error("Lifecycle IO audit waveform preview timed out.");
}

std::string describeIoCounters(const drs::engine::SampleImportIoCounters& counters)
{
    std::ostringstream description;
    description << "fingerprintOpenCount=" << counters.fingerprintOpenCount
                << ", readerOpenCount=" << counters.readerOpenCount
                << ", bytesReadCount=" << counters.bytesReadCount
                << ", fullFrameReadCount=" << counters.fullFrameReadCount
                << ", copyCount=" << counters.copyCount
                << ", peakChunkReadCount=" << counters.peakChunkReadCount;
    return description.str();
}

void requireNoImportIo(const std::string& context)
{
    const auto counters = drs::engine::getSampleImportIoCounters();
    require(counters.fingerprintOpenCount == 0
                && counters.readerOpenCount == 0
                && counters.bytesReadCount == 0
                && counters.fullFrameReadCount == 0
                && counters.copyCount == 0
                && counters.peakChunkReadCount == 0,
            context + " unexpectedly performed sample import IO: " + describeIoCounters(counters));
}

drs::engine::RuntimeProjectModel buildUnloadedProjectState()
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 5;
    project.displayName = "No Project Loaded";
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 4;
    project.authoring.notes = { "Open a project or create a new one to begin authoring." };
    project.notes = { "Lifecycle IO audit unloaded state." };
    return project;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto projectPath = drs::engine::getPhase2ReferenceProjectManifestPath();
        const auto projectLoad = drs::engine::loadRuntimeProjectManifest(projectPath);
        require(projectLoad.loaded,
                "The Phase 2 reference project fixture must load for lifecycle IO auditing.");

        drs::engine::resetSampleImportIoCounters();
        drs::plugin::Processor constructorProcessor;
        constructorProcessor.prepareToPlay(44100.0, 64);
        juce::MemoryBlock constructorState;
        constructorProcessor.getStateInformation(constructorState);
        requireNoImportIo(
            "Processor construction, prepareToPlay, and host scanning state serialization");

        drs::plugin::Processor sourceProcessor;
        require(sourceProcessor.replaceAuthoringProject(projectLoad.project, juce::File(projectPath)),
                "The source processor must accept the Phase 2 project before lifecycle auditing.");
        require(sourceProcessor.getAuthoringSession().selectZone("pad-a3-high").applied,
                "The lifecycle IO audit source must select a non-default zone.");
        sourceProcessor.serviceMessageThreadWork();
        juce::MemoryBlock dirtyState;
        sourceProcessor.getStateInformation(dirtyState);
        require(dirtyState.getSize() > 0,
                "The lifecycle IO audit source must produce a host-state chunk.");

        drs::plugin::Processor editorProcessor;
        require(editorProcessor.replaceAuthoringProject(projectLoad.project, juce::File(projectPath)),
                "The editor audit processor must accept the Phase 2 project.");

        drs::engine::resetSampleImportIoCounters();
        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(editorProcessor.createEditor());
        require(editor != nullptr, "The plugin editor must be constructible for lifecycle IO auditing.");
        editor->addToDesktop(0);
        editor->setVisible(true);
        runMessageLoopFor(350);
        requireNoImportIo("Plugin editor creation and passive shell refresh");

        auto migratedProject = editorProcessor.getAuthoringSession().getProject();
        migratedProject.displayName = "Lifecycle IO Audit Project";
        migratedProject.notes.push_back("Migration path audit");
        drs::engine::resetSampleImportIoCounters();
        require(editorProcessor.applyAuthoringProjectMigration(std::move(migratedProject)),
                "The lifecycle IO audit migration must apply to the loaded project.");
        runMessageLoopFor(350);
        requireNoImportIo("Project migration with an open editor");

        drs::engine::resetSampleImportIoCounters();
        editorProcessor.closeAuthoringProject(buildUnloadedProjectState());
        runMessageLoopFor(350);
        requireNoImportIo("Project close with an open editor");

        drs::engine::resetSampleImportIoCounters();
        require(editorProcessor.replaceAuthoringProject(projectLoad.project, juce::File(projectPath)),
                "The editor audit processor must reload the Phase 2 project.");
        runMessageLoopFor(350);
        requireNoImportIo("Project replace with an open editor");

        drs::engine::resetSampleImportIoCounters();
        drs::standalone::MainComponent standalone(false);
        standalone.setVisible(true);
        require(standalone.getProcessor().replaceAuthoringProject(projectLoad.project,
                                                                  juce::File(projectPath)),
                "The standalone audit processor must accept the Phase 2 project.");
        runMessageLoopFor(350);
        requireNoImportIo("Standalone shell creation and passive shell refresh");

        drs::plugin::Processor restoredProcessor;
        restoredProcessor.prepareToPlay(44100.0, 64);
        drs::engine::resetSampleImportIoCounters();
        restoredProcessor.setStateInformation(dirtyState.getData(),
                                              static_cast<int>(dirtyState.getSize()));
        const auto restored = waitForRestore(restoredProcessor);
        require(restored->state == drs::engine::ProjectRestoreState::ready
                    || restored->state == drs::engine::ProjectRestoreState::active,
                "The lifecycle IO audit restore must reach a ready or active state.");
        requireNoImportIo("Host state load and authored project restore");

        drs::engine::resetSampleImportIoCounters();
        require(editorProcessor.requestAuthoringSourceValidation(),
                "Explicit project source validation should still be accepted on demand.");
        const auto explicitValidation = waitForAuthoringSourceValidation(editorProcessor);
        require(explicitValidation.state == "completed" || explicitValidation.state == "failed",
                "Explicit project source validation should reach a non-canceled terminal state.");
        const auto explicitValidationCounters = drs::engine::getSampleImportIoCounters();
        require(explicitValidationCounters.fingerprintOpenCount > 0
                    && explicitValidationCounters.readerOpenCount > 0
                    && explicitValidationCounters.bytesReadCount > 0,
                "Explicit project source validation should perform sample import IO only when requested.");

        drs::engine::resetSampleImportIoCounters();
        editorProcessor.authorizeAuthoringWaveformPreviewLoad();
        const auto explicitPreview = waitForWaveformPreviewReady(editorProcessor);
        require(explicitPreview.available,
                "An explicit waveform preview authorization should still allow manual preview loading.");
        const auto explicitPreviewCounters = drs::engine::getSampleImportIoCounters();
        require(explicitPreviewCounters.readerOpenCount > 0
                    && explicitPreviewCounters.bytesReadCount > 0,
                "Explicit waveform preview loading should still perform sample import IO on demand.");

        std::cout << "WAV import lifecycle IO audit tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import lifecycle IO audit tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
