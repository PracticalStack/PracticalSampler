#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"
#include "Phase1PerformancePackageSupport.h"
#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/PackageV2StreamingExport.h"
#include "shared/PlayableInstrumentLicenseViewer.h"
#include "shared/WorkspaceMenuPolicy.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;
namespace package_support = drs::tests::performance_package;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

template <typename ComponentType>
ComponentType* findDescendantByType(juce::Component& root)
{
    if (auto* match = dynamic_cast<ComponentType*>(&root))
        return match;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
        if (auto* match = findDescendantByType<ComponentType>(*root.getChildComponent(index)))
            return match;

    return nullptr;
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& componentId)
{
    if (root.getComponentID() == componentId)
        return &root;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
        if (auto* match = findDescendantById(*root.getChildComponent(index), componentId))
            return match;

    return nullptr;
}

std::string summarizeIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return "(none)";

    std::string summary;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index != 0)
            summary += " | ";
        summary += issues[index];
    }
    return summary;
}

float renderQueuedPerformanceSurfaceMagnitude(drs::plugin::Processor& processor,
                                              const int midiNoteNumber,
                                              const float velocity,
                                              const int blockCount = 8,
                                              const bool paceAsAudioDevice = false)
{
    juce::MidiBuffer resetMidiBuffer;
    resetMidiBuffer.addEvent(juce::MidiMessage::controllerEvent(1, 120, 0), 0);
    juce::AudioBuffer<float> resetBuffer(2, 512);
    processor.processBlock(resetBuffer, resetMidiBuffer);

    processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);

    float maxMagnitude = 0.0f;
    juce::MidiBuffer emptyMidiBuffer;
    for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        processor.processBlock(buffer, emptyMidiBuffer);
        maxMagnitude = std::max(maxMagnitude, buffer.getMagnitude(0, buffer.getNumSamples()));
        if (paceAsAudioDevice)
            std::this_thread::sleep_for(std::chrono::milliseconds(12));
    }

    processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
    resetMidiBuffer.clear();
    resetMidiBuffer.addEvent(juce::MidiMessage::controllerEvent(1, 120, 0), 0);
    resetBuffer.clear();
    processor.processBlock(resetBuffer, resetMidiBuffer);
    return maxMagnitude;
}

drs::plugin::PerformancePackageWorkspaceLoadResult loadThroughBackgroundPreparation(
    drs::plugin::Processor& processor,
    const juce::File& packageFile,
    std::chrono::microseconds& preparationElapsed,
    std::chrono::microseconds& activationElapsed)
{
    const auto preparationStarted = Clock::now();
    auto prepared = drs::plugin::preparePerformancePackageWorkspaceInBackground(
        packageFile.getFullPathName().toStdString());
    preparationElapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        Clock::now() - preparationStarted);
    if (!prepared.prepared)
    {
        drs::plugin::PerformancePackageWorkspaceLoadResult failed;
        failed.failureCategory = prepared.failureCategory;
        failed.state = prepared.state;
        failed.issues = prepared.issues;
        failed.timings = prepared.timings;
        return failed;
    }

    const auto activationStarted = Clock::now();
    auto activated = processor.activatePreparedPerformancePackageWorkspace(
        std::move(prepared.activation), packageFile);
    activationElapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        Clock::now() - activationStarted);
    return activated;
}

void requirePerformanceOnlyWorkspace(const drs::engine::WorkspaceDocumentState& state,
                                     const std::string& context)
{
    require(state.kind == drs::engine::WorkspaceDocumentKind::performancePackage,
            context + " should mark the workspace as a performance package.");
    require(state.workspaceMode == drs::engine::WorkspaceMode::performanceOnly,
            context + " should keep the workspace in performance-only mode.");
    require(!state.authoringAvailable,
            context + " should suppress authoring controls.");
}

std::string expectedLicenseDisplayText()
{
    const auto bytes = package_support::buildLicenseTextFixture();
    return { bytes.begin() + 3, bytes.end() };
}

void requireLicenseViewerContract(const std::shared_ptr<const std::string>& licenseText,
                                  const std::string& context)
{
    require(licenseText != nullptr && *licenseText == expectedLicenseDisplayText(),
            context + " should retain the authenticated license display text.");
    drs::app::PlayableInstrumentLicenseViewer viewer(licenseText);
    viewer.setSize(280, 240);
    viewer.resized();
    auto* editor = dynamic_cast<juce::TextEditor*>(
        findDescendantById(viewer, "playableInstrumentLicenseText"));
    auto* closeButton = dynamic_cast<juce::TextButton*>(
        findDescendantById(viewer, "playableInstrumentLicenseCloseButton"));
    require(editor != nullptr && editor->isReadOnly() && editor->isMultiLine()
                && editor->getText().toStdString() == *licenseText
                && closeButton != nullptr && closeButton->getButtonText() == "Close",
            context + " should expose the shared read-only license viewer contract.");
}

