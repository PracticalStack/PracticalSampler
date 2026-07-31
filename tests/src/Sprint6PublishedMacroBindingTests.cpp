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

bool hasFinding(const drs::engine::PublishedMacroBindingBuildResult& result,
                const std::string& code)
{
    return std::any_of(result.findings.begin(), result.findings.end(), [&](const auto& finding)
    {
        return finding.code == code;
    });
}

drs::engine::PublishedMacroBindingBuildRequest baselineRequest()
{
    using namespace drs::engine;
    PublishedMacroBindingBuildRequest request;
    request.revision = 10;
    request.macroSchemaDigest = "schema-10";
    request.hostSlots = { { 0, "macro.tone", "tone" }, { 1, "macro.motion", "motion" } };
    request.authoredMacros = {
        { "tone", "Tone", 0.35, 0.0, 1.0, true, {} },
        { "motion", "Motion", 0.15, 0.0, 1.0, true, {} }
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

        auto dspTargetRequest = baselineRequest();
        dspTargetRequest.authoredMacros.front().targets = {
            { "dsp.zone-gain.gainDb", "curatedDsp.zone-gain.gainDb", "mix",
              "zone-gain", "gainDb", 0.0, 1.0, -24.0, 6.0, "linear" }
        };
        DspParameterControlLayout dspLayout;
        dspLayout.graphPlanDigest = "graph-before-reorder";
        dspLayout.controls = {
            { 4, 3, 9, "other-slot", "gainDb", -96.0, 24.0, 0.0,
              CuratedDspSmoothing::linear },
            { 11, 7, 21, "zone-gain", "gainDb", -96.0, 24.0, 0.0,
              CuratedDspSmoothing::linear }
        };
        dspTargetRequest.dspControlLayout = &dspLayout;
        const auto dspTarget = buildPublishedMacroBindingTable(dspTargetRequest);
        require(dspTarget.built && dspTarget.table != nullptr
                    && bindingFor(*dspTarget.table, "tone").renderTarget
                        == PublishedMacroRenderTarget::dspControl
                    && bindingFor(*dspTarget.table, "tone").dspControlIndex == 11
                    && bindingFor(*dspTarget.table, "tone").destinationMinimum == -24.0
                    && bindingFor(*dspTarget.table, "tone").destinationMaximum == 6.0,
                "Structured DSP macro targets must resolve stable slot and parameter ids into callback indices.");

        std::reverse(dspLayout.controls.begin(), dspLayout.controls.end());
        dspLayout.graphPlanDigest = "graph-after-reorder";
        const auto reorderedDspTarget = buildPublishedMacroBindingTable(dspTargetRequest);
        require(reorderedDspTarget.built
                    && bindingFor(*reorderedDspTarget.table, "tone").dspControlIndex == 11,
                "Reordering graph controls must not break a macro resolved by stable DSP identity.");

        dspLayout.controls.erase(dspLayout.controls.begin());
        const auto deletedDspTarget = buildPublishedMacroBindingTable(dspTargetRequest);
        require(!deletedDspTarget.built
                    && hasFinding(deletedDspTarget, "published-macro-dsp-target-missing"),
                "Deleting a structured DSP target must reject publication with an actionable finding.");

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

        auto hiddenHelper = baselineRequest();
        hiddenHelper.hostSlots = { { 0, "macro.tone", "tone" } };
        hiddenHelper.authoredMacros = {
            { "tone", "Tone", 0.35, 0.0, 1.0, true, {} },
            { "helper", "Helper", 0.1, 0.0, 1.0, false, {} }
        };
        const auto hiddenHelperResult = buildPublishedMacroBindingTable(hiddenHelper);
        require(hiddenHelperResult.built
                    && hiddenHelperResult.table != nullptr
                    && hiddenHelperResult.table->assignedExposedCount == 1
                    && hiddenHelperResult.table->unassignedHiddenCount == 1
                    && hasFinding(hiddenHelperResult, "published-macro-unassigned"),
                "Hidden helper macros may remain unassigned, but the published table must report them explicitly.");

        auto exposedOverflow = hiddenHelper;
        exposedOverflow.authoredMacros[1].exposedInPerformance = true;
        const auto exposedOverflowResult = buildPublishedMacroBindingTable(exposedOverflow);
        require(!exposedOverflowResult.built
                    && hasFinding(exposedOverflowResult, "published-macro-exposed-slot-missing"),
                "Exposed macros without a compatible host slot must reject publication.");

        auto duplicate = baselineRequest();
        duplicate.authoredMacros.push_back(duplicate.authoredMacros.front());
        require(!buildPublishedMacroBindingTable(duplicate).built,
                "Duplicate authored stable ids must reject publication.");
        auto invalidRange = baselineRequest();
        invalidRange.authoredMacros.front().minValue = 2.0;
        require(!buildPublishedMacroBindingTable(invalidRange).built,
                "Invalid authored macro ranges must reject publication.");

        auto maximum = baselineRequest();
        maximum.hostSlots.clear();
        maximum.authoredMacros.clear();
        for (std::size_t index = 0; index < maximumPublishedMacroHostSlots; ++index)
        {
            maximum.hostSlots.push_back(
                { index,
                  "macro.slot-" + std::to_string(index),
                  "macro-" + std::to_string(index) });
            maximum.authoredMacros.push_back(
                { "macro-" + std::to_string(index), "Macro", 0.5, 0.0, 1.0, true, {} });
        }
        require(buildPublishedMacroBindingTable(maximum).built,
                "The declared maximum authored macro count must remain bounded and accepted.");
        maximum.authoredMacros.push_back({ "overflow", "Overflow", 0.5, 0.0, 1.0, true, {} });
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

        auto arbitraryProject = loadedProject.project;
        arbitraryProject.projectId += "-arbitrary-exposed-slot";
        arbitraryProject.authoring.macros = {
            { "room-gain", "Room Gain", 0.45, 0.0, 1.0, {}, true }
        };
        drs::plugin::Processor arbitraryProcessor;
        arbitraryProcessor.prepareToPlay(48000.0, 256);
        arbitraryProcessor.replaceAuthoringProject(arbitraryProject);
        const auto arbitraryRevision
            = arbitraryProcessor.getAuthoringSession().getDocumentState().revision;
        require(arbitraryProcessor.getEngineFacade().publishCurrentDraft()
                    && waitForActivePublish(arbitraryProcessor, arbitraryRevision),
                "An exposed authored macro with a non-reference id must still publish into the fixed host slot topology.");
        const auto arbitraryActive = arbitraryProcessor.getEngineFacade().getActivePublishedMacroBindings();
        require(arbitraryActive != nullptr
                    && arbitraryActive->assignedExposedCount == 1
                    && bindingFor(*arbitraryActive, "room-gain").hostParameterId == "macro.tone",
                "Published bindings must retain the authored stable id while mapping it into a fixed host parameter slot.");
        arbitraryProcessor.setMacroValueFromShell("room-gain", 0.22);
        crossAudioBoundary(arbitraryProcessor);
        const auto arbitraryDescriptors = arbitraryProcessor.getEngineFacade().getMacroDescriptors();
        require(!arbitraryDescriptors.empty()
                    && arbitraryDescriptors.front().id == "tone"
                    && arbitraryDescriptors.front().name == "Room Gain"
                    && std::abs(arbitraryDescriptors.front().currentValue - 0.22) < 0.001,
                "Shell automation routed by authored id must update the published slot-backed runtime descriptor.");

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
