#include "drs/engine/PerformancePublishCommandAdapter.h"
#include "drs/engine/PublishedMacroBinding.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/HostSessionState.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace
{
using namespace drs::engine;

constexpr std::array<std::string_view, 14> redSeams {
    "binding-3-exposed",
    "binding-12-exposed",
    "binding-16-authored",
    "overflow-13-exposed",
    "overflow-17-authored",
    "invalid-dsp-target",
    "failed-then-successful-recovery",
    "processor-topology-lifecycle",
    "shell-diagnostic-parity",
    "published-presentation-model",
    "published-presentation-rename",
    "host-state-roundtrip",
    "automation-boundary-16-slots",
    "republish-churn-realtime"
};

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool isKnownSeam(const std::string_view seam)
{
    return std::find(redSeams.begin(), redSeams.end(), seam) != redSeams.end();
}

RuntimeProjectModel loadThreeLayerFixture()
{
    const auto load = loadRuntimeProjectManifest(DRS_PERFORMANCE_MIXER_S0_FIXTURE_PATH);
    require(load.loaded,
            "The Sprint 0 three-layer mixer fixture must load: "
                + (load.issues.empty() ? std::string("unknown loader failure") : load.issues.front()));

    const auto& authoring = load.project.authoring;
    require(authoring.groups.size() == 3 && authoring.fxSlots.size() == 3
                && authoring.macros.size() == 3,
            "The fixture must provide Bell, EPiano, and Plucks groups with one Gain insert and one macro each.");
    for (const auto& macro : authoring.macros)
    {
        require(macro.exposedInPerformance && macro.targets.size() == 1
                    && macro.targets.front().dspParameterId == "gainDb",
                "Each fixture macro must be exposed and target its group-owned gainDb control.");
    }
    return load.project;
}

RuntimeProjectModel projectWithMacroCapacity(const std::size_t authoredCount,
                                             const std::size_t exposedCount)
{
    auto project = loadThreeLayerFixture();
    project.projectId += "-" + std::to_string(authoredCount) + "-"
        + std::to_string(exposedCount);
    project.authoring.macros.clear();
    project.authoring.macros.reserve(authoredCount);

    // Capacity/topology tests deliberately have no DSP targets. They isolate host
    // slot binding from the separate group-Gain fixture validation above.
    for (std::size_t index = 0; index < authoredCount; ++index)
    {
        RuntimeProjectMacroDefinition macro;
        macro.id = "mixer-control-" + std::to_string(index + 1);
        macro.name = "Mixer Control " + std::to_string(index + 1);
        macro.defaultValue = 0.5;
        macro.minValue = 0.0;
        macro.maxValue = 1.0;
        macro.exposedInPerformance = index < exposedCount;
        project.authoring.macros.push_back(std::move(macro));
    }
    return project;
}

void crossBoundary(drs::plugin::Processor& processor)
{
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    buffer.clear();
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

void setHostMacroValue(drs::plugin::Processor& processor,
                       const std::string& hostParameterId,
                       const double value)
{
    const auto* parameter = processor.getParameterState().getParameter(hostParameterId);
    require(parameter != nullptr, "Fixed host parameter '" + hostParameterId + "' must exist.");
    const_cast<juce::RangedAudioParameter*>(parameter)->setValueNotifyingHost(static_cast<float>(value));
}

bool waitForRestoredPerformance(drs::plugin::Processor& processor)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        crossBoundary(processor);
        const auto restore = processor.getProjectRestoreSnapshot();
        const auto publish = processor.getPerformancePublishControllerSnapshot();
        if (restore != nullptr
            && restore->state == ProjectRestoreState::active
            && publish.hasActiveRequest
            && publish.activationState == PerformancePublishActivationState::active)
            return true;
        if (restore != nullptr
            && (restore->state == ProjectRestoreState::failed || restore->state == ProjectRestoreState::needsLocation))
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

const EngineMacroDescriptor& descriptorForRuntimeId(const std::vector<EngineMacroDescriptor>& descriptors,
                                                     const std::string& runtimeId)
{
    const auto found = std::find_if(descriptors.begin(), descriptors.end(), [&](const auto& descriptor)
    {
        return descriptor.id == runtimeId;
    });
    require(found != descriptors.end(), "Published runtime descriptor '" + runtimeId + "' must exist.");
    return *found;
}

std::shared_ptr<const PerformancePublishPresentationSnapshot> waitForPublishSettlement(
    drs::plugin::Processor& processor,
    const std::size_t revision)
{
    const auto expectedProjectGeneration
        = processor.getEngineFacade().getPerformancePublishProjectGeneration();
    std::optional<PerformancePublishRequestIdentity> requestedIdentity;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        crossBoundary(processor);
        const auto presentation = processor.getPerformancePublishPresentationSnapshot();
        const auto controller = processor.getPerformancePublishControllerSnapshot();
        if (controller.hasRequest
            && controller.currentRequest.identity.projectGeneration == expectedProjectGeneration
            && controller.currentRequest.identity.draftRevision == revision)
        {
            requestedIdentity = controller.currentRequest.identity;
        }
        const auto matchingActive = requestedIdentity.has_value()
            && controller.hasActiveRequest
            && controller.activeRequestIdentity == *requestedIdentity;
        const auto matchingFailure = requestedIdentity.has_value()
            && controller.hasFailedRequest
            && controller.failedRequestIdentity == *requestedIdentity;
        if (presentation != nullptr
            && ((presentation->state == PerformancePublishPresentationState::active
                    && matchingActive)
                || (presentation->state == PerformancePublishPresentationState::failed
                    && matchingFailure)))
        {
            return presentation;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return processor.getPerformancePublishPresentationSnapshot();
}

void requireFixedTopology(const drs::plugin::Processor& processor)
{
    const auto& topology = publishedMacroHostTopology();
    require(maximumExposedPerformanceControls == 12,
            "The product-visible capacity must remain twelve controls.");
    require(topology.size() == maximumPublishedMacroHostSlots,
            "The contract topology must declare all sixteen host slots.");
    require(processor.getParameters().size() == topology.size(),
            "The processor must create all sixteen host parameters before any project is loaded.");

    for (const auto& slot : topology)
    {
        const auto* parameter = processor.getParameterState().getParameter(slot.hostParameterId);
        require(slot.slotIndex < topology.size() && parameter != nullptr
                    && processor.getParameters()[static_cast<int>(slot.slotIndex)] == parameter,
                "The processor is missing ordered host parameter '"
                    + std::string(slot.hostParameterId) + "'.");
    }
}

void requireActiveBindingCount(const std::size_t authoredCount, const std::size_t exposedCount)
{
    auto project = authoredCount == 3 && exposedCount == 3
        ? loadThreeLayerFixture()
        : projectWithMacroCapacity(authoredCount, exposedCount);
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    require(processor.replaceAuthoringProject(project),
            "The capacity fixture must be accepted as an authoring project.");
    requireFixedTopology(processor);

    const auto revision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "The capacity fixture must submit a publish request.");
    const auto presentation = waitForPublishSettlement(processor, revision);
    require(presentation != nullptr
                && presentation->state == PerformancePublishPresentationState::active
                && presentation->activePublishedRevision == revision,
            "All authored controls within the fixed capacity must publish and activate.");
    const auto bindings = processor.getEngineFacade().getActivePublishedMacroBindings();
    require(bindings != nullptr && bindings->bindings.size() == maximumPublishedMacroHostSlots
                && bindings->assignedExposedCount == exposedCount
                && bindings->assignedHiddenCount == authoredCount - exposedCount,
            "Published bindings must retain the full topology and exact exposed/hidden counts.");
    require(std::all_of(publishedMacroHostTopology().begin(), publishedMacroHostTopology().end(),
                        [&](const auto& slot)
                        {
                            return slot.slotIndex < bindings->bindings.size()
                                && bindings->bindings[slot.slotIndex].hostSlotIndex == slot.slotIndex
                                && bindings->bindings[slot.slotIndex].hostParameterId
                                    == slot.hostParameterId;
                        }),
            "Every published binding must use the canonical host-slot order and parameter ID.");
    if (authoredCount == 3 && exposedCount == 3)
    {
        require(std::all_of(bindings->bindings.begin(), bindings->bindings.end(), [](const auto& binding)
                            {
                                return !binding.assigned
                                    || binding.renderTarget == PublishedMacroRenderTarget::dspControl;
                            }),
                "The Bell, EPiano, and Plucks fixture must bind all exposed controls to group Gain DSP targets.");
    }
    if (authoredCount >= 12)
    {
        processor.setMacroValueFromShell("mixer-control-3", 0.22);
        const auto descriptors = processor.getEngineFacade().getMacroDescriptors();
        const auto genericSlot = std::find_if(descriptors.begin(), descriptors.end(), [](const auto& descriptor)
                                              {
                                                  return descriptor.id == "slot.3";
                                              });
        require(genericSlot != descriptors.end()
                    && genericSlot->currentValue > 0.219 && genericSlot->currentValue < 0.221,
                "Automation written through an authored control must reach its assigned generic host slot.");
    }
}

void requireOverflowFinding(const std::size_t authoredCount,
                            const std::size_t exposedCount,
                            const std::string& expectedFinding)
{
    auto project = projectWithMacroCapacity(authoredCount, exposedCount);
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    require(processor.replaceAuthoringProject(project),
            "The overflow fixture must be accepted as a draft before preflight.");
    requireFixedTopology(processor);

    const auto revision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "The overflow fixture must queue for background preflight.");
    const auto presentation = waitForPublishSettlement(processor, revision);
    require(presentation != nullptr
                && presentation->state == PerformancePublishPresentationState::failed
                && presentation->findingCode == expectedFinding
                && presentation->exposedMacroCount == exposedCount
                && presentation->hiddenMacroCount == authoredCount - exposedCount
                && presentation->unassignedMacroCount == authoredCount
                && presentation->availableHostSlotCount == maximumPublishedMacroHostSlots,
            "Overflow must fail preflight with the exact capacity finding '" + expectedFinding + "'.");
}

void requireInvalidDspTargetFinding()
{
    auto project = loadThreeLayerFixture();
    project.authoring.macros.front().targets.front().dspParameterId = "missing-gain-control";
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    require(processor.replaceAuthoringProject(project),
            "The invalid-target fixture must be accepted as a draft before preflight.");
    const auto revision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "A missing DSP control must queue for background preflight.");
    const auto presentation = waitForPublishSettlement(processor, revision);
    require(presentation != nullptr
                && presentation->state == PerformancePublishPresentationState::failed
                && presentation->findingCode == "published-macro-dsp-target-missing"
                && presentation->findingMessage.find("Bell Gain") != std::string::npos,
            "Target preflight must identify the affected macro and missing DSP control.");
}

