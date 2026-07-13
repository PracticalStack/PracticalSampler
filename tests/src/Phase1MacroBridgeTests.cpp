#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(double actual, double expected, double tolerance, const std::string& message)
{
    if (std::abs(actual - expected) > tolerance)
        throw std::runtime_error(message);
}

double findMacroValue(const drs::engine::EngineFacade& engineFacade, const std::string& macroId)
{
    for (const auto& macro : engineFacade.getMacroDescriptors())
    {
        if (macro.id == macroId)
            return macro.currentValue;
    }

    throw std::runtime_error("Macro '" + macroId + "' was not found.");
}

drs::engine::EngineMacroDescriptor findMacroDescriptor(const drs::engine::EngineFacade& engineFacade,
                                                       const std::string& macroId)
{
    for (const auto& macro : engineFacade.getMacroDescriptors())
    {
        if (macro.id == macroId)
            return macro;
    }

    throw std::runtime_error("Macro descriptor '" + macroId + "' was not found.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        drs::standalone::MainComponent standalone;
        require(standalone.setMacroValue("tone", 0.88), "Standalone shell should expose the tone macro.");
        require(standalone.setMacroValue("motion", 0.73), "Standalone shell should expose the motion macro.");
        requireNear(findMacroValue(standalone.getEngineFacade(), "tone"), 0.88, 0.0001,
                    "Standalone shell tone macro did not update.");
        requireNear(findMacroValue(standalone.getEngineFacade(), "motion"), 0.73, 0.0001,
                    "Standalone shell motion macro did not update.");
        require(standalone.getEngineFacade().getStatusSnapshot().detail.find("tone=0.88") != std::string::npos,
                "Standalone shell status snapshot should expose the updated tone macro.");
        const auto& standaloneToneDescriptor = findMacroDescriptor(standalone.getEngineFacade(), "tone");
        const auto& standaloneMotionDescriptor = findMacroDescriptor(standalone.getEngineFacade(), "motion");
        require(standaloneToneDescriptor.ownershipKey == "preview.triggerVelocity",
                "Tone macro ownership key changed unexpectedly.");
        require(standaloneMotionDescriptor.ownershipKey == "preview.noteTravel",
                "Motion macro ownership key changed unexpectedly.");
        require(standaloneToneDescriptor.currentEffect == "Accent attack",
                "Tone macro should report the accent attack effect at high values.");
        require(standaloneMotionDescriptor.currentEffect == "+6 st",
                "Motion macro should report the expected positive pitch offset.");
        require(standalone.getEngineFacade().setSelectedArticulation("lead"),
                "Standalone shell should allow selecting the lead articulation for macro preview.");
        const auto standalonePreview = standalone.getEngineFacade().auditionPreviewNote(69, 100);
        require(standalonePreview.succeeded, "Standalone shell macro preview should play successfully.");
        require(standalonePreview.zoneId == "lead-a4-accent",
                "High tone macro should bias preview routing into the lead accent zone.");
        require(standalonePreview.effectiveMidiNote == 75,
                "Motion macro should offset the preview note by six semitones.");
        require(standalonePreview.effectiveVelocity >= 96,
                "High tone macro should drive an accent-range preview velocity.");

        const auto standaloneState = standalone.exportStateJson();
        drs::standalone::MainComponent restoredStandalone;
        require(restoredStandalone.restoreStateJson(standaloneState).restored,
                "Standalone shell should restore its exported macro state.");
        requireNear(findMacroValue(restoredStandalone.getEngineFacade(), "tone"), 0.88, 0.0001,
                    "Standalone shell tone macro did not persist across reload.");
        requireNear(findMacroValue(restoredStandalone.getEngineFacade(), "motion"), 0.73, 0.0001,
                    "Standalone shell motion macro did not persist across reload.");
        require(findMacroDescriptor(restoredStandalone.getEngineFacade(), "tone").currentEffect == "Accent attack",
                "Standalone shell tone macro effect did not persist across reload.");
        require(findMacroDescriptor(restoredStandalone.getEngineFacade(), "motion").currentEffect == "+6 st",
                "Standalone shell motion macro effect did not persist across reload.");

        drs::plugin::Processor processor;
        const auto pluginMacros = processor.getEngineFacade().getMacroDescriptors();
        require(processor.getParameters().size() == pluginMacros.size(),
                "Plugin parameter count should match the authored macro count.");

        auto* toneParameter = dynamic_cast<juce::RangedAudioParameter*>(
            processor.getParameterState().getParameter("macro.tone"));
        auto* motionParameter = dynamic_cast<juce::RangedAudioParameter*>(
            processor.getParameterState().getParameter("macro.motion"));
        require(toneParameter != nullptr, "Plugin should expose a host-facing tone parameter.");
        require(motionParameter != nullptr, "Plugin should expose a host-facing motion parameter.");
        require(toneParameter->getName(64).toStdString() == "Tone",
                "Tone parameter display name changed unexpectedly.");
        require(motionParameter->getName(64).toStdString() == "Motion",
                "Motion parameter display name changed unexpectedly.");

        toneParameter->setValueNotifyingHost(toneParameter->convertTo0to1(0.88f));
        motionParameter->setValueNotifyingHost(motionParameter->convertTo0to1(0.69f));
        requireNear(findMacroValue(processor.getEngineFacade(), "tone"), 0.88, 0.0001,
                    "Plugin tone parameter did not propagate into runtime macro state.");
        requireNear(findMacroValue(processor.getEngineFacade(), "motion"), 0.69, 0.0001,
                    "Plugin motion parameter did not propagate into runtime macro state.");
        require(processor.getEngineFacade().setSelectedArticulation("lead"),
                "Plugin shell should allow selecting the lead articulation for macro preview.");
        const auto pluginPreview = processor.getEngineFacade().auditionPreviewNote(69, 100);
        require(pluginPreview.succeeded, "Plugin macro preview should play successfully.");
        require(pluginPreview.zoneId == "lead-a4-accent",
                "Plugin tone macro should bias preview routing into the lead accent zone.");
        require(pluginPreview.effectiveMidiNote == 74,
                "Plugin motion macro should offset the preview note by five semitones.");
        require(pluginPreview.effectiveVelocity >= 96,
                "Plugin tone macro should drive an accent-range preview velocity.");

        juce::MemoryBlock stateBlock;
        processor.getStateInformation(stateBlock);

        drs::plugin::Processor restoredProcessor;
        restoredProcessor.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));
        requireNear(findMacroValue(restoredProcessor.getEngineFacade(), "tone"), 0.88, 0.0001,
                    "Plugin tone macro did not persist across reload.");
        requireNear(findMacroValue(restoredProcessor.getEngineFacade(), "motion"), 0.69, 0.0001,
                    "Plugin motion macro did not persist across reload.");
        require(findMacroDescriptor(restoredProcessor.getEngineFacade(), "tone").currentEffect == "Accent attack",
                "Plugin tone macro effect did not persist across reload.");
        require(findMacroDescriptor(restoredProcessor.getEngineFacade(), "motion").currentEffect == "+5 st",
                "Plugin motion macro effect did not persist across reload.");

        auto* restoredToneParameter = dynamic_cast<juce::RangedAudioParameter*>(
            restoredProcessor.getParameterState().getParameter("macro.tone"));
        auto* restoredMotionParameter = dynamic_cast<juce::RangedAudioParameter*>(
            restoredProcessor.getParameterState().getParameter("macro.motion"));
        require(restoredToneParameter != nullptr && restoredMotionParameter != nullptr,
                "Restored plugin should continue exposing the macro parameters.");
        requireNear(static_cast<double>(restoredToneParameter->getValueForText("0.88")), 0.88, 0.01,
                    "Restored tone parameter range changed unexpectedly.");
        requireNear(static_cast<double>(restoredProcessor.getParameterState().getRawParameterValue("macro.tone")->load()),
                    0.88,
                    0.0001,
                    "Restored tone parameter raw value did not match the persisted macro value.");
        requireNear(static_cast<double>(restoredProcessor.getParameterState().getRawParameterValue("macro.motion")->load()),
                    0.69,
                    0.0001,
                    "Restored motion parameter raw value did not match the persisted macro value.");
        require(restoredProcessor.getEngineFacade().setSelectedArticulation("lead"),
                "Restored plugin shell should allow selecting the lead articulation for macro preview.");
        const auto restoredPreview = restoredProcessor.getEngineFacade().auditionPreviewNote(69, 100);
        require(restoredPreview.succeeded, "Restored plugin macro preview should play successfully.");
        require(restoredPreview.zoneId == "lead-a4-accent",
                "Restored plugin tone macro should keep the preview in the lead accent zone.");
        require(restoredPreview.effectiveMidiNote == 74,
                "Restored plugin motion macro should preserve the preview pitch offset after reload.");

        std::cout << "Phase 1 macro bridge tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 macro bridge tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
