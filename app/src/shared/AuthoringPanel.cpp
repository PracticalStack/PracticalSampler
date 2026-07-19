#include "shared/AuthoringPanel.h"

#include "shared/authoring/AuthoringWorkspaceLayout.h"

#include <algorithm>
#include <array>

namespace drs::app
{
namespace
{
constexpr int statusTimerId = 1;
constexpr int previewReleaseTimerId = 2;
const auto authoringPanelBackground = juce::Colour::fromRGB(18, 24, 29);
const auto authoringPanelCard = juce::Colour::fromRGB(250, 247, 240);
const auto authoringPanelAccent = juce::Colour::fromRGB(181, 96, 21);
const auto authoringPanelMuted = juce::Colour::fromRGB(82, 86, 94);
const auto authoringControlSurface = juce::Colour::fromRGB(251, 248, 242);
const auto authoringControlSurfaceHover = juce::Colour::fromRGB(244, 239, 231);
const auto authoringControlOutline = juce::Colour::fromRGB(176, 160, 141);
const auto authoringFocusRing = juce::Colour::fromRGB(24, 29, 33);
const auto authoringFocusHalo = juce::Colour::fromRGBA(255, 255, 255, 232);
const auto authoringButtonFill = juce::Colour::fromRGB(122, 64, 18);
const auto authoringButtonFillPressed = juce::Colour::fromRGB(102, 52, 14);
const auto authoringButtonText = juce::Colours::white;
const auto authoringToggleTick = juce::Colour::fromRGB(28, 108, 88);

struct CuratedMacroAssignment
{
    const char* parameterId;
    const char* parameterPath;
    const char* defaultRole;
    const char* label;
};

constexpr std::array<CuratedMacroAssignment, 4> curatedMacroAssignments
{
    CuratedMacroAssignment{"filter-cutoff", "engine.filter.main.cutoff", "timbre", "Filter cutoff"},
    CuratedMacroAssignment{"voice-pitch", "engine.pitch.main.semitones", "motion", "Voice pitch"},
    CuratedMacroAssignment{"zone-gain", "authoring.zone.gainDb", "mix", "Zone gain"},
    CuratedMacroAssignment{"zone-pan", "authoring.zone.pan", "placement", "Zone pan"}
};

constexpr std::array<const char*, 5> curatedMacroRoles
{
    "timbre",
    "motion",
    "mix",
    "space",
    "placement"
};

constexpr std::array<const char*, 5> curatedFxTypes
{
    "eq",
    "delay",
    "reverb",
    "chorus",
    "saturator"
};

constexpr std::array<const char*, 3> curatedTriggerEvents
{
    "phrase-trigger",
    "key-switch",
    "phrase-latch"
};

constexpr std::array<const char*, 3> curatedChordModes
{
    "off",
    "follow-root",
    "preserve-intervals"
};

void configureEditorSlider(juce::Slider& slider,
                           double minValue,
                           double maxValue,
                           double interval)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 24);
    slider.setRange(minValue, maxValue, interval);
}

void configureSectionLabel(juce::Label& label, const char* text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    label.setFont(juce::FontOptions(16.0f, juce::Font::bold));
}

void configureFieldLabel(juce::Label& label, const char* text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
}

void configureMetadataLabel(juce::Label& label)
{
    label.setColour(juce::Label::textColourId, authoringPanelMuted);
    label.setFont(juce::FontOptions(13.0f));
    label.setJustificationType(juce::Justification::centredLeft);
}

void drawAuthoringFocusRing(juce::Graphics& g,
                            juce::Rectangle<float> bounds,
                            float cornerSize,
                            const juce::Colour& outlineColour)
{
    g.setColour(authoringFocusHalo);
    g.drawRoundedRectangle(bounds.expanded(1.0f), cornerSize + 1.0f, 3.0f);
    g.setColour(outlineColour);
    g.drawRoundedRectangle(bounds, cornerSize, 1.8f);
}

juce::String formatZoneRange(const drs::engine::AuthoringZoneSummary& zone)
{
    return "Keys " + juce::String(zone.keyLow) + "-" + juce::String(zone.keyHigh)
        + " | Vel " + juce::String(zone.velocityLow) + "-" + juce::String(zone.velocityHigh);
}

juce::String formatAuthoringPreviewStatus(const drs::app::AuthoringPreviewStatusSnapshot& status)
{
    if (!status.available)
        return "Preview status unavailable";

    auto text = "Preview " + juce::String::fromUTF8(status.revisionState.empty() ? "Unknown"
                                                                                 : status.revisionState.c_str())
        + " | draft r" + juce::String(static_cast<int>(status.draftRevision));

    if (status.activeRevision > 0)
        text += " | active r" + juce::String(static_cast<int>(status.activeRevision));

    if (status.usingLastKnownGood)
        text += " | auditioning last good r" + juce::String(static_cast<int>(status.audibleRevision));

    if (status.failedRevision > 0 && status.failedRevision != status.activeRevision)
        text += " | failed r" + juce::String(static_cast<int>(status.failedRevision));

    if (status.pendingRevision > 0 && status.pendingRevision != status.activeRevision)
        text += " | pending r" + juce::String(static_cast<int>(status.pendingRevision));

    if (!status.blockingPrerequisite.empty())
        text += " | Fix: " + juce::String::fromUTF8(status.blockingPrerequisite.c_str());

    if (!status.failureState.empty())
        text += " | " + juce::String::fromUTF8(status.failureState.c_str());

    return text;
}

juce::String formatMicros(std::uint64_t micros)
{
    if (micros >= 1000)
        return juce::String(static_cast<double>(micros) / 1000.0, 2) + " ms";

    return juce::String(static_cast<int>(micros)) + " us";
}

juce::String joinIdList(const std::vector<std::string>& values)
{
    if (values.empty())
        return "(none)";

    juce::String result;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
            result << " -> ";
        result << juce::String::fromUTF8(values[index].c_str());
    }

    return result;
}

std::vector<std::string> buildArticulationIds(const drs::engine::RuntimeProjectModel& project)
{
    std::vector<std::string> articulationIds;

    for (const auto& zone : project.authoring.zones)
    {
        if (std::find(articulationIds.begin(), articulationIds.end(), zone.articulationId) == articulationIds.end())
            articulationIds.push_back(zone.articulationId);
    }

    return articulationIds;
}

const CuratedMacroAssignment* findCuratedMacroAssignment(const std::string& parameterId)
{
    for (const auto& assignment : curatedMacroAssignments)
    {
        if (parameterId == assignment.parameterId)
            return &assignment;
    }

    return nullptr;
}

int findAssignmentIndex(const std::string& parameterId)
{
    for (std::size_t index = 0; index < curatedMacroAssignments.size(); ++index)
    {
        if (parameterId == curatedMacroAssignments[index].parameterId)
            return static_cast<int>(index);
    }

    return -1;
}

bool isExpandedLayout(AuthoringPanel::LayoutMode layoutMode)
{
    return layoutMode == AuthoringPanel::LayoutMode::expanded;
}

void configureAccessibleMetadata(juce::Component& component,
                                 const juce::String& title,
                                 const juce::String& description,
                                 const juce::String& helpText = {})
{
    component.setTitle(title);
    component.setDescription(description);

    if (helpText.isNotEmpty())
        component.setHelpText(helpText);
}

void updateDynamicAccessibleText(juce::Component& component,
                                 const juce::String& text,
                                 const juce::String& descriptionPrefix)
{
    component.setTitle(text);
    component.setDescription(descriptionPrefix + text);
}

void updateAccessibleDescriptionAndHelpText(juce::Component& component,
                                            const juce::String& description,
                                            const juce::String& helpText)
{
    component.setDescription(description);
    component.setHelpText(helpText);
}

void setVisibleAndAccessible(juce::Component& component, bool shouldShow)
{
    component.setVisible(shouldShow);
    component.setAccessible(shouldShow);
}

bool isComponentFocusedWithin(const juce::Component* focusedComponent, const juce::Component& ancestor)
{
    for (auto* current = focusedComponent; current != nullptr; current = current->getParentComponent())
    {
        if (current == &ancestor)
            return true;
    }

    return false;
}

juce::String buildMacroListStatusText(const drs::engine::RuntimeProjectMacroDefinition& macro)
{
    if (macro.targets.empty())
        return "Unassigned";

    const auto& target = macro.targets.front();
    juce::String status;

    if (!target.role.empty())
        status << juce::String::fromUTF8(target.role.c_str()) << " | ";

    if (const auto* assignment = findCuratedMacroAssignment(target.parameterId))
        status << assignment->label;
    else if (!target.parameterPath.empty())
        status << juce::String::fromUTF8(target.parameterPath.c_str());
    else if (!target.parameterId.empty())
        status << juce::String::fromUTF8(target.parameterId.c_str());
    else
        status << "Unassigned";

    return status;
}

struct DraftPlaybackGuidance
{
    std::string statusText;
    bool canPrepareDraftPlayback = false;
    bool canPublishDraftPlayback = false;
};

bool hasFindingCode(const std::vector<drs::engine::PlaybackSnapshotFinding>& findings,
                    const std::string& code)
{
    return std::any_of(findings.begin(),
                       findings.end(),
                       [&](const drs::engine::PlaybackSnapshotFinding& finding)
                       {
                           return finding.code == code;
                       });
}

DraftPlaybackGuidance buildDraftPlaybackGuidance(const drs::engine::AuthoringSession& authoringSession,
                                                 const drs::engine::DraftPlaybackStatus& playbackStatus)
{
    DraftPlaybackGuidance guidance;
    const auto hasZones = !authoringSession.getProject().authoring.zones.empty();
    const auto previewReadyForCurrentDraft = playbackStatus.preview.revision == playbackStatus.draftRevision
        && playbackStatus.preview.state == "Ready";

    guidance.canPrepareDraftPlayback = playbackStatus.projectOpen
        && !playbackStatus.deviceRestartInProgress
        && !playbackStatus.pendingPreview.active
        && hasZones;
    guidance.canPublishDraftPlayback = playbackStatus.projectOpen
        && !playbackStatus.deviceRestartInProgress
        && !playbackStatus.pendingPerformance.active
        && previewReadyForCurrentDraft
        && (playbackStatus.performance.revision != playbackStatus.draftRevision
            || playbackStatus.performance.state != "Active");

    if (!playbackStatus.projectOpen)
    {
        guidance.statusText = "playback blocked: Open a project before preparing draft playback.";
        return guidance;
    }

    if (!hasZones
        || hasFindingCode(playbackStatus.preview.findings, "no-playable-zones")
        || hasFindingCode(playbackStatus.performance.findings, "no-playable-zones"))
    {
        guidance.statusText = "playback blocked: Import a sample and create at least one playable zone.";
        return guidance;
    }

    if (playbackStatus.deviceRestartInProgress)
    {
        guidance.statusText = "playback paused: Wait for the device restart to finish before preparing or publishing.";
        return guidance;
    }

    if (playbackStatus.pendingPreview.active || playbackStatus.pendingPerformance.active)
    {
        guidance.statusText = "playback busy: Wait for the current playback build to finish applying.";
        return guidance;
    }

    if (playbackStatus.preview.revision != playbackStatus.draftRevision
        || playbackStatus.preview.state == "Stale")
    {
        guidance.statusText = "playback action: Prepare the latest draft for preview.";
        return guidance;
    }

    if (previewReadyForCurrentDraft
        && (playbackStatus.performance.revision != playbackStatus.draftRevision
            || playbackStatus.performance.state != "Active"))
    {
        guidance.statusText = "playback action: Publish the ready draft to the performance path.";
        return guidance;
    }

    if (playbackStatus.performance.revision == playbackStatus.draftRevision
        && playbackStatus.performance.state == "Active")
    {
        guidance.statusText = "playback ready: The latest draft is active on the performance path.";
    }

    return guidance;
}

} // namespace

AuthoringPanel::AuthoringControlLookAndFeel::AuthoringControlLookAndFeel()
{
    setColour(juce::TextButton::buttonColourId, authoringButtonFill);
    setColour(juce::TextButton::buttonOnColourId, authoringButtonFillPressed);
    setColour(juce::TextButton::textColourOffId, authoringButtonText);
    setColour(juce::TextButton::textColourOnId, authoringButtonText);

    setColour(juce::ToggleButton::textColourId, authoringFocusRing);
    setColour(juce::ToggleButton::tickColourId, authoringToggleTick);
    setColour(juce::ToggleButton::tickDisabledColourId, authoringControlOutline);

    setColour(juce::ComboBox::backgroundColourId, authoringControlSurface);
    setColour(juce::ComboBox::textColourId, authoringFocusRing);
    setColour(juce::ComboBox::arrowColourId, authoringFocusRing.withAlpha(0.82f));
    setColour(juce::ComboBox::outlineColourId, authoringControlOutline);
    setColour(juce::ComboBox::focusedOutlineColourId, authoringFocusRing);

    setColour(juce::TextEditor::backgroundColourId, authoringControlSurface);
    setColour(juce::TextEditor::textColourId, authoringFocusRing);
    setColour(juce::TextEditor::outlineColourId, authoringControlOutline);
    setColour(juce::TextEditor::focusedOutlineColourId, authoringFocusRing);
    setColour(juce::TextEditor::highlightColourId, authoringToggleTick.withAlpha(0.18f));
    setColour(juce::TextEditor::highlightedTextColourId, authoringFocusRing);

    setColour(juce::Label::textColourId, authoringFocusRing);
    setColour(juce::Slider::thumbColourId, authoringButtonFill);
    setColour(juce::Slider::trackColourId, authoringToggleTick.withAlpha(0.76f));
    setColour(juce::Slider::backgroundColourId, authoringControlSurfaceHover);
    setColour(juce::Slider::textBoxTextColourId, authoringFocusRing);
    setColour(juce::Slider::textBoxBackgroundColourId, authoringControlSurface);
    setColour(juce::Slider::textBoxOutlineColourId, authoringControlOutline);

    setColour(juce::ListBox::backgroundColourId, authoringControlSurfaceHover);
    setColour(juce::ListBox::outlineColourId, authoringControlOutline);
}

void AuthoringPanel::AuthoringControlLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                                       juce::Button& button,
                                                                       const juce::Colour& backgroundColour,
                                                                       bool shouldDrawButtonAsHighlighted,
                                                                       bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const auto cornerSize = 7.0f;
    const auto hasFocus = button.hasKeyboardFocus(true);

    if (hasFocus)
    {
        drawAuthoringFocusRing(g, bounds.reduced(1.0f), cornerSize, findColour(juce::TextEditor::focusedOutlineColourId));
        bounds = bounds.reduced(3.0f);
    }

    auto fillColour = backgroundColour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.42f);
    if (shouldDrawButtonAsDown)
        fillColour = fillColour.interpolatedWith(authoringButtonFillPressed, 0.45f);
    else if (shouldDrawButtonAsHighlighted)
        fillColour = fillColour.interpolatedWith(authoringControlSurface, 0.12f);

    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, cornerSize);
    g.setColour(button.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerSize, hasFocus ? 1.2f : 1.0f);
}