void requireFailureThenSuccessfulRecovery()
{
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    const auto validProject = projectWithMacroCapacity(3, 3);
    require(processor.replaceAuthoringProject(validProject), "The valid recovery fixture must load.");
    const auto firstRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "The initial valid fixture must publish.");
    const auto firstActive = waitForPublishSettlement(processor, firstRevision);
    require(firstActive != nullptr && firstActive->state == PerformancePublishPresentationState::active,
            "The initial valid fixture must become active.");

    auto rejectedProject = projectWithMacroCapacity(13, 13);
    rejectedProject.projectId = validProject.projectId;
    require(processor.replaceAuthoringProject(rejectedProject),
            "The over-capacity replacement must load as a draft.");
    const auto rejectedRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "The over-capacity replacement must queue for background preflight.");
    const auto rejected = waitForPublishSettlement(processor, rejectedRevision);
    require(rejected != nullptr && rejected->state == PerformancePublishPresentationState::failed
                && rejected->findingCode == "published-macro-exposed-capacity-exceeded"
                && rejected->hasLastKnownGood
                && rejected->activePublishedRevision == firstRevision
                && rejected->canPublish,
            "A rejected replacement must retain last-known-good Performance and allow an immediate retry.");

    auto correctedProject = validProject;
    correctedProject.displayName += " corrected";
    require(processor.replaceAuthoringProject(correctedProject),
            "The corrected recovery fixture must replace the rejected draft.");
    const auto retryRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "A corrected draft must submit without restarting the processor.");
    const auto retried = waitForPublishSettlement(processor, retryRevision);
    require(retried != nullptr && retried->state == PerformancePublishPresentationState::active
                && retried->activePublishedRevision == retryRevision
                && !retried->hasFailure,
            "A corrected draft must become active after a failed publication without restart.");
}

