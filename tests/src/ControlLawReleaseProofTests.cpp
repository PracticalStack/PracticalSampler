#include "drs/engine/ControlLaw.h"
#include "drs/engine/DspGain.h"
#include "drs/engine/PublishedMacroBinding.h"
#include "shared/PerformanceMixer.h"

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace drs::engine;

constexpr std::array<ControlLawPoint, 7> mixerAnchors {{
    { 0.00, -96.0 }, { 0.05, -60.0 }, { 0.25, -30.0 }, { 0.50, -15.0 },
    { 0.75, -6.0 }, { 0.85, 0.0 }, { 1.00, 6.0 }
}};

void require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

void requireNear(const double actual, const double expected, const double tolerance,
                 const std::string& message)
{
    require(std::abs(actual - expected) <= tolerance, message);
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id) return &root;
    for (int index = 0; index < root.getNumChildComponents(); ++index)
        if (auto* match = findDescendantById(*root.getChildComponent(index), id)) return match;
    return nullptr;
}

ImmutablePublishedMacroBindingTablePtr publishMixerLaw(const double normalized)
{
    PlaybackSnapshotMacroDefault macro;
    macro.id = "release-mixer";
    macro.name = "Release Mixer";
    macro.defaultValue = normalized;
    macro.minValue = 0.0;
    macro.maxValue = 1.0;
    macro.exposedInPerformance = true;
    macro.targets.push_back({ "dsp.release-gain.gainDb", "groups/release/gainDb", "mix",
                              "release-gain", "gainDb", 0.0, 1.0, -96.0, 6.0, "linear",
                              { std::string(controlLawMixerGainV1), 1 } });

    DspParameterControlLayout layout;
    layout.graphPlanDigest = "control-law-release-proof";
    layout.controls.push_back({ 0, 0, 0, "release-gain", "gainDb", -96.0, 24.0, 0.0,
                                CuratedDspSmoothing::linear });

    PublishedMacroPresentation presentation;
    presentation.authoredLabel = "Release Mixer";
    presentation.sectionLabel = "Release";
    presentation.parameterLabel = "Gain";
    presentation.valueUnit = "dB";
    presentation.controlKind = PublishedMacroControlKind::fader;
    presentation.accessibilityDescription = "Release Mixer, Release, Gain, dB";

    PublishedMacroBindingBuildRequest request;
    request.revision = 5;
    request.macroSchemaDigest = "control-law-release-proof";
    request.hostSlots = {{ 0, "macro.tone", macro.id }};
    request.authoredMacros = { macro };
    request.presentationHints = {{ macro.id, presentation }};
    request.dspControlLayout = &layout;
    request.dspGraphDigest = layout.graphPlanDigest;
    const auto result = buildPublishedMacroBindingTable(request);
    require(result.built && result.table != nullptr,
            "The mixer release fixture must publish a compiled callback law.");
    return result.table;
}

void verifyAnchorEndToEnd()
{
    juce::ScopedJuceInitialiser_GUI gui;
    for (const auto& anchor : mixerAnchors)
    {
        const auto table = publishMixerLaw(anchor.normalized);
        const auto& binding = table->bindings.front();
        const auto& callbackSlot = table->callbackView.slots[0];
        require(binding.controlLaw.kind == ControlLawKind::mixerGainV1
                    && callbackSlot.controlLaw.kind == ControlLawKind::mixerGainV1,
                "Publication must retain the same mixer law in presentation and callback data.");

        double publishedPhysical = 0.0;
        require(normalizedToPhysical(callbackSlot.controlLaw, anchor.normalized, publishedPhysical),
                "Every mixer anchor must map through the callback law.");
        requireNear(publishedPhysical, anchor.physical, 1.0e-12,
                    "Published physical gain must equal the frozen mixer anchor.");

        drs::app::PerformanceMixerControlView control;
        control.authoredId = "release-mixer";
        control.runtimeId = "tone";
        control.sectionLabel = binding.presentation.sectionLabel;
        control.controlLabel = binding.publishedName;
        control.parameterLabel = binding.presentation.parameterLabel;
        control.valueUnit = binding.presentation.valueUnit;
        control.accessibilityDescription = binding.presentation.accessibilityDescription;
        control.controlKind = binding.presentation.controlKind;
        control.minimum = binding.minValue;
        control.maximum = binding.maxValue;
        control.displayMinimum = binding.destinationMinimum;
        control.displayMaximum = binding.destinationMaximum;
        control.value = anchor.normalized;
        control.controlLaw = binding.controlLaw;

        drs::app::PerformanceMixer mixer;
        mixer.setSize(260, 220);
        mixer.setControls({ control });
        const auto* valueLabel = dynamic_cast<juce::Label*>(findDescendantById(
            mixer, "performanceMixerValueLabel.release-mixer"));
        require(valueLabel != nullptr, "Every published mixer anchor must have a visible value label.");
        ControlLawFormatOptions format;
        format.precision = 1;
        format.renderMinimumAsNegativeInfinity = true;
        format.negativeInfinityThreshold = -96.0;
        const auto expectedLabel = formatControlLawValue(
            anchor.physical, ControlLawUnit::decibels, format);
        require(valueLabel->getText() == juce::String::fromUTF8(expectedLabel.c_str()),
                "Perform label must match the published physical mixer value at every anchor.");

        std::array<float, 1> sample {{ 0.25f }};
        float* channels[] { sample.data() };
        processDspGain({ channels, 1, 1 }, { publishedPhysical, 0.0, 0.0 });
        const auto expectedAmplitude = 0.25 * std::pow(10.0, anchor.physical / 20.0);
        requireNear(sample.front(), expectedAmplitude, 2.0e-7,
                    "Measured gain output must match the physical dB value published at every mixer anchor.");
    }
}

void verifyBoundedControlScenarios()
{
    for (const auto count : std::array<std::size_t, 4> {{ 1, 2, 3, 12 }})
    {
        PublishedMacroBindingBuildRequest request;
        request.revision = count;
        request.macroSchemaDigest = "control-law-capacity-" + std::to_string(count);
        request.hostSlots.reserve(count);
        request.authoredMacros.reserve(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto id = "macro-" + std::to_string(index);
            request.hostSlots.push_back({ index, "macro.slot." + std::to_string(index + 1), id });
            request.authoredMacros.push_back({ id, "Macro", 0.5, 0.0, 1.0, true, {} });
        }
        const auto result = buildPublishedMacroBindingTable(request);
        require(result.built && result.table != nullptr
                    && result.table->assignedExposedCount == count,
                "Every supported published-control count must compile before activation.");
        for (std::size_t index = 0; index < count; ++index)
            require(isCompiledControlLawValid(result.table->callbackView.slots[index].controlLaw),
                    "Every callback slot must carry a valid compiled law.");
    }
}
} // namespace

int main()
{
    try
    {
        verifyAnchorEndToEnd();
        verifyBoundedControlScenarios();
        std::cout << "Control-law release proof passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Control-law release proof failed: " << error.what() << '\n';
        return 1;
    }
}