void AuthoringPanel::AuthoringControlLookAndFeel::drawToggleButton(juce::Graphics& g,
                                                                   juce::ToggleButton& button,
                                                                   bool shouldDrawButtonAsHighlighted,
                                                                   bool shouldDrawButtonAsDown)
{
    if (button.hasKeyboardFocus(true))
        drawAuthoringFocusRing(g,
                               button.getLocalBounds().toFloat().reduced(1.0f),
                               6.0f,
                               findColour(juce::TextEditor::focusedOutlineColourId));

    juce::LookAndFeel_V4::drawToggleButton(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}

void AuthoringPanel::AuthoringControlLookAndFeel::drawComboBox(juce::Graphics& g,
                                                               int width,
                                                               int height,
                                                               bool,
                                                               int,
                                                               int,
                                                               int,
                                                               int,
                                                               juce::ComboBox& box)
{
    const auto cornerSize = 4.0f;
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    const auto hasFocus = box.hasKeyboardFocus(true);

    if (hasFocus)
    {
        drawAuthoringFocusRing(g, bounds.reduced(1.0f), cornerSize, box.findColour(juce::ComboBox::focusedOutlineColourId));
        bounds = bounds.reduced(3.0f);
    }

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerSize);
    g.setColour(box.findColour(hasFocus ? juce::ComboBox::focusedOutlineColourId
                                        : juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerSize, hasFocus ? 1.6f : 1.0f);

    const auto arrowZone = juce::Rectangle<float>(bounds.getRight() - 24.0f, bounds.getY(), 16.0f, bounds.getHeight());
    juce::Path path;
    path.startNewSubPath(arrowZone.getX() + 1.5f, arrowZone.getCentreY() - 2.0f);
    path.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + 2.5f);
    path.lineTo(arrowZone.getRight() - 1.5f, arrowZone.getCentreY() - 2.0f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.95f : 0.28f));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void AuthoringPanel::AuthoringControlLookAndFeel::drawLinearSliderOutline(juce::Graphics& g,
                                                                          int x,
                                                                          int y,
                                                                          int width,
                                                                          int height,
                                                                          const juce::Slider::SliderStyle style,
                                                                          juce::Slider& slider)
{
    juce::ignoreUnused(x, y, width, height, style);

    if (slider.hasKeyboardFocus(true))
    {
        drawAuthoringFocusRing(g,
                               slider.getLocalBounds().toFloat().reduced(1.0f),
                               6.0f,
                               findColour(juce::TextEditor::focusedOutlineColourId));
    }
    else
    {
        juce::LookAndFeel_V4::drawLinearSliderOutline(g,
                                                      x,
                                                      y,
                                                      width,
                                                      height,
                                                      style,
                                                      slider);
    }
}

AuthoringPanel::AuthoringPanel(drs::engine::AuthoringSession& session,
                               WaveformPreviewProvider previewProvider,
                               AuthoringPreviewStatusProvider nextAuthoringPreviewStatusProvider,
                               ImportResponsivenessProvider responsivenessProvider,
                               LayoutMode nextLayoutMode,
                               NotePreviewStartedCallback notePreviewStarted,
                               NotePreviewEndedCallback notePreviewEnded,
                               RestoreRootKeyCallback restoreRootKeyRequested,
                               DraftPlaybackStatusProvider nextDraftPlaybackStatusProvider,
                               DraftPlaybackActionCallback prepareDraftPlaybackRequested,
                               DraftPlaybackActionCallback publishDraftPlaybackRequested,
                               PreviewCommandCallback nextPreviewCommandCallback)
    : authoringSession(session),
      waveformPreviewProvider(std::move(previewProvider)),
      authoringPreviewStatusProvider(std::move(nextAuthoringPreviewStatusProvider)),
      importResponsivenessProvider(std::move(responsivenessProvider)),
      layoutMode(nextLayoutMode),
      onNotePreviewStarted(std::move(notePreviewStarted)),
      onNotePreviewEnded(std::move(notePreviewEnded)),
      onRestoreRootKeyRequested(std::move(restoreRootKeyRequested)),
      draftPlaybackStatusProvider(std::move(nextDraftPlaybackStatusProvider)),
      onPrepareDraftPlaybackRequested(std::move(prepareDraftPlaybackRequested)),
      onPublishDraftPlaybackRequested(std::move(publishDraftPlaybackRequested)),
      previewCommandCallback(std::move(nextPreviewCommandCallback)),
      macroList("authoringMacroList",
                "authoringMacroListBox",
                "authoringMacroListEmptyState")
{
    setLookAndFeel(&authoringLookAndFeel);
    setComponentID("authoringWorkspace");
    drawerState.open = isExpandedLayout(layoutMode);
    drawerState.activeTab = authoring::DrawerTab::waveform;

    playbackBanner.setComponentID("authoringPlaybackBanner");
    playbackBannerLabel.setComponentID("authoringPlaybackBannerLabel");
    playbackBannerPrepareButton.setComponentID("authoringPlaybackBannerPrepareButton");
    playbackBannerPublishButton.setComponentID("authoringPlaybackBannerPublishButton");
    configureMetadataLabel(waveformScopeLabel);
    configureMetadataLabel(drawerBreadcrumbLabel);
    configureMetadataLabel(waveformStatusLabel);
    configureMetadataLabel(waveformInfoLabel);
    configureMetadataLabel(loopInfoLabel);
    configureMetadataLabel(importMetricsLabel);
    playbackBannerLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    playbackBannerLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    playbackBannerLabel.setJustificationType(juce::Justification::centredLeft);
    macroSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    fxSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    routingSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    performanceSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    phraseSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);

    configureSectionLabel(waveformLabel, "Waveform Detail");
    configureSectionLabel(zoneLabel, "Selected Zone");
    configureSectionLabel(fxSectionLabel, "Selected FX");
    configureSectionLabel(routingSectionLabel, "Selected Bus");

    configureFieldLabel(macroAssignmentLabel, "Parameter");
    configureFieldLabel(macroRoleLabel, "Role");
    configureFieldLabel(macroDefaultLabel, "Default");
    configureFieldLabel(macroMinLabel, "Min");
    configureFieldLabel(macroMaxLabel, "Max");
    configureFieldLabel(fxTypeLabel, "Type");
    configureFieldLabel(routingInputLabel, "Input Source");
    configureFieldLabel(routingInsertOneLabel, "Insert A");
    configureFieldLabel(routingInsertTwoLabel, "Insert B");
    configureFieldLabel(triggerEventLabel, "Trigger");
    configureFieldLabel(targetArticulationLabel, "Articulation");
    configureFieldLabel(phraseAssetLabel, "Phrase");
    configureFieldLabel(chordModeLabel, "Chord Rule");
    configureFieldLabel(phraseImportPathLabel, "MIDI Path");

    configureEditorSlider(macroDefaultSlider, 0.0, 1.0, 0.01);
    configureEditorSlider(macroMinSlider, 0.0, 1.0, 0.01);
    configureEditorSlider(macroMaxSlider, 0.0, 1.0, 0.01);

    zoneSelector.setComponentID("authoringZoneSelector");
    zoneMap.setComponentID("authoringZoneMap");
    drawerRegion.setComponentID("authoringDrawer");
    drawerTabStrip.setComponentID("authoringDrawerTabStrip");
    drawerContentHost.setComponentID("authoringDrawerContentHost");
    drawerToggleButton.setComponentID("authoringDrawerToggleButton");
    drawerWaveformTabButton.setComponentID("authoringDrawerWaveformTab");
    drawerMacrosTabButton.setComponentID("authoringDrawerMacrosTab");
    drawerRoutingTabButton.setComponentID("authoringDrawerRoutingTab");
    drawerPerformanceTabButton.setComponentID("authoringDrawerPerformanceTab");
    waveformLabel.setComponentID("authoringDrawerTitleLabel");
    waveformScopeLabel.setComponentID("authoringDrawerScopeLabel");
    drawerBreadcrumbLabel.setComponentID("authoringDrawerBreadcrumbLabel");
    waveformStatusLabel.setComponentID("authoringWaveformStatusLabel");
    waveformInfoLabel.setComponentID("authoringWaveformInfoLabel");
    loopInfoLabel.setComponentID("authoringWaveformLoopLabel");
    importMetricsLabel.setComponentID("authoringWaveformImportLabel");
    waveformPreview.setComponentID("authoringWaveformPreview");
    macroAssignmentSelector.setComponentID("authoringMacroAssignmentSelector");
    macroRoleSelector.setComponentID("authoringMacroRoleSelector");
    macroDefaultSlider.setComponentID("authoringMacroDefaultSlider");
    macroMinSlider.setComponentID("authoringMacroMinSlider");
    macroMaxSlider.setComponentID("authoringMacroMaxSlider");
    macroMoveUpButton.setComponentID("authoringMacroMoveUpButton");
    macroMoveDownButton.setComponentID("authoringMacroMoveDownButton");
    fxSelector.setComponentID("authoringFxSelector");
    fxTypeSelector.setComponentID("authoringFxTypeSelector");
    fxBypassedToggle.setComponentID("authoringFxBypassedToggle");
    routingBusSelector.setComponentID("authoringRoutingSelector");
    routingInputSelector.setComponentID("authoringRoutingInputSelector");
    routingInsertOneSelector.setComponentID("authoringRoutingInsertOneSelector");
    routingInsertTwoSelector.setComponentID("authoringRoutingInsertTwoSelector");
    performanceBankSelector.setComponentID("authoringPerformanceBankSelector");
    triggerSlotSelector.setComponentID("authoringTriggerSlotSelector");
    triggerEventSelector.setComponentID("authoringTriggerEventSelector");
    targetArticulationSelector.setComponentID("authoringTargetArticulationSelector");
    phraseAssetSelector.setComponentID("authoringPhraseAssetSelector");
    chordModeSelector.setComponentID("authoringChordModeSelector");
    performanceSummaryLabel.setComponentID("authoringPerformanceSummaryLabel");
    phraseSummaryLabel.setComponentID("authoringPhraseSummaryLabel");
    phraseImportPathEditor.setComponentID("authoringPhraseImportPath");

    drawerToggleButton.onClick = [this]
    {
        setDrawerOpen(!drawerState.open);
    };
    drawerWaveformTabButton.setButtonText("Waveform");
    drawerMacrosTabButton.setButtonText("Macros");
    drawerRoutingTabButton.setButtonText("Routing");
    drawerPerformanceTabButton.setButtonText("Performance");
    drawerWaveformTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::waveform); };
    drawerMacrosTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::macros); };
    drawerRoutingTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::routing); };
    drawerPerformanceTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::performance); };
    playbackBannerPrepareButton.setButtonText("Prepare Draft");
    playbackBannerPrepareButton.onClick = [this] { prepareDraftPlaybackPreview(); };
    playbackBannerPublishButton.setButtonText("Publish Draft");
    playbackBannerPublishButton.onClick = [this] { publishDraftPlayback(); };
    phraseImportPathEditor.onTextChange = [this]
    {
        refreshContextualAccessibility();
    };
    configureAccessibilityAndFocus();

    authoring::SelectionSummaryCallbacks summaryCallbacks;
    summaryCallbacks.onPreviewRequested = [this] { previewSelectedZone(); };
    summaryCallbacks.onPrepareDraftPlaybackRequested = [this] { prepareDraftPlaybackPreview(); };
    summaryCallbacks.onPublishDraftPlaybackRequested = [this] { publishDraftPlayback(); };
    summaryCallbacks.onUndoRequested = [this] { undoLastEdit(); };
    summaryCallbacks.onRedoRequested = [this] { redoLastEdit(); };
    summaryCallbacks.onMarkSavedRequested = [this] { markSavedCheckpoint(); };
    summaryStrip.setCallbacks(std::move(summaryCallbacks));

    authoring::ZoneFieldCallbacks zoneCallbacks;
    zoneCallbacks.onCommitRequested = [this](const authoring::ZoneFieldValuesViewModel& values,
                                             const std::string& label)
    {
        applySelectedZoneEdit(values, juce::String::fromUTF8(label.c_str()));
    };
    zoneCallbacks.onRestoreRootKeyRequested = [this]
    {
        if (onRestoreRootKeyRequested)
            onRestoreRootKeyRequested();
    };
    zoneCallbacks.onPreviewRequested = [this]
    {
        previewSelectedZone(drs::engine::AuthoringPreviewAuditionSource::inspector);
    };
    zoneMappingEditor.setCallbacks(std::move(zoneCallbacks));
    zoneMap.setOnZoneSelectionRequested([this](const std::string& zoneId)
    {
        if (isRefreshing)
            return;

        const auto selectedZone = authoringSession.getSelectedZone();
        if (selectedZone.has_value() && selectedZone->id == zoneId)
            return;

        authoringSession.selectZone(zoneId);
        refreshFromSession();
    });
    zoneMap.setOnZoneAuditionRequested([this](const std::string& zoneId,
                                               int midiNote,
                                               int velocity)
    {
        previewSelectedZone(drs::engine::AuthoringPreviewAuditionSource::zoneMap,
                            midiNote, velocity, zoneId);
    });
    zoneMap.setOnZoneRangeCommitRequested([this](const drs::engine::AuthoringZoneSummary& zone,
                                                 const std::string& label)
    {
        authoring::ZoneFieldValuesViewModel values;
        values.hasSelection = true;
        values.rootKey = zone.rootKey;
        values.keyLow = zone.keyLow;
        values.keyHigh = zone.keyHigh;
        values.velocityLow = zone.velocityLow;
        values.velocityHigh = zone.velocityHigh;
        values.gainDb = zone.gainDb;
        values.pan = zone.pan;
        values.loopEnabled = zone.loopEnabled;
        applySelectedZoneEdit(values, juce::String::fromUTF8(label.c_str()));
    });

    zoneSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        const auto zoneIndex = zoneSelector.getSelectedId() - 1;
        const auto zones = authoringSession.getZoneSummaries();
        if (zoneIndex < 0 || static_cast<std::size_t>(zoneIndex) >= zones.size())
            return;

        authoringSession.selectZone(zones[static_cast<std::size_t>(zoneIndex)].id);
        refreshFromSession();
    };

    macroList.setOnSelectionChanged([this](int nextIndex)
    {
        if (isRefreshing)
            return;

        selectedMacroIndex = std::max(0, nextIndex);
        refreshFromSession();
    });

    fxSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedFxSlotIndex = std::max(0, fxSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    routingBusSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedRoutingBusIndex = std::max(0, routingBusSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    performanceBankSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedPerformanceBankIndex = std::max(0, performanceBankSelector.getSelectedId() - 1);
        const auto& performanceBanks = authoringSession.getProject().authoring.performanceBanks;
        if (selectedPerformanceBankIndex >= 0
            && static_cast<std::size_t>(selectedPerformanceBankIndex) < performanceBanks.size())
        {
            authoringSession.selectPerformanceBank(
                performanceBanks[static_cast<std::size_t>(selectedPerformanceBankIndex)].id);
        }

        selectedTriggerSlotIndex = 0;
        refreshFromSession();
    };

    triggerSlotSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedTriggerSlotIndex = std::max(0, triggerSlotSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    auto bindCommitOnDragEnd = [this](juce::Slider& slider, const juce::String& label, auto&& callback)
    {
        slider.onDragEnd = [this, label, callback]
        {
            callback(label);
        };
    };

    bindCommitOnDragEnd(macroDefaultSlider, "Update macro default", [this](const juce::String& label) { applySelectedMacroEdit(label); });
    bindCommitOnDragEnd(macroMinSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label); });
    bindCommitOnDragEnd(macroMaxSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label); });

    macroAssignmentSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Update macro assignment");
    };

    macroRoleSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Update macro role");
    };

    macroMoveUpButton.setButtonText("Move Up");
    macroMoveUpButton.onClick = [this] { moveSelectedMacro(-1); };
    macroMoveDownButton.setButtonText("Move Down");
    macroMoveDownButton.onClick = [this] { moveSelectedMacro(1); };

    fxTypeSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedFxSlotEdit("Update FX type");
    };

    fxBypassedToggle.setButtonText("Bypassed");
    fxBypassedToggle.onClick = [this]
    {
        if (isRefreshing)
            return;

        applySelectedFxSlotEdit("Toggle FX bypass");
    };

    routingInputSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedRoutingBusEdit("Update routing input");
    };

    routingInsertOneSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedRoutingBusEdit("Update routing insert chain");
    };

    routingInsertTwoSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedRoutingBusEdit("Update routing insert chain");
    };

    triggerEventSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger event");
    };

    targetArticulationSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger articulation");
    };

    phraseAssetSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger phrase asset");
    };

    chordModeSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger chord rule");
    };

    phraseImportButton.setButtonText("Import MIDI Phrase");
    phraseImportButton.setComponentID("authoringPhraseImportButton");
    phraseImportButton.onClick = [this]
    {
        importPhraseForSelectedBank();
    };

    for (auto* component : {
             static_cast<juce::Component*>(&summaryStrip),
             static_cast<juce::Component*>(&playbackBanner),
             static_cast<juce::Component*>(&playbackBannerLabel),
             static_cast<juce::Component*>(&playbackBannerPrepareButton),
             static_cast<juce::Component*>(&playbackBannerPublishButton),
             static_cast<juce::Component*>(&drawerRegion),
             static_cast<juce::Component*>(&drawerTabStrip),
             static_cast<juce::Component*>(&drawerContentHost),
             static_cast<juce::Component*>(&waveformLabel),
             static_cast<juce::Component*>(&waveformScopeLabel),
             static_cast<juce::Component*>(&drawerBreadcrumbLabel),
             static_cast<juce::Component*>(&waveformStatusLabel),
             static_cast<juce::Component*>(&waveformInfoLabel),
             static_cast<juce::Component*>(&loopInfoLabel),
             static_cast<juce::Component*>(&importMetricsLabel),
             static_cast<juce::Component*>(&drawerToggleButton),
             static_cast<juce::Component*>(&drawerWaveformTabButton),
             static_cast<juce::Component*>(&drawerMacrosTabButton),
             static_cast<juce::Component*>(&drawerRoutingTabButton),
             static_cast<juce::Component*>(&drawerPerformanceTabButton),
             static_cast<juce::Component*>(&zoneLabel),
             static_cast<juce::Component*>(&zoneSelector),
             static_cast<juce::Component*>(&zoneMap),
             static_cast<juce::Component*>(&zoneMappingEditor),
             static_cast<juce::Component*>(&waveformPreview),
             static_cast<juce::Component*>(&macroList),
             static_cast<juce::Component*>(&macroAssignmentLabel),
             static_cast<juce::Component*>(&macroAssignmentSelector),
             static_cast<juce::Component*>(&macroRoleLabel),
             static_cast<juce::Component*>(&macroRoleSelector),
             static_cast<juce::Component*>(&macroDefaultLabel),
             static_cast<juce::Component*>(&macroDefaultSlider),
             static_cast<juce::Component*>(&macroMinLabel),
             static_cast<juce::Component*>(&macroMinSlider),
             static_cast<juce::Component*>(&macroMaxLabel),
             static_cast<juce::Component*>(&macroMaxSlider),
             static_cast<juce::Component*>(&macroSummaryLabel),
             static_cast<juce::Component*>(&macroMoveUpButton),
             static_cast<juce::Component*>(&macroMoveDownButton),
             static_cast<juce::Component*>(&fxSectionLabel),
             static_cast<juce::Component*>(&fxSelector),
             static_cast<juce::Component*>(&fxTypeLabel),
             static_cast<juce::Component*>(&fxTypeSelector),
             static_cast<juce::Component*>(&fxBypassedToggle),
             static_cast<juce::Component*>(&fxSummaryLabel),
             static_cast<juce::Component*>(&routingSectionLabel),
             static_cast<juce::Component*>(&routingBusSelector),
             static_cast<juce::Component*>(&routingInputLabel),
             static_cast<juce::Component*>(&routingInputSelector),
             static_cast<juce::Component*>(&routingInsertOneLabel),
             static_cast<juce::Component*>(&routingInsertOneSelector),
             static_cast<juce::Component*>(&routingInsertTwoLabel),
             static_cast<juce::Component*>(&routingInsertTwoSelector),
             static_cast<juce::Component*>(&routingSummaryLabel),
             static_cast<juce::Component*>(&performanceBankSelector),
             static_cast<juce::Component*>(&triggerSlotSelector),
             static_cast<juce::Component*>(&triggerEventLabel),
             static_cast<juce::Component*>(&triggerEventSelector),
             static_cast<juce::Component*>(&targetArticulationLabel),
             static_cast<juce::Component*>(&targetArticulationSelector),
             static_cast<juce::Component*>(&phraseAssetLabel),
             static_cast<juce::Component*>(&phraseAssetSelector),
             static_cast<juce::Component*>(&chordModeLabel),
             static_cast<juce::Component*>(&chordModeSelector),
             static_cast<juce::Component*>(&phraseImportPathLabel),
             static_cast<juce::Component*>(&phraseImportPathEditor),
             static_cast<juce::Component*>(&phraseImportButton),
             static_cast<juce::Component*>(&performanceSummaryLabel),
             static_cast<juce::Component*>(&phraseSummaryLabel)
         })
    {
        addAndMakeVisible(component);
    }

    refreshFromSession();
    if (waveformPreviewProvider || authoringPreviewStatusProvider || importResponsivenessProvider || draftPlaybackStatusProvider)
        startTimer(statusTimerId, 250);
}

