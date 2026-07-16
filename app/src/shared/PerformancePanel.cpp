#include "shared/PerformancePanel.h"

#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <optional>

namespace drs::app
{
namespace
{
const auto performancePanelBackground = juce::Colour::fromRGB(14, 20, 27);
const auto performancePanelCard = juce::Colour::fromRGB(245, 247, 250);
const auto performancePanelAccent = juce::Colour::fromRGB(28, 126, 214);
const auto performancePanelSuccess = juce::Colour::fromRGB(27, 128, 84);
const auto performancePanelWarning = juce::Colour::fromRGB(176, 91, 22);
const auto performancePanelDanger = juce::Colour::fromRGB(172, 41, 41);

juce::String formatArticulationLabel(const std::string& articulationName, const std::string& articulationId)
{
    if (!articulationName.empty())
        return juce::String::fromUTF8(articulationName.c_str());

    if (!articulationId.empty())
        return juce::String::fromUTF8(articulationId.c_str());

    return "Unassigned";
}

std::optional<double> findMacroValue(const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                     const std::string& macroId)
{
    const auto iterator = std::find_if(sessionState.macroValues.begin(),
                                       sessionState.macroValues.end(),
                                       [&](const auto& macroValue)
                                       {
                                           return macroValue.id == macroId;
                                       });
    if (iterator == sessionState.macroValues.end())
        return std::nullopt;

    return iterator->value;
}

int computeMotionSemitoneOffset(const drs::engine::RuntimeSessionStateSnapshot& sessionState)
{
    const auto motionValue = findMacroValue(sessionState, "motion").value_or(0.15);
    return static_cast<int>(std::lround((motionValue - 0.5) * 24.0));
}
} // namespace

PerformancePanel::PerformancePanel(drs::engine::EngineFacade& facade,
                                   MacroValueChangedCallback macroValueChanged,
                                   NotePreviewStartedCallback notePreviewStarted,
                                   NotePreviewEndedCallback notePreviewEnded)
    : engineFacade(facade),
      onMacroValueChanged(std::move(macroValueChanged)),
      onNotePreviewStarted(std::move(notePreviewStarted)),
      onNotePreviewEnded(std::move(notePreviewEnded)),
      keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      diagnosticsPanel(facade, onMacroValueChanged)
{
    titleLabel.setText("Phase 1 Performance Surface", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    instrumentLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    patchStatusLabel.setFont(juce::FontOptions(15.0f));
    previewStatusLabel.setFont(juce::FontOptions(15.0f));
    macroStripLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    articulationLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    keyboardHintLabel.setFont(juce::FontOptions(15.0f));
    loadIndicatorLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));

    instrumentLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(14, 20, 27));
    patchStatusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(52, 64, 84));
    previewStatusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(52, 64, 84));
    macroStripLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(14, 20, 27));
    articulationLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(14, 20, 27));
    keyboardHintLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(52, 64, 84));
    loadIndicatorLabel.setJustificationType(juce::Justification::centred);

    loadDefaultButton.setButtonText("Load Default");
    loadDefaultButton.setComponentID("performanceLoadDefaultButton");
    loadLeadButton.setButtonText("Load Lead Demo");
    loadLeadButton.setComponentID("performanceLoadLeadButton");
    diagnosticsToggle.setButtonText("Show Diagnostics");
    diagnosticsToggle.setComponentID("performanceDiagnosticsToggle");

    keyboardComponent.setComponentID("performanceKeyboard");
    keyboardComponent.setKeyWidth(34.0f);
    keyboardComponent.setAvailableRange(36, 96);
    keyboardComponent.setLowestVisibleKey(48);
    keyboardComponent.setWantsKeyboardFocus(true);
    keyboardState.addListener(this);

    diagnosticsPanel.setComponentID("performanceDiagnosticsPanel");
    diagnosticsPanel.setVisible(false);

    loadDefaultButton.onClick = [this]
    {
        engineFacade.resetSessionStateToDefault();
        refreshSurface();
    };

    loadLeadButton.onClick = [this]
    {
        namespace fs = std::filesystem;
        const auto presetPath = fs::path(drs::engine::getPhase1RuntimeRootPath())
            / "preset-state"
            / "reference"
            / "lead-performance-state.drpreset.json";
        engineFacade.restorePresetStateFile(presetPath.generic_string());
        refreshSurface();
    };

    diagnosticsToggle.onClick = [this]
    {
        diagnosticsPanel.setVisible(diagnosticsToggle.getToggleState());
        diagnosticsToggle.setButtonText(diagnosticsToggle.getToggleState() ? "Hide Diagnostics" : "Show Diagnostics");
        resized();
    };

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(instrumentLabel);
    addAndMakeVisible(patchStatusLabel);
    addAndMakeVisible(previewStatusLabel);
    addAndMakeVisible(macroStripLabel);
    addAndMakeVisible(articulationLabel);
    addAndMakeVisible(keyboardHintLabel);
    addAndMakeVisible(loadIndicatorLabel);
    addAndMakeVisible(loadDefaultButton);
    addAndMakeVisible(loadLeadButton);
    addAndMakeVisible(diagnosticsToggle);
    addAndMakeVisible(keyboardComponent);
    addChildComponent(diagnosticsPanel);

    rebuildArticulationButtons();
    rebuildMacroControls();
    refreshSurface();
    startTimerHz(2);
}

