#include "drs/engine/AuthoringSession.h"
#include "drs/engine/AuthoringPreviewPreparation.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SamplerPlaybackContext.h"
#include "drs/engine/SfzImportProjection.h"
#include "plugin/PluginProcessor.h"
#include "shared/ProjectStorage.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <juce_core/juce_core.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string joinIssues(const std::vector<std::string>& issues)
{
    std::string joined;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index != 0)
            joined += " | ";
        joined += issues[index];
    }
    return joined;
}

std::string buildDraftPreviewSummary(const drs::plugin::Processor& processor)
{
    const auto previewStatus = processor.getAuthoringPreviewStatusSnapshot();
    const auto& draftStatus = processor.getEngineFacade().getDraftPlaybackStatus();
    return "stateLabel=" + previewStatus.stateLabel
        + ", presentation=" + std::to_string(static_cast<int>(previewStatus.presentationState))
        + ", activeRevision=" + std::to_string(previewStatus.activeRevision)
        + ", draftRevision=" + std::to_string(previewStatus.draftRevision)
        + ", activePreparedBuildId=" + std::to_string(previewStatus.activePreparedBuildId)
        + ", previewAvailable=" + std::to_string(draftStatus.preview.available)
        + ", previewPreparedZones=" + std::to_string(draftStatus.preview.preparedZoneCount)
        + ", previewPreparedSamples=" + std::to_string(draftStatus.preview.preparedSampleCount)
        + ", previewActivationEligible=" + std::to_string(draftStatus.preview.activationEligible)
        + ", previewFindings=" + std::to_string(draftStatus.preview.findings.size());
}

std::string buildScopedPreparationSummary(const drs::engine::AuthoringPreviewPreparationResult& result)
{
    const auto defaultArticulationIndex = result.model != nullptr
        ? result.model->getPerformanceProgram().defaultArticulationIndex
        : drs::engine::kInvalidPerformanceProgramIndex;
    const auto articulationCount = result.model != nullptr
        ? result.model->getPerformanceProgram().articulationCount
        : 0u;
    const auto firstRouteArticulationIndex = result.model != nullptr && !result.model->getRoutes().empty()
        ? result.model->getRoutes().front().performanceArticulationIndex
        : drs::engine::kInvalidPerformanceProgramIndex;
    return "prepared=" + std::to_string(result.prepared)
        + ", validatedZones=" + std::to_string(result.validatedZoneCount)
        + ", retainedZones=" + std::to_string(result.retainedZoneCount)
        + ", retainedSamples=" + std::to_string(result.retainedSampleCount)
        + ", modelRoutes=" + std::to_string(result.model != nullptr ? result.model->getRoutes().size() : 0)
        + ", modelSamples=" + std::to_string(result.model != nullptr ? result.model->getSamples().size() : 0)
        + ", defaultArticulationIndex=" + std::to_string(defaultArticulationIndex)
        + ", articulationCount=" + std::to_string(articulationCount)
        + ", firstRouteArticulationIndex=" + std::to_string(firstRouteArticulationIndex)
        + ", findings=" + std::to_string(result.findings.size());
}

std::string buildRealtimeSummary(const drs::plugin::ProcessorRealtimeSafetySnapshot& snapshot)
{
    return "previewState=" + snapshot.authoringPreviewRevisionState
        + ", previewFailure=" + snapshot.authoringPreviewFailureState
        + ", previewActiveVoices=" + std::to_string(snapshot.authoringPreviewActiveVoiceCount)
        + ", previewPeakVoices=" + std::to_string(snapshot.authoringPreviewPeakActiveVoiceCount)
        + ", previewDroppedEvents=" + std::to_string(snapshot.authoringPreviewDroppedEventCount)
        + ", previewDroppedNotes=" + std::to_string(snapshot.authoringPreviewDroppedNoteCount)
        + ", previewActivationCount=" + std::to_string(snapshot.authoringPreviewActivationCount)
        + ", activePreviewRevision=" + std::to_string(snapshot.activeAuthoringPreviewRevision)
        + ", pendingPreviewRevision=" + std::to_string(snapshot.pendingAuthoringPreviewRevision)
        + ", activePreparedBuildId=" + std::to_string(snapshot.activePreparedBuildId)
        + ", pendingPreparedBuildId=" + std::to_string(snapshot.pendingPreparedBuildId);
}

