#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"
#include "Phase1PerformancePackageSupport.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
using Clock = std::chrono::steady_clock;
namespace package_support = drs::tests::performance_package;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
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
                                              const int blockCount = 8)
{
    processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);

    float maxMagnitude = 0.0f;
    juce::MidiBuffer emptyMidiBuffer;
    for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        processor.processBlock(buffer, emptyMidiBuffer);
        maxMagnitude = std::max(maxMagnitude, buffer.getMagnitude(0, buffer.getNumSamples()));
    }

    processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
    return maxMagnitude;
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
} // namespace

int main()
{
    try
    {
        const auto checkedInCorpus = package_support::getCheckedInCorpusPaths();
        require(std::filesystem::exists(checkedInCorpus.valid),
                "Checked-in valid performance package fixture must exist for host validation.");

        juce::ScopedJuceInitialiser_GUI gui;

        const auto packageFile = juce::File(juce::String::fromUTF8(checkedInCorpus.valid.generic_string().c_str()));

        drs::standalone::MainComponent standalone(false);
        const auto standaloneOpenStarted = Clock::now();
        const auto standaloneLoad = standalone.getProcessor().loadPerformancePackageWorkspace(packageFile);
        const auto standaloneOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - standaloneOpenStarted);
        require(standaloneLoad.loaded,
                "Standalone host validation should load the checked-in package. state="
                    + standaloneLoad.state + " issues=" + summarizeIssues(standaloneLoad.issues));
        require(standaloneLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::none,
                "Standalone host validation should not publish a package failure category for the valid fixture.");
        requirePerformanceOnlyWorkspace(standalone.getProcessor().getWorkspaceDocumentState(),
                                        "Standalone host validation");
        standalone.getProcessor().prepareToPlay(44100.0, 512);
        standalone.getProcessor().serviceMessageThreadWork();

        const auto standalonePlaybackStarted = Clock::now();
        const auto standaloneMagnitude = renderQueuedPerformanceSurfaceMagnitude(
            standalone.getProcessor(), 69, 0.8f);
        const auto standalonePlaybackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - standalonePlaybackStarted);
        require(standaloneMagnitude > 0.0001f,
                "Standalone host validation should produce audible output from the checked-in package.");
        const auto standaloneDiagnostics = standalone.getEngineFacade().getDiagnosticsSnapshot();
        require(standaloneDiagnostics.available,
                "Standalone host validation should publish diagnostics after opening the checked-in package.");
        require(standaloneOpenElapsed <= std::chrono::milliseconds(3000),
                "Standalone package open exceeded the reviewed host-validation budget.");
        require(standalonePlaybackElapsed <= std::chrono::milliseconds(1000),
                "Standalone initial package playback exceeded the reviewed host-validation budget.");

        drs::plugin::Processor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Plugin editor should construct for package host validation.");

        const auto pluginOpenStarted = Clock::now();
        const auto pluginLoad = processor.loadPerformancePackageWorkspace(packageFile);
        const auto pluginOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - pluginOpenStarted);
        require(pluginLoad.loaded,
                "Plugin host validation should load the checked-in package. state="
                    + pluginLoad.state + " issues=" + summarizeIssues(pluginLoad.issues));
        require(pluginLoad.failureCategory == drs::engine::PerformancePackageFailureCategory::none,
                "Plugin host validation should not publish a package failure category for the valid fixture.");
        requirePerformanceOnlyWorkspace(processor.getWorkspaceDocumentState(),
                                        "Plugin host validation");
        processor.prepareToPlay(44100.0, 512);
        processor.serviceMessageThreadWork();

        const auto pluginPlaybackStarted = Clock::now();
        const auto pluginMagnitude = renderQueuedPerformanceSurfaceMagnitude(processor, 69, 0.8f);
        const auto pluginPlaybackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - pluginPlaybackStarted);
        require(pluginMagnitude > 0.0001f,
                "Plugin host validation should produce audible output from the checked-in package.");
        const auto pluginDiagnostics = processor.getEngineFacade().getDiagnosticsSnapshot();
        require(pluginDiagnostics.available,
                "Plugin host validation should publish diagnostics after opening the checked-in package.");
        require(pluginOpenElapsed <= std::chrono::milliseconds(3000),
                "Plugin package open exceeded the reviewed host-validation budget.");
        require(pluginPlaybackElapsed <= std::chrono::milliseconds(1000),
                "Plugin initial package playback exceeded the reviewed host-validation budget.");

        std::cout << "Performance package host validation passed:"
                  << " standaloneOpenMs=" << standaloneOpenElapsed.count()
                  << " standalonePlaybackMs=" << standalonePlaybackElapsed.count()
                  << " standaloneHeadBytesRead=" << standaloneDiagnostics.headBytesRead
                  << " standaloneCacheMissCount=" << standaloneDiagnostics.cacheMissCount
                  << " pluginOpenMs=" << pluginOpenElapsed.count()
                  << " pluginPlaybackMs=" << pluginPlaybackElapsed.count()
                  << " pluginHeadBytesRead=" << pluginDiagnostics.headBytesRead
                  << " pluginCacheMissCount=" << pluginDiagnostics.cacheMissCount
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 performance package host validation tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
