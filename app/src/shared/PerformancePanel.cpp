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

juce::String summarizeDigest(const std::string& digest)
{
    if (digest.empty())
        return "none";

    constexpr std::size_t prefixLength = 18;
    if (digest.size() <= prefixLength)
        return juce::String::fromUTF8(digest.c_str());

    return juce::String::fromUTF8((digest.substr(0, prefixLength) + "...").c_str());
}

juce::String summarizeFindings(const std::vector<drs::engine::PlaybackSnapshotFinding>& findings)
{
    if (findings.empty())
        return "none";

    juce::String summary = juce::String::fromUTF8(findings.front().message.c_str());
    if (findings.size() > 1)
        summary << " (+" << juce::String(static_cast<int>(findings.size() - 1)) << " more)";

    return summary;
}

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

struct PerformanceMacroSurfaceModel
{
    std::vector<drs::engine::EngineMacroDescriptor> displayedMacros;
    bool showingPublishedMixer = false;
    std::size_t hiddenPublishedMacroCount = 0;
};

PerformanceMacroSurfaceModel buildPerformanceMacroSurfaceModel(
    const std::vector<drs::engine::EngineMacroDescriptor>& macros)
{
    PerformanceMacroSurfaceModel model;
    model.showingPublishedMixer = std::any_of(macros.begin(), macros.end(), [](const auto& macro)
    {
        return macro.publishedControl;
    });

    if (!model.showingPublishedMixer)
    {
        model.displayedMacros = macros;
        return model;
    }

    model.displayedMacros.reserve(macros.size());
    for (const auto& macro : macros)
    {
        if (macro.exposedInPerformance)
            model.displayedMacros.push_back(macro);
        else
            ++model.hiddenPublishedMacroCount;
    }

    std::stable_sort(model.displayedMacros.begin(), model.displayedMacros.end(), [](const auto& left, const auto& right)
    {
        return left.authoredOrder < right.authoredOrder;
    });

    return model;
}

bool sameMacroLayout(const std::vector<std::string>& currentIds,
                     const std::vector<drs::engine::EngineMacroDescriptor>& nextMacros,
                     const bool currentMixerMode,
                     const bool nextMixerMode)
{
    if (currentMixerMode != nextMixerMode || currentIds.size() != nextMacros.size())
        return false;

    for (std::size_t index = 0; index < nextMacros.size(); ++index)
    {
        if (currentIds[index] != nextMacros[index].id)
            return false;
    }

    return true;
}

std::vector<PerformanceMixerControlView> buildPublishedMixerControls(
    const std::vector<drs::engine::EngineMacroDescriptor>& macros)
{
    std::vector<PerformanceMixerControlView> controls;
    controls.reserve(macros.size());
    for (const auto& macro : macros)
    {
        PerformanceMixerControlView control;
        control.authoredId = macro.authoredId.empty() ? macro.id : macro.authoredId;
        control.runtimeId = macro.id;
        control.sectionLabel = macro.sectionLabel;
        control.controlLabel = macro.name;
        control.parameterLabel = macro.parameterLabel;
        control.valueUnit = macro.valueUnit;
        control.accessibilityDescription = macro.accessibilityDescription;
        control.controlKind = macro.controlKind;
        control.authoredOrder = macro.authoredOrder;
        control.minimum = macro.minValue;
        control.maximum = macro.maxValue;
        control.displayMinimum = macro.displayMinimum;
        control.displayMaximum = macro.displayMaximum;
        control.value = macro.currentValue;
        controls.push_back(std::move(control));
    }
    return controls;
}

juce::String buildMacroStripTitle(const PerformanceMacroSurfaceModel& model)
{
    if (!model.showingPublishedMixer)
        return model.displayedMacros.empty() ? "Performance Macros" : "Performance Macros | Preview Controls";

    if (model.displayedMacros.empty())
        return "Published Controls | None Exposed";

    return "Performance Mixer | " + juce::String(static_cast<int>(model.displayedMacros.size()))
        + " Exposed";
}