PerformancePanel::~PerformancePanel()
{
    keyboardState.removeListener(this);
}

void PerformancePanel::paint(juce::Graphics& g)
{
    g.fillAll(performancePanelBackground);

    auto bounds = getLocalBounds().toFloat().reduced(14.0f);
    g.setColour(performancePanelAccent.withAlpha(0.25f));
    g.fillRoundedRectangle(bounds, 20.0f);

    g.setColour(performancePanelCard);
    g.fillRoundedRectangle(bounds.reduced(4.0f), 18.0f);

    auto heroBounds = bounds.reduced(16.0f).removeFromTop(92.0f);
    juce::ColourGradient heroGradient(performancePanelAccent,
                                      heroBounds.getTopLeft(),
                                      juce::Colour::fromRGB(15, 78, 160),
                                      heroBounds.getBottomRight(),
                                      false);
    g.setGradientFill(heroGradient);
    g.fillRoundedRectangle(heroBounds, 16.0f);
}

void PerformancePanel::resized()
{
    auto area = getLocalBounds().reduced(30);

    auto heroArea = area.removeFromTop(88);
    auto heroLeft = heroArea.removeFromLeft(460);
    titleLabel.setBounds(heroLeft.removeFromTop(30));
    heroLeft.removeFromTop(8);
    instrumentLabel.setBounds(heroLeft.removeFromTop(28));
    heroLeft.removeFromTop(6);
    patchStatusLabel.setBounds(heroLeft.removeFromTop(20));

    auto heroRight = heroArea;
    loadIndicatorLabel.setBounds(heroRight.removeFromTop(30).removeFromRight(200));
    heroRight.removeFromTop(10);
    auto heroButtons = heroRight.removeFromTop(28);
    loadDefaultButton.setBounds(heroButtons.removeFromLeft(140));
    heroButtons.removeFromLeft(10);
    loadLeadButton.setBounds(heroButtons.removeFromLeft(150));
    heroButtons.removeFromLeft(10);
    diagnosticsToggle.setBounds(heroButtons.removeFromLeft(150));

    area.removeFromTop(18);
    articulationLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);

    auto articulationRow = area.removeFromTop(28);
    for (auto& button : articulationButtons)
    {
        button->setBounds(articulationRow.removeFromLeft(140));
        articulationRow.removeFromLeft(10);
    }

    area.removeFromTop(14);
    macroStripLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);

    for (auto& control : macroControls)
    {
        auto macroRow = area.removeFromTop(28);
        control->nameLabel.setBounds(macroRow.removeFromLeft(110));
        control->slider.setBounds(macroRow.removeFromLeft(220));
        macroRow.removeFromLeft(10);
        control->valueLabel.setBounds(macroRow.removeFromLeft(190));
        area.removeFromTop(6);
    }

    area.removeFromTop(12);
    previewStatusLabel.setBounds(area.removeFromTop(22));
    keyboardHintLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(8);
    keyboardComponent.setBounds(area.removeFromTop(92));

    if (diagnosticsPanel.isVisible())
    {
        area.removeFromTop(14);
        diagnosticsPanel.setBounds(area);
    }
}

void PerformancePanel::timerCallback()
{
    refreshSurface();
}

void PerformancePanel::handleNoteOn(juce::MidiKeyboardState*, int, int midiNoteNumber, float velocity)
{
    const auto clampedVelocity = std::clamp(static_cast<int>(std::round(velocity * 127.0f)), 1, 127);

    if (onNotePreviewStarted)
        onNotePreviewStarted(midiNoteNumber, velocity);

    engineFacade.auditionPreviewNote(midiNoteNumber, clampedVelocity);
    refreshSurface();
}

