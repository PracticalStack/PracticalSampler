#include "shared/PerformancePanel.h"
#include "shared/MessageThreadMetrics.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>
#include <cmath>

namespace drs::app
{
namespace
{
constexpr int performOuterMargin = 16;
constexpr int performGap = 10;
constexpr int performPanelInset = 10;

void drawPerformSurface(juce::Graphics& graphics,
                        const juce::Rectangle<int> bounds,
                        const juce::Colour fill = authoring::visual::surface)
{
    if (bounds.isEmpty())
        return;

    const auto shape = bounds.toFloat().reduced(0.5f);
    graphics.setColour(fill);
    graphics.fillRoundedRectangle(shape, authoring::visual::panelRadius);
    graphics.setColour(authoring::visual::border);
    graphics.drawRoundedRectangle(shape, authoring::visual::panelRadius,
                                  authoring::visual::borderWidth);
}

juce::String buildInstrumentContext(const drs::engine::EnginePerformanceSnapshot& snapshot)
{
    juce::StringArray parts;
    if (!snapshot.presetId.empty())
        parts.add("Preset " + juce::String::fromUTF8(snapshot.presetId.c_str()));

    const auto articulation = !snapshot.selectedArticulationName.empty()
        ? snapshot.selectedArticulationName : snapshot.selectedArticulationId;
    if (!articulation.empty())
        parts.add("Articulation " + juce::String::fromUTF8(articulation.c_str()));

    if (snapshot.playableRangeAvailable)
    {
        parts.add("Range "
            + juce::MidiMessage::getMidiNoteName(snapshot.lowestPlayableNote, true, true, 3)
            + " - "
            + juce::MidiMessage::getMidiNoteName(snapshot.highestPlayableNote, true, true, 3));
    }

    return parts.isEmpty() ? juce::String("Performance workspace")
                           : parts.joinIntoString("  |  ");
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
        control.controlLaw = macro.controlLaw;
        controls.push_back(std::move(control));
    }
    return controls;
}

juce::String buildMacroStripTitle(const PerformanceMacroSurfaceModel& model)
{
    if (!model.showingPublishedMixer)
        return "Instrument Controls";

    if (model.displayedMacros.empty())
        return "Instrument Controls | None Exposed";

    return "Instrument Controls | " + juce::String(static_cast<int>(model.displayedMacros.size()))
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

PerformancePanel::PerformanceControlLookAndFeel::PerformanceControlLookAndFeel()
{
    setColour(juce::TextButton::buttonColourId, authoring::visual::surfaceRaised);
    setColour(juce::TextButton::buttonOnColourId, authoring::visual::information);
    setColour(juce::TextButton::textColourOffId, authoring::visual::text);
    setColour(juce::TextButton::textColourOnId, authoring::visual::textOnAccent);
    setColour(juce::ToggleButton::textColourId, authoring::visual::text);
    setColour(juce::ToggleButton::tickColourId, authoring::visual::information);
    setColour(juce::ToggleButton::tickDisabledColourId, authoring::visual::textDisabled);
    setColour(juce::Label::textColourId, authoring::visual::text);
    setColour(juce::Slider::thumbColourId, authoring::visual::information);
    setColour(juce::Slider::trackColourId, authoring::visual::information.withAlpha(0.72f));
    setColour(juce::Slider::backgroundColourId, authoring::visual::surfaceSubtle);
    setColour(juce::ScrollBar::backgroundColourId, authoring::visual::surfaceSubtle);
    setColour(juce::ScrollBar::thumbColourId, authoring::visual::borderStrong);
    setColour(juce::TooltipWindow::backgroundColourId, authoring::visual::surfaceRaised);
    setColour(juce::TooltipWindow::textColourId, authoring::visual::text);
    setColour(juce::TooltipWindow::outlineColourId, authoring::visual::borderStrong);
}

void PerformancePanel::PerformanceControlLookAndFeel::drawButtonBackground(
    juce::Graphics& graphics,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    const bool highlighted,
    const bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const auto focused = button.hasKeyboardFocus(true);
    if (focused)
    {
        authoring::visual::drawFocusRing(graphics, bounds.reduced(1.0f));
        bounds = bounds.reduced(3.0f);
    }

    auto fill = button.isEnabled() ? backgroundColour
                                   : authoring::visual::disabled(backgroundColour);
    if (down)
        fill = fill.interpolatedWith(authoring::visual::information, 0.22f);
    else if (highlighted)
        fill = fill.interpolatedWith(authoring::visual::surfaceHover, 0.72f);

    graphics.setColour(fill);
    graphics.fillRoundedRectangle(bounds, authoring::visual::controlRadius);
    graphics.setColour(focused ? authoring::visual::focus : authoring::visual::borderStrong);
    graphics.drawRoundedRectangle(bounds, authoring::visual::controlRadius,
                                  authoring::visual::borderWidth);
}

void PerformancePanel::PerformanceControlLookAndFeel::drawToggleButton(
    juce::Graphics& graphics,
    juce::ToggleButton& button,
    const bool highlighted,
    const bool down)
{
    juce::LookAndFeel_V4::drawToggleButton(graphics, button, highlighted, down);
    if (button.hasKeyboardFocus(true))
        authoring::visual::drawFocusRing(
            graphics, button.getLocalBounds().toFloat().reduced(1.0f));
}

void PerformancePanel::PerformanceControlLookAndFeel::drawLinearSliderOutline(
    juce::Graphics& graphics,
    const int x,
    const int y,
    const int width,
    const int height,
    const juce::Slider::SliderStyle style,
    juce::Slider& slider)
{
    if (slider.hasKeyboardFocus(true))
    {
        authoring::visual::drawFocusRing(
            graphics, slider.getLocalBounds().toFloat().reduced(1.0f));
        return;
    }
    juce::LookAndFeel_V4::drawLinearSliderOutline(
        graphics, x, y, width, height, style, slider);
}

void PerformancePanel::PerformanceControlLookAndFeel::drawRotarySlider(
    juce::Graphics& graphics,
    const int x,
    const int y,
    const int width,
    const int height,
    const float sliderPosition,
    const float rotaryStartAngle,
    const float rotaryEndAngle,
    juce::Slider& slider)
{
    juce::LookAndFeel_V4::drawRotarySlider(
        graphics, x, y, width, height, sliderPosition,
        rotaryStartAngle, rotaryEndAngle, slider);
    if (slider.hasKeyboardFocus(true))
        authoring::visual::drawFocusRing(
            graphics, slider.getLocalBounds().toFloat().reduced(1.0f));
}

void PerformancePanel::ArtworkPanel::setArtwork(juce::Image nextArtwork, juce::String nextDescription)
{
    artwork = std::move(nextArtwork);
    description = std::move(nextDescription);
    setTitle("Performance artwork");
    setDescription(description);
    repaint();
}

void PerformancePanel::ArtworkPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(authoring::visual::mapSurface);
    g.fillRoundedRectangle(bounds, authoring::visual::panelRadius);

    if (artwork.isValid())
    {
        g.reduceClipRegion(getLocalBounds().reduced(1));
        g.drawImageWithin(artwork,
                          getLocalBounds().getX() + 1,
                          getLocalBounds().getY() + 1,
                          std::max(1, getLocalBounds().getWidth() - 2),
                          std::max(1, getLocalBounds().getHeight() - 2),
                          juce::RectanglePlacement::fillDestination);
    }
    else
    {
        g.setColour(authoring::visual::textMuted);
        g.setFont(juce::FontOptions(authoring::visual::bodyTypeSize));
        g.drawFittedText("Artwork unavailable", getLocalBounds().reduced(12),
                         juce::Justification::centred, 1);
    }

    g.setColour(authoring::visual::border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), authoring::visual::panelRadius,
                           authoring::visual::borderWidth);
}