std::string buildFirstDifferingLineSummary(const std::string& left, const std::string& right)
{
    std::size_t leftOffset = 0;
    std::size_t rightOffset = 0;
    std::size_t lineNumber = 1;
    while (leftOffset < left.size() && rightOffset < right.size())
    {
        const auto leftLineEnd = left.find('\n', leftOffset);
        const auto rightLineEnd = right.find('\n', rightOffset);
        const auto leftLine = left.substr(leftOffset, leftLineEnd == std::string::npos ? std::string::npos
                                                                                       : leftLineEnd - leftOffset);
        const auto rightLine = right.substr(rightOffset, rightLineEnd == std::string::npos ? std::string::npos
                                                                                            : rightLineEnd - rightOffset);
        if (leftLine != rightLine)
        {
            return "line " + std::to_string(lineNumber)
                + ", left='" + leftLine + "', right='" + rightLine + "'";
        }

        if (leftLineEnd == std::string::npos || rightLineEnd == std::string::npos)
            break;

        leftOffset = leftLineEnd + 1;
        rightOffset = rightLineEnd + 1;
        ++lineNumber;
    }

    if (left.size() != right.size())
    {
        return "serialized lengths differ: left=" + std::to_string(left.size())
            + ", right=" + std::to_string(right.size());
    }

    return "no differing line found";
}

fs::path resolveFixturePath(const fs::path& relativeFixturePath)
{
    const auto sourceRoot = fs::path(DRS_SOURCE_ROOT);

    const auto localFixturePath = sourceRoot / relativeFixturePath;
    if (fs::exists(localFixturePath))
        return localFixturePath;

    const auto workspaceFixturePath = sourceRoot.parent_path() / relativeFixturePath;
    if (fs::exists(workspaceFixturePath))
        return workspaceFixturePath;

    throw std::runtime_error("Could not locate " + relativeFixturePath.generic_string());
}

fs::path resolveFixturePath()
{
    return resolveFixturePath(
        "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz");
}

drs::engine::RuntimeProjectModel makeBlankProject(const fs::path& fixturePath)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "sprint31.sfz-runtime";
    project.displayName = "Sprint 3.1 SFZ Runtime";
    project.contentRootPath = fixturePath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (fixturePath.parent_path() / "runtime-test.drinst").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    project.authoring.articulations = { { "default", "Default", true, 0, std::nullopt } };
    return project;
}

void processBlock(drs::plugin::Processor& processor,
                  bool emitPerformanceNote,
                  float* peak = nullptr)
{
    juce::AudioBuffer<float> buffer(2, 256);
    buffer.clear();
    juce::MidiBuffer midi;
    if (emitPerformanceNote)
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(96)), 0);
    processor.processBlock(buffer, midi);
    if (peak != nullptr)
    {
        *peak = 0.0f;
        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
            *peak = std::max(*peak, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));
    }
}

float measurePeakOverBlocks(drs::plugin::Processor& processor,
                            bool emitPerformanceNote,
                            int blockCount)
{
    float peak = 0.0f;
    for (int block = 0; block < blockCount; ++block)
    {
        float blockPeak = 0.0f;
        processBlock(processor, emitPerformanceNote && block == 0, &blockPeak);
        peak = std::max(peak, blockPeak);
    }
    return peak;
}

drs::engine::PlaybackActivationPayloadPtr preparePreviewPayload(
    const drs::engine::RuntimeProjectModel& project,
    std::size_t revision)
{
    drs::engine::EngineFacade facade;
    require(facade.replaceDraftPlaybackAuthoringProject(project),
            "EngineFacade rejected the saved-project preview payload regression fixture.");
    require(facade.stageDraftRevision(revision),
            "EngineFacade rejected the saved-project preview payload regression revision.");
    require(facade.refreshPreviewToCurrentDraft(),
            "EngineFacade rejected the saved-project preview payload regression request.");
    require(facade.waitForPreparedPlaybackIdle(std::chrono::milliseconds(10000)),
            "EngineFacade preview payload regression worker did not settle.");
    facade.serviceBackgroundWork();
    const auto payload = facade.getPreviewActivationPayload();
    require(payload != nullptr && payload->revision == revision,
            "EngineFacade preview payload regression fixture did not publish the requested payload.");
    return payload;
}

