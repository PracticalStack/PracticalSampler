#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

juce::File getBuiltPluginBundle()
{
    const auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    const auto configurationDirectory = executable.getParentDirectory();
    const auto buildDirectory = configurationDirectory.getParentDirectory()
        .getParentDirectory().getParentDirectory();
    const auto appDirectory = buildDirectory.getChildFile("app");
    const auto configurationName = configurationDirectory.getFileName();
    return appDirectory.getChildFile("drs_plugin_bundle_artefacts")
        .getChildFile(configurationName)
        .getChildFile("VST3")
        .getChildFile("Decent Rhapsody Studio.vst3");
}

void dispatchHostedPluginMessages()
{
#if JUCE_MODAL_LOOPS_PERMITTED
    if (auto* messageManager = juce::MessageManager::getInstanceWithoutCreating())
        messageManager->runDispatchLoopUntil(2);
#endif
#if JUCE_WINDOWS
    MSG message {};
    while (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != 0)
    {
        ::TranslateMessage(&message);
        ::DispatchMessageW(&message);
    }
#endif
}

bool waitForPublishedPerformance(drs::plugin::Processor& processor)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        juce::AudioBuffer<float> buffer(2, 256);
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
        processor.serviceMessageThreadWork();
        const auto publish = processor.getPerformancePublishControllerSnapshot();
        if (publish.hasActiveRequest
            && publish.activationState == drs::engine::PerformancePublishActivationState::active)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

juce::MemoryBlock wrapAsVst3ComponentState(juce::AudioPluginInstance& hostedPlugin,
                                           const juce::MemoryBlock& processorState)
{
    juce::MemoryBlock wrappedState;
    hostedPlugin.getStateInformation(wrappedState);
    auto wrapperXml = juce::AudioProcessor::getXmlFromBinary(
        wrappedState.getData(), static_cast<int>(wrappedState.getSize()));
    require(wrapperXml != nullptr && wrapperXml->hasTagName("VST3PluginState"),
            "The compiled VST3 host must provide its wrapped component-state container.");
    auto* componentState = wrapperXml->getChildByName("IComponent");
    require(componentState != nullptr,
            "The compiled VST3 host-state container must include an IComponent stream.");
    componentState->deleteAllTextElements();
    componentState->addTextElement(processorState.toBase64Encoding());
    juce::AudioProcessor::copyXmlToBinary(*wrapperXml, wrappedState);
    return wrappedState;
}

