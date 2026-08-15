#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app::authoring
{
class RoutingWorkbenchView final : public juce::Component
{
public:
    enum class LayoutMode
    {
        wideTwoRegion,
        normalBalanced,
        compactStacked
    };

    struct LayoutSnapshot
    {
        LayoutMode mode = LayoutMode::compactStacked;
        juce::Rectangle<int> signalPathRegion;
        juce::Rectangle<int> selectedFxRegion;
    };

    struct Bindings
    {
        juce::Label* fxSectionLabel = nullptr;
        juce::Label* scopeLabel = nullptr;
        juce::ComboBox* scopeSelector = nullptr;
        juce::Label* scopeBreadcrumb = nullptr;
        juce::ComboBox* fxSelector = nullptr;
        juce::TextEditor* fxNameEditor = nullptr;
        juce::Label* fxTypeLabel = nullptr;
        juce::ComboBox* fxTypeSelector = nullptr;
        juce::ToggleButton* fxBypassedToggle = nullptr;
        juce::TextButton* fxAddButton = nullptr;
        juce::TextButton* fxDuplicateButton = nullptr;
        juce::TextButton* fxMoveUpButton = nullptr;
        juce::TextButton* fxMoveDownButton = nullptr;
        juce::TextButton* fxDeleteButton = nullptr;
        juce::ComboBox* fxOwnerSelector = nullptr;
        juce::TextButton* fxMoveOwnerButton = nullptr;
        juce::ComboBox* fxParameterSelector = nullptr;
        juce::Slider* fxParameterSlider = nullptr;
        juce::TextButton* fxParameterResetButton = nullptr;
        juce::TextButton* fxAssignMacroButton = nullptr;
        juce::Label* fxParameterValueLabel = nullptr;
        juce::Label* fxSummaryLabel = nullptr;
        juce::Label* fxDiagnosticsLabel = nullptr;
        juce::Label* routingSectionLabel = nullptr;
        juce::ComboBox* routingBusSelector = nullptr;
        juce::Label* routingInputLabel = nullptr;
        juce::ComboBox* routingInputSelector = nullptr;
        juce::Label* routingInsertOneLabel = nullptr;
        juce::ComboBox* routingInsertOneSelector = nullptr;
        juce::Label* routingInsertTwoLabel = nullptr;
        juce::ComboBox* routingInsertTwoSelector = nullptr;
        juce::Label* routingSummaryLabel = nullptr;
    };

    RoutingWorkbenchView();

    void setBindings(Bindings nextBindings);
    void setPresentationState(bool hasRoutingBus,
                              bool hasSelectedFx,
                              bool warningState,
                              const juce::String& signalPath,
                              const juce::String& selectedFxContext,
                              const juce::String& macroControlSummary);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    LayoutSnapshot getLayoutSnapshot() const noexcept { return layout; }
    static int preferredContentHeight(int contentWidth,
                                      int viewportHeight,
                                      bool shortHost) noexcept;

private:
    void addBoundComponents();
    void layoutSignalPathRegion(juce::Rectangle<int> bounds);
    void layoutSelectedFxRegion(juce::Rectangle<int> bounds);

    Bindings bindings;
    LayoutSnapshot layout;
    bool hasBus = false;
    bool hasFx = false;
    bool hasWarning = false;

    juce::Label signalPathHeading;
    juce::Label signalPathLabel;
    juce::Label selectedFxIdentityHeading;
    juce::Label selectedFxContextLabel;
    juce::Label parameterHeading;
    juce::Label macroControlLabel;
    juce::Label busEmptyLabel;
    juce::Label fxEmptyLabel;
};
} // namespace drs::app::authoring