void PerformancePanel::handleNoteOff(juce::MidiKeyboardState*, int, int midiNoteNumber, float)
{
    if (onNotePreviewEnded)
        onNotePreviewEnded(midiNoteNumber);
}

void PerformancePanel::rebuildMacroControls()
{
    macroControls.clear();

    for (const auto& macro : engineFacade.getMacroDescriptors())
    {
        auto control = std::make_unique<MacroControl>();
        control->id = macro.id;
        control->nameLabel.setText(juce::String::fromUTF8(macro.name.c_str()), juce::dontSendNotification);
        control->nameLabel.setJustificationType(juce::Justification::centredLeft);
        control->slider.setSliderStyle(juce::Slider::LinearHorizontal);
        control->slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        control->slider.setRange(macro.minValue, macro.maxValue, 0.001);
        control->slider.onValueChange = [this, rawControl = control.get()]
        {
            if (rawControl->slider.isMouseButtonDown())
            {
                if (onMacroValueChanged)
                    onMacroValueChanged(rawControl->id, rawControl->slider.getValue());
                else
                    engineFacade.setMacroValue(rawControl->id, rawControl->slider.getValue());
            }
        };
        control->valueLabel.setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(control->nameLabel);
        addAndMakeVisible(control->slider);
        addAndMakeVisible(control->valueLabel);
        macroControls.push_back(std::move(control));
    }
}

void PerformancePanel::rebuildArticulationButtons()
{
    articulationButtons.clear();

    for (const auto& articulation : engineFacade.getArticulationDescriptors())
    {
        auto button = std::make_unique<juce::TextButton>(juce::String::fromUTF8(articulation.name.c_str()));
        button->setClickingTogglesState(false);
        button->onClick = [this, articulationId = articulation.id]
        {
            engineFacade.setSelectedArticulation(articulationId);
            refreshSurface();
        };
        addAndMakeVisible(*button);
        articulationButtons.push_back(std::move(button));
    }
}

void PerformancePanel::refreshSurface()
{
    performanceSnapshot = engineFacade.getPerformanceSnapshot();
    const auto articulations = engineFacade.getArticulationDescriptors();
    const auto macros = engineFacade.getMacroDescriptors();

    if (articulations.size() != articulationButtons.size())
    {
        rebuildArticulationButtons();
        resized();
    }

    if (macros.size() != macroControls.size())
    {
        rebuildMacroControls();
        resized();
    }

    instrumentLabel.setText(juce::String::fromUTF8(performanceSnapshot.instrumentDisplayName.c_str()),
                            juce::dontSendNotification);
    if (!performanceSnapshot.loaded && performanceSnapshot.instrumentDisplayName == "No instrument loaded")
    {
        patchStatusLabel.setText("No instrument loaded yet. Use Load Default or Load Lead Demo.",
                                 juce::dontSendNotification);
    }
    else
    {
        patchStatusLabel.setText(
            "Preset " + juce::String::fromUTF8(performanceSnapshot.presetId.c_str())
                + " | Load " + juce::String::fromUTF8(performanceSnapshot.loadProfileId.c_str())
                + " | Articulation " + formatArticulationLabel(performanceSnapshot.selectedArticulationName,
                                                               performanceSnapshot.selectedArticulationId),
            juce::dontSendNotification);
    }

    loadIndicatorLabel.setText(juce::String::fromUTF8(performanceSnapshot.loadIndicator.c_str()),
                               juce::dontSendNotification);
    const auto hasPreviewError = !performanceSnapshot.previewPlayback.errorMessage.empty();
    loadIndicatorLabel.setColour(juce::Label::backgroundColourId,
                                 hasPreviewError
                                     ? performancePanelWarning
                                     : (performanceSnapshot.loaded ? performancePanelSuccess : performancePanelDanger));
    loadIndicatorLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    juce::String previewText = "Preview: "
        + juce::String::fromUTF8(performanceSnapshot.previewPlayback.state.c_str());
    if (performanceSnapshot.previewPlayback.attempted)
    {
        previewText << " | played " << performanceSnapshot.previewPlayback.midiNote
                    << " -> effective " << performanceSnapshot.previewPlayback.effectiveMidiNote
                    << " | velocity " << performanceSnapshot.previewPlayback.effectiveVelocity
                    << " | zone " << juce::String::fromUTF8(performanceSnapshot.previewPlayback.zoneId.c_str());
    }
    previewText << " | " << juce::String::fromUTF8(performanceSnapshot.previewPlayback.appliedMacroSummary.c_str());
    if (!performanceSnapshot.previewPlayback.errorMessage.empty())
    {
        previewText << " | " << juce::String::fromUTF8(performanceSnapshot.previewPlayback.errorMessage.c_str());
    }
    previewStatusLabel.setText(previewText, juce::dontSendNotification);
    previewStatusLabel.setColour(juce::Label::textColourId,
                                 hasPreviewError ? performancePanelDanger : juce::Colour::fromRGB(52, 64, 84));

    macroStripLabel.setText("Macro Strip", juce::dontSendNotification);
    articulationLabel.setText("Articulations", juce::dontSendNotification);
    syncKeyboardPlayableRange();

    for (std::size_t index = 0; index < std::min(articulations.size(), articulationButtons.size()); ++index)
    {
        const auto& articulation = articulations[index];
        auto& button = articulationButtons[index];
        button->setButtonText(juce::String::fromUTF8(articulation.name.c_str()));
        button->setColour(juce::TextButton::buttonColourId,
                          articulation.selected ? performancePanelAccent : juce::Colour::fromRGB(220, 228, 237));
        button->setColour(juce::TextButton::textColourOffId,
                          articulation.selected ? juce::Colours::white : juce::Colour::fromRGB(14, 20, 27));
    }

    for (std::size_t index = 0; index < std::min(macros.size(), macroControls.size()); ++index)
    {
        auto& control = macroControls[index];
        const auto& macro = macros[index];
        control->nameLabel.setText(juce::String::fromUTF8(macro.name.c_str()), juce::dontSendNotification);
        control->slider.setRange(macro.minValue, macro.maxValue, 0.001);
        control->slider.setValue(macro.currentValue, juce::dontSendNotification);
        control->valueLabel.setText(juce::String(macro.currentValue, 3)
                                        + " | "
                                        + juce::String::fromUTF8(macro.currentEffect.c_str()),
                                    juce::dontSendNotification);
    }

    if (diagnosticsPanel.isVisible())
        diagnosticsPanel.repaint();
}