void requireLifecycleTopology()
{
    auto project = projectWithMacroCapacity(3, 3);
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    requireFixedTopology(processor);
    const auto firstIds = [&processor]
    {
        std::array<std::string, maximumPublishedMacroHostSlots> ids {};
        for (const auto& slot : publishedMacroHostTopology())
            ids[slot.slotIndex] = slot.hostParameterId;
        return ids;
    }();

    require(processor.replaceAuthoringProject(project), "Project load must succeed.");
    requireFixedTopology(processor);
    const auto publishRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "The loaded project must submit a Publish request.");
    const auto activePresentation = waitForPublishSettlement(processor, publishRevision);
    require(activePresentation != nullptr
                && activePresentation->state == PerformancePublishPresentationState::active
                && activePresentation->activePublishedRevision == publishRevision,
            "Publish must not change the fixed host parameter topology.");
    requireFixedTopology(processor);
    auto edited = project;
    edited.displayName += " edited";
    require(processor.replaceAuthoringProject(edited), "Project edit must succeed.");
    requireFixedTopology(processor);
    processor.closeAuthoringProject({});
    requireFixedTopology(processor);
    require(processor.replaceAuthoringProject(project), "Project restore must succeed.");
    requireFixedTopology(processor);
    for (const auto& slot : publishedMacroHostTopology())
        require(firstIds[slot.slotIndex] == slot.hostParameterId,
                "Host IDs must remain ordered and unchanged across project lifecycle operations.");
}