PerformancePanel::PerformancePanel(drs::engine::EngineFacade& facade,
                                   MacroValueChangedCallback macroValueChanged,
                                   PerformanceNoteOnCallback performanceNoteOn,
                                   PerformanceNoteOffCallback performanceNoteOff,
                                   PublishCommandCallback publishCommand,
                                   PublishPresentationProvider presentationProvider,
                                   AudioCallbackActiveProvider callbackActiveProvider,
                                   InstrumentControlsExpandedProvider controlsExpandedProvider,
                                   InstrumentControlsExpandedChangedCallback controlsExpandedChanged)
    : engineFacade(facade),
      onMacroValueChanged(std::move(macroValueChanged)),
      onPerformanceNoteOn(std::move(performanceNoteOn)),
      onPerformanceNoteOff(std::move(performanceNoteOff)),
      publishPresentationProvider(std::move(presentationProvider)),
      audioCallbackActiveProvider(std::move(callbackActiveProvider)),
      instrumentControlsExpandedProvider(std::move(controlsExpandedProvider)),
      onInstrumentControlsExpandedChanged(std::move(controlsExpandedChanged)),
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
    setLookAndFeel(&performanceLookAndFeel);
    if (instrumentControlsExpandedProvider)
        userInstrumentControlsExpandedChoice = instrumentControlsExpandedProvider();
    if (userInstrumentControlsExpandedChoice.has_value())
        instrumentControlsCollapsed = !*userInstrumentControlsExpandedChoice;

    instrumentNameLabel.setComponentID("performanceInstrumentNameLabel");
    instrumentNameLabel.setFont(juce::FontOptions(authoring::visual::titleTypeSize,
                                                   juce::Font::bold));
    instrumentNameLabel.setColour(juce::Label::textColourId, authoring::visual::text);
    instrumentNameLabel.setJustificationType(juce::Justification::centredLeft);
    instrumentNameLabel.setTitle("Performance instrument");

    instrumentContextLabel.setComponentID("performanceInstrumentContextLabel");
    instrumentContextLabel.setFont(juce::FontOptions(authoring::visual::compactTypeSize));
    instrumentContextLabel.setColour(juce::Label::textColourId, authoring::visual::textMuted);
    instrumentContextLabel.setJustificationType(juce::Justification::centredLeft);
    instrumentContextLabel.setTitle("Performance context");

    performanceGuidanceLabel.setComponentID("performanceGuidanceLabel");
    performanceGuidanceLabel.setFont(juce::FontOptions(authoring::visual::metadataTypeSize));
    performanceGuidanceLabel.setColour(juce::Label::textColourId, authoring::visual::textMuted);
    performanceGuidanceLabel.setJustificationType(juce::Justification::centredRight);
    performanceGuidanceLabel.setTitle("Performance status guidance");

    macroStripLabel.setFont(juce::FontOptions(authoring::visual::sectionTypeSize,
                                               juce::Font::bold));
    macroStripLabel.setComponentID("performanceMacroStripLabel");
    macroStripLabel.setColour(juce::Label::textColourId, authoring::visual::text);
    macroStripToggleButton.setComponentID("performanceMacroStripToggleButton");
    macroStripToggleButton.setWantsKeyboardFocus(true);
    macroStripToggleButton.setExplicitFocusOrder(20);
    macroStripToggleButton.onClick = [this]
    {
        userInstrumentControlsExpandedChoice = instrumentControlsCollapsed;
        if (onInstrumentControlsExpandedChanged)
            onInstrumentControlsExpandedChanged(*userInstrumentControlsExpandedChoice);
        setInstrumentControlsCollapsed(!instrumentControlsCollapsed);
    };

    detailsToggleButton.setComponentID("performanceDetailsToggleButton");
    detailsToggleButton.setButtonText("Details");
    detailsToggleButton.setTitle("Show Performance Details");
    detailsToggleButton.setDescription(
        "Show detailed publication, macro, and runtime diagnostics.");
    detailsToggleButton.setTooltip(detailsToggleButton.getDescription());
    detailsToggleButton.setWantsKeyboardFocus(true);
    detailsToggleButton.setExplicitFocusOrder(10);
    detailsToggleButton.onClick = [this]
    {
        setDiagnosticsVisible(!diagnosticsVisible);
    };

    mixerEmptyStateLabel.setFont(juce::FontOptions(authoring::visual::bodyTypeSize));
    mixerEmptyStateLabel.setComponentID("performanceMixerEmptyStateLabel");
    mixerEmptyStateLabel.setJustificationType(juce::Justification::centredLeft);
    mixerEmptyStateLabel.setColour(juce::Label::textColourId, authoring::visual::textMuted);
    loadIndicatorLabel.setFont(juce::FontOptions(authoring::visual::compactTypeSize,
                                                  juce::Font::bold));
    loadIndicatorLabel.setComponentID("performanceLoadIndicatorLabel");
    loadIndicatorLabel.setJustificationType(juce::Justification::centred);
    artworkPanel.setComponentID("performanceArtworkPanel");

    keyboardComponent.setComponentID("performanceKeyboard");
    keyboardComponent.setKeyWidth(34.0f);
    keyboardComponent.setAvailableRange(36, 96);
    keyboardComponent.setLowestVisibleKey(48);
    keyboardComponent.setWantsKeyboardFocus(true);
    keyboardComponent.setExplicitFocusOrder(500);
    keyboardComponent.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,
                                authoring::visual::surface);
    keyboardComponent.setColour(juce::MidiKeyboardComponent::blackNoteColourId,
                                authoring::visual::text);
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId,
                                authoring::visual::borderStrong);
    keyboardComponent.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                                authoring::visual::information.withAlpha(0.18f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                                authoring::visual::information.withAlpha(0.42f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::textLabelColourId,
                                authoring::visual::textMuted);
    keyboardState.addListener(this);

    diagnosticsPanel.setComponentID("performanceDiagnosticsPanel");
    diagnosticsViewport.setComponentID("performanceDiagnosticsViewport");
    diagnosticsViewport.setViewedComponent(&diagnosticsPanel, false);
    diagnosticsViewport.setScrollBarsShown(true, false);
    diagnosticsViewport.setScrollBarThickness(12);
    diagnosticsViewport.setVisible(false);

    addAndMakeVisible(instrumentNameLabel);
    addAndMakeVisible(instrumentContextLabel);
    addAndMakeVisible(performanceGuidanceLabel);
    addAndMakeVisible(artworkPanel);
    addAndMakeVisible(macroStripLabel);
    addAndMakeVisible(macroStripToggleButton);
    addAndMakeVisible(detailsToggleButton);
    addAndMakeVisible(mixerEmptyStateLabel);
    addAndMakeVisible(loadIndicatorLabel);
    addAndMakeVisible(keyboardComponent);
    addChildComponent(publishedMixer);
    addChildComponent(diagnosticsViewport);

    rebuildMacroControls(engineFacade.getMacroDescriptors(), false);
    refreshSurface();
    startTimerHz(2);
}

