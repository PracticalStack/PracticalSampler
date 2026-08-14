#pragma once

#include "shared/authoring/RepeatedStructureList.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app::authoring
{
class MacroWorkbenchView final : public juce::Component
{
public:
    enum class LayoutMode
    {
        wideThreeRegion,
        normalListDetail,
        compactStacked
    };

    struct LayoutSnapshot
    {
        LayoutMode mode = LayoutMode::compactStacked;
        juce::Rectangle<int> listRegion;
        juce::Rectangle<int> definitionRegion;
        juce::Rectangle<int> assignmentsRegion;
    };

    struct Bindings
    {
        RepeatedStructureList* macroList = nullptr;
        juce::TextButton* createButton = nullptr;
        juce::TextButton* duplicateButton = nullptr;
        juce::TextButton* deleteButton = nullptr;
        juce::TextButton* moveUpButton = nullptr;
        juce::TextButton* moveDownButton = nullptr;
        juce::Label* nameLabel = nullptr;
        juce::TextEditor* nameEditor = nullptr;
        juce::Label* exposeLabel = nullptr;
        juce::ToggleButton* exposeToggle = nullptr;
        juce::Label* roleLabel = nullptr;
        juce::ComboBox* roleSelector = nullptr;
        juce::Label* defaultLabel = nullptr;
        juce::Slider* defaultSlider = nullptr;
        juce::Label* minimumLabel = nullptr;
        juce::Slider* minimumSlider = nullptr;
        juce::Label* maximumLabel = nullptr;
        juce::Slider* maximumSlider = nullptr;
        RepeatedStructureList* assignmentList = nullptr;
        juce::Label* assignmentLabel = nullptr;
        juce::ComboBox* assignmentSelector = nullptr;
        juce::TextButton* addAssignmentButton = nullptr;
        juce::TextButton* removeAssignmentButton = nullptr;
        juce::Label* assignmentSummary = nullptr;
    };

    MacroWorkbenchView();

    void setBindings(Bindings nextBindings);
    void setPresentationState(bool hasSelectedMacro,
                              const juce::String& rangeStatus,
                              const juce::String& assignmentDetail);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    LayoutSnapshot getLayoutSnapshot() const noexcept { return layout; }
    static int preferredContentHeight(int contentWidth,
                                      int viewportHeight,
                                      bool shortHost) noexcept;

private:
    void addBoundComponents();
    void layoutListRegion(juce::Rectangle<int> bounds);
    void layoutDefinitionRegion(juce::Rectangle<int> bounds);
    void layoutAssignmentsRegion(juce::Rectangle<int> bounds);
    void clearDefinitionBounds();
    void clearAssignmentBounds();

    Bindings bindings;
    LayoutSnapshot layout;
    bool hasSelection = false;

    juce::Label listHeading;
    juce::Label definitionHeading;
    juce::Label identityHeading;
    juce::Label rangeHeading;
    juce::Label rangeStatusLabel;
    juce::Label assignmentsHeading;
    juce::Label assignmentDetailHeading;
    juce::Label assignmentDetailLabel;
    juce::Label definitionEmptyLabel;
};
} // namespace drs::app::authoring