AuthoringPanel::~AuthoringPanel()
{
    for (std::size_t source = 0; source < timedPreviewNotes.size(); ++source)
        releaseTimedPreview(source);
    stopTimer(statusTimerId);
    stopTimer(previewReleaseTimerId);
    setLookAndFeel(nullptr);
}

void AuthoringPanel::configureAccessibilityAndFocus()
{
    configureAccessibleMetadata(*this,
                                "Authoring workspace",
                                "Phase 2 authoring workspace for zone mapping, compact drawers, routing, and performance editing.");
    configureAccessibleMetadata(playbackBanner,
                                "Draft playback action banner",
                                "Surfaces the next draft playback action close to the mapping workspace.");
    configureAccessibleMetadata(playbackBannerLabel,
                                "Draft playback action",
                                "Displays the next recommended draft playback action for the current workspace state.");
    configureAccessibleMetadata(playbackBannerPrepareButton,
                                "Prepare draft playback banner action",
                                "Builds the latest draft for playback preview from the workspace banner.",
                                "Press to prepare the latest draft for playback preview.");
    configureAccessibleMetadata(playbackBannerPublishButton,
                                "Publish draft playback banner action",
                                "Publishes the latest prepared draft to the performance path from the workspace banner.",
                                "Press to publish the latest prepared draft to the performance path.");
    playbackBannerPrepareButton.setExplicitFocusOrder(22);
    playbackBannerPublishButton.setExplicitFocusOrder(23);
    configureAccessibleMetadata(zoneLabel,
                                "Selected zone label",
                                "Labels the selected zone chooser.");
    configureAccessibleMetadata(zoneSelector,
                                "Zone selector",
                                "Chooses the active zone for map and inspector editing.",
                                "Open the list or use arrow keys to change the selected zone.");
    zoneSelector.setExplicitFocusOrder(24);

    configureAccessibleMetadata(zoneMap,
                                "Zone map",
                                "Displays project zones across key and velocity ranges.",
                                "Use arrow keys to move selection or drag handles to edit ranges.");
    zoneMap.setExplicitFocusOrder(30);

    configureAccessibleMetadata(drawerRegion,
                                "Authoring drawer",
                                "Hosts the waveform, macros, routing, and performance drawer surfaces.");
    configureAccessibleMetadata(drawerTabStrip,
                                "Drawer tab strip",
                                "Contains the drawer visibility control and drawer tab buttons.");
    configureAccessibleMetadata(drawerContentHost,
                                "Drawer content",
                                "Displays the active drawer body when the drawer is open.");

    configureAccessibleMetadata(drawerToggleButton,
                                "Drawer visibility",
                                "Shows or hides the active drawer content.",
                                "Press to collapse or expand the drawer.");
    drawerToggleButton.setExplicitFocusOrder(60);

    configureAccessibleMetadata(drawerWaveformTabButton,
                                "Waveform drawer tab",
                                "Shows zone-scoped waveform detail.",
                                "Press to switch the drawer to waveform detail.");
    configureAccessibleMetadata(drawerMacrosTabButton,
                                "Macros drawer tab",
                                "Shows project-scoped macro assignments.",
                                "Press to switch the drawer to macro editing.");
    configureAccessibleMetadata(drawerRoutingTabButton,
                                "Routing drawer tab",
                                "Shows project-scoped FX and bus routing detail.",
                                "Press to switch the drawer to routing detail.");
    configureAccessibleMetadata(drawerPerformanceTabButton,
                                "Performance drawer tab",
                                "Shows bank-scoped performance and trigger detail.",
                                "Press to switch the drawer to performance detail.");
    drawerWaveformTabButton.setExplicitFocusOrder(61);
    drawerMacrosTabButton.setExplicitFocusOrder(62);
    drawerRoutingTabButton.setExplicitFocusOrder(63);
    drawerPerformanceTabButton.setExplicitFocusOrder(64);

    configureAccessibleMetadata(waveformLabel,
                                "Drawer title",
                                "Names the active drawer surface.");
    configureAccessibleMetadata(waveformScopeLabel,
                                "Drawer scope",
                                "Shows whether the active drawer is zone-, project-, bank-, or trigger-scoped.");
    configureAccessibleMetadata(drawerBreadcrumbLabel,
                                "Drawer breadcrumb",
                                "Shows the selection path for the active drawer.");
    configureAccessibleMetadata(waveformPreview,
                                "Waveform preview",
                                "Displays the selected zone waveform and loop region.");
    configureAccessibleMetadata(waveformStatusLabel,
                                "Waveform preview status",
                                "Shows the current authoring preview revision state for the selected zone.");
    configureAccessibleMetadata(waveformInfoLabel,
                                "Waveform metadata",
                                "Shows source and format information for the selected waveform.");
    configureAccessibleMetadata(loopInfoLabel,
                                "Loop metadata",
                                "Shows loop state information for the selected waveform.");
    configureAccessibleMetadata(importMetricsLabel,
                                "Import responsiveness",
                                "Shows import responsiveness metrics for the current project.");

    configureAccessibleMetadata(macroList,
                                "Macro list",
                                "Lists project macros in compact rows.");
    macroList.getListBox().setExplicitFocusOrder(70);
    configureAccessibleMetadata(macroAssignmentSelector,
                                "Macro parameter",
                                "Chooses the parameter assigned to the selected macro.",
                                "Open the list to choose a parameter target.");
    configureAccessibleMetadata(macroRoleSelector,
                                "Macro role",
                                "Chooses the semantic role for the selected macro.",
                                "Open the list to choose a macro role.");
    configureAccessibleMetadata(macroDefaultSlider,
                                "Macro default",
                                "Adjusts the selected macro default value.",
                                "Drag the slider or enter a numeric value.");
    configureAccessibleMetadata(macroMinSlider,
                                "Macro minimum",
                                "Adjusts the selected macro minimum value.",
                                "Drag the slider or enter a numeric value.");
    configureAccessibleMetadata(macroMaxSlider,
                                "Macro maximum",
                                "Adjusts the selected macro maximum value.",
                                "Drag the slider or enter a numeric value.");
    configureAccessibleMetadata(macroMoveUpButton,
                                "Move macro up",
                                "Moves the selected macro earlier in the list.",
                                "Press to move the selected macro up.");
    configureAccessibleMetadata(macroMoveDownButton,
                                "Move macro down",
                                "Moves the selected macro later in the list.",
                                "Press to move the selected macro down.");
    macroAssignmentSelector.setExplicitFocusOrder(71);
    macroRoleSelector.setExplicitFocusOrder(72);
    macroDefaultSlider.setExplicitFocusOrder(73);
    macroMinSlider.setExplicitFocusOrder(74);
    macroMaxSlider.setExplicitFocusOrder(75);
    macroMoveUpButton.setExplicitFocusOrder(76);
    macroMoveDownButton.setExplicitFocusOrder(77);

    configureAccessibleMetadata(fxSelector,
                                "FX selector",
                                "Chooses the active FX slot for routing detail.",
                                "Open the list to choose an FX slot.");
    configureAccessibleMetadata(fxTypeSelector,
                                "FX type",
                                "Chooses the effect type for the selected FX slot.",
                                "Open the list to choose an effect type.");
    configureAccessibleMetadata(fxBypassedToggle,
                                "FX bypass",
                                "Toggles bypass for the selected FX slot.",
                                "Press to toggle FX bypass.");
    configureAccessibleMetadata(routingBusSelector,
                                "Routing bus selector",
                                "Chooses the active routing bus.",
                                "Open the list to choose a routing bus.");
    configureAccessibleMetadata(routingInputSelector,
                                "Routing input source",
                                "Chooses the input source for the selected routing bus.",
                                "Open the list to choose an input source.");
    configureAccessibleMetadata(routingInsertOneSelector,
                                "Routing insert A",
                                "Chooses the first insert effect for the selected routing bus.",
                                "Open the list to choose the first insert.");
    configureAccessibleMetadata(routingInsertTwoSelector,
                                "Routing insert B",
                                "Chooses the second insert effect for the selected routing bus.",
                                "Open the list to choose the second insert.");
    fxSelector.setExplicitFocusOrder(80);
    fxTypeSelector.setExplicitFocusOrder(81);
    fxBypassedToggle.setExplicitFocusOrder(82);
    routingBusSelector.setExplicitFocusOrder(83);
    routingInputSelector.setExplicitFocusOrder(84);
    routingInsertOneSelector.setExplicitFocusOrder(85);
    routingInsertTwoSelector.setExplicitFocusOrder(86);

    configureAccessibleMetadata(performanceBankSelector,
                                "Performance bank selector",
                                "Chooses the active performance bank.",
                                "Open the list to choose a performance bank.");
    configureAccessibleMetadata(triggerSlotSelector,
                                "Trigger slot selector",
                                "Chooses the active trigger slot within the selected bank.",
                                "Open the list to choose a trigger slot.");
    configureAccessibleMetadata(triggerEventSelector,
                                "Trigger event",
                                "Chooses the event that activates the selected trigger slot.",
                                "Open the list to choose a trigger event.");
    configureAccessibleMetadata(targetArticulationSelector,
                                "Target articulation",
                                "Chooses the articulation targeted by the selected trigger slot.",
                                "Open the list to choose an articulation.");
    configureAccessibleMetadata(phraseAssetSelector,
                                "Phrase asset",
                                "Chooses the phrase asset for the selected trigger slot.",
                                "Open the list to choose a phrase.");
    configureAccessibleMetadata(chordModeSelector,
                                "Chord rule",
                                "Chooses the chord-follow behavior for the selected phrase.",
                                "Open the list to choose a chord rule.");
    configureAccessibleMetadata(phraseImportPathEditor,
                                "MIDI phrase path",
                                "Edits the import path used for phrase import.",
                                "Type a MIDI file path for phrase import.");
    configureAccessibleMetadata(phraseImportButton,
                                "Import MIDI phrase",
                                "Imports the MIDI phrase at the current path into the selected bank.",
                                "Press to import the specified MIDI phrase.");
    configureAccessibleMetadata(performanceSummaryLabel,
                                "Performance summary",
                                "Summarizes the active performance trigger state.");
    configureAccessibleMetadata(phraseSummaryLabel,
                                "Phrase summary",
                                "Summarizes the active phrase library or phrase import state.");
    performanceBankSelector.setExplicitFocusOrder(90);
    triggerSlotSelector.setExplicitFocusOrder(91);
    triggerEventSelector.setExplicitFocusOrder(92);
    targetArticulationSelector.setExplicitFocusOrder(93);
    phraseAssetSelector.setExplicitFocusOrder(94);
    chordModeSelector.setExplicitFocusOrder(95);
    phraseImportPathEditor.setExplicitFocusOrder(96);
    phraseImportButton.setExplicitFocusOrder(97);
}