PerformancePanel::~PerformancePanel()
{
    setLookAndFeel(nullptr);
    keyboardState.removeListener(this);
}

void PerformancePanel::paint(juce::Graphics& g)
{
    g.fillAll(authoring::visual::shell);
    drawPerformSurface(g, layoutSnapshot.headerBounds, authoring::visual::surfaceRaised);
    drawPerformSurface(g, layoutSnapshot.controlsBounds, authoring::visual::surface);
    drawPerformSurface(g, layoutSnapshot.keyboardBounds, authoring::visual::surface);
    drawPerformSurface(g, layoutSnapshot.diagnosticsBounds, authoring::visual::surfaceSubtle);
}

void PerformancePanel::resized()
{
    layoutSnapshot = {};
    layoutSnapshot.compact = getWidth() < 820;
    layoutSnapshot.shortHeight = getHeight() < 680;
    layoutSnapshot.diagnosticsVisible = diagnosticsVisible;

    auto area = getLocalBounds().reduced(performOuterMargin);
    if (area.isEmpty())
        return;

    const auto headerHeight = layoutSnapshot.compact ? 64 : 68;
    layoutSnapshot.headerBounds = area.removeFromTop(std::min(headerHeight, area.getHeight()));
    area.removeFromTop(std::min(performGap, area.getHeight()));

    auto header = layoutSnapshot.headerBounds.reduced(performPanelInset, 6);
    const auto rightWidth = std::clamp(header.getWidth() / 3, 230, 390);
    auto headerRight = header.removeFromRight(std::min(rightWidth, header.getWidth()));
    auto statusRow = headerRight.removeFromTop(authoring::visual::controlHeight);
    detailsToggleButton.setBounds(statusRow.removeFromRight(88));
    statusRow.removeFromRight(6);
    loadIndicatorLabel.setBounds(statusRow);
    performanceGuidanceLabel.setBounds(headerRight);
    instrumentNameLabel.setBounds(header.removeFromTop(30));
    instrumentContextLabel.setBounds(header);

    const auto keyboardHeight = layoutSnapshot.shortHeight
        ? 84 : std::clamp(area.getHeight() / 6, 96, 120);
    layoutSnapshot.keyboardBounds = area.removeFromBottom(
        std::min(keyboardHeight, area.getHeight()));
    keyboardComponent.setBounds(layoutSnapshot.keyboardBounds.reduced(1));
    area.removeFromBottom(std::min(performGap, area.getHeight()));

    if (diagnosticsVisible && area.getHeight() > 150)
    {
        const auto diagnosticsHeight = std::clamp(area.getHeight() / 3, 128, 220);
        layoutSnapshot.diagnosticsBounds = area.removeFromBottom(diagnosticsHeight);
        diagnosticsViewport.setBounds(layoutSnapshot.diagnosticsBounds.reduced(1));
        const auto diagnosticsContentWidth = std::max(
            620, diagnosticsViewport.getWidth() - diagnosticsViewport.getScrollBarThickness());
        diagnosticsPanel.setSize(diagnosticsContentWidth, 920);
        area.removeFromBottom(std::min(performGap, area.getHeight()));
    }
    else
    {
        diagnosticsViewport.setBounds({});
    }

    const auto wideSplit = !layoutSnapshot.compact
        && !layoutSnapshot.shortHeight
        && area.getWidth() >= 880
        && !instrumentControlsCollapsed;
    layoutSnapshot.controlsBesideArtwork = wideSplit;
    if (wideSplit)
    {
        const auto controlsWidth = std::clamp(
            static_cast<int>(std::round(area.getWidth() * 0.58)), 520,
            std::max(520, area.getWidth() - 260));
        layoutSnapshot.controlsBounds = area.removeFromLeft(controlsWidth);
        area.removeFromLeft(std::min(performGap, area.getWidth()));
        layoutSnapshot.artworkBounds = area;
    }
    else
    {
        const auto macroRowsHeight = static_cast<int>(macroControls.size()) * 34;
        auto controlsHeight = 38;
        if (!instrumentControlsCollapsed)
        {
            const auto preferred = showingPublishedMixer
                ? (publishedMixer.getControlCount() == 0 ? 92 : 250)
                : 44 + macroRowsHeight;
            controlsHeight = std::clamp(preferred, 112,
                                        std::max(112, area.getHeight() * 3 / 5));
        }
        layoutSnapshot.controlsBounds = area.removeFromTop(
            std::min(controlsHeight, area.getHeight()));
        area.removeFromTop(std::min(performGap, area.getHeight()));
        layoutSnapshot.artworkBounds = area;
    }

    artworkPanel.setBounds(layoutSnapshot.artworkBounds);

    auto controlArea = layoutSnapshot.controlsBounds.reduced(performPanelInset, 7);
    auto controlHeader = controlArea.removeFromTop(authoring::visual::controlHeight);
    macroStripToggleButton.setBounds(controlHeader.removeFromRight(112));
    controlHeader.removeFromRight(8);
    macroStripLabel.setBounds(controlHeader);
    controlArea.removeFromTop(std::min(7, controlArea.getHeight()));

    if (instrumentControlsCollapsed)
    {
        publishedMixer.setBounds({});
        mixerEmptyStateLabel.setBounds({});
        for (auto& control : macroControls)
        {
            control->nameLabel.setBounds({});
            control->slider.setBounds({});
            control->valueLabel.setBounds({});
        }
    }
    else if (showingPublishedMixer)
    {
        if (publishedMixer.getControlCount() == 0)
        {
            publishedMixer.setBounds({});
            mixerEmptyStateLabel.setBounds(controlArea.removeFromTop(56));
        }
        else
        {
            mixerEmptyStateLabel.setBounds({});
            publishedMixer.setBounds(controlArea);
        }
    }
    else
    {
        publishedMixer.setBounds({});
        mixerEmptyStateLabel.setBounds({});
        for (auto& control : macroControls)
        {
            auto macroRow = controlArea.removeFromTop(
                std::min(authoring::visual::controlHeight, controlArea.getHeight()));
            const auto nameWidth = std::clamp(macroRow.getWidth() / 4, 92, 160);
            const auto valueWidth = std::clamp(macroRow.getWidth() / 4, 100, 190);
            control->nameLabel.setBounds(macroRow.removeFromLeft(nameWidth));
            control->valueLabel.setBounds(macroRow.removeFromRight(valueWidth));
            macroRow.reduce(8, 0);
            control->slider.setBounds(macroRow);
            controlArea.removeFromTop(6);
        }
    }
}

