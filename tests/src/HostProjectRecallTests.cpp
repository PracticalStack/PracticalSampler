#include "plugin/PluginProcessor.h"
#include "drs/engine/RuntimeLoader.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
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

void processBlock(drs::plugin::Processor& processor,
                  const bool emitNote,
                  float* peak = nullptr)
{
    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;
    if (emitNote)
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);
    processor.processBlock(buffer, midi);
    if (peak != nullptr)
    {
        *peak = 0.0f;
        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
            *peak = std::max(*peak, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));
    }
}

void waitForPublishedPerformance(drs::plugin::Processor& processor,
                                 const std::string& context)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processBlock(processor, false);
        processor.serviceMessageThreadWork();
        const auto publish = processor.getPerformancePublishControllerSnapshot();
        if (publish.activationState
                == drs::engine::PerformancePublishActivationState::active
            && publish.hasActiveRequest)
            return;
        if (publish.hasFailedRequest)
            throw std::runtime_error(context + " failed: " + publish.failureFinding.message);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    throw std::runtime_error(context + " timed out.");
}

std::shared_ptr<const drs::engine::ProjectRestoreSnapshot> waitForRestore(
    drs::plugin::Processor& processor,
    const std::uint64_t minimumGeneration = 0,
    const bool acceptReady = false)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    auto readyObserved = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processBlock(processor, false);
        processor.serviceMessageThreadWork();
        const auto snapshot = processor.getProjectRestoreSnapshot();
        if (snapshot != nullptr && snapshot->generation >= minimumGeneration)
        {
            if (snapshot->state == drs::engine::ProjectRestoreState::ready)
            {
                // The coordinator publishes Ready before the processor applies the checkpoint.
                // Require one more service pass so no-published-state recalls are observed only
                // after their document and preset state are committed.
                if (acceptReady && readyObserved)
                    return snapshot;
                readyObserved = true;
            }
            else if (snapshot->state == drs::engine::ProjectRestoreState::active
                     || snapshot->state == drs::engine::ProjectRestoreState::failed
                     || snapshot->state == drs::engine::ProjectRestoreState::needsLocation)
            {
                return snapshot;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    throw std::runtime_error("Fresh processor restore timed out.");
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
                "The non-reference Phase 2 project fixture must load before the recall assertion.");

        auto sourceOwner = std::make_unique<drs::plugin::Processor>();
        auto& source = *sourceOwner;
        source.prepareToPlay(44100.0, 64);
        require(source.replaceAuthoringProject(projectLoad.project, juce::File(projectPath)),
                "The source processor must accept the validated authored-project binding.");
        require(source.getAuthoringSession().getProject().projectId
                    == "drs.phase2.authoring-foundation",
                "The source processor must own the non-reference project before capture.");

        source.setMacroValueFromShell("tone", 0.62);
        source.setMacroValueFromShell("motion", 0.78);
        require(source.submitPerformancePublishCommand(),
                "The source processor must accept a publish request before host capture.");
        waitForPublishedPerformance(source, "Source publish");
        const auto sourcePublish = source.getPerformancePublishControllerSnapshot();

        juce::MemoryBlock state;
        source.getStateInformation(state);
        require(state.getSize() > 0, "The source processor must produce a host-state chunk.");
        require(state.getSize() <= drs::engine::hostSessionStateMaxBytes,
                "The published processor chunk must remain inside the frozen host-state budget.");
        const auto serialized = std::string(
            static_cast<const char*>(state.getData()), state.getSize());
        if (const auto* capturePath = std::getenv("DRS_HOST_STATE_CAPTURE_PATH");
            capturePath != nullptr && *capturePath != '\0')
        {
            std::ofstream capture(capturePath, std::ios::binary | std::ios::trunc);
            require(static_cast<bool>(capture), "The requested host-state capture file must open.");
            capture.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
            require(static_cast<bool>(capture), "The requested host-state capture file must be written.");
        }
        const auto parsed = drs::engine::parseHostSessionState(serialized);
        require(parsed.isValidHostState() && parsed.hostState->publishedState.has_value(),
                "A published authored project must capture a valid project-aware host state.");

        juce::MemoryBlock repeatedState;
        source.getStateInformation(repeatedState);
        require(repeatedState == state,
                "Repeated host callback reads must copy the same immutable publication.");

        auto restoredOwner = std::make_unique<drs::plugin::Processor>();
        auto& restored = *restoredOwner;
        restored.prepareToPlay(44100.0, 64);
        restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        require(restored.getAuthoringSession().getProject().projectId
                    != source.getAuthoringSession().getProject().projectId,
                "setStateInformation must stage work without mutating the session in the callback.");
        float pendingPeak = 0.0f;
        processBlock(restored, true, &pendingPeak);
        require(pendingPeak == 0.0f,
                "A fresh processor must remain silent while project-aware recall is pending.");

        const auto restore = waitForRestore(restored);
        require(restore->state == drs::engine::ProjectRestoreState::active,
                "A published authored project must reach Active: " + restore->message);

        const auto& restoredProject = restored.getAuthoringSession().getProject();
        require(restoredProject.projectId == source.getAuthoringSession().getProject().projectId,
                "A fresh processor restored project '"
                    + restoredProject.projectId
                    + "' instead of the authored non-reference project '"
                    + source.getAuthoringSession().getProject().projectId + "'.");
        require(restored.getAuthoringProjectFile().getFullPathName()
                    == source.getAuthoringProjectFile().getFullPathName(),
                "The authored .drsproj binding must round-trip.");

        const auto restoredPublish = restored.getPerformancePublishControllerSnapshot();
        require(restoredPublish.hasActiveRequest,
                "Restored Performance must have an active exact-identity request.");
        require(restoredPublish.activeRequestIdentity.projectGeneration
                    == sourcePublish.activeRequestIdentity.projectGeneration
                    && restoredPublish.activeRequestIdentity.draftRevision
                        == sourcePublish.activeRequestIdentity.draftRevision
                    && restoredPublish.activeRequestIdentity.authoredContentDigest
                        == sourcePublish.activeRequestIdentity.authoredContentDigest
                    && restoredPublish.activeRequestIdentity.macroSchemaDigest
                        == sourcePublish.activeRequestIdentity.macroSchemaDigest
                    && restoredPublish.activePreparedDigest
                        == sourcePublish.activePreparedDigest,
                "Restored activation must match generation, revision, authored, macro, and prepared digests.");

        const auto& restoredSession = restored.getEngineFacade().getCurrentSessionState();
        require(restoredSession.macroValues.size() == 2
                    && restoredSession.macroValues[0].value == 0.62
                    && restoredSession.macroValues[1].value == 0.78,
                "Staged preset macro values must apply after the matching authored schema exists.");

        const auto stableRevision = restored.getEngineFacade().getStateRevision();
        for (int iteration = 0; iteration < 5; ++iteration)
            restored.serviceMessageThreadWork();
        require(restored.getEngineFacade().getStateRevision() == stableRevision,
                "Parameter synchronization must settle without an automation feedback loop.");

        float activePeak = 0.0f;
        processBlock(restored, true, &activePeak);
        require(std::isfinite(activePeak) && activePeak > 0.0f,
                "The exact restored Performance payload must produce finite, nonzero audio after activation.");

        const auto defaultArticulationProjectPath = juce::File(projectPath)
            .getParentDirectory()
            .getChildFile("host-state-default-articulation.drsproj")
            .getFullPathName()
            .toStdString();
        const auto defaultArticulationProject = drs::engine::loadRuntimeProjectManifest(
            defaultArticulationProjectPath);
        require(defaultArticulationProject.loaded
                    && defaultArticulationProject.project.projectId
                        == "drs.host-state.default-articulation"
                    && defaultArticulationProject.project.authoring.zones.size() == 1
                    && defaultArticulationProject.project.authoring.zones.front().articulationId == "default",
                "The contextual-articulation fixture must expose only the authored default route.");

        auto contextualSource = std::make_unique<drs::plugin::Processor>();
        contextualSource->prepareToPlay(44100.0, 64);
        require(contextualSource->replaceAuthoringProject(
                    defaultArticulationProject.project,
                    juce::File(defaultArticulationProjectPath)),
                "The contextual-articulation source must accept its validated project binding.");
        require(contextualSource->getEngineFacade().getCurrentSessionState().selectedArticulationId == "default",
                "Replacing the fixture project must select its only authored articulation.");
        require(contextualSource->submitPerformancePublishCommand(),
                "The contextual-articulation source must publish before host capture.");
        waitForPublishedPerformance(*contextualSource, "Contextual-articulation source publish");

        juce::MemoryBlock contextualStateBlock;
        contextualSource->getStateInformation(contextualStateBlock);
        const auto contextualStateText = std::string(
            static_cast<const char*>(contextualStateBlock.getData()), contextualStateBlock.getSize());
        const auto contextualParsed = drs::engine::parseHostSessionState(contextualStateText);
        require(contextualParsed.isValidHostState()
                    && contextualParsed.hostState->presetState.selectedArticulationId == "default",
                "The contextual-articulation fixture must capture a valid host state with default selected.");

        auto contextualFailure = std::make_unique<drs::plugin::Processor>();
        contextualFailure->prepareToPlay(44100.0, 64);
        contextualFailure->setStateInformation(
            contextualStateBlock.getData(), static_cast<int>(contextualStateBlock.getSize()));
        const auto contextualFailureRestore = waitForRestore(*contextualFailure);
        require(contextualFailureRestore->state == drs::engine::ProjectRestoreState::failed
                    && contextualFailureRestore->finding
                        == drs::engine::ProjectRestoreFinding::articulationMismatch
                    && contextualFailureRestore->message.find("default") != std::string::npos
                    && contextualFailureRestore->message.find(
                        "drs.host-state.default-articulation") != std::string::npos
                    && contextualFailureRestore->message.find("default: default") != std::string::npos,
                "A contextual articulation rejection must name the saved articulation, restored project, "
                    "and authored default instead of collapsing into CheckpointInvalid.");
        float contextualFailurePeak = 0.0f;
        processBlock(*contextualFailure, true, &contextualFailurePeak);
        require(contextualFailurePeak == 0.0f,
                "A contextual-articulation restore rejected before activation must remain silent.");

        require(contextualFailure->replaceAuthoringProject(
                    defaultArticulationProject.project,
                    juce::File(defaultArticulationProjectPath)),
                "A failed host recall must allow a valid manual project replacement in the same instance.");
        require(contextualFailure->getProjectRestoreSnapshot()->state
                    == drs::engine::ProjectRestoreState::idle,
                "A valid manual project replacement must supersede the stale failed restore generation.");
        require(contextualFailure->submitPerformancePublishCommand(),
                "A valid manual project replacement must be publishable after a failed recall.");
        waitForPublishedPerformance(*contextualFailure, "Manual recovery publish");
        float manualRecoveryPeak = 0.0f;
        processBlock(*contextualFailure, true, &manualRecoveryPeak);
        require(std::isfinite(manualRecoveryPeak) && manualRecoveryPeak > 0.0f,
                "The same instance must render finite, nonzero audio after manual recovery and publish.");

        auto referenceKnownDiagnosticState = *contextualParsed.hostState;
        referenceKnownDiagnosticState.presetState.selectedArticulationId = "sustain";
        const auto referenceKnownDiagnosticSerialized = drs::engine::serializeHostSessionState(
            referenceKnownDiagnosticState);
        require(referenceKnownDiagnosticSerialized.serialized,
                "The single-field reference-articulation diagnostic fixture must serialize.");

        auto referenceKnownDiagnosticRestore = std::make_unique<drs::plugin::Processor>();
        referenceKnownDiagnosticRestore->prepareToPlay(44100.0, 64);
        referenceKnownDiagnosticRestore->setStateInformation(
            referenceKnownDiagnosticSerialized.text.data(),
            static_cast<int>(referenceKnownDiagnosticSerialized.text.size()));
        const auto referenceKnownDiagnosticResult = waitForRestore(*referenceKnownDiagnosticRestore);
        require(referenceKnownDiagnosticResult->state == drs::engine::ProjectRestoreState::active,
                "Characterization: changing only default to reference-known sustain must make the old "
                    "reference-only restore path reach Active.");
        float referenceKnownDiagnosticPeak = 0.0f;
        processBlock(*referenceKnownDiagnosticRestore, true, &referenceKnownDiagnosticPeak);
        require(std::isfinite(referenceKnownDiagnosticPeak) && referenceKnownDiagnosticPeak > 0.0f,
                "Characterization: the reference-known diagnostic substitution must produce finite, "
                    "nonzero audio. This is diagnostic evidence, not the desired authored behavior.");

        auto dirtySourceOwner = std::make_unique<drs::plugin::Processor>();
        auto& dirtySource = *dirtySourceOwner;
        require(dirtySource.replaceAuthoringProject(projectLoad.project, juce::File(projectPath)),
                "The dirty-checkpoint source must accept the authored project binding.");
        require(dirtySource.getAuthoringSession().selectZone("pad-a3-high").applied,
                "The dirty-checkpoint source must commit a distinct selected zone.");
        dirtySource.serviceMessageThreadWork();
        juce::MemoryBlock dirtyStateBlock;
        dirtySource.getStateInformation(dirtyStateBlock);
        const auto dirtyStateText = std::string(
            static_cast<const char*>(dirtyStateBlock.getData()), dirtyStateBlock.getSize());
        const auto dirtyParsed = drs::engine::parseHostSessionState(dirtyStateText);
        require(dirtyParsed.isValidHostState()
                    && dirtyParsed.hostState->authoringState.dirty
                    && dirtyParsed.hostState->authoringState.projectSnapshot.has_value(),
                "A dirty authored document must publish its bounded project snapshot.");

        auto dirtyRestoredOwner = std::make_unique<drs::plugin::Processor>();
        auto& dirtyRestored = *dirtyRestoredOwner;
        dirtyRestored.prepareToPlay(44100.0, 64);
        dirtyRestored.setStateInformation(
            dirtyStateBlock.getData(), static_cast<int>(dirtyStateBlock.getSize()));
        const auto dirtyRestore = waitForRestore(dirtyRestored, 0, true);
        require(dirtyRestore->state == drs::engine::ProjectRestoreState::ready
                    || dirtyRestore->state == drs::engine::ProjectRestoreState::active,
                "A dirty authored snapshot must restore without requiring its saved file content.");
        require(dirtyRestored.getAuthoringSession().getDocumentState().revision == 1
                    && dirtyRestored.getAuthoringSession().getDocumentState().savedRevision == 0
                    && dirtyRestored.getAuthoringSession().getDocumentState().dirty
                    && dirtyRestored.getAuthoringSession().getProject().authoring.selectedZoneId
                        == "pad-a3-high",
                "Checkpoint application must restore revision metadata and selection atomically.");
        const auto dirtyRestoreMetrics = dirtyRestored.getAuthoringImportResponsivenessSnapshot();
        require(dirtyRestoreMetrics.available
                    && dirtyRestoreMetrics.state == "not-run"
                    && dirtyRestoreMetrics.totalItemCount
                        == dirtyRestored.getAuthoringSession().getProject().sampleSources.size()
                    && dirtyRestoreMetrics.processedCount == 0
                    && dirtyRestoreMetrics.pendingCount == 0
                    && dirtyRestoreMetrics.lastProcessedItemId.empty(),
                "Checkpoint application must publish no-I/O authoring import metrics coherently.");

        auto missingState = *parsed.hostState;
        missingState.projectBinding.manifestPath
            = "C:/DefinitelyMissing/phase2-authoring-foundation.drsproj";
        missingState.projectBinding.contentRootHint.clear();
        missingState.projectBinding.portableRelativePath.clear();
        const auto missingSerialized = drs::engine::serializeHostSessionState(missingState);
        require(missingSerialized.serialized,
                "The missing-project recovery fixture must serialize.");

        auto missingRestoreOwner = std::make_unique<drs::plugin::Processor>();
        auto& missingRestore = *missingRestoreOwner;
        missingRestore.prepareToPlay(44100.0, 64);
        missingRestore.setStateInformation(
            missingSerialized.text.data(),
            static_cast<int>(missingSerialized.text.size()));
        const auto missingResult = waitForRestore(missingRestore);
        require(missingResult->state == drs::engine::ProjectRestoreState::needsLocation,
                "A missing saved manifest must request location rather than substitute reference content.");
        float missingPeak = 0.0f;
        processBlock(missingRestore, true, &missingPeak);
        require(missingPeak == 0.0f,
                "Missing project content must remain silent instead of presenting bootstrap audio as restored.");

        require(missingRestore.retryProjectRestoreWithFile(
                    juce::File(drs::engine::getPhase1ReferenceProjectManifestPath())),
                "Locate must submit a user-selected project candidate.");
        const auto wrongLocate = waitForRestore(missingRestore, missingResult->generation + 1);
        require(wrongLocate->state == drs::engine::ProjectRestoreState::needsLocation
                    && wrongLocate->finding
                        == drs::engine::ProjectRestoreFinding::identityMismatch
                    && missingRestore.getAuthoringSession().getProject().projectId
                        != projectLoad.project.projectId,
                "Locate must reject a wrong project ID without rebinding or replacing the session.");

        require(missingRestore.retryProjectRestoreWithFile(juce::File(projectPath)),
                "Locate must accept another explicit candidate after a wrong-ID rejection.");
        const auto repairedLocate = waitForRestore(missingRestore, wrongLocate->generation + 1);
        require(repairedLocate->state == drs::engine::ProjectRestoreState::active
                    && missingRestore.getAuthoringSession().getProject().projectId
                        == projectLoad.project.projectId,
                "A validated located manifest must repair recall without substituting another document.");

        const auto missingGeneration = repairedLocate->generation;
        std::string oversized(drs::engine::hostSessionStateMaxBytes + 1u, 'x');
        missingRestore.setStateInformation(oversized.data(), static_cast<int>(oversized.size()));
        missingRestore.serviceMessageThreadWork();
        require(missingRestore.getProjectRestoreSnapshot()->generation == missingGeneration,
                "An over-budget callback payload must be rejected before copying or enqueueing work.");

        std::cout << "Host project recall integration tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Host project recall integration tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