void requireShellDiagnosticParity()
{
    const auto project = projectWithMacroCapacity(13, 13);
    auto publishAndReadFailure = [&](drs::plugin::Processor& processor)
    {
        processor.prepareToPlay(48000.0, 256);
        require(processor.replaceAuthoringProject(project), "The thirteen-control fixture must load as a draft.");
        const auto revision = processor.getAuthoringSession().getDocumentState().revision;
        require(processor.submitPerformancePublishCommand(
                    {}, PerformancePublishCommandSource::externalApi),
                "The thirteen-control fixture must queue for background preflight.");
        const auto presentation = waitForPublishSettlement(processor, revision);
        const auto controller = processor.getPerformancePublishControllerSnapshot();
        require(presentation != nullptr
                    && presentation->state == PerformancePublishPresentationState::failed,
                "The over-capacity fixture must reach a visible failed Publish state "
                    "(state=" + (presentation != nullptr ? presentation->stateLabel : "null")
                    + ", finding=" + controller.failureFinding.code + ").");
        return presentation;
    };

    drs::plugin::Processor pluginProcessor;
    const auto pluginPresentation = publishAndReadFailure(pluginProcessor);
    juce::ScopedJuceInitialiser_GUI gui;
    drs::standalone::MainComponent standalone(false);
    const auto standalonePresentation = publishAndReadFailure(standalone.getProcessor());

    constexpr std::string_view detailedFinding = "published-macro-exposed-capacity-exceeded";
    require(pluginPresentation->findingCode == detailedFinding
                && standalonePresentation->findingCode == detailedFinding
                && pluginPresentation->findingMessage == standalonePresentation->findingMessage,
            "Both shell surfaces must retain the detailed exposed-slot finding and identical repair text.");
}