void PerformancePanel::refreshNow()
{
    const auto structuralRefresh = initialRevisionCheckPending
        || lastObservedPublishLifecycleRevision
            != engineFacade.getPerformancePublishLifecycleRevision()
        || lastObservedMacroTopologyRevision
            != engineFacade.getPerformanceMacroTopologyRevision();
    if (structuralRefresh)
    {
        refreshSurface();
        initialRevisionCheckPending = false;
    }
    else if (lastObservedMacroValueRevision
             != engineFacade.getPerformanceMacroValueRevision())
    {
        refreshMacroValues();
    }
    if (structuralRefresh || diagnosticsVisible)
        diagnosticsPanel.refreshNow();
}

void PerformancePanel::refreshArtworkNow()
{
    loadedArtworkSourceKey.clear();
    refreshArtwork();
}

void PerformancePanel::timerCallback()
{
    refreshNow();
}

void PerformancePanel::handleNoteOn(juce::MidiKeyboardState*, int, int midiNoteNumber, float velocity)
{
    const ScopedMessageThreadSpan timing(MessageThreadSpanKind::performanceKeyboardCallback);
    const auto clampedVelocity = std::clamp(static_cast<int>(std::round(velocity * 127.0f)), 1, 127);

    if (onPerformanceNoteOn)
        onPerformanceNoteOn(midiNoteNumber,
                            static_cast<float>(clampedVelocity) / 127.0f);
}