std::string unwrapVst3ComponentState(const juce::MemoryBlock& wrappedState)
{
    auto wrapperXml = juce::AudioProcessor::getXmlFromBinary(
        wrappedState.getData(), static_cast<int>(wrappedState.getSize()));
    if (wrapperXml == nullptr || !wrapperXml->hasTagName("VST3PluginState"))
        return {};
    const auto* componentState = wrapperXml->getChildByName("IComponent");
    if (componentState == nullptr)
        return {};
    juce::MemoryBlock processorState;
    if (!processorState.fromBase64Encoding(componentState->getAllSubText()))
        return {};
    return std::string(static_cast<const char*>(processorState.getData()), processorState.getSize());
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        juce::AudioPluginFormatManager formats;
        juce::addHeadlessDefaultFormatsToManager(formats);
        const auto bundle = getBuiltPluginBundle();
        require(bundle.exists(), "The compiled VST3 bundle is missing from the current build output.");

        juce::KnownPluginList pluginList;
        juce::OwnedArray<juce::PluginDescription> descriptions;
        auto discovered = false;
        for (auto index = 0; index < formats.getNumFormats(); ++index)
        {
            auto* format = formats.getFormat(index);
            if (format != nullptr && format->getName() == "VST3")
            {
                discovered = pluginList.scanAndAddFile(bundle.getFullPathName(), false, descriptions, *format);
                break;
            }
        }
        require(discovered && descriptions.size() == 1
                    && descriptions[0]->pluginFormatName == "VST3",
                "The host must discover exactly one VST3 description from the compiled bundle.");

        juce::String creationError;
        auto hostedPlugin = formats.createPluginInstance(*descriptions[0], 44100.0, 256, creationError);
        require(hostedPlugin != nullptr,
                "The host could not instantiate the discovered VST3: " + creationError.toStdString());
        hostedPlugin->setPlayConfigDetails(0, 2, 44100.0, 256);
        hostedPlugin->prepareToPlay(44100.0, 256);

        const auto projectPath = drs::engine::getPhase2ReferenceProjectManifestPath();
        const auto project = drs::engine::loadRuntimeProjectManifest(projectPath);
        require(project.loaded, "The authored VST3 restore fixture must load.");
        auto source = std::make_unique<drs::plugin::Processor>();
        source->prepareToPlay(44100.0, 256);
        require(source->replaceAuthoringProject(project.project, juce::File(projectPath))
                    && source->submitPerformancePublishCommand()
                    && waitForPublishedPerformance(*source),
                "The VST3 qualification source must publish a project-bound host state.");
        juce::AudioBuffer<float> sourceBuffer(2, 256);
        sourceBuffer.clear();
        juce::MidiBuffer sourceMidi;
        sourceMidi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);
        source->processBlock(sourceBuffer, sourceMidi);
        require(sourceBuffer.getMagnitude(0, 0, sourceBuffer.getNumSamples()) > 0.0f,
                "The qualification source must render before its state is injected into VST3.");

        juce::MemoryBlock componentState;
        require(source->waitForHostStatePublication(),
                "The VST3 qualification checkpoint did not reach background host-state publication.");
        source->getStateInformation(componentState);
        const auto capturedText = std::string(
            static_cast<const char*>(componentState.getData()), componentState.getSize());
        const auto captured = drs::engine::parseHostSessionState(capturedText);
        require(captured.isValidHostState()
                    && captured.hostState->projectBinding.projectId == "drs.phase2.authoring-foundation",
                "The VST3 qualification must inject a project-bound drs.hostState component chunk.");

        const auto wrappedComponentState = wrapAsVst3ComponentState(*hostedPlugin, componentState);
        hostedPlugin->setStateInformation(wrappedComponentState.getData(),
                                          static_cast<int>(wrappedComponentState.getSize()));
        auto finiteSamples = std::size_t {};
        auto nonzeroSamples = std::size_t {};
        auto peak = 0.0f;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (std::chrono::steady_clock::now() < deadline && nonzeroSamples == 0)
        {
            dispatchHostedPluginMessages();
            juce::AudioBuffer<float> buffer(2, 256);
            buffer.clear();
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);
            hostedPlugin->processBlock(buffer, midi);
            for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
                {
                    const auto value = buffer.getSample(channel, sample);
                    require(std::isfinite(value), "The hosted VST3 emitted a non-finite sample.");
                    ++finiteSamples;
                    if (std::abs(value) > 0.000001f)
                        ++nonzeroSamples;
                }
            }
            peak = std::max(peak, buffer.getMagnitude(0, 0, buffer.getNumSamples()));
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        juce::MemoryBlock reserializedState;
        hostedPlugin->getStateInformation(reserializedState);
        const auto restoredText = unwrapVst3ComponentState(reserializedState);
        const auto restored = drs::engine::parseHostSessionState(restoredText);
        const auto restoredProjectId = restored.isValidHostState()
            ? restored.hostState->projectBinding.projectId
            : std::string("<invalid host state>");
        require(finiteSamples > 0 && nonzeroSamples > 0 && peak > 0.0f,
                "The hosted VST3 recall must render finite, nonzero MIDI audio. "
                    "Reserialized project binding: " + restoredProjectId + ".");
        require(restored.isValidHostState()
                    && restored.hostState->projectBinding.projectId == "drs.phase2.authoring-foundation",
                "The hosted VST3 round trip must retain drs.hostState and its authored binding.");
        hostedPlugin->releaseResources();

        std::cout << "VST3 host-state qualification tests passed. peak=" << peak
                  << ", finiteSamples=" << finiteSamples
                  << ", nonzeroSamples=" << nonzeroSamples << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "VST3 host-state qualification tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
