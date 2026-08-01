#include "drs/engine/PerformancePublishCommandAdapter.h"
#include "drs/engine/PublishedMacroBinding.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace
{
using namespace drs::engine;

constexpr std::array<std::string_view, 9> redSeams {
    "binding-3-exposed",
    "binding-12-exposed",
    "binding-16-authored",
    "overflow-13-exposed",
    "overflow-17-authored",
    "invalid-dsp-target",
    "failed-then-successful-recovery",
    "processor-topology-lifecycle",
    "shell-diagnostic-parity"
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

std::shared_ptr<const PerformancePublishPresentationSnapshot> waitForPublishSettlement(
    drs::plugin::Processor& processor,
    const std::size_t revision)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        crossBoundary(processor);
        const auto presentation = processor.getPerformancePublishPresentationSnapshot();
        if (presentation != nullptr
            && (presentation->state == PerformancePublishPresentationState::active
                || presentation->state == PerformancePublishPresentationState::failed)
            && (presentation->activePublishedRevision == revision
                || presentation->failedRevision == revision))
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
    require(!processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "The overflow fixture must be rejected before worker preparation.");
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
    require(!processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "A missing DSP control must be rejected before worker preparation.");
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
    require(!processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "The over-capacity replacement must reject before preparation.");
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
        require(!processor.submitPerformancePublishCommand(
                    {}, PerformancePublishCommandSource::externalApi),
                "The thirteen-control fixture must reject before worker preparation.");
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