void PerformancePanel::handleNoteOff(juce::MidiKeyboardState*, int, int midiNoteNumber, float)
{
    const ScopedMessageThreadSpan timing(MessageThreadSpanKind::performanceKeyboardCallback);
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

    if (mixerControl)
    {
        visibleMacroIds.reserve(macros.size());
        for (const auto& macro : macros)
            visibleMacroIds.push_back(macro.id);
        publishedMixer.setControls(buildPublishedMixerControls(macros));
        updateInstrumentControlsVisibility();
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
                                  authoring::visual::information.withAlpha(
                                      mixerControl ? 0.72f : 0.58f));
        control->slider.setColour(juce::Slider::thumbColourId,
                                  authoring::visual::information);
        control->slider.setWantsKeyboardFocus(true);
        control->slider.setExplicitFocusOrder(
            100 + static_cast<int>(macroControls.size()));
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
    updateInstrumentControlsVisibility();
}

void PerformancePanel::setInstrumentControlsCollapsed(const bool shouldCollapse)
{
    if (instrumentControlsCollapsed == shouldCollapse)
        return;

    auto* focusedComponent = juce::Component::getCurrentlyFocusedComponent();
    const auto focusedPublishedControl = focusedComponent != nullptr
        && (focusedComponent == &publishedMixer || publishedMixer.isParentOf(focusedComponent));
    const auto focusedLegacyControl = focusedComponent != nullptr
        && std::any_of(macroControls.begin(), macroControls.end(), [&](const auto& control)
        {
            return focusedComponent == &control->slider
                || focusedComponent == &control->nameLabel
                || focusedComponent == &control->valueLabel;
        });

    instrumentControlsCollapsed = shouldCollapse;
    updateInstrumentControlsVisibility();
    resized();
    repaint();

    if (shouldCollapse && (focusedPublishedControl || focusedLegacyControl))
        macroStripToggleButton.grabKeyboardFocus();
}