fs::path buildSemanticPackageV2Fixture(const fs::path& scratchDirectory)
{
    fs::remove_all(scratchDirectory);
    fs::create_directories(scratchDirectory);
    const auto packagePath = scratchDirectory / "semantic-route-v2.drpkg";
    auto packagePlan = package_support::buildPackagePlan(
        scratchDirectory / "runtime", packagePath);
    packagePlan.manifest.schemaVersion
        = drs::engine::performancePackageFxRoutingSchemaVersion;
    packagePlan.manifest.minimumReaderSchemaVersion
        = drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion;
    packagePlan.compiledRuntime.instrument.schemaVersion
        = std::max(drs::engine::runtimeInstrumentFxRoutingSchemaVersion,
                   drs::engine::instrumentControlInstrumentSchemaVersion);
    for (auto& group : packagePlan.compiledRuntime.instrument.groups)
        group.routingBusId = "master";
    packagePlan.compiledRuntime.instrument.fxSlots = {
        { "host-master-gain", "Host Master Gain", "drs.gain", false, 1,
          { { "gainDb", -3.0 }, { "polarity", 0.0 }, { "mute", 0.0 } } }
    };
    drs::engine::RuntimeMacroDefinition macro;
    macro.id = "Instrument";
    macro.name = "Instrument";
    macro.defaultValue = 0.75;
    macro.exposedInPerformance = true;
    drs::engine::RuntimeProjectMacroTargetDefinition target;
    target.parameterId = "host-master-gain-db";
    target.parameterPath = "fx.host-master-gain.gainDb";
    target.role = "dsp-control";
    target.dspSlotId = "host-master-gain";
    target.dspParameterId = "gainDb";
    target.destinationMinimum = -12.0;
    target.destinationMaximum = 0.0;
    macro.targets.push_back(std::move(target));
    packagePlan.compiledRuntime.instrument.macros = { std::move(macro) };
    drs::engine::RuntimeProjectInstrumentControlDefinition control;
    control.id = "package.instrument.gain";
    control.displayName = "Package Instrument Gain";
    control.category = drs::engine::RuntimeInstrumentControlCategory::mixer;
    control.kind = drs::engine::RuntimeInstrumentControlKind::normalized;
    control.unit = drs::engine::RuntimeInstrumentControlUnit::generic;
    control.normalizedDefault = 1.0;
    control.importedSourceController = 20;
    packagePlan.compiledRuntime.instrument.instrumentControls.push_back(control);
    drs::engine::RuntimeProjectInstrumentControlTargetDefinition controlTarget;
    controlTarget.id = "target.package.instrument.gain";
    controlTarget.controlId = control.id;
    controlTarget.targetKind = drs::engine::RuntimeInstrumentControlTargetKind::gain;
    controlTarget.contributionMode = drs::engine::RuntimeInstrumentControlContributionMode::multiply;
    controlTarget.destinationMinimum = 0.0;
    controlTarget.destinationMaximum = 1.0;
    packagePlan.compiledRuntime.instrument.instrumentControlTargets.push_back(controlTarget);
    drs::engine::RuntimeProjectMidiControlBindingDefinition controlBinding;
    controlBinding.id = "binding.package.instrument.gain.cc20";
    controlBinding.controlId = control.id;
    controlBinding.controllerNumber = 20;
    packagePlan.compiledRuntime.instrument.midiControlBindings.push_back(controlBinding);
    drs::engine::RuntimeProjectInstrumentControlDefinition channelControl = control;
    channelControl.id = "package.instrument.channel.gain";
    channelControl.displayName = "Channel Scoped Instrument Gain";
    channelControl.importedSourceController = 21;
    packagePlan.compiledRuntime.instrument.instrumentControls.push_back(channelControl);
    drs::engine::RuntimeProjectInstrumentControlTargetDefinition channelTarget = controlTarget;
    channelTarget.id = "target.package.instrument.channel.gain";
    channelTarget.controlId = channelControl.id;
    packagePlan.compiledRuntime.instrument.instrumentControlTargets.push_back(channelTarget);
    drs::engine::RuntimeProjectMidiControlBindingDefinition channelBinding = controlBinding;
    channelBinding.id = "binding.package.instrument.channel.gain.cc21.ch2";
    channelBinding.controlId = channelControl.id;
    channelBinding.controllerNumber = 21;
    channelBinding.channelScope.kind = drs::engine::RuntimeMidiChannelScopeKind::exact;
    channelBinding.channelScope.channel = 2;
    packagePlan.compiledRuntime.instrument.midiControlBindings.push_back(channelBinding);
    packagePlan.compiledRuntime.instrument.routingBuses = {
        { "host-master-bus", "Host Master Bus", "master",
          { "host-master-gain" }, false }
    };
    const auto v2Plan = drs::engine::buildPerformancePackageV2StreamingExportPlan(
        packagePlan.manifest,
        packagePlan.compiledRuntime,
        packagePath.generic_string(),
        packagePlan.additionalPayloads);
    require(v2Plan.built,
            "The semantic package-v2 host fixture plan must build.");
    const auto written = drs::engine::writePackageV2Streaming(v2Plan.plan);
    require(written.written && written.verified && written.atomicallyPublished,
            "The semantic package-v2 host fixture must export and verify.");
    return packagePath;
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        const auto* captureStatePathValue
            = std::getenv("DRS_PACKAGE_HOST_STATE_CAPTURE_PATH");
        const auto captureStatePath = captureStatePathValue != nullptr
                && *captureStatePathValue != '\0'
            ? fs::absolute(fs::path(captureStatePathValue))
            : fs::path {};
        const auto generatedFixtureRoot = captureStatePath.empty()
            ? fs::temp_directory_path() / "drs-semantic-package-v2-host-validation"
            : captureStatePath.parent_path() / "fx-routing-package-fixture";
        const auto selectedPackage = argc >= 2
            ? std::filesystem::absolute(std::filesystem::path(argv[1]))
            : buildSemanticPackageV2Fixture(generatedFixtureRoot);
        const auto largeV2Qualification = argc >= 2;
        require(std::filesystem::exists(selectedPackage),
                "The selected performance package fixture must exist for host validation.");

        juce::ScopedJuceInitialiser_GUI gui;

        const auto packageFile = juce::File(juce::String::fromUTF8(selectedPackage.generic_string().c_str()));

        auto standalone = std::make_unique<drs::standalone::MainComponent>(false);
        std::chrono::microseconds standalonePreparationElapsed {};
        std::chrono::microseconds standaloneActivationElapsed {};
        const auto standaloneLoad = loadThroughBackgroundPreparation(
            standalone->getProcessor(), packageFile,
            standalonePreparationElapsed, standaloneActivationElapsed);
        const auto standaloneOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            standalonePreparationElapsed + standaloneActivationElapsed);
        require(standaloneLoad.loaded,
                "Standalone host validation should load the checked-in package. state="
                    + standaloneLoad.state + " issues=" + summarizeIssues(standaloneLoad.issues));
        require(standaloneLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::none,
                "Standalone host validation should not publish a package failure category for the valid fixture.");
        requirePerformanceOnlyWorkspace(standalone->getProcessor().getWorkspaceDocumentState(),
                                        "Standalone host validation");
        const auto standaloneLicense
            = standalone->getEngineFacade().getPerformancePackageLicenseText();
        require(drs::app::shouldShowViewLicenseMenuItem(true, standaloneLicense != nullptr),
                "Standalone Performance File menu should expose View License.");
        requireLicenseViewerContract(standaloneLicense, "Standalone host validation");
        standalone->getProcessor().prepareToPlay(44100.0, 512);
        standalone->getProcessor().serviceMessageThreadWork();
        standalone->getProcessor().releaseResources();
        standalone->getProcessor().prepareToPlay(48000.0, 256);
        standalone->getProcessor().serviceMessageThreadWork();
        standalone->getProcessor().releaseResources();
        standalone->getProcessor().prepareToPlay(44100.0, 512);
        standalone->getProcessor().serviceMessageThreadWork();
        const auto standalonePayload
            = standalone->getEngineFacade().getPerformancePackageActivationPayload();
        require(standalonePayload != nullptr && standalonePayload->snapshot != nullptr,
                "Standalone host validation did not retain the package graph snapshot.");
        const auto standaloneGraph = drs::engine::compileDspGraphPlan(
            *standalonePayload->snapshot);
        require(standaloneGraph.compiled && standaloneGraph.plan.nodes.size() == 1
                    && standaloneGraph.plan.nodes.front().slotId == "host-master-gain",
                "Standalone host validation did not compile the package DSP graph.");

        const auto standalonePlaybackStarted = Clock::now();
        const auto standaloneMagnitude = renderQueuedPerformanceSurfaceMagnitude(
            standalone->getProcessor(), 69, 0.8f,
            largeV2Qualification ? 375 : 8, largeV2Qualification);
        const auto standalonePlaybackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - standalonePlaybackStarted);
        standalone->getProcessor().serviceMessageThreadWork();
        require(standaloneMagnitude > 0.0001f,
                "Standalone host validation should produce audible output from the checked-in package."
                " magnitude=" + std::to_string(standaloneMagnitude)
                    + " controls=" + std::to_string(standalone->getEngineFacade()
                                                         .getInstrumentControlDescriptors().size()));
        const auto renderChannelScopedGain = [](drs::plugin::Processor& host,
                                                const int midiChannel,
                                                const int ccValue)
        {
            require(host.setInstrumentControlValueFromShell(
                        "package.instrument.gain", 1.0),
                    "Host matrix could not prime the any-channel gain control.");
            require(host.setInstrumentControlValueFromShell(
                        "package.instrument.channel.gain", 1.0),
                    "Host matrix could not prime the exact-channel gain control.");
            juce::MidiBuffer controlMidi;
            controlMidi.addEvent(juce::MidiMessage::controllerEvent(
                                     midiChannel, 21, ccValue), 0);
            juce::AudioBuffer<float> controlBuffer(2, 512);
            controlBuffer.clear();
            host.processBlock(controlBuffer, controlMidi);
            return renderQueuedPerformanceSurfaceMagnitude(host, 69, 0.8f);
        };
        const auto standaloneAbsentSourceMagnitude = renderChannelScopedGain(
            standalone->getProcessor(), 1, 0);
        const auto standaloneExactChannelMagnitude = renderChannelScopedGain(
            standalone->getProcessor(), 2, 0);
        require(standaloneAbsentSourceMagnitude > 0.0001f
                    && standaloneAbsentSourceMagnitude > standaloneExactChannelMagnitude * 2.5f,
                "Standalone host matrix did not distinguish an absent source from exact MIDI channel 2."
                    + std::string(" absent=") + std::to_string(standaloneAbsentSourceMagnitude)
                    + " exact=" + std::to_string(standaloneExactChannelMagnitude));
        const auto renderStandaloneUnassignedSource = [&](const int controller,
                                                           const int value)
        {
            require(standalone->getProcessor().setInstrumentControlValueFromShell(
                        "package.instrument.gain", 1.0),
                    "Standalone host matrix could not prime the unassigned-source case.");
            require(standalone->getProcessor().setInstrumentControlValueFromShell(
                        "package.instrument.channel.gain", 1.0),
                    "Standalone host matrix could not prime the exact-channel case.");
            juce::MidiBuffer reservedMidi;
            reservedMidi.addEvent(juce::MidiMessage::controllerEvent(1, controller, value), 0);
            juce::AudioBuffer<float> reservedBuffer(2, 512);
            reservedBuffer.clear();
            standalone->getProcessor().processBlock(reservedBuffer, reservedMidi);
            return renderQueuedPerformanceSurfaceMagnitude(standalone->getProcessor(), 69, 0.8f);
        };
        require(renderStandaloneUnassignedSource(22, 0) > 0.0001f,
                "Standalone unassigned CC source must not silence playback.");
        for (const auto reservedController : { 64, 120, 123 })
            require(renderStandaloneUnassignedSource(reservedController, 0) > 0.0001f,
                    "Standalone reserved CC semantics must preserve playable output.");
        const auto standaloneDiagnostics = standalone->getEngineFacade().getDiagnosticsSnapshot();
        const auto standaloneRealtime = standalone->getProcessor().getRealtimeSafetySnapshot();
        require(standaloneDiagnostics.available,
                "Standalone host validation should publish diagnostics after opening the checked-in package.");
        require(standaloneRealtime.getAudioThreadViolationCount() == 0,
                "Standalone package playback reported a realtime-thread violation.");
        require(standaloneRealtime.blockingLockAttemptsOnAudioThread == 0,
                "Standalone package playback attempted a blocking audio-thread lock.");
        require(!largeV2Qualification
                    || (standaloneDiagnostics.cacheMissCount > 0
                        && standaloneDiagnostics.backgroundReadCount > 0),
                "Standalone large-package playback did not service pages beyond the resident head.");
        require(standaloneOpenElapsed <= std::chrono::milliseconds(3000),
                "Standalone package open exceeded the reviewed host-validation budget.");
        require(standalonePlaybackElapsed <= (largeV2Qualification
                    ? std::chrono::milliseconds(6000) : std::chrono::milliseconds(1000)),
                "Standalone initial package playback exceeded the reviewed host-validation budget.");

        auto processor = std::make_unique<drs::plugin::Processor>();
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());
        require(editor != nullptr, "Plugin editor should construct for package host validation.");
        auto* initialPerformancePanel
            = findDescendantByType<drs::app::PerformancePanel>(*editor);
        auto* initialControlsToggle = dynamic_cast<juce::Button*>(
            findDescendantById(*editor, "performanceMacroStripToggleButton"));
        require(initialPerformancePanel != nullptr && initialControlsToggle != nullptr,
                "Plugin editor should expose the Instrument Controls disclosure.");
        initialPerformancePanel->refreshNow();
        require(initialControlsToggle->getButtonText() == "Show Controls",
                "A newly opened plugin should default Instrument Controls to hidden.");

        std::chrono::microseconds pluginPreparationElapsed {};
        std::chrono::microseconds pluginActivationElapsed {};
        const auto pluginLoad = loadThroughBackgroundPreparation(
            *processor, packageFile, pluginPreparationElapsed, pluginActivationElapsed);
        const auto pluginOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            pluginPreparationElapsed + pluginActivationElapsed);
        require(pluginLoad.loaded,
                "Plugin host validation should load the checked-in package. state="
                    + pluginLoad.state + " issues=" + summarizeIssues(pluginLoad.issues));
        require(pluginLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::none,
                "Plugin host validation should not publish a package failure category for the valid fixture.");
        requirePerformanceOnlyWorkspace(processor->getWorkspaceDocumentState(),
                                        "Plugin host validation");
        const auto pluginLicense
            = processor->getEngineFacade().getPerformancePackageLicenseText();
        require(drs::app::shouldShowViewLicenseMenuItem(true, pluginLicense != nullptr),
                "Plugin Performance File menu should expose View License.");
        requireLicenseViewerContract(pluginLicense, "Plugin host validation");
        processor->prepareToPlay(44100.0, 512);
        processor->serviceMessageThreadWork();
        processor->releaseResources();
        processor->prepareToPlay(48000.0, 256);
        processor->serviceMessageThreadWork();
        processor->releaseResources();
        processor->prepareToPlay(44100.0, 512);
        processor->serviceMessageThreadWork();
        initialPerformancePanel->refreshNow();
        auto* packageMixer = findDescendantByType<drs::app::PerformanceMixer>(
            *initialPerformancePanel);
        require(initialControlsToggle->getButtonText() == "Hide Controls"
                    && packageMixer != nullptr && packageMixer->isVisible()
                    && packageMixer->getControlCount() >= 3,
                "Loading a package with exposed controls should automatically show Instrument Controls. "
                    "button=" + initialControlsToggle->getButtonText().toStdString()
                    + " mixer=" + (packageMixer != nullptr ? "present" : "missing")
                    + " visible=" + (packageMixer != nullptr && packageMixer->isVisible()
                                         ? "true" : "false")
                    + " controls=" + std::to_string(packageMixer != nullptr
                                                         ? packageMixer->getControlCount() : 0)
                    + " snapshotLoaded="
                    + (processor->getEngineFacade().getPerformanceSnapshot().loaded
                           ? "true" : "false")
                    + " userChoice="
                    + (processor->getInstrumentControlsExpandedChoice().has_value()
                           ? (*processor->getInstrumentControlsExpandedChoice()
                                  ? "expanded" : "collapsed")
                           : "unset"));
        const auto performanceControlDescriptors = processor->getEngineFacade().getMacroDescriptors();
        require(std::any_of(performanceControlDescriptors.begin(), performanceControlDescriptors.end(),
                            [](const auto& descriptor)
                            {
                                return descriptor.id == "package.instrument.gain"
                                    && descriptor.publishedControl
                                    && descriptor.exposedInPerformance;
                            }),
                "The Performance mixer must surface packaged Instrument Controls alongside authored macros.");
        processor->setMacroValueFromShell("package.instrument.gain", 0.0);
        const auto minimumPerformanceControl = processor->getEngineFacade().getInstrumentControlDescriptors();
        require(std::any_of(minimumPerformanceControl.begin(), minimumPerformanceControl.end(),
                            [](const auto& descriptor)
                            {
                                return descriptor.id == "package.instrument.gain"
                                    && descriptor.currentValue == 0.0;
                            }),
                "The Performance mixer callback must route Instrument Control edits into sampler state.");
        processor->setMacroValueFromShell("package.instrument.gain", 1.0);
        initialControlsToggle->onClick();
        require(initialControlsToggle->getButtonText() == "Show Controls"
                    && !packageMixer->isVisible(),
                "The user should be able to hide automatically shown package controls.");
        editor.reset();
        editor.reset(processor->createEditor());
        require(editor != nullptr,
                "Plugin editor should reopen after choosing to hide Instrument Controls.");
        auto* reopenedPerformancePanel
            = findDescendantByType<drs::app::PerformancePanel>(*editor);
        auto* reopenedControlsToggle = dynamic_cast<juce::Button*>(
            findDescendantById(*editor, "performanceMacroStripToggleButton"));
        auto* reopenedPackageMixer = reopenedPerformancePanel != nullptr
            ? findDescendantByType<drs::app::PerformanceMixer>(*reopenedPerformancePanel)
            : nullptr;
        require(reopenedPerformancePanel != nullptr && reopenedControlsToggle != nullptr
                    && reopenedPackageMixer != nullptr,
                "Reopened plugin editor should restore the package Performance UI.");
        reopenedPerformancePanel->refreshNow();
        require(reopenedControlsToggle->getButtonText() == "Show Controls"
                    && !reopenedPackageMixer->isVisible(),
                "Instrument Controls should retain the user's hidden choice while the plugin session remains active.");
        require(processor->getEngineFacade().getPerformancePackageLicenseText() == pluginLicense,
                "Reopening the plugin editor must preserve the package-owned immutable license pointer.");
        const auto pluginPayload
            = processor->getEngineFacade().getPerformancePackageActivationPayload();
        require(pluginPayload != nullptr && pluginPayload->snapshot != nullptr,
                "Plugin host validation did not retain the package graph snapshot.");
        const auto pluginGraph = drs::engine::compileDspGraphPlan(*pluginPayload->snapshot);
        require(pluginGraph.compiled
                    && pluginGraph.plan.planDigest == standaloneGraph.plan.planDigest,
                "Plugin and standalone hosts did not compile the same package DSP graph.");
        const auto packageControlDescriptors = processor->getEngineFacade()
            .getInstrumentControlDescriptors();
        require(std::any_of(packageControlDescriptors.begin(), packageControlDescriptors.end(),
                             [](const auto& descriptor)
                             {
                                 return descriptor.id == "package.instrument.gain"
                                     && descriptor.importedSourceController == 20;
                             }),
                "Plugin host validation did not restore the packaged Instrument Control descriptor.");
        const auto renderPackageControlFromCc = [&](const int ccValue)
        {
            require(processor->setInstrumentControlValueFromShell("package.instrument.gain", 1.0),
                    "Plugin host validation could not prime the packaged Instrument Control.");
            require(processor->setInstrumentControlValueFromShell(
                        "package.instrument.channel.gain", 1.0),
                    "Plugin host validation could not reset the channel-scoped Instrument Control.");
            juce::MidiBuffer controlMidi;
            controlMidi.addEvent(juce::MidiMessage::controllerEvent(1, 20, ccValue), 0);
            juce::AudioBuffer<float> controlBuffer(2, 512);
            controlBuffer.clear();
            processor->processBlock(controlBuffer, controlMidi);
            return renderQueuedPerformanceSurfaceMagnitude(*processor, 69, 0.8f);
        };
        const auto midiControlMinimumMagnitude = renderPackageControlFromCc(0);
        const auto midiControlMaximumMagnitude = renderPackageControlFromCc(127);
        require(midiControlMinimumMagnitude < midiControlMaximumMagnitude * 0.5f
                    && midiControlMaximumMagnitude > 0.0001f,
                "Host MIDI CC20 did not drive the packaged Instrument Control gain.");
        const auto pluginMacroBindings = processor->getEngineFacade()
            .getActivePublishedMacroBindings();
        require(pluginMacroBindings != nullptr
                    && std::any_of(pluginMacroBindings->bindings.begin(),
                                   pluginMacroBindings->bindings.end(), [](const auto& binding)
                    {
                        return binding.assigned
                            && binding.stableAuthoredId == "Instrument"
                            && binding.renderTarget
                                == drs::engine::PublishedMacroRenderTarget::dspControl
                            && binding.dspSlotId == "host-master-gain"
                            && binding.dspParameterId == "gainDb";
                    }),
                "Plugin host validation did not restore the packaged macro-to-FX binding.");

        const auto pluginPlaybackStarted = Clock::now();
        const auto pluginMagnitude = renderQueuedPerformanceSurfaceMagnitude(
            *processor, 69, 0.8f,
            largeV2Qualification ? 375 : 8, largeV2Qualification);
        const auto pluginPlaybackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - pluginPlaybackStarted);
        processor->serviceMessageThreadWork();
        require(pluginMagnitude > 0.0001f,
                "Plugin host validation should produce audible output from the checked-in package.");
        require(processor->setInstrumentControlValueFromShell("package.instrument.gain", 0.0),
                "Plugin host validation could not set the packaged Instrument Control to minimum.");
        require(processor->setInstrumentControlValueFromShell(
                    "package.instrument.channel.gain", 1.0),
                "Plugin host validation could not reset the channel-scoped Instrument Control before minimum render.");
        const auto packageControlMinimumMagnitude = renderQueuedPerformanceSurfaceMagnitude(
            *processor, 69, 0.8f);
        require(processor->setInstrumentControlValueFromShell("package.instrument.gain", 1.0),
                "Plugin host validation could not set the packaged Instrument Control to maximum.");
        require(processor->setInstrumentControlValueFromShell(
                    "package.instrument.channel.gain", 1.0),
                "Plugin host validation could not reset the channel-scoped Instrument Control before maximum render.");
        const auto packageControlMaximumMagnitude = renderQueuedPerformanceSurfaceMagnitude(
            *processor, 69, 0.8f);
        require(packageControlMinimumMagnitude < packageControlMaximumMagnitude * 0.5f
                    && packageControlMaximumMagnitude > 0.0001f,
                "Packaged Instrument Control gain did not change host-rendered output.");
        const auto pluginAbsentSourceMagnitude = renderChannelScopedGain(*processor, 1, 0);
        const auto pluginExactChannelMagnitude = renderChannelScopedGain(*processor, 2, 0);
        require(pluginAbsentSourceMagnitude > 0.0001f
                    && pluginAbsentSourceMagnitude > pluginExactChannelMagnitude * 2.5f,
                "Plugin host matrix did not distinguish an absent source from exact MIDI channel 2."
                    + std::string(" absent=") + std::to_string(pluginAbsentSourceMagnitude)
                    + " exact=" + std::to_string(pluginExactChannelMagnitude));
        const auto renderPluginUnassignedSource = [&](const int controller,
                                                      const int value)
        {
            require(processor->setInstrumentControlValueFromShell(
                        "package.instrument.gain", 1.0),
                    "Plugin host matrix could not prime the unassigned-source case.");
            require(processor->setInstrumentControlValueFromShell(
                        "package.instrument.channel.gain", 1.0),
                    "Plugin host matrix could not prime the exact-channel case.");
            juce::MidiBuffer reservedMidi;
            reservedMidi.addEvent(juce::MidiMessage::controllerEvent(1, controller, value), 0);
            juce::AudioBuffer<float> reservedBuffer(2, 512);
            reservedBuffer.clear();
            processor->processBlock(reservedBuffer, reservedMidi);
            return renderQueuedPerformanceSurfaceMagnitude(*processor, 69, 0.8f);
        };
        require(renderPluginUnassignedSource(22, 0) > 0.0001f,
                "Plugin unassigned CC source must not silence playback.");
        for (const auto reservedController : { 64, 120, 123 })
            require(renderPluginUnassignedSource(reservedController, 0) > 0.0001f,
                    "Plugin reserved CC semantics must preserve playable output.");
        processor->setMacroValueFromShell("Instrument", 0.0);
        processor->setInstrumentControlValueFromShell("package.instrument.channel.gain", 1.0);
        processor->serviceMessageThreadWork();
        const auto minimumGainMagnitude = renderQueuedPerformanceSurfaceMagnitude(
            *processor, 69, 0.8f);
        processor->setMacroValueFromShell("Instrument", 1.0);
        processor->setInstrumentControlValueFromShell("package.instrument.channel.gain", 1.0);
        processor->serviceMessageThreadWork();
        const auto maximumGainMagnitude = renderQueuedPerformanceSurfaceMagnitude(
            *processor, 69, 0.8f);
        require(minimumGainMagnitude > 0.0001f
                    && maximumGainMagnitude > minimumGainMagnitude * 2.5f,
                "The packaged Instrument gain control did not materially change rendered output."
                " minimum=" + std::to_string(minimumGainMagnitude)
                    + " maximum=" + std::to_string(maximumGainMagnitude));
        const auto pluginDiagnostics = processor->getEngineFacade().getDiagnosticsSnapshot();
        const auto pluginRealtime = processor->getRealtimeSafetySnapshot();
        require(pluginDiagnostics.available,
                "Plugin host validation should publish diagnostics after opening the checked-in package.");
        require(pluginRealtime.getAudioThreadViolationCount() == 0,
                "Plugin package playback reported a realtime-thread violation.");
        require(pluginRealtime.blockingLockAttemptsOnAudioThread == 0,
                "Plugin package playback attempted a blocking audio-thread lock.");
        require(!largeV2Qualification
                    || (pluginDiagnostics.cacheMissCount > 0
                        && pluginDiagnostics.backgroundReadCount > 0),
                "Plugin large-package playback did not service pages beyond the resident head.");
        require(pluginOpenElapsed <= std::chrono::milliseconds(3000),
                "Plugin package open exceeded the reviewed host-validation budget.");
        require(pluginPlaybackElapsed <= (largeV2Qualification
                    ? std::chrono::milliseconds(6000) : std::chrono::milliseconds(1000)),
                "Plugin initial package playback exceeded the reviewed host-validation budget.");
        require(standaloneActivationElapsed <= std::chrono::milliseconds(16)
                    && pluginActivationElapsed <= std::chrono::milliseconds(16),
                "Prepared package activation exceeded the 16 ms message-thread budget: standalone="
                    + std::to_string(standaloneActivationElapsed.count()) + " us, plugin="
                    + std::to_string(pluginActivationElapsed.count()) + " us.");

        for (int iteration = 0; iteration < 8; ++iteration)
        {
            std::unique_ptr<juce::AudioProcessorEditor> churnedEditor(processor->createEditor());
            require(churnedEditor != nullptr,
                    "Plugin editor churn failed to construct an editor.");
        }

        juce::MemoryBlock savedState;
        require(processor->waitForHostStatePublication(),
                "Package locator state did not reach background host-state publication.");
        processor->getStateInformation(savedState);
        const std::string savedStateText(
            static_cast<const char*>(savedState.getData()), savedState.getSize());
        require(savedStateText.find("performancePackageBinding") != std::string::npos
                    && savedStateText.find(selectedPackage.filename().generic_string())
                        != std::string::npos,
                "Package host state did not persist its explicit package locator binding.");
        if (!captureStatePath.empty())
        {
            fs::create_directories(captureStatePath.parent_path());
            std::ofstream capture(captureStatePath, std::ios::binary | std::ios::trunc);
            require(static_cast<bool>(capture),
                    "Package host-state capture output could not be opened.");
            capture.write(savedStateText.data(),
                          static_cast<std::streamsize>(savedStateText.size()));
            require(static_cast<bool>(capture),
                    "Package host-state capture output could not be written.");
        }

        const auto restoreStarted = Clock::now();
        auto restoredProcessor = std::make_unique<drs::plugin::Processor>();
        restoredProcessor->prepareToPlay(44100.0, 512);
        const auto setStateStarted = Clock::now();
        restoredProcessor->setStateInformation(savedState.getData(),
                                               static_cast<int>(savedState.getSize()));
        const auto setStateElapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - setStateStarted);
        const auto restoreDeadline = Clock::now() + std::chrono::seconds(15);
        while (Clock::now() < restoreDeadline
               && restoredProcessor->getWorkspaceDocumentState().kind
                   != drs::engine::WorkspaceDocumentKind::performancePackage)
        {
            restoredProcessor->serviceMessageThreadWork();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        restoredProcessor->serviceMessageThreadWork();
        const auto restoreElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - restoreStarted);
        require(setStateElapsed <= std::chrono::milliseconds(16),
                "setStateInformation blocked while restoring a package locator.");
        require(restoredProcessor->getWorkspaceDocumentState().kind
                    == drs::engine::WorkspaceDocumentKind::performancePackage
                    && restoredProcessor->getWorkspaceDocumentState().playable,
                "Editor-closed host recall did not restore the package workspace to playable.");
        require(renderQueuedPerformanceSurfaceMagnitude(
                    *restoredProcessor, 69, 0.8f, largeV2Qualification ? 64 : 8,
                    largeV2Qualification) > 0.0001f,
                "Editor-closed host recall did not restore audible package playback.");
        const auto restoredPayload
            = restoredProcessor->getEngineFacade().getPerformancePackageActivationPayload();
        require(restoredPayload != nullptr && restoredPayload->snapshot != nullptr
                    && drs::engine::compileDspGraphPlan(*restoredPayload->snapshot).plan.planDigest
                        == pluginGraph.plan.planDigest,
                "Editor-closed host recall did not restore the active package DSP graph.");
        const auto restoredLicense
            = restoredProcessor->getEngineFacade().getPerformancePackageLicenseText();
        require(drs::app::shouldShowViewLicenseMenuItem(true, restoredLicense != nullptr),
                "Package locator restore should restore View License availability.");
        requireLicenseViewerContract(restoredLicense, "Package locator restore");
        const auto restoredRealtime = restoredProcessor->getRealtimeSafetySnapshot();
        require(restoredRealtime.getAudioThreadViolationCount() == 0,
                "License-enabled package locator restore reported audio-thread I/O or allocation.");
        std::unique_ptr<juce::AudioProcessorEditor> restoredEditor(
            restoredProcessor->createEditor());
        require(restoredEditor != nullptr,
                "An editor could not open after editor-closed package recall.");

        std::cout << "Performance package host validation passed:"
                  << " standaloneOpenMs=" << standaloneOpenElapsed.count()
                  << " standalonePrepareUs=" << standalonePreparationElapsed.count()
                  << " standaloneActivateUs=" << standaloneActivationElapsed.count()
                  << " standaloneEngineActivateUs="
                  << standaloneLoad.timings.engineSessionActivationMicros
                  << " standaloneWorkspaceUs="
                  << standaloneLoad.timings.workspaceTransitionMicros
                  << " standalonePlaybackMs=" << standalonePlaybackElapsed.count()
                  << " standaloneAbsentSourceMagnitude=" << standaloneAbsentSourceMagnitude
                  << " standaloneExactChannelMagnitude=" << standaloneExactChannelMagnitude
                  << " standaloneHeadBytesRead=" << standaloneDiagnostics.headBytesRead
                  << " standaloneCacheMissCount=" << standaloneDiagnostics.cacheMissCount
                  << " standaloneBackgroundReadCount=" << standaloneDiagnostics.backgroundReadCount
                  << " pluginOpenMs=" << pluginOpenElapsed.count()
                  << " pluginPrepareUs=" << pluginPreparationElapsed.count()
                  << " pluginActivateUs=" << pluginActivationElapsed.count()
                  << " pluginEngineActivateUs="
                  << pluginLoad.timings.engineSessionActivationMicros
                  << " pluginWorkspaceUs="
                  << pluginLoad.timings.workspaceTransitionMicros
                  << " pluginPlaybackMs=" << pluginPlaybackElapsed.count()
                  << " pluginAbsentSourceMagnitude=" << pluginAbsentSourceMagnitude
                  << " pluginExactChannelMagnitude=" << pluginExactChannelMagnitude
                  << " pluginHeadBytesRead=" << pluginDiagnostics.headBytesRead
                  << " pluginCacheMissCount=" << pluginDiagnostics.cacheMissCount
                  << " pluginBackgroundReadCount=" << pluginDiagnostics.backgroundReadCount
                  << " restoreMs=" << restoreElapsed.count()
                  << " setStateUs=" << setStateElapsed.count()
                  << " standaloneRtViolations="
                  << standaloneRealtime.getAudioThreadViolationCount()
                  << " standaloneMaxCallbackUs=" << standaloneRealtime.maxProcessBlockMicros
                  << " standaloneLockAttempts=" << standaloneRealtime.blockingLockAttemptsOnAudioThread
                  << " pluginRtViolations=" << pluginRealtime.getAudioThreadViolationCount()
                  << " pluginMaxCallbackUs=" << pluginRealtime.maxProcessBlockMicros
                  << " pluginLockAttempts=" << pluginRealtime.blockingLockAttemptsOnAudioThread
                  << std::endl;
        restoredEditor.reset();
        restoredProcessor.reset();
        editor.reset();
        processor.reset();
        standalone.reset();
        if (argc < 2 && captureStatePath.empty())
            fs::remove_all(generatedFixtureRoot);
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 performance package host validation tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