void AuthoringPanel::paint(juce::Graphics& g)
{
    g.fillAll(authoringPanelBackground);

    auto bounds = getLocalBounds().toFloat().reduced(14.0f);
    g.setColour(authoringPanelAccent.withAlpha(0.22f));
    g.fillRoundedRectangle(bounds, 20.0f);

    g.setColour(authoringPanelCard);
    g.fillRoundedRectangle(bounds.reduced(4.0f), 18.0f);

    if (playbackBanner.isVisible())
    {
        auto bannerBounds = playbackBanner.getBounds().toFloat().expanded(2.0f, 1.0f);
        auto text = playbackBannerLabel.getText();
        auto bannerColour = juce::Colour::fromRGB(234, 223, 206);

        if (text.startsWithIgnoreCase("playback blocked"))
            bannerColour = juce::Colour::fromRGB(246, 223, 212);
        else if (text.startsWithIgnoreCase("playback action"))
            bannerColour = juce::Colour::fromRGB(238, 227, 208);
        else if (text.startsWithIgnoreCase("playback busy") || text.startsWithIgnoreCase("playback paused"))
            bannerColour = juce::Colour::fromRGB(231, 231, 214);

        g.setColour(bannerColour);
        g.fillRoundedRectangle(bannerBounds, 10.0f);
        g.setColour(authoringControlOutline);
        g.drawRoundedRectangle(bannerBounds, 10.0f, 1.0f);
    }
}

void AuthoringPanel::resized()
{
    auto area = getLocalBounds().reduced(28);

    summaryStrip.setBounds(area.removeFromTop(authoring::heroHeight));

    area.removeFromTop(12);
    auto toolbarRow = area.removeFromTop(28);
    zoneLabel.setBounds(toolbarRow.removeFromLeft(96));
    toolbarRow.removeFromLeft(8);
    zoneSelector.setBounds(toolbarRow.removeFromLeft(std::min(360, toolbarRow.getWidth())));

    if (playbackBanner.isVisible())
    {
        area.removeFromTop(8);
        auto bannerRow = area.removeFromTop(32);
        playbackBanner.setBounds(bannerRow);
        auto bannerContent = bannerRow.reduced(12, 4);
        auto actionWidth = 96;
        auto labelArea = bannerContent;
        if (playbackBannerPublishButton.isVisible())
            labelArea.removeFromRight(actionWidth + 8);
        if (playbackBannerPrepareButton.isVisible())
            labelArea.removeFromRight(actionWidth + 8);
        playbackBannerLabel.setBounds(labelArea);

        auto actionArea = bannerContent.removeFromRight(bannerContent.getRight() - labelArea.getRight()).withTrimmedLeft(8);
        if (playbackBannerPrepareButton.isVisible() && playbackBannerPublishButton.isVisible())
        {
            playbackBannerPrepareButton.setBounds(actionArea.removeFromLeft(actionWidth));
            actionArea.removeFromLeft(8);
            playbackBannerPublishButton.setBounds(actionArea.removeFromLeft(actionWidth));
        }
        else if (playbackBannerPrepareButton.isVisible())
        {
            playbackBannerPrepareButton.setBounds(actionArea.removeFromLeft(actionWidth));
            playbackBannerPublishButton.setBounds({});
        }
        else if (playbackBannerPublishButton.isVisible())
        {
            playbackBannerPublishButton.setBounds(actionArea.removeFromLeft(actionWidth));
            playbackBannerPrepareButton.setBounds({});
        }
        else
        {
            playbackBannerPrepareButton.setBounds({});
            playbackBannerPublishButton.setBounds({});
        }
    }
    else
    {
        playbackBanner.setBounds({});
        playbackBannerLabel.setBounds({});
        playbackBannerPrepareButton.setBounds({});
        playbackBannerPublishButton.setBounds({});
    }

    auto layoutLabelAndField = [](juce::Rectangle<int> row,
                                  juce::Label& label,
                                  juce::Component& field,
                                  int labelWidth)
    {
        label.setBounds(row.removeFromLeft(labelWidth));
        row.removeFromLeft(6);
        field.setBounds(row);
    };

    auto layoutDualLabelAndFieldRow = [&](juce::Rectangle<int> row,
                                          juce::Label& leftLabel,
                                          juce::Component& leftField,
                                          int leftLabelWidth,
                                          juce::Label& rightLabel,
                                          juce::Component& rightField,
                                          int rightLabelWidth)
    {
        auto left = row.removeFromLeft((row.getWidth() - 12) / 2);
        row.removeFromLeft(12);
        auto right = row;
        layoutLabelAndField(left, leftLabel, leftField, leftLabelWidth);
        layoutLabelAndField(right, rightLabel, rightField, rightLabelWidth);
    };

    area.removeFromTop(12);
    const auto expanded = isExpandedLayout(layoutMode);
    const auto drawerOpenHeight = expanded ? authoring::expandedDrawerOpenHeight
                                           : authoring::compactDrawerOpenHeight;
    const auto drawerHeight = authoring::drawerTabStripHeight + (drawerState.open ? drawerOpenHeight : 0);
    auto drawerArea = area.removeFromBottom(std::min(drawerHeight, area.getHeight()));
    drawerRegion.setBounds(drawerArea);
    drawerTabStrip.setBounds(drawerArea.removeFromTop(authoring::drawerTabStripHeight));
    drawerContentHost.setBounds(drawerArea);

    auto toggleArea = drawerTabStrip.getBounds().reduced(0, 4);
    drawerToggleButton.setBounds(toggleArea.removeFromRight(110));

    auto tabArea = drawerTabStrip.getBounds().reduced(0, 4);
    const auto tabWidth = expanded ? 104 : 96;
    drawerWaveformTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(8);
    drawerMacrosTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(8);
    drawerRoutingTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(8);
    drawerPerformanceTabButton.setBounds(tabArea.removeFromLeft(tabWidth + 10));

    auto drawerEditorArea = drawerContentHost.getBounds().reduced(12, 10);
    waveformLabel.setBounds(drawerEditorArea.removeFromTop(22));
    drawerEditorArea.removeFromTop(2);
    waveformScopeLabel.setBounds(drawerEditorArea.removeFromTop(14));
    drawerEditorArea.removeFromTop(1);
    drawerBreadcrumbLabel.setBounds(drawerEditorArea.removeFromTop(14));
    drawerEditorArea.removeFromTop(3);

    if (drawerState.activeTab == authoring::DrawerTab::waveform)
    {
        const auto waveformMetadataHeight = 18 + 2 + 18 + 2 + 18 + 2 + 24;
        const auto waveformPreviewHeight = juce::jlimit(72,
                                                        authoring::waveformPreviewHeight,
                                                        drawerEditorArea.getHeight() - waveformMetadataHeight);
        waveformPreview.setBounds(drawerEditorArea.removeFromTop(waveformPreviewHeight));
        drawerEditorArea.removeFromTop(4);
        waveformStatusLabel.setBounds(drawerEditorArea.removeFromTop(18));
        drawerEditorArea.removeFromTop(2);
        waveformInfoLabel.setBounds(drawerEditorArea.removeFromTop(18));
        drawerEditorArea.removeFromTop(2);
        loopInfoLabel.setBounds(drawerEditorArea.removeFromTop(18));
        drawerEditorArea.removeFromTop(2);
        importMetricsLabel.setBounds(drawerEditorArea.removeFromTop(24));
    }

    if (drawerState.activeTab == authoring::DrawerTab::macros)
    {
        constexpr auto macroListHeight = 48;
        auto selectorRow = drawerEditorArea.removeFromTop(macroListHeight);
        const auto buttonColumnWidth = selectorRow.getWidth() < 420 ? 96 : 110;
        const auto listWidth = std::max(180, selectorRow.getWidth() - buttonColumnWidth - 8);
        macroList.setBounds(selectorRow.removeFromLeft(listWidth));
        selectorRow.removeFromLeft(8);
        auto buttonColumn = selectorRow.removeFromLeft(buttonColumnWidth);
        macroMoveUpButton.setBounds(buttonColumn.removeFromTop(20));
        buttonColumn.removeFromTop(8);
        macroMoveDownButton.setBounds(buttonColumn.removeFromTop(20));

        drawerEditorArea.removeFromTop(2);
        auto row = drawerEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   macroAssignmentLabel,
                                   macroAssignmentSelector,
                                   76,
                                   macroRoleLabel,
                                   macroRoleSelector,
                                   56);
        drawerEditorArea.removeFromTop(2);

        row = drawerEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   macroDefaultLabel,
                                   macroDefaultSlider,
                                   56,
                                   macroMinLabel,
                                   macroMinSlider,
                                   40);
        drawerEditorArea.removeFromTop(2);

        row = drawerEditorArea.removeFromTop(28);
        layoutLabelAndField(row, macroMaxLabel, macroMaxSlider, 44);
        if (expanded)
        {
            drawerEditorArea.removeFromTop(2);
            macroSummaryLabel.setBounds(drawerEditorArea.removeFromTop(20));
        }
    }
    else if (drawerState.activeTab == authoring::DrawerTab::routing)
    {
        auto headerRow = drawerEditorArea.removeFromTop(24);
        auto leftHeader = headerRow.removeFromLeft((headerRow.getWidth() - 12) / 2);
        headerRow.removeFromLeft(12);
        fxSectionLabel.setBounds(leftHeader);
        routingSectionLabel.setBounds(headerRow);
        drawerEditorArea.removeFromTop(4);

        auto row = drawerEditorArea.removeFromTop(28);
        auto left = row.removeFromLeft((row.getWidth() - 12) / 2);
        row.removeFromLeft(12);
        auto right = row;
        fxSelector.setBounds(left);
        routingBusSelector.setBounds(right);
        drawerEditorArea.removeFromTop(4);

        row = drawerEditorArea.removeFromTop(28);
        left = row.removeFromLeft((row.getWidth() - 12) / 2);
        row.removeFromLeft(12);
        right = row;
        layoutLabelAndField(left, fxTypeLabel, fxTypeSelector, 44);
        layoutLabelAndField(right, routingInputLabel, routingInputSelector, 44);
        drawerEditorArea.removeFromTop(4);

        row = drawerEditorArea.removeFromTop(28);
        left = row.removeFromLeft((row.getWidth() - 12) / 2);
        row.removeFromLeft(12);
        right = row;
        fxBypassedToggle.setBounds(left);
        layoutLabelAndField(right, routingInsertOneLabel, routingInsertOneSelector, 44);
        drawerEditorArea.removeFromTop(4);

        row = drawerEditorArea.removeFromTop(28);
        layoutLabelAndField(row, routingInsertTwoLabel, routingInsertTwoSelector, 56);
        if (expanded)
        {
            drawerEditorArea.removeFromTop(4);
            fxSummaryLabel.setBounds(drawerEditorArea.removeFromTop(20));
            drawerEditorArea.removeFromTop(2);
            routingSummaryLabel.setBounds(drawerEditorArea.removeFromTop(20));
        }
    }
    else if (drawerState.activeTab == authoring::DrawerTab::performance)
    {
        if (drawerEditorArea.getWidth() < 420)
        {
            performanceBankSelector.setBounds(drawerEditorArea.removeFromTop(28));
            drawerEditorArea.removeFromTop(4);
            triggerSlotSelector.setBounds(drawerEditorArea.removeFromTop(28));
        }
        else
        {
            auto selectorRow = drawerEditorArea.removeFromTop(28);
            performanceBankSelector.setBounds(selectorRow.removeFromLeft(280));
            selectorRow.removeFromLeft(10);
            triggerSlotSelector.setBounds(selectorRow.removeFromLeft(280));
        }

        drawerEditorArea.removeFromTop(4);
        auto row = drawerEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   triggerEventLabel,
                                   triggerEventSelector,
                                   52,
                                   targetArticulationLabel,
                                   targetArticulationSelector,
                                   72);
        drawerEditorArea.removeFromTop(4);

        row = drawerEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   phraseAssetLabel,
                                   phraseAssetSelector,
                                   48,
                                   chordModeLabel,
                                   chordModeSelector,
                                   72);
        drawerEditorArea.removeFromTop(4);

        row = drawerEditorArea.removeFromTop(28);
        auto buttonArea = row.removeFromRight(180);
        row.removeFromRight(10);
        layoutLabelAndField(row, phraseImportPathLabel, phraseImportPathEditor, 56);
        phraseImportButton.setBounds(buttonArea);
        if (expanded)
        {
            drawerEditorArea.removeFromTop(6);
            performanceSummaryLabel.setBounds(drawerEditorArea.removeFromTop(20));
            drawerEditorArea.removeFromTop(4);
            phraseSummaryLabel.setBounds(drawerEditorArea.removeFromTop(24));
        }
    }

    area.removeFromTop(8);
    auto shellArea = area;
    const auto desiredInspectorWidth = expanded ? authoring::expandedInspectorPreferredWidth
                                                : authoring::compactInspectorPreferredWidth;
    const auto minimumInspectorWidth = expanded ? authoring::expandedInspectorMinWidth
                                                : authoring::compactInspectorMinWidth;
    const auto maximumInspectorWidth = expanded ? authoring::expandedInspectorMaxWidth
                                                : authoring::compactInspectorMaxWidth;
    const auto inspectorWidth = juce::jlimit(minimumInspectorWidth,
                                             std::min(maximumInspectorWidth, std::max(minimumInspectorWidth, shellArea.getWidth() / 2)),
                                             desiredInspectorWidth);

    auto inspector = shellArea.removeFromRight(inspectorWidth);
    shellArea.removeFromRight(14);
    zoneMap.setBounds(shellArea);
    zoneMappingEditor.setBounds(inspector);
}

void AuthoringPanel::reloadFromSession()
{
    refreshFromSession();
}

void AuthoringPanel::refreshNow()
{
    selectionSummaryViewModel = buildSelectionSummaryViewModel();
    summaryStrip.setViewModel(selectionSummaryViewModel);
    refreshDraftPlaybackBanner();
    refreshWaveformDrawerContent();
}

void AuthoringPanel::refreshDraftPlaybackBanner()
{
    const auto previousBannerVisible = playbackBanner.isVisible();
    const auto previousPrepareVisible = playbackBannerPrepareButton.isVisible();
    const auto previousPublishVisible = playbackBannerPublishButton.isVisible();
    const auto previousBannerText = playbackBannerLabel.getText();

    auto setButtonState = [](juce::TextButton& button,
                             bool shouldShow,
                             const juce::String& enabledDescription,
                             const juce::String& disabledDescription,
                             const juce::String& enabledHelpText,
                             const juce::String& disabledHelpText)
    {
        button.setEnabled(shouldShow);
        setVisibleAndAccessible(button, shouldShow);
        updateAccessibleDescriptionAndHelpText(button,
                                              shouldShow ? enabledDescription : disabledDescription,
                                              shouldShow ? enabledHelpText : disabledHelpText);
    };

    if (!draftPlaybackStatusProvider)
    {
        playbackBannerLabel.setText({}, juce::dontSendNotification);
        setVisibleAndAccessible(playbackBanner, false);
        setVisibleAndAccessible(playbackBannerLabel, false);
        setButtonState(playbackBannerPrepareButton,
                       false,
                       "Builds the latest draft for playback preview from the workspace banner.",
                       "Unavailable because draft playback status is not available in this shell.",
                       "Press to prepare the latest draft for playback preview.",
                       "Draft playback status is unavailable in this shell.");
        setButtonState(playbackBannerPublishButton,
                       false,
                       "Publishes the latest prepared draft to the performance path from the workspace banner.",
                       "Unavailable because draft playback status is not available in this shell.",
                       "Press to publish the latest prepared draft to the performance path.",
                       "Draft playback status is unavailable in this shell.");
    }
    else
    {
        const auto playbackStatus = draftPlaybackStatusProvider();
        const auto playbackGuidance = buildDraftPlaybackGuidance(authoringSession, playbackStatus);
        const auto bannerText = juce::String::fromUTF8(playbackGuidance.statusText.c_str());
        const auto shouldShowBanner = bannerText.isNotEmpty()
            && !bannerText.startsWithIgnoreCase("playback ready:");
        const auto shouldShowPrepare = shouldShowBanner && playbackGuidance.canPrepareDraftPlayback;
        const auto shouldShowPublish = shouldShowBanner && playbackGuidance.canPublishDraftPlayback;

        playbackBannerLabel.setText(bannerText, juce::dontSendNotification);
        setVisibleAndAccessible(playbackBanner, shouldShowBanner);
        setVisibleAndAccessible(playbackBannerLabel, shouldShowBanner);
        playbackBanner.setTitle("Draft playback action banner");
        playbackBanner.setDescription("Workspace draft playback guidance: " + bannerText);
        playbackBanner.setHelpText(shouldShowBanner
                                       ? "Follow the workspace draft playback guidance before moving back into performance playback."
                                       : "Draft playback is already current on the performance path.");
        updateDynamicAccessibleText(playbackBannerLabel, bannerText, "Draft playback action: ");

        setButtonState(playbackBannerPrepareButton,
                       shouldShowPrepare,
                       "Builds the latest draft for playback preview from the workspace banner.",
                       "Unavailable because the latest draft does not currently need preview preparation.",
                       "Press to prepare the latest draft for playback preview.",
                       "Wait until the banner asks you to prepare the latest draft.");
        setButtonState(playbackBannerPublishButton,
                       shouldShowPublish,
                       "Publishes the latest prepared draft to the performance path from the workspace banner.",
                       "Unavailable because the latest draft is not ready to publish yet.",
                       "Press to publish the latest prepared draft to the performance path.",
                       "Wait until the banner asks you to publish the ready draft.");
    }

    const auto bannerVisibilityChanged = previousBannerVisible != playbackBanner.isVisible()
        || previousPrepareVisible != playbackBannerPrepareButton.isVisible()
        || previousPublishVisible != playbackBannerPublishButton.isVisible();
    const auto bannerTextChanged = previousBannerText != playbackBannerLabel.getText();

    if (bannerVisibilityChanged)
        resized();

    if (bannerVisibilityChanged || bannerTextChanged)
        repaint();
}

