#include "shared/StatusPanel.h"

#include "drs/engine/RuntimeLoader.h"

#include <filesystem>

namespace drs::app
{
namespace
{
juce::String toBulletList(const std::vector<std::string>& lines)
{
    juce::String result;

    for (const auto& line : lines)
        result << juce::String::fromUTF8(line.c_str()) << "\n";

    return result.trimEnd();
}

juce::String joinLines(const std::vector<std::string>& lines)
{
    juce::String result;

    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        if (index != 0)
            result << " | ";

        result << juce::String::fromUTF8(lines[index].c_str());
    }

    return result;
}
} // namespace

StatusPanel::StatusPanel(drs::engine::EngineFacade& facade,
                         MacroValueChangedCallback macroValueChanged)
    : engineFacade(facade),
      onMacroValueChanged(std::move(macroValueChanged))
{
    titleLabel.setText("Engine Status", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));

    modeLabel.setJustificationType(juce::Justification::centredLeft);
    stateLabel.setJustificationType(juce::Justification::centredLeft);
    diagnosticsHeadlineLabel.setJustificationType(juce::Justification::centredLeft);
    sessionLabel.setJustificationType(juce::Justification::centredLeft);
    voicesLabel.setJustificationType(juce::Justification::centredLeft);
    cacheLabel.setJustificationType(juce::Justification::centredLeft);
    latencyLabel.setJustificationType(juce::Justification::centredLeft);
    failureLabel.setJustificationType(juce::Justification::centredLeft);
    routedZonesLabel.setJustificationType(juce::Justification::centredLeft);

    diagnosticsHeadlineLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    actionsLabel.setText("Developer actions", juce::dontSendNotification);
    actionsLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    draftPlaybackLabel.setText("Draft playback", juce::dontSendNotification);
    draftPlaybackLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    macrosLabel.setText("Macro bridge", juce::dontSendNotification);
    macrosLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    contentProbeLabel.setText("Content probes", juce::dontSendNotification);
    contentProbeLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));

    resetStateButton.setButtonText("Reset Default State");
    loadLeadFixtureButton.setButtonText("Load Lead Fixture");
    injectInvalidStateButton.setButtonText("Inject Invalid State");
    stageDraftButton.setButtonText("Stage Draft");
    preparePreviewButton.setButtonText("Prepare Preview");
    publishDraftButton.setButtonText("Publish Draft");
    probeMissingContentButton.setButtonText("Probe Missing");
    probeBadChecksumButton.setButtonText("Probe Checksum");
    probeSchemaMismatchButton.setButtonText("Probe Schema");
    probePartialArtifactButton.setButtonText("Probe Partial");
    clearProbeButton.setButtonText("Clear Probe");

    resetStateButton.onClick = [this]
    {
        engineFacade.resetSessionStateToDefault();
        refreshSnapshot();
    };

    loadLeadFixtureButton.onClick = [this]
    {
        namespace fs = std::filesystem;
        const auto presetPath = fs::path(drs::engine::getPhase1RuntimeRootPath())
            / "preset-state"
            / "reference"
            / "lead-performance-state.drpreset.json";
        engineFacade.restorePresetStateFile(presetPath.generic_string());
        refreshSnapshot();
    };

    injectInvalidStateButton.onClick = [this]
    {
        namespace fs = std::filesystem;
        const auto presetPath = fs::path(drs::engine::getPhase1RuntimeRootPath())
            / "preset-state"
            / "negative"
            / "transient-diagnostics-leak.drpreset.json";
        engineFacade.restorePresetStateFile(presetPath.generic_string());
        refreshSnapshot();
    };

    stageDraftButton.onClick = [this]
    {
        const auto nextRevision = engineFacade.getDraftPlaybackStatus().draftRevision + 1;
        engineFacade.stageDraftRevision(nextRevision);
        refreshSnapshot();
    };

    preparePreviewButton.onClick = [this]
    {
        engineFacade.refreshPreviewToCurrentDraft();
        refreshSnapshot();
    };

    publishDraftButton.onClick = [this]
    {
        engineFacade.publishCurrentDraft();
        refreshSnapshot();
    };

    probeMissingContentButton.onClick = [this]
    {
        engineFacade.probeContentFailure(drs::engine::EngineContentFailureCategory::missingContent);
        refreshSnapshot();
    };

    probeBadChecksumButton.onClick = [this]
    {
        engineFacade.probeContentFailure(drs::engine::EngineContentFailureCategory::badChecksum);
        refreshSnapshot();
    };

    probeSchemaMismatchButton.onClick = [this]
    {
        engineFacade.probeContentFailure(drs::engine::EngineContentFailureCategory::schemaMismatch);
        refreshSnapshot();
    };

    probePartialArtifactButton.onClick = [this]
    {
        engineFacade.probeContentFailure(drs::engine::EngineContentFailureCategory::partialCompiledArtifact);
        refreshSnapshot();
    };

    clearProbeButton.onClick = [this]
    {
        engineFacade.clearContentFailureProbe();
        refreshSnapshot();
    };

    detailEditor.setMultiLine(true);
    detailEditor.setReadOnly(true);
    detailEditor.setScrollbarsShown(true);
    detailEditor.setCaretVisible(false);
    detailEditor.setPopupMenuEnabled(false);

    nextStepsLabel.setText("Current next steps", juce::dontSendNotification);
    nextStepsLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));

    nextStepsEditor.setMultiLine(true);
    nextStepsEditor.setReadOnly(true);
    nextStepsEditor.setScrollbarsShown(true);
    nextStepsEditor.setCaretVisible(false);
    nextStepsEditor.setPopupMenuEnabled(false);

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(modeLabel);
    addAndMakeVisible(stateLabel);
    addAndMakeVisible(diagnosticsHeadlineLabel);
    addAndMakeVisible(sessionLabel);
    addAndMakeVisible(voicesLabel);
    addAndMakeVisible(cacheLabel);
    addAndMakeVisible(latencyLabel);
    addAndMakeVisible(failureLabel);
    addAndMakeVisible(routedZonesLabel);
    addAndMakeVisible(actionsLabel);
    addAndMakeVisible(draftPlaybackLabel);
    addAndMakeVisible(macrosLabel);
    addAndMakeVisible(resetStateButton);
    addAndMakeVisible(loadLeadFixtureButton);
    addAndMakeVisible(injectInvalidStateButton);
    addAndMakeVisible(stageDraftButton);
    addAndMakeVisible(preparePreviewButton);
    addAndMakeVisible(publishDraftButton);
    addAndMakeVisible(contentProbeLabel);
    addAndMakeVisible(probeMissingContentButton);
    addAndMakeVisible(probeBadChecksumButton);
    addAndMakeVisible(probeSchemaMismatchButton);
    addAndMakeVisible(probePartialArtifactButton);
    addAndMakeVisible(clearProbeButton);
    addAndMakeVisible(detailEditor);
    addAndMakeVisible(nextStepsLabel);
    addAndMakeVisible(nextStepsEditor);

    rebuildMacroControls();
    refreshSnapshot();
    startTimerHz(2);
}

void StatusPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll(juce::Colour::fromRGB(18, 22, 28));

    g.setColour(juce::Colour::fromRGB(47, 84, 235));
    g.fillRoundedRectangle(bounds.reduced(12.0f), 18.0f);

    g.setColour(juce::Colour::fromRGB(247, 249, 252));
    g.fillRoundedRectangle(bounds.reduced(16.0f), 14.0f);
}

void StatusPanel::resized()
{
    auto area = getLocalBounds().reduced(32);

    titleLabel.setBounds(area.removeFromTop(32));
    area.removeFromTop(8);
    modeLabel.setBounds(area.removeFromTop(24));
    stateLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(12);
    diagnosticsHeadlineLabel.setBounds(area.removeFromTop(24));
    sessionLabel.setBounds(area.removeFromTop(22));
    voicesLabel.setBounds(area.removeFromTop(22));
    cacheLabel.setBounds(area.removeFromTop(22));
    latencyLabel.setBounds(area.removeFromTop(22));
    failureLabel.setBounds(area.removeFromTop(22));
    routedZonesLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(10);

    auto actionsRow = area.removeFromTop(28);
    actionsLabel.setBounds(actionsRow.removeFromLeft(140));
    auto buttonRow = actionsRow;
    resetStateButton.setBounds(buttonRow.removeFromLeft(150));
    buttonRow.removeFromLeft(10);
    loadLeadFixtureButton.setBounds(buttonRow.removeFromLeft(140));
    buttonRow.removeFromLeft(10);
    injectInvalidStateButton.setBounds(buttonRow.removeFromLeft(150));

    area.removeFromTop(12);
    auto draftPlaybackRow = area.removeFromTop(28);
    draftPlaybackLabel.setBounds(draftPlaybackRow.removeFromLeft(120));
    auto draftButtons = draftPlaybackRow;
    stageDraftButton.setBounds(draftButtons.removeFromLeft(110));
    draftButtons.removeFromLeft(8);
    preparePreviewButton.setBounds(draftButtons.removeFromLeft(130));
    draftButtons.removeFromLeft(8);
    publishDraftButton.setBounds(draftButtons.removeFromLeft(120));

    area.removeFromTop(12);
    auto macroHeaderRow = area.removeFromTop(24);
    macrosLabel.setBounds(macroHeaderRow.removeFromLeft(120));
    area.removeFromTop(6);

    for (auto& macroControl : macroControls)
    {
        auto macroRow = area.removeFromTop(28);
        macroControl->nameLabel.setBounds(macroRow.removeFromLeft(110));
        macroControl->slider.setBounds(macroRow.removeFromLeft(260));
        macroRow.removeFromLeft(10);
        macroControl->valueLabel.setBounds(macroRow.removeFromLeft(90));
        area.removeFromTop(6);
    }

    area.removeFromTop(6);
    auto contentProbeRow = area.removeFromTop(28);
    contentProbeLabel.setBounds(contentProbeRow.removeFromLeft(120));
    auto contentProbeButtons = contentProbeRow;
    probeMissingContentButton.setBounds(contentProbeButtons.removeFromLeft(110));
    contentProbeButtons.removeFromLeft(8);
    probeBadChecksumButton.setBounds(contentProbeButtons.removeFromLeft(120));
    contentProbeButtons.removeFromLeft(8);
    probeSchemaMismatchButton.setBounds(contentProbeButtons.removeFromLeft(110));
    contentProbeButtons.removeFromLeft(8);
    probePartialArtifactButton.setBounds(contentProbeButtons.removeFromLeft(110));
    contentProbeButtons.removeFromLeft(8);
    clearProbeButton.setBounds(contentProbeButtons.removeFromLeft(100));

    area.removeFromTop(12);
    detailEditor.setBounds(area.removeFromTop(170));
    area.removeFromTop(12);
    nextStepsLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    nextStepsEditor.setBounds(area);
}