float renderModelPeak(const drs::engine::SamplerRenderModelPtr& model,
                      int note,
                      float velocity,
                      int blockCount)
{
    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::preview);
    require(context.prepare(44100.0), "Scoped preview render context must prepare.");
    require(context.stageActivation(model), "Scoped preview render context must stage activation.");

    float peak = 0.0f;
    for (int block = 0; block < blockCount; ++block)
    {
        juce::AudioBuffer<float> buffer(2, 256);
        buffer.clear();
        std::array<float*, 2> channels { buffer.getWritePointer(0), buffer.getWritePointer(1) };
        drs::engine::SamplerAudioBufferView output { channels.data(), 2, 256 };

        std::array<drs::engine::SamplerRenderEvent, 1> events {};
        drs::engine::SamplerRenderEventView eventView;
        if (block == 0)
        {
            events[0].type = drs::engine::SamplerRenderEventType::noteOn;
            events[0].sampleOffset = 0;
            events[0].midiNote = static_cast<std::uint8_t>(note);
            events[0].velocity = velocity;
            eventView = { events.data(), events.size() };
        }

        const auto result = context.renderBlock(output, eventView);
        require(result.accepted, "Scoped preview render context must accept render blocks.");
        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
            peak = std::max(peak, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));
    }

    return peak;
}

void waitForPublishedPerformance(drs::plugin::Processor& processor, const std::string& context)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processBlock(processor, false);
        processor.serviceMessageThreadWork();

        const auto publish = processor.getPerformancePublishControllerSnapshot();
        if (publish.activationState == drs::engine::PerformancePublishActivationState::active
            && publish.hasActiveRequest)
        {
            return;
        }

        if (publish.hasFailedRequest)
        {
            throw std::runtime_error(context + " failed: " + publish.failureFinding.message);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    throw std::runtime_error(context + " timed out.");
}

void waitForAuthoringPreviewReady(drs::plugin::Processor& processor, const std::string& context)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processBlock(processor, false);
        processor.serviceMessageThreadWork();

        const auto controller = processor.getAuthoringPreviewControllerSnapshot();
        if (controller.preparationState == drs::engine::AuthoringPreviewPreparationState::ready)
            return;

        if (controller.preparationState == drs::engine::AuthoringPreviewPreparationState::failed)
        {
            const auto status = processor.getAuthoringPreviewStatusSnapshot();
            const auto detail = !status.failureState.empty()
                ? status.failureState
                : (!status.stateLabel.empty() ? status.stateLabel : std::string("preview preparation failed"));
            throw std::runtime_error(context + " failed: " + detail);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    throw std::runtime_error(context + " timed out.");
}

