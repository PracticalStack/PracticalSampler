#include "shared/AuthoringPanel.h"

#include <algorithm>

namespace drs::app
{
namespace
{
const auto authoringPanelBackground = juce::Colour::fromRGB(18, 24, 29);
const auto authoringPanelCard = juce::Colour::fromRGB(250, 247, 240);
const auto authoringPanelAccent = juce::Colour::fromRGB(181, 96, 21);
const auto authoringPanelMuted = juce::Colour::fromRGB(82, 86, 94);
const auto authoringPanelSelected = juce::Colour::fromRGB(28, 108, 88);
const auto authoringPanelGrid = juce::Colour::fromRGB(230, 220, 207);

void configureEditorSlider(juce::Slider& slider,
                           double minValue,
                           double maxValue,
                           double interval)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 24);
    slider.setRange(minValue, maxValue, interval);
}

juce::String formatZoneRange(const drs::engine::AuthoringZoneSummary& zone)
{
    return "Keys " + juce::String(zone.keyLow) + "-" + juce::String(zone.keyHigh)
        + " | Vel " + juce::String(zone.velocityLow) + "-" + juce::String(zone.velocityHigh);
}

juce::String formatMicros(std::uint64_t micros)
{
    if (micros >= 1000)
        return juce::String(static_cast<double>(micros) / 1000.0, 2) + " ms";

    return juce::String(static_cast<int>(micros)) + " us";
}
} // namespace

void AuthoringPanel::ZoneMapComponent::setZoneSummaries(std::vector<drs::engine::AuthoringZoneSummary> summaries)
{
    zoneSummaries = std::move(summaries);
    repaint();
}

void AuthoringPanel::ZoneMapComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colours::transparentBlack);
    g.setColour(authoringPanelGrid);
    g.fillRoundedRectangle(bounds, 14.0f);

    const auto inner = bounds.reduced(12.0f);
    g.setColour(juce::Colour::fromRGBA(24, 29, 33, 24));

    for (int key = 0; key <= 8; ++key)
    {
        const auto x = inner.getX() + (inner.getWidth() * static_cast<float>(key) / 8.0f);
        g.drawVerticalLine(static_cast<int>(x), inner.getY(), inner.getBottom());
    }

    for (int velocity = 0; velocity <= 4; ++velocity)
    {
        const auto y = inner.getY() + (inner.getHeight() * static_cast<float>(velocity) / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), inner.getX(), inner.getRight());
    }

    for (const auto& zone : zoneSummaries)
    {
        const auto x = inner.getX() + inner.getWidth() * (static_cast<float>(zone.keyLow) / 127.0f);
        const auto width = std::max(10.0f,
                                    inner.getWidth() * (static_cast<float>(zone.keyHigh - zone.keyLow + 1) / 128.0f));
        const auto normalizedVelocityLow = 1.0f - (static_cast<float>(zone.velocityHigh) / 127.0f);
        const auto normalizedVelocityHigh = 1.0f - (static_cast<float>(zone.velocityLow) / 127.0f);
        const auto y = inner.getY() + inner.getHeight() * normalizedVelocityLow;
        const auto height = std::max(14.0f, inner.getHeight() * (normalizedVelocityHigh - normalizedVelocityLow));

        const juce::Rectangle<float> zoneBounds(x, y, width, height);
        g.setColour(zone.selected ? authoringPanelSelected : authoringPanelAccent.withMultipliedAlpha(0.72f));
        g.fillRoundedRectangle(zoneBounds, 8.0f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawFittedText(juce::String::fromUTF8(zone.displayName.c_str()),
                         zoneBounds.toNearestInt().reduced(6, 4),
                         juce::Justification::centredLeft,
                         1);
    }
}

