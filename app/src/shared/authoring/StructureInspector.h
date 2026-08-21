#pragma once

#include "shared/authoring/AuthoringStructureSelection.h"
#include "drs/engine/RuntimeModel.h"
#include "drs/engine/AuthoringSession.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <functional>

namespace drs::app::authoring
{
struct StructureInspectorSnapshot
{
    StructureSelectionKind kind = StructureSelectionKind::none;
    int selectedCount = 0;
    std::string title = "Nothing selected";
    std::vector<std::pair<std::string, std::string>> fields;
};

enum class StructureInspectorAction
{
    showZones,
    openWaveform,
    audition,
    selectChildren,
    selectVisibleChildren
};

StructureInspectorSnapshot buildStructureInspectorSnapshot(
    const drs::engine::RuntimeProjectModel& project,
    const AuthoringStructureSelection& selection);

class StructureInspector : public juce::Component
{
public:
    using PatchCallback = std::function<void(StructureSelectionKind,
                                             drs::engine::AuthoringStructureBatchPatch)>;
    using ActionCallback = std::function<void(StructureInspectorAction)>;
    StructureInspector();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void setSnapshot(StructureInspectorSnapshot nextSnapshot);
    void setOnPatchRequested(PatchCallback nextCallback) { onPatchRequested = std::move(nextCallback); }
    void setOnActionRequested(ActionCallback nextCallback) { onActionRequested = std::move(nextCallback); }

    const StructureInspectorSnapshot& getSnapshot() const noexcept { return snapshot; }

private:
    juce::Label header;
    juce::Label selectionSummary;
    juce::Label editHint;
    juce::TextEditor nameEditor;
    juce::TextEditor parentEditor;
    juce::TextEditor gainEditor;
    juce::TextEditor panEditor;
    juce::TextEditor releaseEditor;
    juce::ToggleButton visibilityToggle;
    juce::TextButton gainMinusButton;
    juce::TextButton gainPlusButton;
    juce::TextButton panMinusButton;
    juce::TextButton panPlusButton;
    juce::TextButton applyButton;
    juce::TextButton primaryActionButton;
    juce::TextButton secondaryActionButton;
    juce::TextButton tertiaryActionButton;
    std::vector<std::unique_ptr<juce::Label>> fieldLabels;
    std::vector<std::unique_ptr<juce::Label>> valueLabels;
    StructureInspectorSnapshot snapshot;
    PatchCallback onPatchRequested;
    ActionCallback onActionRequested;
    bool visibilityTouched = false;
};
} // namespace drs::app::authoring