void StatusPanel::timerCallback()
{
    refreshSnapshot();
}

void StatusPanel::rebuildMacroControls()
{
    macroControls.clear();

    for (const auto& macro : engineFacade.getMacroDescriptors())
    {
        auto control = std::make_unique<MacroControl>();
        control->id = macro.id;
        control->minValue = macro.minValue;
        control->maxValue = macro.maxValue;
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

void StatusPanel::refreshSnapshot()
{
    snapshot = engineFacade.getStatusSnapshot();
    const auto& diagnostics = snapshot.diagnostics;
    const auto macros = engineFacade.getMacroDescriptors();

    modeLabel.setText("Mode: " + juce::String::fromUTF8(snapshot.mode.c_str()), juce::dontSendNotification);
    stateLabel.setText("State: " + juce::String::fromUTF8(snapshot.integrationState.c_str()), juce::dontSendNotification);
    diagnosticsHeadlineLabel.setText("Diagnostics: " + juce::String::fromUTF8(diagnostics.headline.c_str()),
                                     juce::dontSendNotification);
    sessionLabel.setText(
        "Session: preset=" + juce::String::fromUTF8(diagnostics.presetId.c_str())
            + " | loadProfile=" + juce::String::fromUTF8(diagnostics.loadProfileId.c_str())
            + " | articulation=" + juce::String::fromUTF8(diagnostics.selectedArticulationId.c_str())
            + " | draft=" + juce::String(static_cast<juce::int64>(diagnostics.draftRevision))
            + " | preview=" + juce::String(static_cast<juce::int64>(diagnostics.previewRevision))
            + " (" + juce::String::fromUTF8(diagnostics.previewRevisionState.c_str()) + ")"
            + " | published=" + juce::String(static_cast<juce::int64>(diagnostics.publishedRevision))
            + " (" + juce::String::fromUTF8(diagnostics.publishedRevisionState.c_str()) + ")",
        juce::dontSendNotification);
    voicesLabel.setText(
        "Voices: active=" + juce::String(static_cast<int>(diagnostics.activeVoiceCount))
            + " | peak=" + juce::String(static_cast<int>(diagnostics.peakActiveVoiceCount))
            + " | pageMisses=" + juce::String(static_cast<int>(diagnostics.pageMissCount))
            + " | backgroundReads=" + juce::String(static_cast<int>(diagnostics.backgroundReadCount)),
        juce::dontSendNotification);
    cacheLabel.setText(
        "Cache: budget=" + juce::String(static_cast<int>(diagnostics.configuredMaxCachedPages))
            + " pages | prefetch=" + juce::String(static_cast<juce::int64>(diagnostics.maxPrefetchBytesPerVoice))
            + " bytes | resident=" + juce::String(static_cast<int>(diagnostics.residentPageCount))
            + " | pending=" + juce::String(static_cast<int>(diagnostics.pendingPageCount))
            + " | evicted=" + juce::String(static_cast<int>(diagnostics.evictedPageCount))
            + " | lastPurgeEvictions=" + juce::String(static_cast<int>(diagnostics.lastPurgeEvictedPageCount)),
        juce::dontSendNotification);
    latencyLabel.setText(
        "Latency: avg=" + juce::String(static_cast<juce::int64>(diagnostics.averageReadLatencyMicros))
            + " us | max=" + juce::String(static_cast<juce::int64>(diagnostics.maxReadLatencyMicros))
            + " us | purgePasses=" + juce::String(static_cast<int>(diagnostics.purgePassCount))
            + " | dormantPurges=" + juce::String(static_cast<int>(diagnostics.dormantPurgeCount)),
        juce::dontSendNotification);

    const auto failureText = diagnostics.failureState.empty()
        ? juce::String("Failure state: none")
        : juce::String("Failure state: ") + juce::String::fromUTF8(diagnostics.failureState.c_str());
    failureLabel.setText(failureText, juce::dontSendNotification);
    failureLabel.setColour(juce::Label::textColourId,
                           diagnostics.failureState.empty()
                               ? juce::Colour::fromRGB(37, 99, 63)
                               : juce::Colour::fromRGB(166, 35, 35));
    routedZonesLabel.setText("Routed zones: " + joinLines(diagnostics.routedZones), juce::dontSendNotification);

    if (macros.size() != macroControls.size())
    {
        rebuildMacroControls();
        resized();
    }

    for (std::size_t index = 0; index < std::min(macros.size(), macroControls.size()); ++index)
    {
        auto& control = macroControls[index];
        const auto& macro = macros[index];
        control->nameLabel.setText(juce::String::fromUTF8(macro.name.c_str()), juce::dontSendNotification);
        control->slider.setRange(macro.minValue, macro.maxValue, 0.001);
        control->slider.setValue(macro.currentValue, juce::dontSendNotification);
        control->valueLabel.setText(juce::String(macro.currentValue, 3), juce::dontSendNotification);
    }

    detailEditor.setText(juce::String::fromUTF8(snapshot.detail.c_str()), false);
    nextStepsEditor.setText(toBulletList(snapshot.nextSteps), false);
}
} // namespace drs::app