juce::String buildMacroStripDescription(const PerformanceMacroSurfaceModel& model)
{
    if (!model.showingPublishedMixer)
        return "Reference preview macros stay visible until a published performance binding becomes active.";

    if (model.displayedMacros.empty())
        return "Published helper controls remain active, but this instrument does not expose any end-user performance controls.";

    return "Published exposed controls stay in authored order while hidden helper controls remain available in Diagnostics.";
}

juce::String buildMixerEmptyStateText(const std::size_t hiddenPublishedMacroCount)
{
    auto text = juce::String("This instrument publishes no exposed performance controls.");
    if (hiddenPublishedMacroCount > 0)
    {
        text << " " << juce::String(static_cast<int>(hiddenPublishedMacroCount))
             << " hidden helper control";
        if (hiddenPublishedMacroCount != 1)
            text << "s";
        text << " remain available in Diagnostics.";
    }

    return text;
}
} // namespace

PerformancePanel::PerformancePanel(drs::engine::EngineFacade& facade,
                                   MacroValueChangedCallback macroValueChanged,
                                   PerformanceNoteOnCallback performanceNoteOn,
                                   PerformanceNoteOffCallback performanceNoteOff,
                                   PublishCommandCallback publishCommand,
                                   PublishPresentationProvider presentationProvider,
                                   AudioCallbackActiveProvider callbackActiveProvider)
    : engineFacade(facade),
      onMacroValueChanged(std::move(macroValueChanged)),
      onPerformanceNoteOn(std::move(performanceNoteOn)),
      onPerformanceNoteOff(std::move(performanceNoteOff)),
      publishPresentationProvider(std::move(presentationProvider)),
      audioCallbackActiveProvider(std::move(callbackActiveProvider)),
      publishedMixer([this](const std::string& macroId, const double value)
      {
          if (onMacroValueChanged)
              onMacroValueChanged(macroId, value);
          else
              engineFacade.setMacroValue(macroId, value);
      }),
      keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      diagnosticsPanel(facade, onMacroValueChanged, std::move(publishCommand),
                       publishPresentationProvider)
{
    titleLabel.setText("Performance Mixer", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    instrumentLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    instrumentLabel.setComponentID("performanceInstrumentLabel");
    patchStatusLabel.setFont(juce::FontOptions(15.0f));
    patchStatusLabel.setComponentID("performancePatchStatusLabel");
    previewStatusLabel.setFont(juce::FontOptions(15.0f));
    previewStatusLabel.setComponentID("performancePreviewStatusLabel");
    macroStripLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    macroStripLabel.setComponentID("performanceMacroStripLabel");
    mixerEmptyStateLabel.setFont(juce::FontOptions(15.0f));
    mixerEmptyStateLabel.setComponentID("performanceMixerEmptyStateLabel");
    mixerEmptyStateLabel.setJustificationType(juce::Justification::centredLeft);
    articulationLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    articulationLabel.setComponentID("performanceArticulationLabel");
    keyboardHintLabel.setFont(juce::FontOptions(15.0f));
    keyboardHintLabel.setComponentID("performanceKeyboardHintLabel");
    loadIndicatorLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    loadIndicatorLabel.setComponentID("performanceLoadIndicatorLabel");

    instrumentLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(14, 20, 27));
    patchStatusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(52, 64, 84));
    previewStatusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(52, 64, 84));
    macroStripLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(14, 20, 27));
    mixerEmptyStateLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(52, 64, 84));
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
    addAndMakeVisible(mixerEmptyStateLabel);
    addAndMakeVisible(articulationLabel);
    addAndMakeVisible(keyboardHintLabel);
    addAndMakeVisible(loadIndicatorLabel);
    addAndMakeVisible(loadDefaultButton);
    addAndMakeVisible(loadLeadButton);
    addAndMakeVisible(diagnosticsToggle);
    addAndMakeVisible(keyboardComponent);
    addChildComponent(publishedMixer);
    addChildComponent(diagnosticsPanel);

    rebuildArticulationButtons();
    rebuildMacroControls(engineFacade.getMacroDescriptors(), false);
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

    if (showingPublishedMixer)
    {
        if (publishedMixer.getControlCount() == 0)
        {
            publishedMixer.setBounds({});
            mixerEmptyStateLabel.setBounds(area.removeFromTop(56));
            area.removeFromTop(8);
        }
        else
        {
            const auto mixerHeight = std::min(396, std::max(184, area.getHeight() - 180));
            publishedMixer.setBounds(area.removeFromTop(mixerHeight));

            area.removeFromTop(10);
        }
    }
    else
    {
        publishedMixer.setBounds({});
        mixerEmptyStateLabel.setBounds({});
        for (auto& control : macroControls)
        {
            auto macroRow = area.removeFromTop(28);
            control->nameLabel.setBounds(macroRow.removeFromLeft(110));
            control->slider.setBounds(macroRow.removeFromLeft(220));
            macroRow.removeFromLeft(10);
            control->valueLabel.setBounds(macroRow.removeFromLeft(190));
            area.removeFromTop(6);
        }
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

void PerformancePanel::refreshNow()
{
    refreshSurface();
    diagnosticsPanel.refreshNow();
}

void PerformancePanel::timerCallback()
{
    if (lastObservedStateRevision != engineFacade.getStateRevision())
        refreshSurface();
}

void PerformancePanel::handleNoteOn(juce::MidiKeyboardState*, int, int midiNoteNumber, float velocity)
{
    const auto clampedVelocity = std::clamp(static_cast<int>(std::round(velocity * 127.0f)), 1, 127);

    if (onPerformanceNoteOn)
        onPerformanceNoteOn(midiNoteNumber,
                            static_cast<float>(clampedVelocity) / 127.0f);
    refreshSurface();
}

void PerformancePanel::handleNoteOff(juce::MidiKeyboardState*, int, int midiNoteNumber, float)
{
    if (onPerformanceNoteOff)
        onPerformanceNoteOff(midiNoteNumber);
}

void PerformancePanel::rebuildMacroControls(
    const std::vector<drs::engine::EngineMacroDescriptor>& macros,
    const bool mixerControl)
{
    visibleMacroIds.clear();
    macroControls.clear();
    showingPublishedMixer = mixerControl;
    publishedMixer.setVisible(mixerControl);

    if (mixerControl)
    {
        publishedMixer.setControls(buildPublishedMixerControls(macros));
        return;
    }

    publishedMixer.setControls({});

    for (const auto& macro : macros)
    {
        auto control = std::make_unique<MacroControl>();
        control->id = macro.id;
        control->mixerControl = mixerControl;
        visibleMacroIds.push_back(macro.id);
        const auto macroId = juce::String::fromUTF8(macro.id.c_str());
        control->nameLabel.setComponentID("performanceMacroNameLabel." + macroId);
        control->slider.setComponentID("performanceMacroSlider." + macroId);
        control->valueLabel.setComponentID("performanceMacroValueLabel." + macroId);
        control->nameLabel.setText(juce::String::fromUTF8(macro.name.c_str()), juce::dontSendNotification);
        control->nameLabel.setJustificationType(mixerControl
                                                    ? juce::Justification::centred
                                                    : juce::Justification::centredLeft);
        control->slider.setSliderStyle(mixerControl
                                           ? juce::Slider::LinearVertical
                                           : juce::Slider::LinearHorizontal);
        control->slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        control->slider.setRange(macro.minValue, macro.maxValue, 0.001);
        control->slider.setColour(juce::Slider::trackColourId,
                                  performancePanelAccent.withAlpha(mixerControl ? 0.55f : 0.35f));
        control->slider.setColour(juce::Slider::thumbColourId, performancePanelAccent);
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
        control->valueLabel.setJustificationType(mixerControl
                                                     ? juce::Justification::centred
                                                     : juce::Justification::centredRight);

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
    lastObservedStateRevision = engineFacade.getStateRevision();
    const auto articulations = engineFacade.getArticulationDescriptors();
    const auto allMacros = engineFacade.getMacroDescriptors();
    const auto macroSurface = buildPerformanceMacroSurfaceModel(allMacros);
    const auto publishPresentation = publishPresentationProvider
        ? publishPresentationProvider()
        : engineFacade.getPerformancePublishPresentationSnapshot();

    if (articulations.size() != articulationButtons.size())
    {
        rebuildArticulationButtons();
        resized();
    }

    hiddenPublishedMacroCount = macroSurface.hiddenPublishedMacroCount;
    if (!sameMacroLayout(visibleMacroIds,
                         macroSurface.displayedMacros,
                         showingPublishedMixer,
                         macroSurface.showingPublishedMixer))
    {
        rebuildMacroControls(macroSurface.displayedMacros, macroSurface.showingPublishedMixer);
        resized();
    }
    else if (macroSurface.showingPublishedMixer)
    {
        publishedMixer.setControls(buildPublishedMixerControls(macroSurface.displayedMacros));
    }

    instrumentLabel.setText(juce::String::fromUTF8(performanceSnapshot.instrumentDisplayName.c_str()),
                            juce::dontSendNotification);
    if (!performanceSnapshot.loaded && performanceSnapshot.instrumentDisplayName == "No instrument loaded")
    {
        patchStatusLabel.setText("No performance instrument loaded yet. Surface "
                                     + juce::String::fromUTF8(performanceSnapshot.surfaceStateSource.c_str())
                                     + " | renderer "
                                     + juce::String::fromUTF8(performanceSnapshot.rendererMode.c_str())
                                     + ". Use Load Default or Load Lead Demo.",
                                 juce::dontSendNotification);
    }
    else
    {
        patchStatusLabel.setText(
            "Preset " + juce::String::fromUTF8(performanceSnapshot.presetId.c_str())
                + " | Load " + juce::String::fromUTF8(performanceSnapshot.loadProfileId.c_str())
                + " | Articulation " + formatArticulationLabel(performanceSnapshot.selectedArticulationName,
                                                               performanceSnapshot.selectedArticulationId)
                + " | Draft r" + juce::String(static_cast<juce::int64>(performanceSnapshot.draftRevision))
                + " | Preview r" + juce::String(static_cast<juce::int64>(performanceSnapshot.previewRevision))
                + " (" + juce::String::fromUTF8(performanceSnapshot.previewRevisionState.c_str()) + ")"
                + " | Published r" + juce::String(static_cast<juce::int64>(performanceSnapshot.publishedRevision))
                + " (" + juce::String::fromUTF8(drs::engine::toString(
                    performanceSnapshot.publishedPresentationState)) + ")"
                + " | Preview build #" + juce::String(static_cast<juce::int64>(performanceSnapshot.previewBuildId))
                + " | Publish build #" + juce::String(static_cast<juce::int64>(performanceSnapshot.publishedBuildId))
                + " | Surface " + juce::String::fromUTF8(performanceSnapshot.surfaceStateSource.c_str())
                + " | Renderer " + juce::String::fromUTF8(performanceSnapshot.rendererMode.c_str()),
            juce::dontSendNotification);
    }

    juce::String loadIndicatorText = juce::String::fromUTF8(performanceSnapshot.loadIndicator.c_str());
    auto publishFailed = false;
    auto publishDiagnostic = juce::String();
    if (publishPresentation != nullptr)
    {
        hasActivePublishedPerformance = publishPresentation->hasActivePublished;
        publishedPerformanceStateLabel = juce::String::fromUTF8(
            publishPresentation->stateLabel.c_str());
        publishedPerformanceGuidance = juce::String::fromUTF8(
            publishPresentation->guidance.c_str());
        publishedPerformanceFindingCode = juce::String::fromUTF8(
            publishPresentation->findingCode.c_str());
        publishFailed = publishPresentation->state
            == drs::engine::PerformancePublishPresentationState::failed;
        loadIndicatorText = "Publish " + publishedPerformanceStateLabel;
        if (publishPresentation->hasActivePublished)
            loadIndicatorText << " r" << static_cast<juce::int64>(publishPresentation->activePublishedRevision);
        publishDiagnostic = loadIndicatorText + ": " + publishedPerformanceGuidance;
        if (publishedPerformanceFindingCode.isNotEmpty())
            publishDiagnostic << " [" << publishedPerformanceFindingCode << "]";
    }
    loadIndicatorLabel.setText(loadIndicatorText,
                               juce::dontSendNotification);
    loadIndicatorLabel.setTooltip(publishDiagnostic);
    loadIndicatorLabel.setDescription(publishDiagnostic);
    const auto hasPreviewError = !performanceSnapshot.previewPlayback.errorMessage.empty();
    loadIndicatorLabel.setColour(juce::Label::backgroundColourId,
                                 publishFailed
                                     ? performancePanelDanger
                                     : (hasPreviewError
                                            ? performancePanelWarning
                                            : (performanceSnapshot.loaded
                                                   ? performancePanelSuccess
                                                   : performancePanelDanger)));
    loadIndicatorLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    juce::String previewText = "Preview: draft r"
        + juce::String(static_cast<juce::int64>(performanceSnapshot.previewPlayback.draftRevision))
        + " -> prepared r"
        + juce::String(static_cast<juce::int64>(performanceSnapshot.previewPlayback.preparedRevision))
        + " | "
        + juce::String::fromUTF8(performanceSnapshot.previewPlayback.revisionState.c_str());
    if (performanceSnapshot.previewPlayback.attempted)
    {
        previewText << " | played " << performanceSnapshot.previewPlayback.midiNote
                    << " -> effective " << performanceSnapshot.previewPlayback.effectiveMidiNote
                    << " | velocity " << performanceSnapshot.previewPlayback.effectiveVelocity
                    << " | zone " << juce::String::fromUTF8(performanceSnapshot.previewPlayback.zoneId.c_str());
    }
    if (performanceSnapshot.previewPlayback.pendingBuild)
    {
        previewText << " | preparing";
    }
    previewText << " | digest " << summarizeDigest(performanceSnapshot.previewContentDigest);
    if (!performanceSnapshot.previewFindings.empty())
        previewText << " | findings " << summarizeFindings(performanceSnapshot.previewFindings);
    previewText << " | " << juce::String::fromUTF8(performanceSnapshot.previewPlayback.appliedMacroSummary.c_str());
    if (!performanceSnapshot.previewPlayback.errorMessage.empty())
    {
        previewText << " | " << juce::String::fromUTF8(performanceSnapshot.previewPlayback.errorMessage.c_str());
    }
    previewStatusLabel.setText(previewText, juce::dontSendNotification);
    previewStatusLabel.setColour(juce::Label::textColourId,
                                 hasPreviewError ? performancePanelDanger : juce::Colour::fromRGB(52, 64, 84));

    const auto macroStripDescription = buildMacroStripDescription(macroSurface);
    macroStripLabel.setText(buildMacroStripTitle(macroSurface), juce::dontSendNotification);
    macroStripLabel.setTooltip(macroStripDescription);
    macroStripLabel.setDescription(macroStripDescription);
    mixerEmptyStateLabel.setVisible(macroSurface.showingPublishedMixer
                                    && macroSurface.displayedMacros.empty());
    const auto mixerEmptyText = buildMixerEmptyStateText(macroSurface.hiddenPublishedMacroCount);
    mixerEmptyStateLabel.setText(mixerEmptyText, juce::dontSendNotification);
    mixerEmptyStateLabel.setTooltip(mixerEmptyText);
    mixerEmptyStateLabel.setDescription(mixerEmptyText);
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

    for (std::size_t index = 0;
         index < std::min(macroSurface.displayedMacros.size(), macroControls.size());
         ++index)
    {
        auto& control = macroControls[index];
        const auto& macro = macroSurface.displayedMacros[index];
        const auto currentEffect = juce::String::fromUTF8(macro.currentEffect.c_str());
        const auto currentValueText = juce::String(macro.currentValue, 3);
        control->nameLabel.setText(juce::String::fromUTF8(macro.name.c_str()), juce::dontSendNotification);
        control->slider.setRange(macro.minValue, macro.maxValue, 0.001);
        control->slider.setValue(macro.currentValue, juce::dontSendNotification);
        control->valueLabel.setText(
            currentEffect.isNotEmpty() ? currentValueText + " | " + currentEffect : currentValueText,
            juce::dontSendNotification);
        auto controlDescription = juce::String::fromUTF8(macro.soundIntent.c_str());
        if (currentEffect.isNotEmpty())
            controlDescription << " Current effect: " << currentEffect << ".";
        control->slider.setTooltip(controlDescription);
        control->slider.setDescription(controlDescription);
        control->nameLabel.setTooltip(controlDescription);
        control->valueLabel.setTooltip(controlDescription);
    }

    if (diagnosticsPanel.isVisible())
        diagnosticsPanel.repaint();
}

void PerformancePanel::syncKeyboardPlayableRange()
{
    int lowestPlayableNote = 36;
    int highestPlayableNote = 96;

    if (performanceSnapshot.playableRangeAvailable)
    {
        lowestPlayableNote = std::clamp(performanceSnapshot.lowestPlayableNote, 0, 127);
        highestPlayableNote = std::clamp(performanceSnapshot.highestPlayableNote, lowestPlayableNote, 127);
    }

    keyboardComponent.setAvailableRange(lowestPlayableNote, highestPlayableNote);

    const auto audioCallbackActive = !audioCallbackActiveProvider
        || audioCallbackActiveProvider();
    keyboardComponent.setEnabled(hasActivePublishedPerformance);

    const auto currentLowestVisibleKey = keyboardComponent.getLowestVisibleKey();
    if (currentLowestVisibleKey < lowestPlayableNote || currentLowestVisibleKey > highestPlayableNote)
        keyboardComponent.setLowestVisibleKey(lowestPlayableNote);

    auto keyboardHint = juce::String();
    if (!audioCallbackActive)
    {
        keyboardHint = "Audio inactive - open Settings > Audio Device Settings, or enable host FX processing. ";
    }
    else if (!hasActivePublishedPerformance)
    {
        if (publishedPerformanceStateLabel == "Failed")
        {
            keyboardHint = "Publish failed: " + publishedPerformanceGuidance;
            if (publishedPerformanceFindingCode.isNotEmpty())
                keyboardHint << " [" << publishedPerformanceFindingCode << "] ";
        }
        else
        {
            keyboardHint = "Keyboard waiting for an active publication (Publish "
                + publishedPerformanceStateLabel
                + "). Keep audio processing running until Publish becomes Active. ";
        }
    }
    else
    {
        if (showingPublishedMixer && publishedMixer.getControlCount() == 0)
        {
            keyboardHint = "Published performance is active. This instrument publishes no exposed performance controls. ";
            if (hiddenPublishedMacroCount > 0)
                keyboardHint << "Hidden helper controls remain available in Diagnostics. ";
        }
        else
        {
            keyboardHint = "Play the keyboard to audition the active Performance path, routing, and macro state. ";
        }
    }
    keyboardHint << "Range "
            + juce::MidiMessage::getMidiNoteName(lowestPlayableNote, true, true, 3)
            + " - "
            + juce::MidiMessage::getMidiNoteName(highestPlayableNote, true, true, 3)
            + " follows the current playable zone window.";
    keyboardHintLabel.setText(
        keyboardHint,
        juce::dontSendNotification);
    keyboardHintLabel.setTooltip(keyboardHint);
    keyboardHintLabel.setDescription(keyboardHint);
    keyboardComponent.setTitle("Performance keyboard");
    keyboardComponent.setDescription(keyboardHint);
}
} // namespace drs::app