void PerformancePanel::setDiagnosticsVisible(const bool shouldShow)
{
    if (diagnosticsVisible == shouldShow)
        return;

    auto* focusedComponent = juce::Component::getCurrentlyFocusedComponent();
    const auto focusedInDiagnostics = focusedComponent != nullptr
        && (focusedComponent == &diagnosticsPanel
            || diagnosticsPanel.isParentOf(focusedComponent));

    diagnosticsVisible = shouldShow;
    diagnosticsViewport.setVisible(diagnosticsVisible);
    detailsToggleButton.setButtonText(diagnosticsVisible ? "Hide Details" : "Details");
    detailsToggleButton.setTitle(diagnosticsVisible
        ? "Hide Performance Details" : "Show Performance Details");
    const auto description = diagnosticsVisible
        ? juce::String("Detailed publication, macro, and runtime diagnostics are visible. Press to hide them.")
        : juce::String("Show detailed publication, macro, and runtime diagnostics.");
    detailsToggleButton.setDescription(description);
    detailsToggleButton.setTooltip(description);

    if (diagnosticsVisible)
        diagnosticsPanel.refreshNow();
    resized();
    repaint();

    if (!diagnosticsVisible && focusedInDiagnostics)
        detailsToggleButton.grabKeyboardFocus();
}

void PerformancePanel::updateInstrumentControlsVisibility()
{
    const auto expanded = !instrumentControlsCollapsed;
    macroStripToggleButton.setButtonText(expanded ? "Hide Controls" : "Show Controls");
    macroStripToggleButton.setTitle(expanded
        ? "Collapse Instrument Controls" : "Expand Instrument Controls");
    const auto description = expanded
        ? juce::String("Instrument Controls are expanded. Press to collapse the panel.")
        : juce::String("Instrument Controls are collapsed. Press to expand the panel.");
    macroStripToggleButton.setTooltip(description);
    macroStripToggleButton.setDescription(description);

    publishedMixer.setVisible(expanded && showingPublishedMixer
                              && publishedMixer.getControlCount() > 0);
    mixerEmptyStateLabel.setVisible(expanded && showingPublishedMixer
                                    && publishedMixer.getControlCount() == 0);
    for (auto& control : macroControls)
    {
        const auto visible = expanded && !showingPublishedMixer;
        control->nameLabel.setVisible(visible);
        control->slider.setVisible(visible);
        control->valueLabel.setVisible(visible);
    }
}