authoring::SelectionSummaryViewModel AuthoringPanel::buildSelectionSummaryViewModel() const
{
    authoring::SelectionSummaryViewModel viewModel;
    const auto& documentState = authoringSession.getDocumentState();
    viewModel.title = "Phase 2 Authoring Workspace";
    viewModel.statusText = "Revision " + std::to_string(documentState.revision)
        + " | dirty=" + std::string(documentState.dirty ? "yes" : "no")
        + " | undo=" + std::to_string(documentState.undoDepth)
        + " | redo=" + std::to_string(documentState.redoDepth);
    viewModel.sourceText = "Sample source: none";
    viewModel.articulationText = "Articulation: none";
    viewModel.playbackText = "Draft playback: status unavailable";
    viewModel.canUndo = documentState.undoDepth > 0;
    viewModel.canRedo = documentState.redoDepth > 0;
    viewModel.dirty = documentState.dirty;

    if (draftPlaybackStatusProvider)
    {
        const auto playbackStatus = draftPlaybackStatusProvider();
        const auto playbackGuidance = buildDraftPlaybackGuidance(authoringSession, playbackStatus);
        viewModel.playbackText = "Draft playback: draft r" + std::to_string(playbackStatus.draftRevision)
            + " | preview r" + std::to_string(playbackStatus.preview.revision)
            + " (" + playbackStatus.preview.state + ")"
            + " | published r" + std::to_string(playbackStatus.performance.revision)
            + " (" + playbackStatus.performance.state + ")";
        viewModel.canPrepareDraftPlayback = playbackGuidance.canPrepareDraftPlayback;
        viewModel.canPublishDraftPlayback = playbackGuidance.canPublishDraftPlayback;
        if (!playbackGuidance.statusText.empty())
            viewModel.statusText += " | " + playbackGuidance.statusText;
    }

    if (authoringPreviewStatusProvider)
    {
        const auto previewStatus = authoringPreviewStatusProvider();
        if (previewStatus.available)
        {
            viewModel.playbackText += " | authoring preview r" + std::to_string(previewStatus.draftRevision)
                + " (" + previewStatus.revisionState + ")";

            if (!previewStatus.failureState.empty())
            {
                viewModel.statusText += " | preview blocked: " + previewStatus.failureState;

                if (!previewStatus.blockingPrerequisite.empty())
                    viewModel.statusText += " | fix: " + previewStatus.blockingPrerequisite;
            }
        }
    }

    if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
    {
        viewModel.sourceText = "Sample source: " + zone->sampleSourceId;
        viewModel.articulationText = "Articulation: " + zone->articulationId;
        viewModel.canPreview = true;
        viewModel.canRestoreRootKey = true;
    }

    return viewModel;
}

authoring::ZoneFieldValuesViewModel AuthoringPanel::buildZoneFieldValuesViewModel() const
{
    authoring::ZoneFieldValuesViewModel viewModel;
    viewModel.emptyStateText = "Select a zone to edit mapping values.";

    if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
    {
        viewModel.hasSelection = true;
        viewModel.rootKey = zone->rootKey;
        viewModel.keyLow = zone->keyLow;
        viewModel.keyHigh = zone->keyHigh;
        viewModel.velocityLow = zone->velocityLow;
        viewModel.velocityHigh = zone->velocityHigh;
        viewModel.gainDb = zone->gainDb;
        viewModel.pan = zone->pan;
        viewModel.loopEnabled = zone->loopEnabled;
    }

    return viewModel;
}

void AuthoringPanel::rebuildZoneSelector()
{
    const auto zones = authoringSession.getZoneSummaries();
    zoneSelector.clear(juce::dontSendNotification);

    int itemId = 1;
    int selectedItemId = 0;
    for (const auto& zone : zones)
    {
        zoneSelector.addItem(juce::String::fromUTF8(zone.displayName.c_str())
                                 + "  ["
                                 + formatZoneRange(zone)
                                 + "]",
                             itemId);
        if (zone.selected)
            selectedItemId = itemId;
        ++itemId;
    }

    zoneSelector.setSelectedId(selectedItemId, juce::dontSendNotification);
}

void AuthoringPanel::rebuildMacroList()
{
    const auto& macros = authoringSession.getProject().authoring.macros;
    authoring::RepeatedStructureListViewModel viewModel;
    viewModel.emptyStateText = "No macros are authored in this project yet.";

    if (macros.empty())
    {
        selectedMacroIndex = -1;
        macroList.setViewModel(std::move(viewModel));
        return;
    }

    selectedMacroIndex = std::clamp(selectedMacroIndex, 0, static_cast<int>(macros.size()) - 1);
    viewModel.selectedIndex = selectedMacroIndex;
    viewModel.rows.reserve(macros.size());

    for (std::size_t index = 0; index < macros.size(); ++index)
    {
        auto row = authoring::RepeatedStructureRowViewModel{};
        row.key = macros[index].id;
        row.title = macros[index].name;
        row.statusText = buildMacroListStatusText(macros[index]).toStdString();
        viewModel.rows.push_back(std::move(row));
    }

    macroList.setViewModel(std::move(viewModel));
}