void requirePublishedPresentationModel()
{
    const auto project = loadThreeLayerFixture();
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    require(processor.replaceAuthoringProject(project), "The presentation fixture must load.");
    const auto revision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand({}, PerformancePublishCommandSource::externalApi),
            "The presentation fixture must publish.");
    const auto presentation = waitForPublishSettlement(processor, revision);
    require(presentation != nullptr && presentation->state == PerformancePublishPresentationState::active,
            "The presentation fixture must become active.");

    const auto bindings = processor.getEngineFacade().getActivePublishedMacroBindings();
    require(bindings != nullptr, "Active published bindings must carry presentation metadata.");
    const std::array<std::string, 3> labels { "Bell Gain", "EPiano Gain", "Plucks Gain" };
    const std::array<std::string, 3> sections { "Bell", "EPiano", "Plucks" };
    for (std::size_t index = 0; index < labels.size(); ++index)
    {
        const auto binding = std::find_if(bindings->bindings.begin(), bindings->bindings.end(),
                                          [&](const auto& candidate) { return candidate.publishedName == labels[index]; });
        require(binding != bindings->bindings.end()
                    && binding->presentation.authoredLabel == labels[index]
                    && binding->presentation.sectionLabel == sections[index]
                    && binding->presentation.parameterLabel == "Gain"
                    && binding->presentation.valueUnit == "dB"
                    && binding->presentation.controlKind == PublishedMacroControlKind::fader
                    && binding->presentation.authoredOrder == index
                    && !binding->presentation.accessibilityDescription.empty(),
                "Each published group gain must retain immutable label, source, unit, kind, and authored order.");
    }

    const auto descriptors = processor.getEngineFacade().getMacroDescriptors();
    const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [](const auto& candidate)
    {
        return candidate.name == "Bell Gain";
    });
    require(descriptor != descriptors.end()
                && descriptor->sectionLabel == "Bell"
                && descriptor->parameterLabel == "Gain"
                && descriptor->valueUnit == "dB"
                && descriptor->controlKind == PublishedMacroControlKind::fader
                && !descriptor->accessibilityDescription.empty(),
            "Perform descriptors must expose published presentation metadata without authoring traversal.");
}

void requirePublishedPresentationRename()
{
    auto project = loadThreeLayerFixture();
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    require(processor.replaceAuthoringProject(project), "The rename fixture must load.");
    const auto firstRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand({}, PerformancePublishCommandSource::externalApi),
            "The initial rename fixture must publish.");
    require(waitForPublishSettlement(processor, firstRevision)->state == PerformancePublishPresentationState::active,
            "The initial rename fixture must become active.");
    const auto firstBindings = processor.getEngineFacade().getActivePublishedMacroBindings();
    const auto firstBell = std::find_if(firstBindings->bindings.begin(), firstBindings->bindings.end(), [](const auto& binding)
    {
        return binding.stableAuthoredId == "bell-gain";
    });
    require(firstBell != firstBindings->bindings.end(), "Bell Gain must receive a fixed host slot.");
    const auto firstHostId = firstBell->hostParameterId;
    constexpr std::string_view macroPrefix { "macro." };
    require(firstHostId.rfind(macroPrefix.data(), 0) == 0
                && processor.getEngineFacade().setMacroValue(firstHostId.substr(macroPrefix.size()), 0.23),
            "The active Bell Gain host binding must accept a migrated current value.");

    project.authoring.groups.front().displayName = "Bell Pad";
    require(processor.replaceAuthoringProject(project), "The renamed draft must load.");
    const auto renamedRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand({}, PerformancePublishCommandSource::externalApi),
            "The renamed draft must publish.");
    const auto renamedPresentation = waitForPublishSettlement(processor, renamedRevision);
    require(renamedPresentation != nullptr && renamedPresentation->state == PerformancePublishPresentationState::active,
            "The renamed draft must become active.");
    const auto renamedBindings = processor.getEngineFacade().getActivePublishedMacroBindings();
    const auto renamedBell = std::find_if(renamedBindings->bindings.begin(), renamedBindings->bindings.end(), [](const auto& binding)
    {
        return binding.stableAuthoredId == "bell-gain";
    });
    require(renamedBell != renamedBindings->bindings.end()
                && renamedBell->hostParameterId == firstHostId
                && renamedBell->presentation.sectionLabel == "Bell Pad"
                && std::abs(renamedBell->publishedValue - 0.23) < 0.001,
            "A source rename must update only the next published presentation snapshot while preserving host identity and value"
                " (host=" + (renamedBell != renamedBindings->bindings.end() ? renamedBell->hostParameterId : "missing")
                + ", section=" + (renamedBell != renamedBindings->bindings.end()
                                      ? renamedBell->presentation.sectionLabel : "missing")
                + ", value=" + (renamedBell != renamedBindings->bindings.end()
                                    ? std::to_string(renamedBell->publishedValue) : "missing") + ").");
}