void PerformancePanel::refreshArtwork()
{
    const auto artworkSourceKey = !performanceSnapshot.backgroundArtworkSourceKey.empty()
        ? performanceSnapshot.backgroundArtworkSourceKey
        : performanceSnapshot.contentRootPath;
    if (loadedArtworkSourceKey == artworkSourceKey)
        return;

    loadedArtworkSourceKey = artworkSourceKey;
    auto artwork = juce::Image();
    auto description = juce::String("Performance artwork unavailable. Using fallback background.");

    if (performanceSnapshot.backgroundArtworkJpgBytes != nullptr
        && !performanceSnapshot.backgroundArtworkJpgBytes->empty())
    {
        juce::MemoryInputStream input(performanceSnapshot.backgroundArtworkJpgBytes->data(),
                                      performanceSnapshot.backgroundArtworkJpgBytes->size(),
                                      false);
        juce::JPEGImageFormat jpegFormat;
        artwork = jpegFormat.decodeImage(input);
        if (artwork.isValid())
        {
            description = "Performance artwork loaded from playable package payload "
                + juce::String::fromUTF8(performanceSnapshot.backgroundArtworkSourceKey.c_str());
        }
    }

    if (!artwork.isValid() && !performanceSnapshot.contentRootPath.empty())
    {
        const auto artworkFile = juce::File(
            juce::String::fromUTF8(performanceSnapshot.contentRootPath.c_str()))
            .getChildFile("Images")
            .getChildFile("background.jpg");
        if (artworkFile.existsAsFile())
        {
            artwork = juce::ImageFileFormat::loadFrom(artworkFile);
            if (artwork.isValid())
            {
                description = "Performance artwork loaded from "
                    + artworkFile.getFullPathName();
            }
        }
    }

    artworkPanel.setArtwork(std::move(artwork), description);
}

