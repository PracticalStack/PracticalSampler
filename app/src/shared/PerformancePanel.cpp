#include "shared/PerformancePanel.h"
#include "shared/MessageThreadMetrics.h"

#include <algorithm>
#include <cmath>

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
    g.setColour(juce::Colour::fromRGB(46, 48, 51));
    g.fillRoundedRectangle(bounds, 18.0f);

    if (artwork.isValid())
    {
        g.reduceClipRegion(getLocalBounds());
        g.drawImageWithin(artwork,
                          getLocalBounds().getX(),
                          getLocalBounds().getY(),
                          getLocalBounds().getWidth(),
                          getLocalBounds().getHeight(),
                          juce::RectanglePlacement::fillDestination);
    }

    g.setColour(performancePanelAccent.withAlpha(0.22f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 18.0f, 1.5f);
}

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
    macroStripLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    macroStripLabel.setComponentID("performanceMacroStripLabel");
    macroStripToggleButton.setComponentID("performanceMacroStripToggleButton");
    macroStripToggleButton.setWantsKeyboardFocus(true);
    macroStripToggleButton.onClick = [this]
    {
        setInstrumentControlsCollapsed(!instrumentControlsCollapsed);
    };
    mixerEmptyStateLabel.setFont(juce::FontOptions(15.0f));
    mixerEmptyStateLabel.setComponentID("performanceMixerEmptyStateLabel");
    mixerEmptyStateLabel.setJustificationType(juce::Justification::centredLeft);
    loadIndicatorLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    loadIndicatorLabel.setComponentID("performanceLoadIndicatorLabel");
    artworkPanel.setComponentID("performanceArtworkPanel");

    macroStripLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(14, 20, 27));
    mixerEmptyStateLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(52, 64, 84));
    loadIndicatorLabel.setJustificationType(juce::Justification::centredRight);

    keyboardComponent.setComponentID("performanceKeyboard");
    keyboardComponent.setKeyWidth(34.0f);
    keyboardComponent.setAvailableRange(36, 96);
    keyboardComponent.setLowestVisibleKey(48);
    keyboardComponent.setWantsKeyboardFocus(true);
    keyboardState.addListener(this);

    diagnosticsPanel.setComponentID("performanceDiagnosticsPanel");
    diagnosticsPanel.setVisible(false);

    addAndMakeVisible(artworkPanel);
    addAndMakeVisible(macroStripLabel);
    addAndMakeVisible(macroStripToggleButton);
    addAndMakeVisible(mixerEmptyStateLabel);
    addAndMakeVisible(loadIndicatorLabel);
    addAndMakeVisible(keyboardComponent);
    addChildComponent(publishedMixer);
    addChildComponent(diagnosticsPanel);

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
}

void PerformancePanel::resized()
{
    auto area = getLocalBounds().reduced(30);

    auto statusRow = area.removeFromTop(28);
    loadIndicatorLabel.setBounds(statusRow.removeFromRight(260));

    area.removeFromTop(14);

    const auto keyboardHeight = std::clamp(area.getHeight() / 7, 96, 124);
    auto keyboardArea = area.removeFromBottom(keyboardHeight);
    keyboardComponent.setBounds(keyboardArea);

    area.removeFromBottom(16);

    const auto macroRowsHeight = static_cast<int>(macroControls.size()) * 34;
    int controlSectionHeight = 32;

    if (!instrumentControlsCollapsed && showingPublishedMixer)
    {
        controlSectionHeight += publishedMixer.getControlCount() == 0
            ? 64
            : std::min(420, std::max(220, area.getHeight() / 3));
    }
    else if (!instrumentControlsCollapsed && !macroControls.empty())
    {
        controlSectionHeight += macroRowsHeight + 8;
    }

    auto controlArea = area.removeFromBottom(std::min(controlSectionHeight, area.getHeight()));
    auto controlHeader = controlArea.removeFromTop(24);
    macroStripToggleButton.setBounds(controlHeader.removeFromRight(112));
    controlHeader.removeFromRight(8);
    macroStripLabel.setBounds(controlHeader);
    controlArea.removeFromTop(8);

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
            auto macroRow = controlArea.removeFromTop(28);
            control->nameLabel.setBounds(macroRow.removeFromLeft(110));
            control->slider.setBounds(macroRow.removeFromLeft(220));
            macroRow.removeFromLeft(10);
            control->valueLabel.setBounds(macroRow.removeFromLeft(190));
            controlArea.removeFromTop(6);
        }
    }

    if (diagnosticsPanel.isVisible())
    {
        area.removeFromBottom(14);
        auto diagnosticsArea = area.removeFromBottom(std::min(220, std::max(120, area.getHeight() / 3)));
        diagnosticsPanel.setBounds(diagnosticsArea);
        area.removeFromBottom(14);
    }
    else
    {
        diagnosticsPanel.setBounds({});
    }

    area.removeFromBottom(18);
    artworkPanel.setBounds(area);
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
    if (structuralRefresh || diagnosticsPanel.isVisible())
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
    const auto publishPresentation = publishPresentationProvider
        ? publishPresentationProvider()
        : engineFacade.getPerformancePublishPresentationSnapshot();
    refreshArtwork();

    hiddenPublishedMacroCount = macroSurface.hiddenPublishedMacroCount;
    if (!sameMacroLayout(visibleMacroIds,
                         macroSurface.displayedMacros,
                         showingPublishedMixer,
                         macroSurface.showingPublishedMixer))
    {
        rebuildMacroControls(macroSurface.displayedMacros, macroSurface.showingPublishedMixer);
        resized();
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
    keyboardComponent.setTitle("Performance keyboard");
    keyboardComponent.setDescription(keyboardHint);
}
} // namespace drs::app