void requireHostStateRoundTrip()
{
    const auto fixture = loadThreeLayerFixture();
    drs::plugin::Processor dspSource;
    dspSource.prepareToPlay(48000.0, 256);
    require(dspSource.replaceAuthoringProject(fixture), "The DSP host-state fixture must load.");
    const auto revision = dspSource.getAuthoringSession().getDocumentState().revision;
    require(dspSource.submitPerformancePublishCommand({}, PerformancePublishCommandSource::externalApi),
            "The DSP host-state fixture must publish.");
    require(waitForPublishSettlement(dspSource, revision)->state == PerformancePublishPresentationState::active,
            "The DSP host-state fixture must become active.");
    setHostMacroValue(dspSource, "macro.tone", 0.21);
    setHostMacroValue(dspSource, "macro.motion", 0.47);
    setHostMacroValue(dspSource, "macro.slot.3", 0.83);
    crossBoundary(dspSource);
    dspSource.serviceMessageThreadWork();

    juce::MemoryBlock dspState;
    require(dspSource.waitForHostStatePublication(),
            "The DSP mixer checkpoint did not reach the background host-state publication.");
    dspSource.getStateInformation(dspState);
    const auto parsedDspState = parseHostSessionState(std::string(
        static_cast<const char*>(dspState.getData()), dspState.getSize()));
    require(parsedDspState.isValidHostState()
                && parsedDspState.hostState->presetState.dspMacroTargets.size() == 3
                && std::any_of(parsedDspState.hostState->presetState.macroValues.begin(),
                               parsedDspState.hostState->presetState.macroValues.end(), [](const auto& value)
                               { return value.id == "slot.3" && std::abs(value.value - 0.83) < 0.001; }),
            "A DSP-targeted three-control mixer must serialize target identity and fixed-slot values.");

    const auto projectPath = getPhase2ReferenceProjectManifestPath();
    const auto project = loadRuntimeProjectManifest(projectPath);
    require(project.loaded, "The validated host recall project must load.");
    drs::plugin::Processor source;
    source.prepareToPlay(48000.0, 256);
    require(source.replaceAuthoringProject(project.project, juce::File(projectPath)),
            "The validated host recall project must load.");
    const auto sourceRevision = source.getAuthoringSession().getDocumentState().revision;
    require(source.submitPerformancePublishCommand({}, PerformancePublishCommandSource::externalApi),
            "The validated host recall project must publish.");
    require(waitForPublishSettlement(source, sourceRevision)->state == PerformancePublishPresentationState::active,
            "The validated host recall project must become active.");
    setHostMacroValue(source, "macro.tone", 0.21);
    setHostMacroValue(source, "macro.motion", 0.47);
    crossBoundary(source);
    source.serviceMessageThreadWork();

    juce::MemoryBlock state;
    require(source.waitForHostStatePublication(),
            "The active mixer checkpoint did not reach the background host-state publication.");
    source.getStateInformation(state);
    require(state.getSize() > 0, "An active published mixer must produce host state.");

    drs::plugin::Processor restored;
    restored.prepareToPlay(48000.0, 256);
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    require(waitForRestoredPerformance(restored),
            "Restored host state must rebuild the exact published Performance identity.");
    requireFixedTopology(restored);
    const auto bindings = restored.getEngineFacade().getActivePublishedMacroBindings();
    require(bindings != nullptr && bindings->assignedExposedCount == 2,
            "A compatibility host-state restore must preserve the permanent first two published bindings.");
    const auto descriptors = restored.getEngineFacade().getMacroDescriptors();
    require(std::abs(descriptorForRuntimeId(descriptors, "tone").currentValue - 0.21) < 0.001
                && std::abs(descriptorForRuntimeId(descriptors, "motion").currentValue - 0.47) < 0.001,
            "A compatibility host-state round trip must retain Tone and Motion values by slot.");

    auto twelve = projectWithMacroCapacity(12, 12);
    drs::plugin::Processor twelveSource;
    twelveSource.prepareToPlay(48000.0, 256);
    require(twelveSource.replaceAuthoringProject(twelve), "The twelve-control host-state fixture must load.");
    const auto twelveRevision = twelveSource.getAuthoringSession().getDocumentState().revision;
    require(twelveSource.submitPerformancePublishCommand({}, PerformancePublishCommandSource::externalApi),
            "The twelve-control host-state fixture must publish.");
    require(waitForPublishSettlement(twelveSource, twelveRevision)->state == PerformancePublishPresentationState::active,
            "The twelve-control host-state fixture must become active.");
    setHostMacroValue(twelveSource, "macro.slot.12", 0.64);
    crossBoundary(twelveSource);
    twelveSource.serviceMessageThreadWork();
    juce::MemoryBlock twelveState;
    require(twelveSource.waitForHostStatePublication(),
            "The twelve-control checkpoint did not reach the background host-state publication.");
    twelveSource.getStateInformation(twelveState);
    const auto parsedTwelveState = parseHostSessionState(std::string(
        static_cast<const char*>(twelveState.getData()), twelveState.getSize()));
    require(parsedTwelveState.isValidHostState()
                && parsedTwelveState.hostState->publishedState.has_value()
                && std::any_of(parsedTwelveState.hostState->presetState.macroValues.begin(),
                               parsedTwelveState.hostState->presetState.macroValues.end(), [](const auto& value)
                               { return value.id == "slot.12" && std::abs(value.value - 0.64) < 0.001; }),
            "Twelve published controls must serialize the fixed slot-12 value and published identity for host recall.");
}