void AuthoringPanel::rebuildFxSelector()
{
    const auto& fxSlots = authoringSession.getProject().authoring.fxSlots;
    fxSelector.clear(juce::dontSendNotification);

    if (fxSlots.empty())
    {
        selectedFxSlotIndex = 0;
        return;
    }

    selectedFxSlotIndex = std::clamp(selectedFxSlotIndex, 0, static_cast<int>(fxSlots.size()) - 1);
    for (std::size_t index = 0; index < fxSlots.size(); ++index)
    {
        fxSelector.addItem(juce::String::fromUTF8(fxSlots[index].displayName.c_str()),
                           static_cast<int>(index) + 1);
    }

    fxSelector.setSelectedId(selectedFxSlotIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildRoutingBusSelector()
{
    const auto& routingBuses = authoringSession.getProject().authoring.routingBuses;
    routingBusSelector.clear(juce::dontSendNotification);

    if (routingBuses.empty())
    {
        selectedRoutingBusIndex = 0;
        return;
    }

    selectedRoutingBusIndex = std::clamp(selectedRoutingBusIndex, 0, static_cast<int>(routingBuses.size()) - 1);
    for (std::size_t index = 0; index < routingBuses.size(); ++index)
    {
        routingBusSelector.addItem(juce::String::fromUTF8(routingBuses[index].displayName.c_str()),
                                   static_cast<int>(index) + 1);
    }

    routingBusSelector.setSelectedId(selectedRoutingBusIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildPerformanceBankSelector()
{
    const auto& performanceBanks = authoringSession.getProject().authoring.performanceBanks;
    performanceBankSelector.clear(juce::dontSendNotification);

    if (performanceBanks.empty())
    {
        selectedPerformanceBankIndex = 0;
        return;
    }

    selectedPerformanceBankIndex = std::clamp(selectedPerformanceBankIndex,
                                              0,
                                              static_cast<int>(performanceBanks.size()) - 1);
    for (std::size_t index = 0; index < performanceBanks.size(); ++index)
    {
        performanceBankSelector.addItem(juce::String::fromUTF8(performanceBanks[index].displayName.c_str()),
                                        static_cast<int>(index) + 1);
    }

    if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank(); selectedPerformanceBank.has_value())
    {
        const auto iterator = std::find_if(performanceBanks.begin(),
                                           performanceBanks.end(),
                                           [&](const auto& performanceBank)
                                           {
                                               return performanceBank.id == selectedPerformanceBank->id;
                                           });
        if (iterator != performanceBanks.end())
            selectedPerformanceBankIndex = static_cast<int>(std::distance(performanceBanks.begin(), iterator));
    }

    performanceBankSelector.setSelectedId(selectedPerformanceBankIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildTriggerSlotSelector()
{
    triggerSlotSelector.clear(juce::dontSendNotification);

    const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
    if (!selectedPerformanceBank.has_value() || selectedPerformanceBank->triggerSlots.empty())
    {
        selectedTriggerSlotIndex = 0;
        return;
    }

    selectedTriggerSlotIndex = std::clamp(selectedTriggerSlotIndex,
                                          0,
                                          static_cast<int>(selectedPerformanceBank->triggerSlots.size()) - 1);
    for (std::size_t index = 0; index < selectedPerformanceBank->triggerSlots.size(); ++index)
    {
        triggerSlotSelector.addItem(
            juce::String::fromUTF8(selectedPerformanceBank->triggerSlots[index].displayName.c_str()),
            static_cast<int>(index) + 1);
    }

    triggerSlotSelector.setSelectedId(selectedTriggerSlotIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::setDrawerOpen(bool shouldOpen)
{
    if (drawerState.open == shouldOpen)
        return;

    drawerState.open = shouldOpen;
    refreshDrawerVisibility();
    resized();
}

void AuthoringPanel::setActiveDrawerTab(authoring::DrawerTab nextTab)
{
    drawerState.activeTab = nextTab;
    drawerState.open = true;
    refreshDrawerVisibility();
    resized();
}

void AuthoringPanel::timerCallback(int timerId)
{
    if (timerId == statusTimerId)
    {
        refreshNow();
        return;
    }
    if (timerId != previewReleaseTimerId)
        return;

    const auto now = juce::Time::getMillisecondCounterHiRes();
    auto anyPending = false;
    for (std::size_t source = 0; source < timedPreviewNotes.size(); ++source)
    {
        if (timedPreviewNotes[source].active
            && now >= timedPreviewNotes[source].releaseAtMillis)
            releaseTimedPreview(source);
        anyPending = anyPending || timedPreviewNotes[source].active;
    }
    if (!anyPending)
        stopTimer(previewReleaseTimerId);
}

void AuthoringPanel::refreshDrawerVisibility()
{
    const auto waveformTab = drawerState.activeTab == authoring::DrawerTab::waveform;
    const auto macrosTab = drawerState.activeTab == authoring::DrawerTab::macros;
    const auto routingTab = drawerState.activeTab == authoring::DrawerTab::routing;
    const auto performanceTab = drawerState.activeTab == authoring::DrawerTab::performance;
    const auto drawerContentVisible = drawerState.open;
    const auto expanded = isExpandedLayout(layoutMode);
    const auto* focusedComponent = juce::Component::getCurrentlyFocusedComponent();
    const auto focusWithinWaveform = isComponentFocusedWithin(focusedComponent, waveformPreview)
        || isComponentFocusedWithin(focusedComponent, waveformStatusLabel)
        || isComponentFocusedWithin(focusedComponent, waveformInfoLabel)
        || isComponentFocusedWithin(focusedComponent, loopInfoLabel)
        || isComponentFocusedWithin(focusedComponent, importMetricsLabel);
    const auto focusWithinMacros = isComponentFocusedWithin(focusedComponent, macroList)
        || isComponentFocusedWithin(focusedComponent, macroAssignmentSelector)
        || isComponentFocusedWithin(focusedComponent, macroRoleSelector)
        || isComponentFocusedWithin(focusedComponent, macroDefaultSlider)
        || isComponentFocusedWithin(focusedComponent, macroMinSlider)
        || isComponentFocusedWithin(focusedComponent, macroMaxSlider)
        || isComponentFocusedWithin(focusedComponent, macroMoveUpButton)
        || isComponentFocusedWithin(focusedComponent, macroMoveDownButton);
    const auto focusWithinRouting = isComponentFocusedWithin(focusedComponent, fxSelector)
        || isComponentFocusedWithin(focusedComponent, fxTypeSelector)
        || isComponentFocusedWithin(focusedComponent, fxBypassedToggle)
        || isComponentFocusedWithin(focusedComponent, routingBusSelector)
        || isComponentFocusedWithin(focusedComponent, routingInputSelector)
        || isComponentFocusedWithin(focusedComponent, routingInsertOneSelector)
        || isComponentFocusedWithin(focusedComponent, routingInsertTwoSelector);
    const auto focusWithinPerformance = isComponentFocusedWithin(focusedComponent, performanceBankSelector)
        || isComponentFocusedWithin(focusedComponent, triggerSlotSelector)
        || isComponentFocusedWithin(focusedComponent, triggerEventSelector)
        || isComponentFocusedWithin(focusedComponent, targetArticulationSelector)
        || isComponentFocusedWithin(focusedComponent, phraseAssetSelector)
        || isComponentFocusedWithin(focusedComponent, chordModeSelector)
        || isComponentFocusedWithin(focusedComponent, phraseImportPathEditor)
        || isComponentFocusedWithin(focusedComponent, phraseImportButton);

    refreshDrawerContextLabels();

    drawerToggleButton.setButtonText(drawerState.open ? "Hide Drawer" : "Show Drawer");
    drawerToggleButton.setTitle(drawerToggleButton.getButtonText());
    setVisibleAndAccessible(drawerContentHost, drawerContentVisible);
    setVisibleAndAccessible(waveformLabel, drawerContentVisible);
    setVisibleAndAccessible(waveformScopeLabel, drawerContentVisible);
    setVisibleAndAccessible(drawerBreadcrumbLabel, drawerContentVisible);
    setVisibleAndAccessible(waveformPreview, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(waveformStatusLabel, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(waveformInfoLabel, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(loopInfoLabel, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(importMetricsLabel, drawerContentVisible && waveformTab);

    setVisibleAndAccessible(macroList, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroAssignmentLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroAssignmentSelector, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroRoleLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroRoleSelector, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroDefaultLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroDefaultSlider, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMinLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMinSlider, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMaxLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMaxSlider, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMoveUpButton, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMoveDownButton, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroSummaryLabel, drawerContentVisible && macrosTab && expanded);

    setVisibleAndAccessible(fxSectionLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxTypeLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxTypeSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxBypassedToggle, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxSummaryLabel, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(routingSectionLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingBusSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInputLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInputSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertOneLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertOneSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertTwoLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertTwoSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingSummaryLabel, drawerContentVisible && routingTab && expanded);

    setVisibleAndAccessible(performanceBankSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(triggerSlotSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(triggerEventLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(triggerEventSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(targetArticulationLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(targetArticulationSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseAssetLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseAssetSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(chordModeLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(chordModeSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseImportPathLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseImportPathEditor, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseImportButton, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(performanceSummaryLabel, drawerContentVisible && performanceTab && expanded);
    setVisibleAndAccessible(phraseSummaryLabel, drawerContentVisible && performanceTab && expanded);

    drawerWaveformTabButton.setToggleState(waveformTab, juce::dontSendNotification);
    drawerMacrosTabButton.setToggleState(macrosTab, juce::dontSendNotification);
    drawerRoutingTabButton.setToggleState(routingTab, juce::dontSendNotification);
    drawerPerformanceTabButton.setToggleState(performanceTab, juce::dontSendNotification);

    const auto focusedDrawerContentBecameHidden = !drawerContentVisible
        ? (focusWithinWaveform || focusWithinMacros || focusWithinRouting || focusWithinPerformance)
        : (waveformTab ? (focusWithinMacros || focusWithinRouting || focusWithinPerformance)
                       : macrosTab ? (focusWithinWaveform || focusWithinRouting || focusWithinPerformance)
                                   : routingTab ? (focusWithinWaveform || focusWithinMacros || focusWithinPerformance)
                                                : (focusWithinWaveform || focusWithinMacros || focusWithinRouting));

    if (focusedDrawerContentBecameHidden)
    {
        if (!drawerContentVisible)
            drawerToggleButton.grabKeyboardFocus();
        else if (waveformTab)
            drawerWaveformTabButton.grabKeyboardFocus();
        else if (macrosTab)
            drawerMacrosTabButton.grabKeyboardFocus();
        else if (routingTab)
            drawerRoutingTabButton.grabKeyboardFocus();
        else
            drawerPerformanceTabButton.grabKeyboardFocus();
    }
}

void AuthoringPanel::refreshContextualAccessibility()
{
    const auto& project = authoringSession.getProject();
    const auto describeCurrentValue = [](const juce::String& value, const juce::String& fallback)
    {
        const auto trimmed = value.trim();
        return trimmed.isNotEmpty() ? trimmed : fallback;
    };
    const auto hasSelectedMacro = !project.authoring.macros.empty()
        && selectedMacroIndex >= 0
        && static_cast<std::size_t>(selectedMacroIndex) < project.authoring.macros.size();
    const auto macroName = hasSelectedMacro
        ? juce::String::fromUTF8(project.authoring.macros[static_cast<std::size_t>(selectedMacroIndex)].name.c_str())
        : juce::String("the selected macro");

    updateAccessibleDescriptionAndHelpText(macroAssignmentSelector,
                                           hasSelectedMacro
                                               ? "Chooses the parameter assigned to " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Open the list to choose a parameter target for " + macroName + "."
                                               : "Author a macro before editing its parameter assignment.");
    updateAccessibleDescriptionAndHelpText(macroRoleSelector,
                                           hasSelectedMacro
                                               ? "Chooses the semantic role for " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Open the list to choose a role for " + macroName + "."
                                               : "Author a macro before editing its role.");
    updateAccessibleDescriptionAndHelpText(macroDefaultSlider,
                                           hasSelectedMacro
                                               ? "Adjusts the default value for " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Drag the slider or enter a numeric default value for " + macroName + "."
                                               : "Author a macro before editing its default value.");
    updateAccessibleDescriptionAndHelpText(macroMinSlider,
                                           hasSelectedMacro
                                               ? "Adjusts the minimum value for " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Drag the slider or enter a numeric minimum value for " + macroName + "."
                                               : "Author a macro before editing its range.");
    updateAccessibleDescriptionAndHelpText(macroMaxSlider,
                                           hasSelectedMacro
                                               ? "Adjusts the maximum value for " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Drag the slider or enter a numeric maximum value for " + macroName + "."
                                               : "Author a macro before editing its range.");
    updateAccessibleDescriptionAndHelpText(macroMoveUpButton,
                                           hasSelectedMacro
                                               ? (macroMoveUpButton.isEnabled()
                                                      ? "Moves " + macroName + " earlier in the list."
                                                      : macroName + " is already the first macro.")
                                               : "Unavailable because no macros are authored.",
                                           hasSelectedMacro
                                               ? (macroMoveUpButton.isEnabled()
                                                      ? "Press to move " + macroName + " toward the start of the macro list."
                                                      : "Select a later macro to enable moving upward.")
                                               : "Author macros before changing their order.");
    updateAccessibleDescriptionAndHelpText(macroMoveDownButton,
                                           hasSelectedMacro
                                               ? (macroMoveDownButton.isEnabled()
                                                      ? "Moves " + macroName + " later in the list."
                                                      : macroName + " is already the last macro.")
                                               : "Unavailable because no macros are authored.",
                                           hasSelectedMacro
                                               ? (macroMoveDownButton.isEnabled()
                                                      ? "Press to move " + macroName + " toward the end of the macro list."
                                                      : "Select an earlier macro to enable moving downward.")
                                               : "Author macros before changing their order.");

    const auto hasSelectedFxSlot = !project.authoring.fxSlots.empty()
        && selectedFxSlotIndex >= 0
        && static_cast<std::size_t>(selectedFxSlotIndex) < project.authoring.fxSlots.size();
    const auto fxName = hasSelectedFxSlot
        ? juce::String::fromUTF8(project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].displayName.c_str())
        : juce::String("the selected FX slot");
    const auto fxType = hasSelectedFxSlot
        ? describeCurrentValue(juce::String::fromUTF8(project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].effectType.c_str()),
                               "(unspecified)")
        : juce::String{};
    const auto fxState = hasSelectedFxSlot
        ? juce::String(project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].bypassed ? "bypassed"
                                                                                                            : "active")
        : juce::String{};

    updateAccessibleDescriptionAndHelpText(fxSelector,
                                           hasSelectedFxSlot
                                               ? "Chooses the active FX slot for routing detail. Current FX slot: " + fxName + "."
                                               : "Unavailable because no FX slots are authored.",
                                           hasSelectedFxSlot
                                               ? "Open the list to switch routing detail to another FX slot."
                                               : "Author an FX slot before editing routing FX detail.");
    updateAccessibleDescriptionAndHelpText(fxTypeSelector,
                                           hasSelectedFxSlot
                                               ? "Chooses the effect type for " + fxName + ". Current effect type: " + fxType + "."
                                               : "Unavailable because no FX slots are authored.",
                                           hasSelectedFxSlot
                                               ? "Open the list to choose a new effect type for " + fxName + "."
                                               : "Author an FX slot before choosing an effect type.");
    updateAccessibleDescriptionAndHelpText(fxBypassedToggle,
                                           hasSelectedFxSlot
                                               ? "Toggles bypass for " + fxName + ". Current state: " + fxState + "."
                                               : "Unavailable because no FX slots are authored.",
                                           hasSelectedFxSlot
                                               ? "Press to toggle whether " + fxName + " is bypassed."
                                               : "Author an FX slot before toggling bypass.");

    const auto hasSelectedRoutingBus = !project.authoring.routingBuses.empty()
        && selectedRoutingBusIndex >= 0
        && static_cast<std::size_t>(selectedRoutingBusIndex) < project.authoring.routingBuses.size();
    const auto busName = hasSelectedRoutingBus
        ? juce::String::fromUTF8(project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].displayName.c_str())
        : juce::String("the selected routing bus");
    const auto inputSource = hasSelectedRoutingBus
        ? describeCurrentValue(juce::String::fromUTF8(project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].inputSourceId.c_str()),
                               "(none)")
        : juce::String{};
    const auto insertOne = hasSelectedRoutingBus
        ? (project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].fxSlotIds.empty()
               ? juce::String("(none)")
               : juce::String::fromUTF8(project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].fxSlotIds.front().c_str()))
        : juce::String{};
    const auto insertTwo = hasSelectedRoutingBus
        ? (project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].fxSlotIds.size() < 2
               ? juce::String("(none)")
               : juce::String::fromUTF8(project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].fxSlotIds[1].c_str()))
        : juce::String{};

    updateAccessibleDescriptionAndHelpText(routingBusSelector,
                                           hasSelectedRoutingBus
                                               ? "Chooses the active routing bus. Current bus: " + busName + "."
                                               : "Unavailable because no routing buses are authored.",
                                           hasSelectedRoutingBus
                                               ? "Open the list to switch routing detail to another routing bus."
                                               : "Author a routing bus before editing its signal path.");
    updateAccessibleDescriptionAndHelpText(routingInputSelector,
                                           hasSelectedRoutingBus
                                               ? "Chooses the input source for " + busName + ". Current source: " + inputSource + "."
                                               : "Unavailable because no routing buses are authored.",
                                           hasSelectedRoutingBus
                                               ? "Open the list to choose a new input source for " + busName + "."
                                               : "Author a routing bus before choosing an input source.");
    updateAccessibleDescriptionAndHelpText(routingInsertOneSelector,
                                           hasSelectedRoutingBus
                                               ? "Chooses the first insert effect for " + busName + ". Current insert A: " + insertOne + "."
                                               : "Unavailable because no routing buses are authored.",
                                           hasSelectedRoutingBus
                                               ? "Open the list to choose the first insert effect for " + busName + "."
                                               : "Author a routing bus before assigning insert effects.");
    updateAccessibleDescriptionAndHelpText(routingInsertTwoSelector,
                                           hasSelectedRoutingBus
                                               ? "Chooses the second insert effect for " + busName + ". Current insert B: " + insertTwo + "."
                                               : "Unavailable because no routing buses are authored.",
                                           hasSelectedRoutingBus
                                               ? "Open the list to choose the second insert effect for " + busName + "."
                                               : "Author a routing bus before assigning insert effects.");

    if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank(); selectedPerformanceBank.has_value())
    {
        const auto bankName = juce::String::fromUTF8(selectedPerformanceBank->displayName.c_str());
        const auto hasSelectedTriggerSlot = selectedTriggerSlotIndex >= 0
            && static_cast<std::size_t>(selectedTriggerSlotIndex) < selectedPerformanceBank->triggerSlots.size();
        const auto triggerName = hasSelectedTriggerSlot
            ? juce::String::fromUTF8(selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)].displayName.c_str())
            : juce::String("the selected trigger slot");
        const auto triggerEvent = hasSelectedTriggerSlot
            ? describeCurrentValue(juce::String::fromUTF8(
                                       selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)].triggerEvent.c_str()),
                                   "(unspecified)")
            : juce::String{};
        const auto targetArticulation = hasSelectedTriggerSlot
            ? describeCurrentValue(
                  juce::String::fromUTF8(
                      selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)].targetArticulationId.c_str()),
                  "(none)")
            : juce::String{};
        const auto selectedPhraseText = describeCurrentValue(phraseAssetSelector.getText(), "(none)");
        const auto selectedChordMode = describeCurrentValue(chordModeSelector.getText(), "(unspecified)");
        const auto midiPath = phraseImportPathEditor.getText().trim();

        updateAccessibleDescriptionAndHelpText(performanceBankSelector,
                                               "Chooses the active performance bank. Current bank: " + bankName + ".",
                                               "Open the list to switch to another performance bank.");
        updateAccessibleDescriptionAndHelpText(triggerSlotSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the active trigger slot within " + bankName + ". Current trigger slot: " + triggerName + "."
                                                   : "Unavailable because " + bankName + " has no trigger slots.",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose a different trigger slot in " + bankName + "."
                                                   : "Author a trigger slot in " + bankName + " before editing trigger detail.");
        updateAccessibleDescriptionAndHelpText(triggerEventSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the event that activates " + triggerName + " in " + bankName
                                                         + ". Current event: " + triggerEvent + "."
                                                   : "Unavailable because no trigger slot is selected in " + bankName + ".",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose the trigger event for " + triggerName + "."
                                                   : "Select a trigger slot before changing its trigger event.");
        updateAccessibleDescriptionAndHelpText(targetArticulationSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the articulation targeted by " + triggerName + " in " + bankName
                                                         + ". Current articulation: " + targetArticulation + "."
                                                   : "Unavailable because no trigger slot is selected in " + bankName + ".",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose the target articulation for " + triggerName + "."
                                                   : "Select a trigger slot before changing its target articulation.");
        updateAccessibleDescriptionAndHelpText(phraseAssetSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the phrase asset for " + triggerName + " in " + bankName
                                                         + ". Current phrase asset: " + selectedPhraseText + "."
                                                   : "Unavailable because no trigger slot is selected in " + bankName + ".",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose the phrase asset used by " + triggerName + "."
                                                   : "Select a trigger slot before assigning a phrase asset.");
        updateAccessibleDescriptionAndHelpText(chordModeSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the chord-follow behavior for " + triggerName + " in " + bankName
                                                         + ". Current chord rule: " + selectedChordMode + "."
                                                   : "Unavailable because no trigger slot is selected in " + bankName + ".",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose the chord-follow rule for " + triggerName + "."
                                                   : "Select a trigger slot before changing its chord-follow rule.");
        updateAccessibleDescriptionAndHelpText(phraseImportPathEditor,
                                               midiPath.isEmpty()
                                                   ? "Edits the MIDI import path for " + bankName + ". No MIDI file path is entered yet."
                                                   : "Edits the MIDI import path for " + bankName + ". Current path: " + midiPath,
                                               midiPath.isEmpty()
                                                   ? "Type a MIDI file path before pressing Import MIDI Phrase."
                                                   : "Edit the current MIDI file path before importing it into " + bankName + ".");
        updateAccessibleDescriptionAndHelpText(phraseImportButton,
                                               midiPath.isEmpty()
                                                   ? "Unavailable until a MIDI file path is entered for " + bankName + "."
                                                   : "Imports the MIDI phrase at " + midiPath + " into " + bankName + ".",
                                               midiPath.isEmpty()
                                                   ? "Enter a MIDI file path before importing."
                                                   : "Press to import the current MIDI file into " + bankName + ".");
    }
    else
    {
        updateAccessibleDescriptionAndHelpText(performanceBankSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank before editing performance trigger detail.");
        updateAccessibleDescriptionAndHelpText(triggerSlotSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank before choosing a trigger slot.");
        updateAccessibleDescriptionAndHelpText(triggerEventSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank and trigger slot before changing the trigger event.");
        updateAccessibleDescriptionAndHelpText(targetArticulationSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank and trigger slot before changing the target articulation.");
        updateAccessibleDescriptionAndHelpText(phraseAssetSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank and trigger slot before assigning a phrase asset.");
        updateAccessibleDescriptionAndHelpText(chordModeSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank and trigger slot before changing the chord-follow rule.");
        updateAccessibleDescriptionAndHelpText(phraseImportPathEditor,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank before entering a MIDI file path.");
        updateAccessibleDescriptionAndHelpText(phraseImportButton,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank before importing a phrase.");
    }
}

void AuthoringPanel::refreshInspectorVisibility()
{
    zoneMap.setVisible(true);
    zoneMappingEditor.setVisible(true);

    refreshDrawerVisibility();
}

void AuthoringPanel::refreshDrawerContextLabels()
{
    const auto& project = authoringSession.getProject();

    switch (drawerState.activeTab)
    {
        case authoring::DrawerTab::waveform:
        {
            waveformLabel.setText("Waveform Detail", juce::dontSendNotification);
            waveformScopeLabel.setText("Zone-scoped selection detail", juce::dontSendNotification);

            if (const auto selectedZone = authoringSession.getSelectedZone(); selectedZone.has_value())
            {
                drawerBreadcrumbLabel.setText("Project > Zones > "
                                                  + juce::String::fromUTF8(selectedZone->displayName.c_str()),
                                              juce::dontSendNotification);
            }
            else
            {
                drawerBreadcrumbLabel.setText("Project > Zones", juce::dontSendNotification);
            }
            break;
        }
        case authoring::DrawerTab::macros:
        {
            waveformLabel.setText("Macro Assignment", juce::dontSendNotification);
            waveformScopeLabel.setText("Project-scoped automation detail", juce::dontSendNotification);

            juce::String breadcrumb = "Project > Macros";
            if (!project.authoring.macros.empty()
                && static_cast<std::size_t>(selectedMacroIndex) < project.authoring.macros.size())
            {
                breadcrumb << " > "
                           << juce::String::fromUTF8(project.authoring.macros[static_cast<std::size_t>(selectedMacroIndex)].name.c_str());
            }
            drawerBreadcrumbLabel.setText(breadcrumb, juce::dontSendNotification);
            break;
        }
        case authoring::DrawerTab::routing:
        {
            waveformLabel.setText("Routing Detail", juce::dontSendNotification);
            waveformScopeLabel.setText("Project-scoped FX and bus detail", juce::dontSendNotification);

            juce::String fxName = "(none)";
            if (!project.authoring.fxSlots.empty()
                && static_cast<std::size_t>(selectedFxSlotIndex) < project.authoring.fxSlots.size())
            {
                fxName = juce::String::fromUTF8(project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].displayName.c_str());
            }

            juce::String busName = "(none)";
            if (!project.authoring.routingBuses.empty()
                && static_cast<std::size_t>(selectedRoutingBusIndex) < project.authoring.routingBuses.size())
            {
                busName = juce::String::fromUTF8(project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].displayName.c_str());
            }

            drawerBreadcrumbLabel.setText("Project > Routing > FX: " + fxName + " | Bus: " + busName,
                                          juce::dontSendNotification);
            break;
        }
        case authoring::DrawerTab::performance:
        {
            waveformLabel.setText("Performance Detail", juce::dontSendNotification);

            if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
                selectedPerformanceBank.has_value())
            {
                juce::String breadcrumb = "Project > Performance > "
                    + juce::String::fromUTF8(selectedPerformanceBank->displayName.c_str());

                if (selectedTriggerSlotIndex >= 0
                    && static_cast<std::size_t>(selectedTriggerSlotIndex) < selectedPerformanceBank->triggerSlots.size())
                {
                    waveformScopeLabel.setText("Bank-scoped trigger detail", juce::dontSendNotification);
                    breadcrumb << " > "
                               << juce::String::fromUTF8(
                                      selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)].displayName.c_str());
                }
                else
                {
                    waveformScopeLabel.setText("Bank-scoped performance detail", juce::dontSendNotification);
                }

                drawerBreadcrumbLabel.setText(breadcrumb, juce::dontSendNotification);
            }
            else
            {
                waveformScopeLabel.setText("Bank-scoped performance detail", juce::dontSendNotification);
                drawerBreadcrumbLabel.setText("Project > Performance", juce::dontSendNotification);
            }
            break;
        }
        default:
            break;
    }

    updateDynamicAccessibleText(waveformLabel, waveformLabel.getText(), "Active drawer title: ");
    updateDynamicAccessibleText(waveformScopeLabel, waveformScopeLabel.getText(), "Active drawer scope: ");
    updateDynamicAccessibleText(drawerBreadcrumbLabel, drawerBreadcrumbLabel.getText(), "Active drawer breadcrumb: ");
}

void AuthoringPanel::refreshWaveformDrawerContent()
{
    AuthoringWaveformPreview preview;
    if (waveformPreviewProvider)
        preview = waveformPreviewProvider();

    AuthoringPreviewStatusSnapshot previewStatus;
    if (authoringPreviewStatusProvider)
        previewStatus = authoringPreviewStatusProvider();

    waveformPreview.setPreview(preview);
    waveformStatusLabel.setText(formatAuthoringPreviewStatus(previewStatus), juce::dontSendNotification);

    if (preview.available)
    {
        const auto sourceFile = preview.sourcePath.empty()
            ? juce::String("(source unavailable)")
            : juce::File(juce::String::fromUTF8(preview.sourcePath.c_str())).getFileName();
        waveformInfoLabel.setText(
            "File " + sourceFile
                + " | " + juce::String::fromUTF8(preview.formatName.c_str())
                + " | " + juce::String(static_cast<int>(preview.sampleRate)) + " Hz"
                + " | " + juce::String(static_cast<int>(preview.channelCount)) + " ch"
                + " | " + juce::String(preview.durationSeconds, 3) + " s",
            juce::dontSendNotification);
        loopInfoLabel.setText(
            preview.loopEnabled
                ? "Loop " + juce::String(static_cast<int>(preview.loopStartFrame))
                    + " - " + juce::String(static_cast<int>(preview.loopEndFrame))
                : "Loop disabled for selected zone",
            juce::dontSendNotification);
    }
    else
    {
        waveformInfoLabel.setText(
            preview.state.empty() ? "Waveform metadata unavailable"
                                  : "Waveform: " + juce::String::fromUTF8(preview.state.c_str()),
            juce::dontSendNotification);
        loopInfoLabel.setText("Loop metadata unavailable", juce::dontSendNotification);
    }

    if (importResponsivenessProvider)
    {
        const auto metrics = importResponsivenessProvider();
        importMetricsLabel.setText(
            metrics.available
                ? "Import " + juce::String(static_cast<int>(metrics.processedCount))
                    + "/" + juce::String(static_cast<int>(metrics.totalItemCount))
                    + " | warn " + juce::String(static_cast<int>(metrics.warningItemCount))
                    + " | fail " + juce::String(static_cast<int>(metrics.failedItemCount))
                    + " | last " + formatMicros(metrics.lastProcessDurationMicros)
                    + " avg " + formatMicros(metrics.averageProcessDurationMicros)
                    + " max " + formatMicros(metrics.maxProcessDurationMicros)
                : "Import responsiveness unavailable",
            juce::dontSendNotification);
    }
    else
    {
        importMetricsLabel.setText("Import responsiveness unavailable", juce::dontSendNotification);
    }

    waveformStatusLabel.setTitle(waveformStatusLabel.getText());
    auto waveformStatusDescription = "Waveform preview status: " + waveformStatusLabel.getText().toStdString();
    if (!previewStatus.blockingGuidance.empty())
        waveformStatusDescription += " Next step: " + previewStatus.blockingGuidance;
    waveformStatusLabel.setDescription(waveformStatusDescription);
    updateDynamicAccessibleText(waveformInfoLabel, waveformInfoLabel.getText(), "Waveform metadata: ");
    updateDynamicAccessibleText(loopInfoLabel, loopInfoLabel.getText(), "Loop metadata: ");
    updateDynamicAccessibleText(importMetricsLabel, importMetricsLabel.getText(), "Import responsiveness: ");
}

void AuthoringPanel::refreshFromSession()
{
    const juce::ScopedValueSetter<bool> refreshGuard(isRefreshing, true);

    rebuildZoneSelector();
    rebuildMacroList();
    rebuildFxSelector();
    rebuildRoutingBusSelector();
    rebuildPerformanceBankSelector();
    rebuildTriggerSlotSelector();
    zoneMap.setZoneSummaries(authoringSession.getZoneSummaries());

    const auto& project = authoringSession.getProject();
    selectionSummaryViewModel = buildSelectionSummaryViewModel();
    zoneFieldValuesViewModel = buildZoneFieldValuesViewModel();

    summaryStrip.setViewModel(selectionSummaryViewModel);
    refreshDraftPlaybackBanner();
    zoneMappingEditor.setViewModel(zoneFieldValuesViewModel);

    if (!project.authoring.macros.empty())
    {
        const auto& macro = project.authoring.macros[static_cast<std::size_t>(selectedMacroIndex)];
        macroAssignmentSelector.clear(juce::dontSendNotification);
        int selectedAssignmentId = 0;
        for (std::size_t index = 0; index < curatedMacroAssignments.size(); ++index)
        {
            macroAssignmentSelector.addItem(curatedMacroAssignments[index].label, static_cast<int>(index) + 1);
        }

        if (!macro.targets.empty())
        {
            const auto assignmentIndex = findAssignmentIndex(macro.targets.front().parameterId);
            if (assignmentIndex >= 0)
            {
                selectedAssignmentId = assignmentIndex + 1;
            }
            else
            {
                const auto customItemId = static_cast<int>(curatedMacroAssignments.size()) + 1;
                macroAssignmentSelector.addItem("Custom: "
                                                   + juce::String::fromUTF8(macro.targets.front().parameterId.c_str()),
                                               customItemId);
                selectedAssignmentId = customItemId;
            }
        }
        macroAssignmentSelector.setSelectedId(selectedAssignmentId > 0 ? selectedAssignmentId : 1,
                                              juce::dontSendNotification);

        macroRoleSelector.clear(juce::dontSendNotification);
        int selectedRoleId = 0;
        const auto currentRole = !macro.targets.empty() ? macro.targets.front().role : std::string{};
        for (std::size_t index = 0; index < curatedMacroRoles.size(); ++index)
        {
            macroRoleSelector.addItem(curatedMacroRoles[index], static_cast<int>(index) + 1);
            if (currentRole == curatedMacroRoles[index])
                selectedRoleId = static_cast<int>(index) + 1;
        }
        if (selectedRoleId == 0 && !currentRole.empty())
        {
            const auto customRoleId = static_cast<int>(curatedMacroRoles.size()) + 1;
            macroRoleSelector.addItem("Custom: " + juce::String::fromUTF8(currentRole.c_str()), customRoleId);
            selectedRoleId = customRoleId;
        }
        macroRoleSelector.setSelectedId(selectedRoleId > 0 ? selectedRoleId : 1, juce::dontSendNotification);

        macroDefaultSlider.setRange(macro.minValue, macro.maxValue, 0.01);
        macroDefaultSlider.setValue(macro.defaultValue, juce::dontSendNotification);
        macroMinSlider.setValue(macro.minValue, juce::dontSendNotification);
        macroMaxSlider.setValue(macro.maxValue, juce::dontSendNotification);
        macroSummaryLabel.setText(
            "Target "
                + juce::String::fromUTF8(macro.targets.empty() ? "" : macro.targets.front().parameterPath.c_str())
                + " | range " + juce::String(macro.minValue, 2)
                + " to " + juce::String(macro.maxValue, 2),
            juce::dontSendNotification);
        macroAssignmentSelector.setEnabled(true);
        macroRoleSelector.setEnabled(true);
        macroDefaultSlider.setEnabled(true);
        macroMinSlider.setEnabled(true);
        macroMaxSlider.setEnabled(true);
        macroMoveUpButton.setEnabled(selectedMacroIndex > 0);
        macroMoveDownButton.setEnabled(selectedMacroIndex + 1 < static_cast<int>(project.authoring.macros.size()));
    }
    else
    {
        macroSummaryLabel.setText("No macros are authored in this project yet.", juce::dontSendNotification);
        macroAssignmentSelector.setEnabled(false);
        macroRoleSelector.setEnabled(false);
        macroDefaultSlider.setEnabled(false);
        macroMinSlider.setEnabled(false);
        macroMaxSlider.setEnabled(false);
        macroMoveUpButton.setEnabled(false);
        macroMoveDownButton.setEnabled(false);
    }

    if (!project.authoring.fxSlots.empty())
    {
        const auto& fxSlot = project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)];
        fxTypeSelector.clear(juce::dontSendNotification);
        int selectedFxTypeId = 0;
        for (std::size_t index = 0; index < curatedFxTypes.size(); ++index)
        {
            fxTypeSelector.addItem(curatedFxTypes[index], static_cast<int>(index) + 1);
            if (fxSlot.effectType == curatedFxTypes[index])
                selectedFxTypeId = static_cast<int>(index) + 1;
        }
        if (selectedFxTypeId == 0 && !fxSlot.effectType.empty())
        {
            const auto customTypeId = static_cast<int>(curatedFxTypes.size()) + 1;
            fxTypeSelector.addItem("Custom: " + juce::String::fromUTF8(fxSlot.effectType.c_str()), customTypeId);
            selectedFxTypeId = customTypeId;
        }
        fxTypeSelector.setSelectedId(selectedFxTypeId > 0 ? selectedFxTypeId : 1, juce::dontSendNotification);
        fxBypassedToggle.setToggleState(fxSlot.bypassed, juce::dontSendNotification);
        fxSummaryLabel.setText(
            juce::String::fromUTF8(fxSlot.id.c_str()) + " | "
                + (fxSlot.bypassed ? "bypassed" : "active"),
            juce::dontSendNotification);
    }
    else
    {
        fxSummaryLabel.setText("No FX slots are authored in this project yet.", juce::dontSendNotification);
    }

    if (!project.authoring.routingBuses.empty())
    {
        const auto& routingBus = project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)];
        std::vector<std::string> inputSources;
        inputSources.push_back("master");
        for (const auto& zone : project.authoring.zones)
            inputSources.push_back(zone.id);

        routingInputSelector.clear(juce::dontSendNotification);
        int selectedInputId = 0;
        for (std::size_t index = 0; index < inputSources.size(); ++index)
        {
            routingInputSelector.addItem(juce::String::fromUTF8(inputSources[index].c_str()),
                                         static_cast<int>(index) + 1);
            if (routingBus.inputSourceId == inputSources[index])
                selectedInputId = static_cast<int>(index) + 1;
        }
        routingInputSelector.setSelectedId(selectedInputId > 0 ? selectedInputId : 1, juce::dontSendNotification);

        auto refreshInsertSelector = [&](juce::ComboBox& combo, const std::string& selectedFxId)
        {
            combo.clear(juce::dontSendNotification);
            combo.addItem("(none)", 1);

            int selectedId = 1;
            for (std::size_t index = 0; index < project.authoring.fxSlots.size(); ++index)
            {
                const auto itemId = static_cast<int>(index) + 2;
                combo.addItem(juce::String::fromUTF8(project.authoring.fxSlots[index].id.c_str()), itemId);
                if (project.authoring.fxSlots[index].id == selectedFxId)
                    selectedId = itemId;
            }

            combo.setSelectedId(selectedId, juce::dontSendNotification);
        };

        refreshInsertSelector(routingInsertOneSelector,
                              routingBus.fxSlotIds.empty() ? std::string{} : routingBus.fxSlotIds.front());
        refreshInsertSelector(routingInsertTwoSelector,
                              routingBus.fxSlotIds.size() < 2 ? std::string{} : routingBus.fxSlotIds[1]);

        routingSummaryLabel.setText(
            "Bus " + juce::String::fromUTF8(routingBus.id.c_str())
                + " | chain " + joinIdList(routingBus.fxSlotIds),
            juce::dontSendNotification);
    }
    else
    {
        routingSummaryLabel.setText("No routing buses are authored in this project yet.", juce::dontSendNotification);
    }

    if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank(); selectedPerformanceBank.has_value())
    {
        triggerEventSelector.clear(juce::dontSendNotification);
        int selectedTriggerEventId = 0;
        for (std::size_t index = 0; index < curatedTriggerEvents.size(); ++index)
        {
            triggerEventSelector.addItem(curatedTriggerEvents[index], static_cast<int>(index) + 1);
        }

        targetArticulationSelector.clear(juce::dontSendNotification);
        const auto articulationIds = buildArticulationIds(project);
        int selectedArticulationId = 0;
        for (std::size_t index = 0; index < articulationIds.size(); ++index)
        {
            targetArticulationSelector.addItem(juce::String::fromUTF8(articulationIds[index].c_str()),
                                               static_cast<int>(index) + 1);
        }

        phraseAssetSelector.clear(juce::dontSendNotification);
        phraseAssetSelector.addItem("(none)", 1);
        int selectedPhraseAssetId = 1;
        for (std::size_t index = 0; index < selectedPerformanceBank->phraseAssets.size(); ++index)
        {
            const auto itemId = static_cast<int>(index) + 2;
            phraseAssetSelector.addItem(juce::String::fromUTF8(selectedPerformanceBank->phraseAssets[index].displayName.c_str()),
                                        itemId);
        }

        chordModeSelector.clear(juce::dontSendNotification);
        int selectedChordModeId = 0;
        for (std::size_t index = 0; index < curatedChordModes.size(); ++index)
        {
            chordModeSelector.addItem(curatedChordModes[index], static_cast<int>(index) + 1);
        }

        if (selectedTriggerSlotIndex >= 0
            && static_cast<std::size_t>(selectedTriggerSlotIndex) < selectedPerformanceBank->triggerSlots.size())
        {
            const auto& triggerSlot = selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)];
            for (std::size_t index = 0; index < curatedTriggerEvents.size(); ++index)
            {
                if (triggerSlot.triggerEvent == curatedTriggerEvents[index])
                    selectedTriggerEventId = static_cast<int>(index) + 1;
            }

            for (std::size_t index = 0; index < articulationIds.size(); ++index)
            {
                if (triggerSlot.targetArticulationId == articulationIds[index])
                    selectedArticulationId = static_cast<int>(index) + 1;
            }

            for (std::size_t index = 0; index < selectedPerformanceBank->phraseAssets.size(); ++index)
            {
                if (triggerSlot.phraseAssetId == selectedPerformanceBank->phraseAssets[index].id)
                    selectedPhraseAssetId = static_cast<int>(index) + 2;
            }

            for (std::size_t index = 0; index < curatedChordModes.size(); ++index)
            {
                if (triggerSlot.chordMode == curatedChordModes[index])
                    selectedChordModeId = static_cast<int>(index) + 1;
            }

            performanceSummaryLabel.setText(
                "Trigger " + juce::String::fromUTF8(triggerSlot.displayName.c_str())
                    + " | event " + juce::String::fromUTF8(triggerSlot.triggerEvent.c_str())
                    + " | articulation " + juce::String::fromUTF8(triggerSlot.targetArticulationId.c_str()),
                juce::dontSendNotification);
        }
        else
        {
            performanceSummaryLabel.setText("No trigger slot is selected in the active performance bank.",
                                            juce::dontSendNotification);
        }

        triggerEventSelector.setSelectedId(selectedTriggerEventId > 0 ? selectedTriggerEventId : 1,
                                           juce::dontSendNotification);
        targetArticulationSelector.setSelectedId(selectedArticulationId > 0 ? selectedArticulationId : 1,
                                                 juce::dontSendNotification);
        phraseAssetSelector.setSelectedId(selectedPhraseAssetId, juce::dontSendNotification);
        chordModeSelector.setSelectedId(selectedChordModeId > 0 ? selectedChordModeId : 1,
                                        juce::dontSendNotification);

        if (!selectedPerformanceBank->phraseAssets.empty())
        {
            const auto phraseAssetIndex = std::max(0, phraseAssetSelector.getSelectedId() - 2);
            if (phraseAssetSelector.getSelectedId() > 1
                && static_cast<std::size_t>(phraseAssetIndex) < selectedPerformanceBank->phraseAssets.size())
            {
                const auto& phraseAsset = selectedPerformanceBank->phraseAssets[static_cast<std::size_t>(phraseAssetIndex)];
                phraseSummaryLabel.setText(
                    juce::String::fromUTF8(phraseAsset.displayName.c_str())
                        + " | notes=" + juce::String(static_cast<int>(phraseAsset.notes.size()))
                        + " | beats=" + juce::String(phraseAsset.lengthBeats, 2)
                        + " | chord=" + juce::String::fromUTF8(phraseAsset.chordHint.c_str()),
                    juce::dontSendNotification);
            }
            else
            {
                phraseSummaryLabel.setText("Phrase library ready. Select a phrase asset to inspect it.",
                                           juce::dontSendNotification);
            }
        }
        else
        {
            phraseSummaryLabel.setText("No MIDI phrases have been imported for the active performance bank yet.",
                                       juce::dontSendNotification);
        }
    }
    else
    {
        performanceSummaryLabel.setText("No performance bank is selected.", juce::dontSendNotification);
        phraseSummaryLabel.setText("Performance phrases unavailable.", juce::dontSendNotification);
    }

    updateDynamicAccessibleText(performanceSummaryLabel,
                                performanceSummaryLabel.getText(),
                                "Performance summary: ");
    updateDynamicAccessibleText(phraseSummaryLabel,
                                phraseSummaryLabel.getText(),
                                "Phrase summary: ");
    refreshContextualAccessibility();

    refreshWaveformDrawerContent();
    refreshInspectorVisibility();
}