AuthoringPanel::AuthoringPanel(drs::engine::AuthoringSession& session,
                               WaveformPreviewProvider previewProvider,
                               ImportResponsivenessProvider responsivenessProvider,
                               NotePreviewStartedCallback notePreviewStarted,
                               NotePreviewEndedCallback notePreviewEnded)
    : authoringSession(session),
      waveformPreviewProvider(std::move(previewProvider)),
      importResponsivenessProvider(std::move(responsivenessProvider)),
      onNotePreviewStarted(std::move(notePreviewStarted)),
      onNotePreviewEnded(std::move(notePreviewEnded))
{
    titleLabel.setText("Phase 2 Mapping Workspace", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    statusLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    sourceLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    articulationLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    waveformLabel.setText("Waveform Preview", juce::dontSendNotification);
    waveformLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    waveformLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    waveformInfoLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    loopInfoLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    importMetricsLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    zoneLabel.setText("Selected Zone", juce::dontSendNotification);
    zoneLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    zoneLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));

    configureEditorSlider(rootKeySlider, 0, 127, 1);
    configureEditorSlider(keyLowSlider, 0, 127, 1);
    configureEditorSlider(keyHighSlider, 0, 127, 1);
    configureEditorSlider(velocityLowSlider, 1, 127, 1);
    configureEditorSlider(velocityHighSlider, 1, 127, 1);
    configureEditorSlider(gainSlider, -24.0, 12.0, 0.1);
    configureEditorSlider(panSlider, -1.0, 1.0, 0.01);

    rootKeyLabel.setText("Root Key", juce::dontSendNotification);
    keyLowLabel.setText("Key Low", juce::dontSendNotification);
    keyHighLabel.setText("Key High", juce::dontSendNotification);
    velocityLowLabel.setText("Velocity Low", juce::dontSendNotification);
    velocityHighLabel.setText("Velocity High", juce::dontSendNotification);
    gainLabel.setText("Gain (dB)", juce::dontSendNotification);
    panLabel.setText("Pan", juce::dontSendNotification);

    for (auto* label : { &rootKeyLabel, &keyLowLabel, &keyHighLabel, &velocityLowLabel, &velocityHighLabel, &gainLabel, &panLabel })
    {
        label->setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
        label->setFont(juce::FontOptions(14.0f, juce::Font::bold));
    }

    zoneSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        const auto zoneId = zoneSelector.getSelectedId() - 1;
        const auto zones = authoringSession.getZoneSummaries();
        if (zoneId < 0 || static_cast<std::size_t>(zoneId) >= zones.size())
            return;

        authoringSession.selectZone(zones[static_cast<std::size_t>(zoneId)].id);
        refreshFromSession();
    };

    auto bindCommitOnDragEnd = [this](juce::Slider& slider, const juce::String& label)
    {
        slider.onDragEnd = [this, label]
        {
            applySelectedZoneEdit(label);
        };
    };

    bindCommitOnDragEnd(rootKeySlider, "Update zone root key");
    bindCommitOnDragEnd(keyLowSlider, "Update zone key range");
    bindCommitOnDragEnd(keyHighSlider, "Update zone key range");
    bindCommitOnDragEnd(velocityLowSlider, "Update zone velocity range");
    bindCommitOnDragEnd(velocityHighSlider, "Update zone velocity range");
    bindCommitOnDragEnd(gainSlider, "Update zone gain");
    bindCommitOnDragEnd(panSlider, "Update zone pan");

    loopEnabledToggle.setButtonText("Loop Enabled");
    zoneSelector.setComponentID("authoringZoneSelector");
    zoneMap.setComponentID("authoringZoneMap");
    waveformPreview.setComponentID("authoringWaveformPreview");
    previewButton.setComponentID("authoringPreviewButton");
    undoButton.setComponentID("authoringUndoButton");
    redoButton.setComponentID("authoringRedoButton");
    saveCheckpointButton.setComponentID("authoringSaveButton");
    loopEnabledToggle.onClick = [this]
    {
        if (isRefreshing)
            return;

        applySelectedZoneEdit("Toggle zone loop");
    };

    previewButton.setButtonText("Preview Selected Zone");
    previewButton.onClick = [this]
    {
        const auto request = authoringSession.buildSelectedZonePreviewRequest();
        if (!request.available)
            return;

        if (onNotePreviewStarted)
            onNotePreviewStarted(request.midiNote, static_cast<float>(request.velocity) / 127.0f);

        if (onNotePreviewEnded)
        {
            juce::Timer::callAfterDelay(180,
                                        [callback = onNotePreviewEnded, midiNote = request.midiNote]()
                                        {
                                            callback(midiNote);
                                        });
        }
    };

    undoButton.setButtonText("Undo");
    undoButton.onClick = [this]
    {
        authoringSession.undo();
        refreshFromSession();
    };

    redoButton.setButtonText("Redo");
    redoButton.onClick = [this]
    {
        authoringSession.redo();
        refreshFromSession();
    };

    saveCheckpointButton.setButtonText("Mark Saved");
    saveCheckpointButton.onClick = [this]
    {
        authoringSession.markSaved();
        refreshFromSession();
    };

    for (auto* component : {
             static_cast<juce::Component*>(&titleLabel),
             static_cast<juce::Component*>(&statusLabel),
             static_cast<juce::Component*>(&sourceLabel),
             static_cast<juce::Component*>(&articulationLabel),
             static_cast<juce::Component*>(&waveformLabel),
             static_cast<juce::Component*>(&waveformInfoLabel),
             static_cast<juce::Component*>(&loopInfoLabel),
             static_cast<juce::Component*>(&importMetricsLabel),
             static_cast<juce::Component*>(&zoneLabel),
             static_cast<juce::Component*>(&zoneSelector),
             static_cast<juce::Component*>(&zoneMap),
             static_cast<juce::Component*>(&waveformPreview),
             static_cast<juce::Component*>(&rootKeySlider),
             static_cast<juce::Component*>(&keyLowSlider),
             static_cast<juce::Component*>(&keyHighSlider),
             static_cast<juce::Component*>(&velocityLowSlider),
             static_cast<juce::Component*>(&velocityHighSlider),
             static_cast<juce::Component*>(&gainSlider),
             static_cast<juce::Component*>(&panSlider),
             static_cast<juce::Component*>(&rootKeyLabel),
             static_cast<juce::Component*>(&keyLowLabel),
             static_cast<juce::Component*>(&keyHighLabel),
             static_cast<juce::Component*>(&velocityLowLabel),
             static_cast<juce::Component*>(&velocityHighLabel),
             static_cast<juce::Component*>(&gainLabel),
             static_cast<juce::Component*>(&panLabel),
             static_cast<juce::Component*>(&loopEnabledToggle),
             static_cast<juce::Component*>(&previewButton),
             static_cast<juce::Component*>(&undoButton),
             static_cast<juce::Component*>(&redoButton),
             static_cast<juce::Component*>(&saveCheckpointButton)
         })
    {
        addAndMakeVisible(component);
    }

    refreshFromSession();
}

