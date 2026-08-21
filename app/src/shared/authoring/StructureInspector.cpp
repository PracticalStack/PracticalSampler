#include "shared/authoring/StructureInspector.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace drs::app::authoring
{
namespace
{
std::string mixedOr(const std::vector<std::string>& values)
{
    if (values.empty()) return "—";
    if (std::all_of(values.begin(), values.end(), [&](const auto& value) { return value == values.front(); }))
        return values.front();
    return "Mixed";
}

template <typename Value, typename Formatter>
std::string collectField(const std::vector<const Value*>& values, Formatter formatter)
{
    std::vector<std::string> formatted;
    formatted.reserve(values.size());
    for (const auto* value : values)
        formatted.push_back(formatter(*value));
    return mixedOr(formatted);
}

std::string kindName(const StructureSelectionKind kind)
{
    if (kind == StructureSelectionKind::instrument) return "Instrument";
    if (kind == StructureSelectionKind::layer) return "Layer";
    if (kind == StructureSelectionKind::group) return "Group";
    if (kind == StructureSelectionKind::zone) return "Zone";
    return "Selection";
}
} // namespace

StructureInspectorSnapshot buildStructureInspectorSnapshot(
    const drs::engine::RuntimeProjectModel& project,
    const AuthoringStructureSelection& selection)
{
    StructureInspectorSnapshot result;
    result.kind = selection.getKind();
    result.selectedCount = static_cast<int>(selection.getIds().size());
    if (selection.getKind() == StructureSelectionKind::none || selection.getIds().empty())
        return result;

    if (selection.getKind() == StructureSelectionKind::instrument)
    {
        result.title = "Instrument";
        result.fields = {
            { "Layers", std::to_string(project.authoring.layers.size()) },
            { "Groups", std::to_string(project.authoring.groups.size()) },
            { "Zones", std::to_string(project.authoring.zones.size()) },
            { "Map scope", "Instrument" }
        };
    }
    else if (selection.getKind() == StructureSelectionKind::layer)
    {
        std::vector<const drs::engine::RuntimeProjectLayerDefinition*> layers;
        for (const auto& layer : project.authoring.layers)
            if (selection.contains(layer.id)) layers.push_back(&layer);
        if (!layers.empty())
        {
            result.title = kindName(result.kind) + ": " + (selection.getPrimaryId().empty()
                ? layers.front()->displayName : std::string { [&]() { for (const auto* l : layers) if (l->id == selection.getPrimaryId()) return l->displayName; return layers.front()->displayName; }() });
            result.fields = {
                { "Selected", std::to_string(layers.size()) },
                { "Name", collectField(layers, [](const auto& value) { return value.displayName; }) },
                { "Order", collectField(layers, [](const auto& value) { return std::to_string(value.displayOrder); }) },
                { "Visibility", collectField(layers, [](const auto& value) { return value.workspaceVisible ? "Visible" : "Hidden"; }) },
                { "Gain", collectField(layers, [](const auto& value) { return std::to_string(value.gainDb) + " dB"; }) },
                { "Pan", collectField(layers, [](const auto& value) { return std::to_string(value.pan) + "%"; }) },
                { "Routing", collectField(layers, [](const auto& value) { return value.routingBusId.empty() ? "Default" : value.routingBusId; }) },
                { "Audition anchor", collectField(layers, [](const auto& value) { return value.auditionAnchorGroupId.empty() ? "None" : value.auditionAnchorGroupId; }) },
                { "Crossfade", collectField(layers, [](const auto& value)
                    {
                        return std::to_string(value.crossfade.low) + "–" + std::to_string(value.crossfade.high);
                    }) },
                { "Effective summary", std::to_string(std::count_if(project.authoring.groups.begin(), project.authoring.groups.end(), [&](const auto& group) { return std::any_of(layers.begin(), layers.end(), [&](const auto* layer) { return group.layerId == layer->id; }); })) + " groups" }
            };
        }
    }
    else if (selection.getKind() == StructureSelectionKind::group)
    {
        std::vector<const drs::engine::RuntimeProjectGroupDefinition*> groups;
        for (const auto& group : project.authoring.groups)
            if (selection.contains(group.id)) groups.push_back(&group);
        if (!groups.empty())
        {
            result.title = kindName(result.kind) + ": " + groups.front()->displayName;
            result.fields = {
                { "Selected", std::to_string(groups.size()) },
                { "Name", collectField(groups, [](const auto& value) { return value.displayName; }) },
                { "Layer", collectField(groups, [](const auto& value) { return value.layerId; }) },
                { "Order", collectField(groups, [](const auto& value) { return std::to_string(value.displayOrder); }) },
                { "Visibility", collectField(groups, [](const auto& value) { return value.workspaceVisible ? "Visible" : "Hidden"; }) },
                { "Gain", collectField(groups, [](const auto& value) { return std::to_string(value.gainDb) + " dB"; }) },
                { "Pan", collectField(groups, [](const auto& value) { return std::to_string(value.pan) + "%"; }) },
                { "Routing", collectField(groups, [](const auto& value) { return value.routingBusId.empty() ? "Default" : value.routingBusId; }) },
                { "Audition anchor", collectField(groups, [](const auto& value) { return value.auditionAnchorZoneId.empty() ? "None" : value.auditionAnchorZoneId; }) },
                { "Round robin", std::to_string(std::count_if(project.authoring.zones.begin(), project.authoring.zones.end(), [&](const auto& zone)
                    {
                        return std::any_of(groups.begin(), groups.end(), [&](const auto* group) { return zone.groupId == group->id; })
                            && zone.roundRobinLength > 0;
                    })) + " zones" },
                { "Effective summary", std::to_string(std::count_if(project.authoring.zones.begin(), project.authoring.zones.end(), [&](const auto& zone) { return std::any_of(groups.begin(), groups.end(), [&](const auto* group) { return zone.groupId == group->id; }); })) + " zones" }
            };
        }
    }
    else
    {
        std::vector<const drs::engine::RuntimeProjectZoneDefinition*> zones;
        for (const auto& zone : project.authoring.zones)
            if (selection.contains(zone.id)) zones.push_back(&zone);
        if (!zones.empty())
        {
            result.title = kindName(result.kind) + ": " + zones.front()->displayName;
            result.fields = {
                { "Selected", std::to_string(zones.size()) },
                { "Name", collectField(zones, [](const auto& value) { return value.displayName; }) },
                { "Group", collectField(zones, [](const auto& value) { return value.groupId; }) },
                { "Key range", collectField(zones, [](const auto& value) { return std::to_string(value.keyLow) + "–" + std::to_string(value.keyHigh); }) },
                { "Velocity", collectField(zones, [](const auto& value) { return std::to_string(value.velocityLow) + "–" + std::to_string(value.velocityHigh); }) },
                { "Root key", collectField(zones, [](const auto& value) { return std::to_string(value.rootKey); }) },
                { "Articulation", collectField(zones, [](const auto& value) { return value.articulationId; }) },
                { "Round robin", collectField(zones, [](const auto& value) { return value.roundRobinLength > 0 ? std::to_string(value.roundRobinPosition) + "/" + std::to_string(value.roundRobinLength) : "—"; }) },
                { "Release", collectField(zones, [](const auto& value) { return std::to_string(value.releaseSeconds) + " s"; }) },
                { "Performance event", collectField(zones, [](const auto& value) { return std::to_string(static_cast<int>(value.performance.event)); }) }
            };
        }
    }
    return result;
}

StructureInspector::StructureInspector()
{
    setComponentID("authoringStructureInspector");
    setTitle("Structure inspector");
    setDescription("Context-sensitive inspector for the active layer, group, or zone selection.");
    header.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    selectionSummary.setFont(juce::FontOptions(11.0f));
    header.setColour(juce::Label::textColourId, visual::text);
    selectionSummary.setColour(juce::Label::textColourId, visual::textMuted);
    editHint.setText("Edit shared values", juce::dontSendNotification);
    editHint.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    editHint.setColour(juce::Label::textColourId, visual::textMuted);
    applyButton.setButtonText("Apply to selection");
    applyButton.setComponentID("authoringStructureInspectorApplyButton");
    primaryActionButton.setComponentID("authoringStructureInspectorPrimaryAction");
    secondaryActionButton.setComponentID("authoringStructureInspectorSecondaryAction");
    tertiaryActionButton.setComponentID("authoringStructureInspectorTertiaryAction");
    for (auto* editor : { &nameEditor, &gainEditor, &panEditor, &releaseEditor })
    {
        editor->setColour(juce::TextEditor::backgroundColourId, visual::surfaceRaised);
        editor->setColour(juce::TextEditor::outlineColourId, visual::border);
        editor->setColour(juce::TextEditor::textColourId, visual::text);
    }
    parentEditor.setColour(juce::TextEditor::backgroundColourId, visual::surfaceRaised);
    parentEditor.setColour(juce::TextEditor::outlineColourId, visual::border);
    parentEditor.setColour(juce::TextEditor::textColourId, visual::text);
    parentEditor.setComponentID("authoringStructureInspectorParentEditor");
    parentEditor.setTextToShowWhenEmpty("parent stable ID", visual::textMuted);
    visibilityToggle.setButtonText("Visible");
    visibilityToggle.setComponentID("authoringStructureInspectorVisibility");
    gainMinusButton.setButtonText("Gain −1");
    gainPlusButton.setButtonText("Gain +1");
    panMinusButton.setButtonText("Pan −1");
    panPlusButton.setButtonText("Pan +1");
    gainMinusButton.setComponentID("authoringStructureInspectorGainMinus");
    gainPlusButton.setComponentID("authoringStructureInspectorGainPlus");
    panMinusButton.setComponentID("authoringStructureInspectorPanMinus");
    panPlusButton.setComponentID("authoringStructureInspectorPanPlus");
    gainMinusButton.setTitle("Nudge gain down");
    gainPlusButton.setTitle("Nudge gain up");
    panMinusButton.setTitle("Nudge pan left");
    panPlusButton.setTitle("Nudge pan right");
    addAndMakeVisible(header);
    addAndMakeVisible(selectionSummary);
    addAndMakeVisible(editHint);
    addAndMakeVisible(nameEditor);
    addAndMakeVisible(parentEditor);
    addAndMakeVisible(gainEditor);
    addAndMakeVisible(panEditor);
    addAndMakeVisible(releaseEditor);
    addAndMakeVisible(visibilityToggle);
    addAndMakeVisible(gainMinusButton);
    addAndMakeVisible(gainPlusButton);
    addAndMakeVisible(panMinusButton);
    addAndMakeVisible(panPlusButton);
    addAndMakeVisible(applyButton);
    addAndMakeVisible(primaryActionButton);
    addAndMakeVisible(secondaryActionButton);
    addAndMakeVisible(tertiaryActionButton);
    const auto relativePatch = [this](const double gainDelta, const double panDelta)
    {
        if (onPatchRequested == nullptr || snapshot.kind == StructureSelectionKind::none)
            return;
        drs::engine::AuthoringStructureBatchPatch patch;
        if (gainDelta != 0.0) patch.gainDelta = gainDelta;
        if (panDelta != 0.0) patch.panDelta = panDelta;
        onPatchRequested(snapshot.kind, std::move(patch));
    };
    gainMinusButton.onClick = [relativePatch] { relativePatch(-1.0, 0.0); };
    gainPlusButton.onClick = [relativePatch] { relativePatch(1.0, 0.0); };
    panMinusButton.onClick = [relativePatch] { relativePatch(0.0, -1.0); };
    panPlusButton.onClick = [relativePatch] { relativePatch(0.0, 1.0); };
    applyButton.onClick = [this]
    {
        if (onPatchRequested == nullptr || snapshot.kind == StructureSelectionKind::none)
            return;
        drs::engine::AuthoringStructureBatchPatch patch;
        if (nameEditor.getText().isNotEmpty()) patch.displayName = nameEditor.getText().toStdString();
        if (parentEditor.isVisible() && parentEditor.getText().isNotEmpty())
        {
            if (snapshot.kind == StructureSelectionKind::group)
                patch.layerId = parentEditor.getText().toStdString();
            else if (snapshot.kind == StructureSelectionKind::zone)
                patch.groupId = parentEditor.getText().toStdString();
        }
        if (gainEditor.getText().isNotEmpty()) patch.gainDb = gainEditor.getText().getDoubleValue();
        if (panEditor.getText().isNotEmpty()) patch.pan = panEditor.getText().getDoubleValue();
        if (releaseEditor.isVisible() && releaseEditor.getText().isNotEmpty())
            patch.releaseSeconds = releaseEditor.getText().getDoubleValue();
        if (visibilityTouched)
            patch.workspaceVisible = visibilityToggle.getToggleState();
        onPatchRequested(snapshot.kind, std::move(patch));
    };
    visibilityToggle.onClick = [this] { visibilityTouched = true; };
    primaryActionButton.onClick = [this]
    {
        if (onActionRequested == nullptr)
            return;
        onActionRequested(StructureInspectorAction::showZones);
    };
    secondaryActionButton.onClick = [this]
    {
        if (onActionRequested == nullptr)
            return;
        if (snapshot.kind == StructureSelectionKind::zone)
            onActionRequested(StructureInspectorAction::openWaveform);
        else
            onActionRequested(StructureInspectorAction::selectVisibleChildren);
    };
    tertiaryActionButton.onClick = [this]
    {
        if (onActionRequested != nullptr && snapshot.kind != StructureSelectionKind::none)
            onActionRequested(StructureInspectorAction::audition);
    };
}

void StructureInspector::setSnapshot(StructureInspectorSnapshot nextSnapshot)
{
    snapshot = std::move(nextSnapshot);
    header.setText(juce::String::fromUTF8(snapshot.title.c_str()), juce::dontSendNotification);
    selectionSummary.setText(snapshot.selectedCount > 0
                                 ? juce::String(snapshot.selectedCount) + " selected · values support multi-selection"
                                 : "Select a layer, group, or zone to inspect it",
                             juce::dontSendNotification);
    std::unordered_map<std::string, std::string> fieldValues;
    for (const auto& field : snapshot.fields)
        fieldValues[field.first] = field.second;
    const auto valueOrBlank = [&](const std::string& key)
    {
        const auto value = fieldValues[key];
        return value == "Mixed" ? std::string {} : value;
    };
    const auto stripSuffix = [&](const std::string& value, const char suffix)
    {
        const auto position = value.find(suffix);
        return value.substr(0, position == std::string::npos ? value.size() : position);
    };
    nameEditor.setText(valueOrBlank("Name"), juce::dontSendNotification);
    const auto parentValue = snapshot.kind == StructureSelectionKind::group
        ? valueOrBlank("Layer") : valueOrBlank("Group");
    parentEditor.setText(parentValue, juce::dontSendNotification);
    gainEditor.setText(stripSuffix(valueOrBlank("Gain"), ' '), juce::dontSendNotification);
    panEditor.setText(stripSuffix(valueOrBlank("Pan"), '%'), juce::dontSendNotification);
    releaseEditor.setText(stripSuffix(valueOrBlank("Release"), ' '), juce::dontSendNotification);
    const auto editable = snapshot.kind == StructureSelectionKind::layer
        || snapshot.kind == StructureSelectionKind::group
        || snapshot.kind == StructureSelectionKind::zone;
    const auto actionable = snapshot.kind != StructureSelectionKind::none;
    visibilityTouched = false;
    const auto visibilityValue = fieldValues["Visibility"];
    visibilityToggle.setButtonText(visibilityValue == "Mixed" ? "Visibility: Mixed" : "Visible");
    visibilityToggle.setToggleState(visibilityValue == "Visible", juce::dontSendNotification);
    editHint.setText(editable ? "Edit shared values" : "Instrument actions", juce::dontSendNotification);
    editHint.setVisible(actionable);
    nameEditor.setVisible(editable);
    parentEditor.setVisible(editable && snapshot.kind != StructureSelectionKind::layer);
    gainEditor.setVisible(editable);
    panEditor.setVisible(editable);
    releaseEditor.setVisible(snapshot.kind == StructureSelectionKind::zone);
    visibilityToggle.setVisible(snapshot.kind != StructureSelectionKind::zone && editable);
    applyButton.setVisible(editable);
    gainMinusButton.setVisible(editable);
    gainPlusButton.setVisible(editable);
    panMinusButton.setVisible(editable);
    panPlusButton.setVisible(editable);
    primaryActionButton.setVisible(actionable);
    secondaryActionButton.setVisible(actionable);
    tertiaryActionButton.setVisible(actionable);
    if (snapshot.kind == StructureSelectionKind::zone)
    {
        primaryActionButton.setButtonText("Show Zones");
        secondaryActionButton.setButtonText("Open Waveform");
        tertiaryActionButton.setButtonText("Audition Zone");
    }
    else if (snapshot.kind == StructureSelectionKind::instrument)
    {
        primaryActionButton.setButtonText("Show Zones");
        secondaryActionButton.setButtonText("Select Layers");
        tertiaryActionButton.setButtonText("Audition Anchor");
    }
    else
    {
        primaryActionButton.setButtonText("Show Zones");
        secondaryActionButton.setButtonText(snapshot.kind == StructureSelectionKind::layer
                                                ? "Select visible Groups" : "Select visible Zones");
        tertiaryActionButton.setButtonText("Audition Anchor");
    }
    fieldLabels.clear();
    valueLabels.clear();
    for (const auto& [label, value] : snapshot.fields)
    {
        auto field = std::make_unique<juce::Label>();
        auto output = std::make_unique<juce::Label>();
        field->setText(juce::String::fromUTF8(label.c_str()), juce::dontSendNotification);
        output->setText(juce::String::fromUTF8(value.c_str()), juce::dontSendNotification);
        field->setColour(juce::Label::textColourId, visual::textMuted);
        output->setColour(juce::Label::textColourId, visual::text);
        addAndMakeVisible(*field);
        addAndMakeVisible(*output);
        fieldLabels.push_back(std::move(field));
        valueLabels.push_back(std::move(output));
    }
    resized();
}

void StructureInspector::resized()
{
    auto area = getLocalBounds().reduced(14);
    header.setBounds(area.removeFromTop(28));
    selectionSummary.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    for (std::size_t index = 0; index < fieldLabels.size(); ++index)
    {
        auto row = area.removeFromTop(26);
        fieldLabels[index]->setBounds(row.removeFromLeft(104));
        valueLabels[index]->setBounds(row);
    }
    if (editHint.isVisible())
    {
        area.removeFromTop(12);
        editHint.setBounds(area.removeFromTop(20));
        nameEditor.setBounds(area.removeFromTop(26));
        if (parentEditor.isVisible())
            parentEditor.setBounds(area.removeFromTop(26));
        gainEditor.setBounds(area.removeFromTop(26));
        panEditor.setBounds(area.removeFromTop(26));
        if (releaseEditor.isVisible())
            releaseEditor.setBounds(area.removeFromTop(26));
        if (visibilityToggle.isVisible())
            visibilityToggle.setBounds(area.removeFromTop(26));
        auto gainNudgeRow = area.removeFromTop(26);
        gainMinusButton.setBounds(gainNudgeRow.removeFromLeft(76));
        gainNudgeRow.removeFromLeft(4);
        gainPlusButton.setBounds(gainNudgeRow.removeFromLeft(76));
        auto panNudgeRow = area.removeFromTop(26);
        panMinusButton.setBounds(panNudgeRow.removeFromLeft(76));
        panNudgeRow.removeFromLeft(4);
        panPlusButton.setBounds(panNudgeRow.removeFromLeft(76));
        applyButton.setBounds(area.removeFromTop(28));
        area.removeFromTop(4);
        auto actionRow = area.removeFromTop(26);
        primaryActionButton.setBounds(actionRow.removeFromLeft(std::max(96, actionRow.getWidth() / 3)));
        actionRow.removeFromLeft(4);
        secondaryActionButton.setBounds(actionRow.removeFromLeft(std::max(96, actionRow.getWidth() / 2)));
        actionRow.removeFromLeft(4);
        tertiaryActionButton.setBounds(actionRow);
    }
}

void StructureInspector::paint(juce::Graphics& g)
{
    g.setColour(visual::surface);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), visual::panelRadius);
    g.setColour(visual::border);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), visual::panelRadius, visual::borderWidth);
}
} // namespace drs::app::authoring