void AuthoringPanel::applySelectedZoneEdit(const authoring::ZoneFieldValuesViewModel& values,
                                           const juce::String& label)
{
    const auto currentZone = authoringSession.getSelectedZone();
    if (!currentZone.has_value())
        return;

    auto editedZone = *currentZone;
    editedZone.rootKey = values.rootKey;
    editedZone.keyLow = values.keyLow;
    editedZone.keyHigh = values.keyHigh;
    editedZone.velocityLow = values.velocityLow;
    editedZone.velocityHigh = values.velocityHigh;
    editedZone.gainDb = values.gainDb;
    editedZone.pan = values.pan;
    editedZone.loopEnabled = values.loopEnabled;

    authoringSession.updateSelectedZone(editedZone, label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::previewSelectedZone(
    drs::engine::AuthoringPreviewAuditionSource source,
    int explicitMidiNote,
    int explicitVelocity,
    std::string explicitZoneId)
{
    const auto request = authoringSession.buildSelectedZonePreviewRequest();
    if (!request.available)
        return;

    const auto midiNote = explicitMidiNote >= 0 ? explicitMidiNote : request.midiNote;
    const auto velocity = explicitVelocity > 0 ? explicitVelocity : request.velocity;
    const auto sourceIndex = std::min<std::size_t>(static_cast<std::size_t>(source),
                                                   timedPreviewNotes.size() - 1);
    releaseTimedPreview(sourceIndex);

    if (previewCommandCallback)
    {
        drs::engine::AuthoringPreviewCommand command;
        command.type = drs::engine::AuthoringPreviewCommandType::auditionSelectedZone;
        command.source = source;
        command.midiNote = midiNote;
        command.velocity = static_cast<float>(velocity) / 127.0f;
        command.selectedZoneId = explicitZoneId.empty()
            ? authoringSession.getSelectedZone()->id
            : std::move(explicitZoneId);
        previewCommandCallback(command);
    }
    else if (onNotePreviewStarted)
    {
        onNotePreviewStarted(midiNote, static_cast<float>(velocity) / 127.0f);
    }

    timedPreviewNotes[sourceIndex] = { true, midiNote,
                                       juce::Time::getMillisecondCounterHiRes() + 180.0 };
    startTimer(previewReleaseTimerId, 10);
}

void AuthoringPanel::releaseTimedPreview(std::size_t sourceIndex)
{
    if (sourceIndex >= timedPreviewNotes.size() || !timedPreviewNotes[sourceIndex].active)
        return;

    const auto note = timedPreviewNotes[sourceIndex].midiNote;
    timedPreviewNotes[sourceIndex] = {};
    if (previewCommandCallback)
    {
        drs::engine::AuthoringPreviewCommand command;
        command.type = drs::engine::AuthoringPreviewCommandType::noteOff;
        command.source = static_cast<drs::engine::AuthoringPreviewAuditionSource>(sourceIndex);
        command.midiNote = note;
        previewCommandCallback(command);
    }
    else if (onNotePreviewEnded)
        onNotePreviewEnded(note);
}

void AuthoringPanel::prepareDraftPlaybackPreview()
{
    if (onPrepareDraftPlaybackRequested)
        onPrepareDraftPlaybackRequested();

    refreshNow();
}

void AuthoringPanel::publishDraftPlayback()
{
    if (onPublishDraftPlaybackRequested)
        onPublishDraftPlaybackRequested();

    refreshNow();
}

void AuthoringPanel::undoLastEdit()
{
    authoringSession.undo();
    refreshFromSession();
}

void AuthoringPanel::redoLastEdit()
{
    authoringSession.redo();
    refreshFromSession();
}

void AuthoringPanel::markSavedCheckpoint()
{
    authoringSession.markSaved();
    refreshFromSession();
}

void AuthoringPanel::applySelectedMacroEdit(const juce::String& label)
{
    const auto& macros = authoringSession.getProject().authoring.macros;
    if (selectedMacroIndex < 0 || static_cast<std::size_t>(selectedMacroIndex) >= macros.size())
        return;

    auto editedMacro = macros[static_cast<std::size_t>(selectedMacroIndex)];
    auto minValue = macroMinSlider.getValue();
    auto maxValue = macroMaxSlider.getValue();
    if (minValue > maxValue)
        std::swap(minValue, maxValue);

    editedMacro.minValue = minValue;
    editedMacro.maxValue = maxValue;
    editedMacro.defaultValue = std::clamp(macroDefaultSlider.getValue(), editedMacro.minValue, editedMacro.maxValue);

    if (editedMacro.targets.empty())
        editedMacro.targets.push_back({});

    const auto assignmentId = macroAssignmentSelector.getSelectedId();
    if (assignmentId > 0 && assignmentId <= static_cast<int>(curatedMacroAssignments.size()))
    {
        const auto& assignment = curatedMacroAssignments[static_cast<std::size_t>(assignmentId - 1)];
        editedMacro.targets.front().parameterId = assignment.parameterId;
        editedMacro.targets.front().parameterPath = assignment.parameterPath;
        if (editedMacro.targets.front().role.empty())
            editedMacro.targets.front().role = assignment.defaultRole;
    }

    auto selectedRoleText = macroRoleSelector.getText().toStdString();
    if (selectedRoleText.rfind("Custom: ", 0) == 0)
        selectedRoleText = selectedRoleText.substr(8);
    editedMacro.targets.front().role = selectedRoleText;

    authoringSession.updateMacro(static_cast<std::size_t>(selectedMacroIndex),
                                 editedMacro,
                                 label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::moveSelectedMacro(int direction)
{
    const auto result = authoringSession.moveMacro(static_cast<std::size_t>(selectedMacroIndex),
                                                   direction,
                                                   direction < 0 ? "Move macro earlier" : "Move macro later");
    if (result.applied)
        selectedMacroIndex = std::max(0, selectedMacroIndex + direction);

    refreshFromSession();
}

void AuthoringPanel::applySelectedFxSlotEdit(const juce::String& label)
{
    const auto& fxSlots = authoringSession.getProject().authoring.fxSlots;
    if (selectedFxSlotIndex < 0 || static_cast<std::size_t>(selectedFxSlotIndex) >= fxSlots.size())
        return;

    auto editedFxSlot = fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)];
    auto effectType = fxTypeSelector.getText().toStdString();
    if (effectType.rfind("Custom: ", 0) == 0)
        effectType = effectType.substr(8);
    editedFxSlot.effectType = effectType;
    editedFxSlot.bypassed = fxBypassedToggle.getToggleState();

    authoringSession.updateFxSlot(static_cast<std::size_t>(selectedFxSlotIndex),
                                  editedFxSlot,
                                  label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::applySelectedRoutingBusEdit(const juce::String& label)
{
    const auto& routingBuses = authoringSession.getProject().authoring.routingBuses;
    if (selectedRoutingBusIndex < 0
        || static_cast<std::size_t>(selectedRoutingBusIndex) >= routingBuses.size())
    {
        return;
    }

    auto editedRoutingBus = routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)];
    editedRoutingBus.inputSourceId = routingInputSelector.getText().toStdString();
    editedRoutingBus.fxSlotIds.clear();

    auto appendFxId = [&](const juce::ComboBox& selector)
    {
        const auto fxId = selector.getText().toStdString();
        if (fxId.empty() || fxId == "(none)")
            return;

        if (std::find(editedRoutingBus.fxSlotIds.begin(), editedRoutingBus.fxSlotIds.end(), fxId)
            == editedRoutingBus.fxSlotIds.end())
        {
            editedRoutingBus.fxSlotIds.push_back(fxId);
        }
    };

    appendFxId(routingInsertOneSelector);
    appendFxId(routingInsertTwoSelector);

    authoringSession.updateRoutingBus(static_cast<std::size_t>(selectedRoutingBusIndex),
                                      editedRoutingBus,
                                      label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::applySelectedTriggerSlotEdit(const juce::String& label)
{
    const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
    if (!selectedPerformanceBank.has_value()
        || selectedTriggerSlotIndex < 0
        || static_cast<std::size_t>(selectedTriggerSlotIndex) >= selectedPerformanceBank->triggerSlots.size())
    {
        return;
    }

    auto editedPerformanceBank = *selectedPerformanceBank;
    auto& triggerSlot = editedPerformanceBank.triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)];
    triggerSlot.triggerEvent = triggerEventSelector.getText().toStdString();
    triggerSlot.targetArticulationId = targetArticulationSelector.getText().toStdString();
    triggerSlot.phraseAssetId = phraseAssetSelector.getSelectedId() > 1 ? phraseAssetSelector.getText().toStdString() : std::string{};
    triggerSlot.chordMode = chordModeSelector.getText().toStdString();

    authoringSession.updatePerformanceBank(static_cast<std::size_t>(selectedPerformanceBankIndex),
                                           editedPerformanceBank,
                                           label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::importPhraseForSelectedBank()
{
    const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
    if (!selectedPerformanceBank.has_value())
        return;

    const auto midiPath = phraseImportPathEditor.getText().trim().toStdString();
    if (midiPath.empty())
    {
        phraseSummaryLabel.setText("Choose a MIDI file path before importing a phrase.",
                                   juce::dontSendNotification);
        updateDynamicAccessibleText(phraseSummaryLabel,
                                    phraseSummaryLabel.getText(),
                                    "Phrase summary: ");
        return;
    }

    const juce::File midiFile(midiPath);
    const auto phraseId = midiFile.getFileNameWithoutExtension().replaceCharacters(" ", "-").toLowerCase().toStdString();
    const auto importResult = importMidiPhraseAsset(midiPath,
                                                    phraseId,
                                                    midiFile.getFileNameWithoutExtension().toStdString());
    if (!importResult.imported)
    {
        phraseSummaryLabel.setText("Import failed: "
                                       + juce::String::fromUTF8(importResult.issues.empty()
                                                                    ? importResult.state.c_str()
                                                                    : importResult.issues.front().c_str()),
                                   juce::dontSendNotification);
        updateDynamicAccessibleText(phraseSummaryLabel,
                                    phraseSummaryLabel.getText(),
                                    "Phrase summary: ");
        return;
    }

    auto editedPerformanceBank = *selectedPerformanceBank;
    const auto existingPhraseIterator = std::find_if(
        editedPerformanceBank.phraseAssets.begin(),
        editedPerformanceBank.phraseAssets.end(),
        [&](const auto& phraseAsset)
        {
            return phraseAsset.id == importResult.phraseAsset.id;
        });

    if (existingPhraseIterator == editedPerformanceBank.phraseAssets.end())
        editedPerformanceBank.phraseAssets.push_back(importResult.phraseAsset);
    else
        *existingPhraseIterator = importResult.phraseAsset;

    if (selectedTriggerSlotIndex >= 0
        && static_cast<std::size_t>(selectedTriggerSlotIndex) < editedPerformanceBank.triggerSlots.size())
    {
        auto& triggerSlot = editedPerformanceBank.triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)];
        if (triggerSlot.triggerEvent == "phrase-trigger")
        {
            triggerSlot.phraseAssetId = importResult.phraseAsset.id;
            if (triggerSlot.chordMode.empty())
                triggerSlot.chordMode = "follow-root";
        }
    }

    authoringSession.updatePerformanceBank(static_cast<std::size_t>(selectedPerformanceBankIndex),
                                           editedPerformanceBank,
                                           "Import performance phrase");
    phraseSummaryLabel.setText(
        juce::String::fromUTF8(importResult.phraseAsset.displayName.c_str())
            + " imported | notes=" + juce::String(static_cast<int>(importResult.phraseAsset.notes.size()))
            + " | chord=" + juce::String::fromUTF8(importResult.phraseAsset.chordHint.c_str()),
        juce::dontSendNotification);
    updateDynamicAccessibleText(phraseSummaryLabel,
                                phraseSummaryLabel.getText(),
                                "Phrase summary: ");
    refreshFromSession();
}
} // namespace drs::app