void AuthoringPanel::paint(juce::Graphics& g)
{
    g.fillAll(authoringPanelBackground);

    auto bounds = getLocalBounds().toFloat().reduced(14.0f);
    g.setColour(authoringPanelAccent.withAlpha(0.22f));
    g.fillRoundedRectangle(bounds, 20.0f);

    g.setColour(authoringPanelCard);
    g.fillRoundedRectangle(bounds.reduced(4.0f), 18.0f);
}

void AuthoringPanel::WaveformPreviewComponent::setPreview(AuthoringWaveformPreview nextPreview)
{
    preview = std::move(nextPreview);
    repaint();
}

void AuthoringPanel::WaveformPreviewComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(authoringPanelGrid);
    g.fillRoundedRectangle(bounds, 14.0f);

    const auto inner = bounds.reduced(12.0f);
    g.setColour(juce::Colour::fromRGBA(24, 29, 33, 42));
    g.drawHorizontalLine(static_cast<int>(inner.getCentreY()), inner.getX(), inner.getRight());

    if (!preview.available || preview.points.empty())
    {
        g.setColour(authoringPanelMuted);
        g.drawFittedText(preview.state.empty() ? "Waveform unavailable" : juce::String::fromUTF8(preview.state.c_str()),
                         getLocalBounds().reduced(12),
                         juce::Justification::centred,
                         2);
        return;
    }

    juce::Path waveformPath;
    const auto widthPerPoint = inner.getWidth() / static_cast<float>(preview.points.size());
    for (std::size_t index = 0; index < preview.points.size(); ++index)
    {
        const auto x = inner.getX() + (static_cast<float>(index) + 0.5f) * widthPerPoint;
        const auto minY = juce::jmap(preview.points[index].minValue, -1.0f, 1.0f, inner.getBottom(), inner.getY());
        const auto maxY = juce::jmap(preview.points[index].maxValue, -1.0f, 1.0f, inner.getBottom(), inner.getY());
        waveformPath.startNewSubPath(x, minY);
        waveformPath.lineTo(x, maxY);
    }

    g.setColour(authoringPanelSelected);
    g.strokePath(waveformPath, juce::PathStrokeType(1.3f));

    if (preview.loopEnabled && preview.frameCount > 0)
    {
        const auto startX = inner.getX() + inner.getWidth() * (static_cast<float>(preview.loopStartFrame) / static_cast<float>(preview.frameCount));
        const auto endX = inner.getX() + inner.getWidth() * (static_cast<float>(preview.loopEndFrame) / static_cast<float>(preview.frameCount));
        g.setColour(authoringPanelAccent);
        g.drawVerticalLine(static_cast<int>(startX), inner.getY(), inner.getBottom());
        g.drawVerticalLine(static_cast<int>(endX), inner.getY(), inner.getBottom());
    }
}

