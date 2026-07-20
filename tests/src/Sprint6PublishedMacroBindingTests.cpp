#include "drs/engine/PublishedMacroBinding.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

const drs::engine::PublishedMacroBinding& bindingFor(
    const drs::engine::ImmutablePublishedMacroBindingTable& table,
    const std::string& stableId)
{
    const auto found = std::find_if(table.bindings.begin(), table.bindings.end(),
                                    [&](const auto& binding)
                                    {
                                        return binding.stableAuthoredId == stableId;
                                    });
    require(found != table.bindings.end(), "Expected fixed host binding was not present.");
    return *found;
}

drs::engine::PublishedMacroBindingBuildRequest baselineRequest()
{
    using namespace drs::engine;
    PublishedMacroBindingBuildRequest request;
    request.revision = 10;
    request.macroSchemaDigest = "schema-10";
    request.hostSlots = { { 0, "macro.tone", "tone" }, { 1, "macro.motion", "motion" } };
    request.authoredMacros = {
        { "tone", "Tone", 0.35, 0.0, 1.0, {} },
        { "motion", "Motion", 0.15, 0.0, 1.0, {} }
    };
    request.currentValues = { { "tone", 0.8 }, { "motion", 0.7 } };
    return request;
}

void crossBlockBoundary(drs::plugin::Processor& processor)
{
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    buffer.clear();
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

void crossAudioBoundary(drs::plugin::Processor& processor)
{
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    buffer.clear();
    processor.processBlock(buffer, midi);
}

bool waitForPendingPublish(drs::plugin::Processor& processor, std::size_t revision)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        const auto controller = processor.getPerformancePublishControllerSnapshot();
        const auto realtime = processor.getRealtimeSafetySnapshot();
        if (controller.activationState
                == drs::engine::PerformancePublishActivationState::pending
            && controller.currentRequest.identity.draftRevision == revision
            && realtime.pendingPublishedRevision == revision)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

bool waitForActivePublish(drs::plugin::Processor& processor, std::size_t revision)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        crossBlockBoundary(processor);
        const auto controller = processor.getPerformancePublishControllerSnapshot();
        if (controller.hasActiveRequest
            && controller.activationState
                == drs::engine::PerformancePublishActivationState::active
            && controller.activeRequestIdentity.draftRevision == revision)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

float renderPerformanceKeyboardNote(drs::plugin::Processor& processor,
                                    int midiNote,
                                    float velocity)
{
    processor.queuePerformanceSurfaceNoteOn(midiNote, velocity);
    auto maximumMagnitude = 0.0f;
    juce::MidiBuffer midi;
    for (auto block = 0; block < 4; ++block)
    {
        juce::AudioBuffer<float> output(2, 256);
        output.clear();
        processor.processBlock(output, midi);
        maximumMagnitude = std::max(maximumMagnitude,
                                    output.getMagnitude(0, output.getNumSamples()));
    }
    processor.queuePerformanceSurfaceNoteOff(midiNote);
    return maximumMagnitude;
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        const auto baseline = buildPublishedMacroBindingTable(baselineRequest());
        require(baseline.built && baseline.table != nullptr
                    && baseline.table->bindings.size() == 2
                    && bindingFor(*baseline.table, "tone").publishedValue == 0.8
                    && bindingFor(*baseline.table, "motion").publishedValue == 0.7,
                "Initial publication must capture current fixed-slot values by stable id.");

        auto migratedRequest = baselineRequest();
        migratedRequest.revision = 11;
        migratedRequest.macroSchemaDigest = "schema-11";
        migratedRequest.previousActiveTable = baseline.table;
        migratedRequest.currentValues = { { "tone", 0.9 }, { "motion", 0.6 } };
        migratedRequest.authoredMacros = {
            { "bloom", "Bloom", 0.4, 0.0, 1.0, {} },
            { "tone", "Renamed Tone", 0.25, 0.2, 0.6, {} }
        };
        const auto migrated = buildPublishedMacroBindingTable(migratedRequest);
        require(migrated.built && migrated.table != nullptr
                    && migrated.table->callbackView.revision == 11
                    && bindingFor(*migrated.table, "tone").assigned
                    && bindingFor(*migrated.table, "tone").publishedName == "Renamed Tone"
                    && bindingFor(*migrated.table, "tone").publishedValue == 0.6
                    && !bindingFor(*migrated.table, "motion").assigned
                    && migrated.table->retiredStableAuthoredIds
                        == std::vector<std::string> { "motion" }
                    && migrated.table->unassignedStableAuthoredIds
                        == std::vector<std::string> { "bloom" },
                "Reorder/rename/range/remove/add migration must remain stable-id based and deterministic.");

        auto readdedRequest = baselineRequest();
        readdedRequest.revision = 12;
        readdedRequest.macroSchemaDigest = "schema-12";
        readdedRequest.previousActiveTable = migrated.table;
        readdedRequest.currentValues = { { "motion", 0.95 } };
        const auto readded = buildPublishedMacroBindingTable(readdedRequest);
        require(readded.built && readded.table != nullptr
                    && bindingFor(*readded.table, "motion").publishedValue == 0.15,
                "A newly re-added binding must start from its authored default, not a retired slot value.");

        auto duplicate = baselineRequest();
        duplicate.authoredMacros.push_back(duplicate.authoredMacros.front());
        require(!buildPublishedMacroBindingTable(duplicate).built,
                "Duplicate authored stable ids must reject publication.");
        auto invalidRange = baselineRequest();
        invalidRange.authoredMacros.front().minValue = 2.0;
        require(!buildPublishedMacroBindingTable(invalidRange).built,
                "Invalid authored macro ranges must reject publication.");

        auto maximum = baselineRequest();
        maximum.authoredMacros.clear();
        for (std::size_t index = 0; index < maximumPublishedMacroHostSlots; ++index)
        {
            maximum.authoredMacros.push_back(
                { "macro-" + std::to_string(index), "Macro", 0.5, 0.0, 1.0, {} });
        }
        require(buildPublishedMacroBindingTable(maximum).built,
                "The declared maximum authored macro count must remain bounded and accepted.");
        maximum.authoredMacros.push_back({ "overflow", "Overflow", 0.5, 0.0, 1.0, {} });
        require(!buildPublishedMacroBindingTable(maximum).built,
                "Authored macros beyond the fixed maximum must reject publication.");

        const auto loadedProject = loadPhase2ReferenceProjectManifest();
        require(loadedProject.loaded, "Sprint 6.7 requires the authored reference project.");

        auto narrowProject = loadedProject.project;
        require(!narrowProject.authoring.zones.empty(),
                "Perform keyboard regression requires one authored zone.");
        narrowProject.projectId += "-perform-keyboard-regression";
        narrowProject.authoring.zones.resize(1);
        auto& narrowZone = narrowProject.authoring.zones.front();
        narrowZone.keyLow = narrowZone.rootKey;
        narrowZone.keyHigh = narrowZone.rootKey;
        narrowZone.velocityLow = 101;
        narrowZone.velocityHigh = 127;
        narrowZone.articulationId = "default";
        narrowProject.authoring.selectedZoneId = narrowZone.id;

        drs::plugin::Processor narrowProcessor;
        narrowProcessor.prepareToPlay(48000.0, 256);
        narrowProcessor.replaceAuthoringProject(narrowProject);
        const auto narrowRevision
            = narrowProcessor.getAuthoringSession().getDocumentState().revision;
        require(narrowProcessor.submitPerformancePublishCommand(
                    {}, PerformancePublishCommandSource::externalApi)
                    && waitForActivePublish(narrowProcessor, narrowRevision),
                "A single-key imported project must publish when its authored articulation differs from the bootstrap selection.");
        require(renderPerformanceKeyboardNote(narrowProcessor, narrowZone.rootKey, 0.8f)
                    > 0.0001f,
                "The Perform keyboard root key must remain audible when published macro modulation changes pitch or velocity.");

        drs::plugin::Processor processor;
        processor.prepareToPlay(48000.0, 256);
        processor.replaceAuthoringProject(loadedProject.project);
        processor.serviceMessageThreadWork();
        const auto stableHostParameterCount = processor.getParameters().size();
        const auto firstRevision = processor.getAuthoringSession().getDocumentState().revision;
        require(processor.getEngineFacade().publishCurrentDraft()
                    && waitForActivePublish(processor, firstRevision),
                "The baseline schema must publish and activate.");
        const auto firstActive = processor.getEngineFacade().getActivePublishedMacroBindings();
        require(firstActive != nullptr && firstActive->revision == firstRevision,
                "Activation acknowledgement must publish the exact immutable macro table.");

        processor.setMacroValueFromShell("tone", 0.9);
        auto changedProject = loadedProject.project;
        auto tone = changedProject.authoring.macros.front();
        tone.name = "Tone Renamed";
        tone.minValue = 0.2;
        tone.maxValue = 0.6;
        changedProject.authoring.macros = {
            { "bloom", "Bloom", 0.4, 0.0, 1.0, {} }, tone
        };
        processor.replaceAuthoringProject(changedProject);
        const auto changedRevision = processor.getAuthoringSession().getDocumentState().revision;
        require(processor.getEngineFacade().getActivePublishedMacroBindings() == firstActive,
                "Draft macro edits must not replace the active Performance binding before Publish.");
        require(processor.getParameters().size() == stableHostParameterCount,
                "Authored schema edits must never change the host parameter topology.");
        require(processor.getEngineFacade().publishCurrentDraft()
                    && waitForPendingPublish(processor, changedRevision),
                "The changed authored macro schema must reach an exact pending activation.");
        processor.setMacroValueFromShell("tone", 0.3);
        crossAudioBoundary(processor);
        auto boundaryDiagnostics = processor.getRealtimeSafetySnapshot();
        require(boundaryDiagnostics.activePublishedRevision == changedRevision
                    && boundaryDiagnostics.activePublishedMacroRevision == changedRevision
                    && boundaryDiagnostics.activePublishedMacroFixedVelocity == 89
                    && boundaryDiagnostics.activePublishedMacroMidiNoteOffset == 0,
                "Automation immediately before activation must remain on the old binding while the new callback view and audio generation cut over together.");
        processor.setMacroValueFromShell("tone", 0.4);
        crossAudioBoundary(processor);
        processor.serviceMessageThreadWork();
        boundaryDiagnostics = processor.getRealtimeSafetySnapshot();
        require(boundaryDiagnostics.activePublishedMacroRevision == changedRevision
                    && boundaryDiagnostics.activePublishedMacroFixedVelocity == 70,
                "Automation immediately after activation must route to the active published binding without rebuilding audio.");
        const auto changedActive = processor.getEngineFacade().getActivePublishedMacroBindings();
        require(changedActive != nullptr && changedActive->revision == changedRevision
                    && changedActive->macroSchemaDigest != firstActive->macroSchemaDigest
                    && bindingFor(*changedActive, "tone").publishedValue == 0.6
                    && !bindingFor(*changedActive, "motion").assigned
                    && processor.getParameters().size() == stableHostParameterCount,
                "Audio, automation binding, revision, and stable host topology must cut over coherently.");

        processor.setMacroValueFromShell("motion", 0.95);
        crossAudioBoundary(processor);
        require(processor.getEngineFacade().getActivePublishedMacroBindings() == changedActive
                    && processor.getRealtimeSafetySnapshot().activePublishedMacroMidiNoteOffset == 0,
                "Automation for a retired slot must not mutate the active immutable published table.");

        std::cout << "Mini Sprint 6.7 published macro and automation binding matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.7 published macro binding matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