void PerformancePanel::refreshSurface()
{
    const ScopedMessageThreadSpan timing(MessageThreadSpanKind::performanceRefresh);
    performanceSnapshot = engineFacade.getPerformanceSnapshot();
    lastObservedPublishLifecycleRevision
        = engineFacade.getPerformancePublishLifecycleRevision();
    lastObservedMacroTopologyRevision
        = engineFacade.getPerformanceMacroTopologyRevision();
    lastObservedMacroValueRevision
        = engineFacade.getPerformanceMacroValueRevision();
    const auto allMacros = engineFacade.getMacroDescriptors();
    const auto macroSurface = buildPerformanceMacroSurfaceModel(allMacros);
    const auto controlsWereCollapsed = instrumentControlsCollapsed;
    if (!userInstrumentControlsExpandedChoice.has_value())
    {
        const auto packageHasExposedControls
            = engineFacade.getPerformancePackageActivationPayload() != nullptr
                && macroSurface.showingPublishedMixer
                && !macroSurface.displayedMacros.empty();
        instrumentControlsCollapsed = !packageHasExposedControls;
    }
    const auto publishPresentation = publishPresentationProvider
        ? publishPresentationProvider()
        : engineFacade.getPerformancePublishPresentationSnapshot();
    audioCallbackActive = !audioCallbackActiveProvider
        || audioCallbackActiveProvider();
    refreshArtwork();

    hiddenPublishedMacroCount = macroSurface.hiddenPublishedMacroCount;
    const auto macroLayoutChanged = !sameMacroLayout(
        visibleMacroIds, macroSurface.displayedMacros,
        showingPublishedMixer, macroSurface.showingPublishedMixer);
    if (macroLayoutChanged)
    {
        rebuildMacroControls(macroSurface.displayedMacros, macroSurface.showingPublishedMixer);
        resized();
    }
    else if (controlsWereCollapsed != instrumentControlsCollapsed)
    {
        resized();
    }

    juce::String loadIndicatorText = juce::String::fromUTF8(performanceSnapshot.loadIndicator.c_str());
    auto publishFailed = false;
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
    }
    auto guidanceText = publishedPerformanceGuidance;
    auto statusColour = authoring::visual::information;
    if (!audioCallbackActive)
    {
        loadIndicatorText = "Audio Inactive";
        guidanceText = "Open audio settings or enable host processing to audition the instrument.";
        statusColour = authoring::visual::warning;
    }
    else if (publishFailed || !performanceSnapshot.loaded)
    {
        statusColour = authoring::visual::error;
    }
    else if (!performanceSnapshot.previewPlayback.errorMessage.empty())
    {
        statusColour = authoring::visual::warning;
    }
    else if (hasActivePublishedPerformance || performanceSnapshot.loaded)
    {
        statusColour = authoring::visual::success;
    }

    const auto instrumentName = !performanceSnapshot.instrumentDisplayName.empty()
        ? juce::String::fromUTF8(performanceSnapshot.instrumentDisplayName.c_str())
        : (performanceSnapshot.loaded ? juce::String("Untitled Instrument")
                                      : juce::String("No Instrument Loaded"));
    const auto instrumentContext = buildInstrumentContext(performanceSnapshot);
    instrumentNameLabel.setText(instrumentName, juce::dontSendNotification);
    instrumentNameLabel.setDescription("Loaded performance instrument: " + instrumentName + ".");
    instrumentNameLabel.setTooltip(instrumentName);
    instrumentContextLabel.setText(instrumentContext, juce::dontSendNotification);
    instrumentContextLabel.setDescription(instrumentContext);
    instrumentContextLabel.setTooltip(instrumentContext);

    if (guidanceText.isEmpty())
        guidanceText = juce::String::fromUTF8(performanceSnapshot.loadIndicator.c_str());
    performanceGuidanceLabel.setText(guidanceText, juce::dontSendNotification);
    performanceGuidanceLabel.setDescription(guidanceText);
    performanceGuidanceLabel.setTooltip(guidanceText);

    loadIndicatorLabel.setText(loadIndicatorText, juce::dontSendNotification);
    auto statusDescription = loadIndicatorText;
    if (guidanceText.isNotEmpty())
        statusDescription << ": " << guidanceText;
    if (publishedPerformanceFindingCode.isNotEmpty() && publishFailed)
        statusDescription << " [" << publishedPerformanceFindingCode << "]";
    loadIndicatorLabel.setTooltip(statusDescription);
    loadIndicatorLabel.setDescription(statusDescription);
    loadIndicatorLabel.setColour(juce::Label::backgroundColourId,
                                 statusColour.withAlpha(0.13f));
    loadIndicatorLabel.setColour(juce::Label::textColourId, statusColour);
    loadIndicatorLabel.setColour(juce::Label::outlineColourId,
                                 statusColour.withAlpha(0.72f));

    const auto macroStripDescription = buildMacroStripDescription(macroSurface);
    macroStripLabel.setText(buildMacroStripTitle(macroSurface), juce::dontSendNotification);
    macroStripLabel.setTooltip(macroStripDescription);
    macroStripLabel.setDescription(macroStripDescription);
    updateInstrumentControlsVisibility();
    const auto mixerEmptyText = buildMixerEmptyStateText(macroSurface.hiddenPublishedMacroCount);
    mixerEmptyStateLabel.setText(mixerEmptyText, juce::dontSendNotification);
    mixerEmptyStateLabel.setTooltip(mixerEmptyText);
    mixerEmptyStateLabel.setDescription(mixerEmptyText);
    syncKeyboardPlayableRange();

    updateMacroValues(macroSurface.displayedMacros);

    diagnosticsPanel.repaint();
}

void PerformancePanel::refreshMacroValues()
{
    const auto macroSurface = buildPerformanceMacroSurfaceModel(
        engineFacade.getMacroDescriptors());
    if (!sameMacroLayout(visibleMacroIds, macroSurface.displayedMacros,
                         showingPublishedMixer,
                         macroSurface.showingPublishedMixer))
    {
        refreshSurface();
        return;
    }

    updateMacroValues(macroSurface.displayedMacros);
    lastObservedMacroValueRevision
        = engineFacade.getPerformanceMacroValueRevision();
}

void PerformancePanel::updateMacroValues(
    const std::vector<drs::engine::EngineMacroDescriptor>& macros)
{
    if (showingPublishedMixer)
    {
        publishedMixer.updateControlValues(buildPublishedMixerControls(macros));
        return;
    }

    for (auto& control : macroControls)
    {
        const auto found = std::find_if(macros.begin(), macros.end(), [&](const auto& macro)
        {
            return macro.id == control->id;
        });
        if (found == macros.end())
            continue;

        const auto& macro = *found;
        const auto currentEffect = juce::String::fromUTF8(macro.currentEffect.c_str());
        const auto currentValueText = juce::String(macro.currentValue, 3);
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

    keyboardComponent.setEnabled(hasActivePublishedPerformance && audioCallbackActive);

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
    keyboardComponent.setTitle("Performance keyboard");
    keyboardComponent.setDescription(keyboardHint);
}
} // namespace drs::app