void AuthoringPanel::resized()
{
    auto area = getLocalBounds().reduced(28);

    auto hero = area.removeFromTop(76);
    auto heroLeft = hero.removeFromLeft(hero.proportionOfWidth(0.62f));
    titleLabel.setBounds(heroLeft.removeFromTop(30));
    heroLeft.removeFromTop(6);
    statusLabel.setBounds(heroLeft.removeFromTop(20));
    sourceLabel.setBounds(heroLeft.removeFromTop(20));
    articulationLabel.setBounds(heroLeft.removeFromTop(20));

    auto heroButtons = hero.removeFromRight(320);
    auto topRow = heroButtons.removeFromTop(28);
    undoButton.setBounds(topRow.removeFromLeft(92));
    topRow.removeFromLeft(8);
    redoButton.setBounds(topRow.removeFromLeft(92));
    topRow.removeFromLeft(8);
    saveCheckpointButton.setBounds(topRow.removeFromLeft(120));
    heroButtons.removeFromTop(10);
    previewButton.setBounds(heroButtons.removeFromTop(30));

    area.removeFromTop(12);
    zoneLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(6);
    zoneSelector.setBounds(area.removeFromTop(28).removeFromLeft(340));

    area.removeFromTop(12);
    waveformLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(6);
    auto waveformArea = area.removeFromTop(150);
    waveformPreview.setBounds(waveformArea);
    area.removeFromTop(6);
    waveformInfoLabel.setBounds(area.removeFromTop(22));
    loopInfoLabel.setBounds(area.removeFromTop(22));
    importMetricsLabel.setBounds(area.removeFromTop(38));
    area.removeFromTop(12);
    auto mapArea = area.removeFromTop(170);
    zoneMap.setBounds(mapArea);

    area.removeFromTop(12);
    auto inspector = area;
    auto leftColumn = inspector.removeFromLeft(inspector.proportionOfWidth(0.5f));
    auto rightColumn = inspector;

    auto layoutSliderRow = [](juce::Rectangle<int>& column, juce::Label& label, juce::Slider& slider)
    {
        auto row = column.removeFromTop(30);
        label.setBounds(row.removeFromLeft(120));
        slider.setBounds(row);
        column.removeFromTop(8);
    };

    layoutSliderRow(leftColumn, rootKeyLabel, rootKeySlider);
    layoutSliderRow(leftColumn, keyLowLabel, keyLowSlider);
    layoutSliderRow(leftColumn, keyHighLabel, keyHighSlider);
    layoutSliderRow(leftColumn, velocityLowLabel, velocityLowSlider);
    layoutSliderRow(leftColumn, velocityHighLabel, velocityHighSlider);

    layoutSliderRow(rightColumn, gainLabel, gainSlider);
    layoutSliderRow(rightColumn, panLabel, panSlider);
    loopEnabledToggle.setBounds(rightColumn.removeFromTop(28));
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

void AuthoringPanel::refreshFromSession()
{
    const juce::ScopedValueSetter<bool> refreshGuard(isRefreshing, true);

    rebuildZoneSelector();
    zoneMap.setZoneSummaries(authoringSession.getZoneSummaries());

    const auto& documentState = authoringSession.getDocumentState();
    statusLabel.setText("Revision " + juce::String(static_cast<int>(documentState.revision))
                            + " | dirty=" + juce::String(documentState.dirty ? "yes" : "no")
                            + " | undo=" + juce::String(static_cast<int>(documentState.undoDepth))
                            + " | redo=" + juce::String(static_cast<int>(documentState.redoDepth)),
                        juce::dontSendNotification);

    if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
    {
        sourceLabel.setText("Sample source: " + juce::String::fromUTF8(zone->sampleSourceId.c_str()),
                            juce::dontSendNotification);
        articulationLabel.setText("Articulation: " + juce::String::fromUTF8(zone->articulationId.c_str()),
                                  juce::dontSendNotification);

        rootKeySlider.setValue(zone->rootKey, juce::dontSendNotification);
        keyLowSlider.setValue(zone->keyLow, juce::dontSendNotification);
        keyHighSlider.setValue(zone->keyHigh, juce::dontSendNotification);
        velocityLowSlider.setValue(zone->velocityLow, juce::dontSendNotification);
        velocityHighSlider.setValue(zone->velocityHigh, juce::dontSendNotification);
        gainSlider.setValue(zone->gainDb, juce::dontSendNotification);
        panSlider.setValue(zone->pan, juce::dontSendNotification);
        loopEnabledToggle.setToggleState(zone->loopEnabled, juce::dontSendNotification);
        previewButton.setEnabled(true);
    }
    else
    {
        sourceLabel.setText("Sample source: none", juce::dontSendNotification);
        articulationLabel.setText("Articulation: none", juce::dontSendNotification);
        previewButton.setEnabled(false);
    }

    if (waveformPreviewProvider)
    {
        const auto preview = waveformPreviewProvider();
        waveformPreview.setPreview(preview);
        waveformInfoLabel.setText(
            preview.available
                ? "Source " + juce::String::fromUTF8(preview.formatName.c_str())
                    + " | " + juce::String(static_cast<int>(preview.sampleRate)) + " Hz"
                    + " | " + juce::String(static_cast<int>(preview.channelCount)) + " ch"
                    + " | " + juce::String(preview.durationSeconds, 3) + " s"
                : "Waveform: " + juce::String::fromUTF8(preview.state.c_str()),
            juce::dontSendNotification);
        loopInfoLabel.setText(
            preview.available
                ? (preview.loopEnabled
                       ? "Loop " + juce::String(static_cast<int>(preview.loopStartFrame))
                           + " - " + juce::String(static_cast<int>(preview.loopEndFrame))
                       : "Loop disabled for selected zone")
                : "Loop metadata unavailable",
            juce::dontSendNotification);
    }

    if (importResponsivenessProvider)
    {
        const auto metrics = importResponsivenessProvider();
        importMetricsLabel.setText(
            metrics.available
                ? "Import responsiveness: items=" + juce::String(static_cast<int>(metrics.totalItemCount))
                    + ", processed=" + juce::String(static_cast<int>(metrics.processedCount))
                    + ", warnings=" + juce::String(static_cast<int>(metrics.warningItemCount))
                    + ", failures=" + juce::String(static_cast<int>(metrics.failedItemCount))
                    + " | last=" + formatMicros(metrics.lastProcessDurationMicros)
                    + ", avg=" + formatMicros(metrics.averageProcessDurationMicros)
                    + ", max=" + formatMicros(metrics.maxProcessDurationMicros)
                : "Import responsiveness unavailable",
            juce::dontSendNotification);
    }

    undoButton.setEnabled(documentState.undoDepth > 0);
    redoButton.setEnabled(documentState.redoDepth > 0);
}

void AuthoringPanel::applySelectedZoneEdit(const juce::String& label)
{
    const auto currentZone = authoringSession.getSelectedZone();
    if (!currentZone.has_value())
        return;

    auto editedZone = *currentZone;
    editedZone.rootKey = static_cast<int>(rootKeySlider.getValue());
    editedZone.keyLow = static_cast<int>(keyLowSlider.getValue());
    editedZone.keyHigh = static_cast<int>(keyHighSlider.getValue());
    editedZone.velocityLow = static_cast<int>(velocityLowSlider.getValue());
    editedZone.velocityHigh = static_cast<int>(velocityHighSlider.getValue());
    editedZone.gainDb = gainSlider.getValue();
    editedZone.pan = panSlider.getValue();
    editedZone.loopEnabled = loopEnabledToggle.getToggleState();

    authoringSession.updateSelectedZone(editedZone, label.toStdString());
    refreshFromSession();
}
} // namespace drs::app