void requireAutomationBoundaryForAllRelevantSlots()
{
    auto project = projectWithMacroCapacity(16, 12);
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    require(processor.replaceAuthoringProject(project), "The sixteen-slot automation fixture must load.");
    const auto initialRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand({}, PerformancePublishCommandSource::externalApi),
            "The sixteen-slot automation fixture must publish.");
    require(waitForPublishSettlement(processor, initialRevision)->state == PerformancePublishPresentationState::active,
            "The initial sixteen-slot fixture must become active.");

    const std::array<std::pair<std::string, double>, 4> beforeValues {{
        { "macro.tone", 0.11 }, { "macro.slot.3", 0.33 },
        { "macro.slot.12", 0.77 }, { "macro.slot.16", 0.91 }
    }};
    for (const auto& [id, value] : beforeValues)
        setHostMacroValue(processor, id, value);
    crossBoundary(processor);

    project.displayName += " boundary";
    require(processor.replaceAuthoringProject(project), "The compatible replacement must load.");
    const auto replacementRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand({}, PerformancePublishCommandSource::externalApi),
            "The compatible replacement must publish.");
    const std::array<std::pair<std::string, double>, 4> afterValues {{
        { "macro.tone", 0.22 }, { "macro.slot.3", 0.44 },
        { "macro.slot.12", 0.66 }, { "macro.slot.16", 0.88 }
    }};
    for (const auto& [id, value] : afterValues)
        setHostMacroValue(processor, id, value);
    const auto settled = waitForPublishSettlement(processor, replacementRevision);
    require(settled != nullptr && settled->state == PerformancePublishPresentationState::active,
            "Automation written at the replacement boundary must reach the activated generation.");
    crossBoundary(processor);
    const auto descriptors = processor.getEngineFacade().getMacroDescriptors();
    require(std::abs(descriptorForRuntimeId(descriptors, "tone").currentValue - 0.22) < 0.001
                && std::abs(descriptorForRuntimeId(descriptors, "slot.3").currentValue - 0.44) < 0.001
                && std::abs(descriptorForRuntimeId(descriptors, "slot.12").currentValue - 0.66) < 0.001
                && std::abs(descriptorForRuntimeId(descriptors, "slot.16").currentValue - 0.88) < 0.001,
            "Slots 1, 3, 12, and hidden helper 16 must retain their intended automation writes across activation.");
}