void waitForAuthoringPreviewActive(drs::plugin::Processor& processor, const std::string& context)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        processBlock(processor, false);
        processor.serviceMessageThreadWork();

        const auto status = processor.getAuthoringPreviewStatusSnapshot();
        if (status.presentationState == drs::engine::AuthoringPreviewPresentationState::active
            && status.activePreparedBuildId != 0
            && status.activeRevision == status.draftRevision)
        {
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    throw std::runtime_error(context + " did not become active.");
}

} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto fixturePath = resolveFixturePath();
        const auto blankProject = makeBlankProject(fixturePath);
        const auto projection = drs::engine::projectSfzImportDocument(
            blankProject,
            fixturePath.generic_string());
        require(projection.projected,
                "SFZ runtime regression requires a projectable fixture. state="
                    + projection.state + " issues=" + joinIssues(projection.issues));

        drs::engine::AuthoringSession session(blankProject);
        const auto applyResult = drs::engine::applySfzImportProjection(
            session,
            projection,
            "Import SFZ runtime regression fixture");
        require(applyResult.applied, "Imported SFZ fixture must apply to an authoring project.");

        auto importedProject = session.getProject();
        require(!importedProject.authoring.zones.empty(),
                "Imported SFZ runtime regression requires authored zones.");
        require(importedProject.authoring.articulations.size() >= 2
                    && importedProject.authoring.articulations.front().id == "default"
                    && importedProject.authoring.zones.front().articulationId == "sustain",
                "Imported SFZ regression must preserve the placeholder default articulation while authoring sustain zones.");
        {
            auto corruptedProject = importedProject;
            for (auto& zone : corruptedProject.authoring.zones)
            {
                if (zone.roundRobin.has_value() && zone.velocityHigh < 127)
                {
                    zone.roundRobin.reset();
                    zone.roundRobinLength = 0;
                    zone.roundRobinPosition = 0;
                }
            }
            drs::engine::AuthoringSession repairedSession(corruptedProject);
            const auto repairedValidation = drs::engine::validateRuntimeProjectModel(repairedSession.getProject());
            require(repairedValidation.valid,
                    "Imported SFZ reopen repair must reconstruct valid round-robin topology after partial metadata loss. "
                        + repairedValidation.state + " :: " + joinIssues(repairedValidation.issues));
            require(repairedSession.getProject().authoring.zones.at(0).roundRobin.has_value(),
                    "Imported SFZ reopen repair must restore Round Robin metadata on the lower velocity layers.");
        }

        const auto savedProjectRoot = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                          .getNonexistentChildFile("drs-sprint31-sfz-runtime", {}, false);
        require(savedProjectRoot.createDirectory(),
                "Runtime playback regression must create a temporary authored-project directory.");
        const auto savedProjectFile = savedProjectRoot.getChildFile("ImportedRhodes.drsproj");
        importedProject.contentRootPath = savedProjectRoot.getFullPathName().toStdString();
        importedProject.defaultInstrumentManifestPath
            = savedProjectFile.withFileExtension(".drinst").getFullPathName().toStdString();
        require(drs::app::saveProjectFiles(importedProject, savedProjectFile).saved,
                "Runtime playback regression must save the authored project pair.");
        const auto reloadedProject = drs::engine::loadRuntimeProjectManifest(
            savedProjectFile.getFullPathName().toStdString());
        require(reloadedProject.loaded,
                "Runtime playback regression must reload its persisted authored project manifest. "
                    + reloadedProject.state + " :: " + joinIssues(reloadedProject.issues));
        const auto importedSerialized = drs::engine::serializeRuntimeProjectManifest(
            importedProject,
            savedProjectFile.getFullPathName().toStdString());
        const auto reloadedSerialized = drs::engine::serializeRuntimeProjectManifest(
            reloadedProject.project,
            savedProjectFile.getFullPathName().toStdString());
        require(importedSerialized == reloadedSerialized,
                "Imported SFZ save/reload must preserve the authored manifest model: "
                    + buildFirstDifferingLineSummary(importedSerialized, reloadedSerialized));

        drs::plugin::Processor processor;
        processor.prepareToPlay(44100.0, 256);
        require(processor.replaceAuthoringProject(reloadedProject.project, savedProjectFile),
                "Imported SFZ fixture must replace the processor authoring project after a saved-project reload.");
        const auto validationAfterReplace = drs::engine::validateRuntimeProjectModel(
            processor.getAuthoringSession().getProject());
        require(validationAfterReplace.valid,
                "Imported SFZ saved-project regression must reopen into a valid authored project. "
                    + validationAfterReplace.state + " :: " + joinIssues(validationAfterReplace.issues));
        require(processor.getEngineFacade().getCurrentSessionState().selectedArticulationId == "sustain",
                "Imported SFZ saved-project regression must restore the sustain articulation as the active selection.");

        const auto selectedZone = processor.getAuthoringSession().getSelectedZone();
        require(selectedZone.has_value(),
                "Imported SFZ fixture must keep a selected zone after apply.");
        const auto previewPayload = preparePreviewPayload(reloadedProject.project, 1);
        drs::engine::SamplerRenderModelBuildOptions publishedSelectionOptions;
        publishedSelectionOptions.selectedArticulationId = "sustain";
        const auto publishedSelectionModel = drs::engine::buildSamplerRenderModel(
            previewPayload,
            publishedSelectionOptions);
        require(publishedSelectionModel.built && publishedSelectionModel.model != nullptr,
                "Imported SFZ full-project sustain render model must build successfully.");
        require(!publishedSelectionModel.model->getRoutes().empty(),
                "Imported SFZ full-project sustain render model must retain playable routes.");
        require(
            publishedSelectionModel.model->getPerformanceProgram().defaultArticulationIndex
                == publishedSelectionModel.model->getRoutes().front().performanceArticulationIndex,
            "Imported SFZ full-project sustain render model must seed the active articulation from the selected authored sustain routes.");
        const auto publishedSelectionPeak = renderModelPeak(
            publishedSelectionModel.model,
            selectedZone->rootKey,
            static_cast<float>(selectedZone->velocityHigh) / 127.0f,
            8);
        require(std::isfinite(publishedSelectionPeak) && publishedSelectionPeak > 0.0f,
                "Imported SFZ full-project sustain render model must produce finite nonzero audio even when the placeholder default articulation remains in the authored schema.");
        drs::engine::AuthoringPreviewRequest directSelectedZoneRequest;
        directSelectedZoneRequest.identity.draftRevision = 1;
        directSelectedZoneRequest.identity.scope = drs::engine::AuthoringPreviewScope::selectedZone;
        directSelectedZoneRequest.identity.selectedZoneId = selectedZone->id;
        directSelectedZoneRequest.identity.selectedGroupId = selectedZone->groupId;
        directSelectedZoneRequest.reason = drs::engine::AuthoringPreviewRequestReason::explicitSelectedZoneAudition;
        directSelectedZoneRequest.invalidationCategory
            = drs::engine::AuthoringPreviewInvalidationCategory::previewScope;
        const auto directSelectedZonePreparation = drs::engine::prepareAuthoringPreviewRenderModel(
            previewPayload,
            directSelectedZoneRequest);
        require(directSelectedZonePreparation.prepared,
                "Imported SFZ selected-zone direct preparation after saved-project reload must succeed. "
                    + buildScopedPreparationSummary(directSelectedZonePreparation));
        const auto directSelectedZonePeak = renderModelPeak(
            directSelectedZonePreparation.model,
            selectedZone->rootKey,
            static_cast<float>(selectedZone->velocityHigh) / 127.0f,
            8);
        require(std::isfinite(directSelectedZonePeak) && directSelectedZonePeak > 0.0f,
                "Imported SFZ selected-zone direct render after saved-project reload must produce finite nonzero audio. "
                    + buildScopedPreparationSummary(directSelectedZonePreparation));

        processor.requestAuthoringPreview(drs::engine::AuthoringPreviewScope::selectedZone);
        waitForAuthoringPreviewReady(processor, "Imported SFZ selected-zone preview after saved-project reload");
        waitForAuthoringPreviewActive(processor, "Imported SFZ selected-zone preview after saved-project reload");
        processor.queueAuthoringPreviewNoteOn(selectedZone->rootKey,
                                              static_cast<float>(selectedZone->velocityHigh) / 127.0f);

        const auto reloadedSelectedZonePreviewPeak = measurePeakOverBlocks(processor, false, 8);
        const auto realtimeSnapshot = processor.getRealtimeSafetySnapshot();
        require(std::isfinite(reloadedSelectedZonePreviewPeak) && reloadedSelectedZonePreviewPeak > 0.0f,
                "Imported SFZ selected-zone preview after saved-project reload must produce finite nonzero audio. "
                    + buildDraftPreviewSummary(processor) + " | "
                    + buildScopedPreparationSummary(directSelectedZonePreparation)
                    + ", directPeak=" + std::to_string(directSelectedZonePeak) + " | "
                    + buildRealtimeSummary(realtimeSnapshot));

        const auto selectedGroupPreviewRequest =
            processor.getAuthoringSession().buildSelectedGroupPreviewRequest();
        require(selectedGroupPreviewRequest.available,
                "Imported SFZ fixture must expose a selected-group preview request.");

        processor.requestAuthoringPreview(drs::engine::AuthoringPreviewScope::selectedGroup);
        waitForAuthoringPreviewReady(processor, "Imported SFZ selected-group preview");
        waitForAuthoringPreviewActive(processor, "Imported SFZ selected-group preview");
        processor.queueAuthoringPreviewNoteOn(selectedGroupPreviewRequest.midiNote,
                                              static_cast<float>(selectedGroupPreviewRequest.velocity) / 127.0f);

        const auto selectedGroupPreviewPeak = measurePeakOverBlocks(processor, false, 8);
        require(std::isfinite(selectedGroupPreviewPeak) && selectedGroupPreviewPeak > 0.0f,
                "Imported SFZ selected-group preview must produce finite nonzero audio.");

        processor.requestAuthoringPreview(drs::engine::AuthoringPreviewScope::selectedZone);
        waitForAuthoringPreviewReady(processor, "Imported SFZ selected-zone preview");
        processor.queueAuthoringPreviewNoteOn(selectedZone->rootKey,
                                              static_cast<float>(selectedZone->velocityHigh) / 127.0f);

        const auto previewPeak = measurePeakOverBlocks(processor, false, 8);
        require(std::isfinite(previewPeak) && previewPeak > 0.0f,
                "Imported SFZ selected-zone preview must produce finite nonzero audio.");

        require(processor.submitPerformancePublishCommand(),
                "Imported SFZ fixture must publish successfully.");
        waitForPublishedPerformance(processor, "Imported SFZ published performance");

        const auto performancePeak = measurePeakOverBlocks(processor, true, 8);
        require(std::isfinite(performancePeak) && performancePeak > 0.0f,
                "Imported SFZ published performance must produce finite nonzero audio.");

        {
            const auto violaFixturePath = resolveFixturePath(
                "DemoSFVInstruments/VSCO-2-CE-1.1.0/VSCO-2-CE-1.1.0/ViolaEnsSusVib.sfz");
            const auto violaBlankProject = makeBlankProject(violaFixturePath);
            const auto violaProjection = drs::engine::projectSfzImportDocument(
                violaBlankProject,
                violaFixturePath.generic_string());
            require(violaProjection.projected,
                    "VSCO viola publish regression requires a projectable fixture. state="
                        + violaProjection.state + " issues=" + joinIssues(violaProjection.issues));

            drs::engine::AuthoringSession violaSession(violaBlankProject);
            const auto violaApplyResult = drs::engine::applySfzImportProjection(
                violaSession,
                violaProjection,
                "Import VSCO viola publish regression fixture");
            require(violaApplyResult.applied,
                    "VSCO viola publish regression fixture must apply to the authoring project.");
            require(std::all_of(violaSession.getProject().authoring.zones.begin(),
                                violaSession.getProject().authoring.zones.end(),
                                [](const drs::engine::RuntimeProjectZoneDefinition& zone)
                                {
                                    return zone.velocityLow >= 1 && zone.velocityHigh <= 127;
                                }),
                    "VSCO viola imported velocity ranges must be renderer eligible.");

            drs::plugin::Processor violaProcessor;
            violaProcessor.prepareToPlay(44100.0, 256);
            require(violaProcessor.replaceAuthoringProject(violaSession.getProject()),
                    "VSCO viola publish regression must replace the processor authoring project.");
            require(violaProcessor.submitPerformancePublishCommand(),
                    "VSCO viola fixture must submit a Performance publish request.");
            waitForPublishedPerformance(violaProcessor, "VSCO viola published performance");

            const auto violaPeak = measurePeakOverBlocks(violaProcessor, true, 8);
            require(std::isfinite(violaPeak) && violaPeak > 0.0f,
                    "VSCO viola published performance must produce finite nonzero audio.");
        }

        auto editedZone = *selectedZone;
        editedZone.gainDb += 1.0;
        const auto validationBeforeEdit = drs::engine::validateRuntimeProjectModel(
            processor.getAuthoringSession().getProject());
        const auto editResult = processor.getAuthoringSession().updateSelectedZone(
            editedZone,
            "Sprint 3.1 save regression edit");
        require(editResult.applied,
                "Imported SFZ saved-project regression must allow an authored zone edit. editState="
                    + editResult.state + ", editIssues=" + joinIssues(editResult.issues)
                    + ", validationState=" + validationBeforeEdit.state
                    + ", validationIssues=" + joinIssues(validationBeforeEdit.issues));
        require(drs::app::saveProjectFiles(processor.getAuthoringSession().getProject(), savedProjectFile).saved,
                "Imported SFZ saved-project regression must save the edited project pair.");
        require(processor.bindAuthoringProjectFile(savedProjectFile),
                "Imported SFZ saved-project regression must accept the manifest after an authored edit is persisted.");

        std::cout << "Sprint 3.1 SFZ runtime playback tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1 SFZ runtime playback tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