void PerformancePanel::syncKeyboardPlayableRange()
{
    const auto manifest = engineFacade.loadPhase1ReferenceInstrument();
    const auto& sessionState = engineFacade.getCurrentSessionState();

    int lowestPlayableNote = 36;
    int highestPlayableNote = 96;

    if (manifest.loaded && !manifest.instrument.zones.empty())
    {
        const auto motionOffset = computeMotionSemitoneOffset(sessionState);
        auto includeMatchingZones = [&](const std::string& articulationId)
        {
            auto foundMatchingZone = false;

            for (const auto& zone : manifest.instrument.zones)
            {
                if (!articulationId.empty() && zone.articulationId != articulationId)
                    continue;

                lowestPlayableNote = std::min(lowestPlayableNote, zone.keyLow - motionOffset);
                highestPlayableNote = std::max(highestPlayableNote, zone.keyHigh - motionOffset);
                foundMatchingZone = true;
            }

            return foundMatchingZone;
        };

        lowestPlayableNote = 127;
        highestPlayableNote = 0;

        if (!includeMatchingZones(sessionState.selectedArticulationId))
        {
            lowestPlayableNote = 127;
            highestPlayableNote = 0;
            includeMatchingZones({});
        }

        if (lowestPlayableNote <= highestPlayableNote)
        {
            lowestPlayableNote = std::clamp(lowestPlayableNote, 0, 127);
            highestPlayableNote = std::clamp(highestPlayableNote, lowestPlayableNote, 127);
        }
        else
        {
            lowestPlayableNote = 36;
            highestPlayableNote = 96;
        }
    }

    keyboardComponent.setAvailableRange(lowestPlayableNote, highestPlayableNote);

    const auto currentLowestVisibleKey = keyboardComponent.getLowestVisibleKey();
    if (currentLowestVisibleKey < lowestPlayableNote || currentLowestVisibleKey > highestPlayableNote)
        keyboardComponent.setLowestVisibleKey(lowestPlayableNote);

    keyboardHintLabel.setText(
        "Play the keyboard to audition routing, macro state, and the selected articulation. Range "
            + juce::MidiMessage::getMidiNoteName(lowestPlayableNote, true, true, 3)
            + " - "
            + juce::MidiMessage::getMidiNoteName(highestPlayableNote, true, true, 3)
            + " follows the current playable zone window.",
        juce::dontSendNotification);
}
} // namespace drs::app