void requireRepublishChurnRealtimeSafety()
{
    auto project = projectWithMacroCapacity(16, 12);
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    require(processor.replaceAuthoringProject(project), "The churn fixture must load.");
    for (int iteration = 0; iteration < 6; ++iteration)
    {
        project.displayName = "Sixteen-slot churn " + std::to_string(iteration);
        require(processor.replaceAuthoringProject(project), "Every compatible churn draft must load.");
        const auto revision = processor.getAuthoringSession().getDocumentState().revision;
        require(processor.submitPerformancePublishCommand({}, PerformancePublishCommandSource::externalApi),
                "Every compatible churn draft must submit.");
        for (const auto& slot : publishedMacroHostTopology())
            setHostMacroValue(processor, slot.hostParameterId, (iteration + slot.slotIndex) % 10 / 10.0);
        const auto settled = waitForPublishSettlement(processor, revision);
        require(settled != nullptr && settled->state == PerformancePublishPresentationState::active,
                "Every compatible churn draft must activate.");
    }
    crossBoundary(processor);
    const auto realtime = processor.getRealtimeSafetySnapshot();
    require(realtime.activePublishedMacroRevision == processor.getAuthoringSession().getDocumentState().revision
                && realtime.getAudioThreadViolationCount() == 0,
            "Rapid 16-slot republish churn must leave the final callback view active without realtime violations.");
}

void runSeam(const std::string_view seam)
{
    if (seam == "binding-3-exposed")
        requireActiveBindingCount(3, 3);
    else if (seam == "binding-12-exposed")
        requireActiveBindingCount(12, 12);
    else if (seam == "binding-16-authored")
        requireActiveBindingCount(16, 12);
    else if (seam == "overflow-13-exposed")
        requireOverflowFinding(13, 13, "published-macro-exposed-capacity-exceeded");
    else if (seam == "overflow-17-authored")
        requireOverflowFinding(17, 12, "published-macro-authored-capacity-exceeded");
    else if (seam == "invalid-dsp-target")
        requireInvalidDspTargetFinding();
    else if (seam == "failed-then-successful-recovery")
        requireFailureThenSuccessfulRecovery();
    else if (seam == "processor-topology-lifecycle")
        requireLifecycleTopology();
    else if (seam == "shell-diagnostic-parity")
        requireShellDiagnosticParity();
    else if (seam == "published-presentation-model")
        requirePublishedPresentationModel();
    else if (seam == "published-presentation-rename")
        requirePublishedPresentationRename();
    else if (seam == "host-state-roundtrip")
        requireHostStateRoundTrip();
    else if (seam == "automation-boundary-16-slots")
        requireAutomationBoundaryForAllRelevantSlots();
    else if (seam == "republish-churn-realtime")
        requireRepublishChurnRealtimeSafety();
}
} // namespace

// Sprint 0 created direct-only red characterization. Sprints 1 and 2 promote
// the topology, capacity, diagnostic-parity, and recovery seams to CTest.
int main(int argc, char** argv)
{
    if (argc != 2 || !isKnownSeam(argv[1]))
    {
        std::cerr << "Usage: drs_performance_mixer_s0_red_tests <named-missing-seam>\n";
        for (const auto seam : redSeams)
            std::cerr << "  " << seam << '\n';
        return 2;
    }

    try
    {
        runSeam(argv[1]);
        std::cout << "PASS: Performance mixer contract seam '" << argv[1]
                  << "' is implemented.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "EXPECTED RED: Sprint 0 missing seam '" << argv[1]
                  << "': " << exception.what() << '\n';
        return 1;
    }
}
