#pragma once

#include "drs/engine/EngineFacade.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <string>
#include <vector>

namespace drs::app
{
// Immutable message-thread view data for the published player surface. Runtime
// IDs drive values; authored IDs only define stable UI identity and ordering.
struct PerformanceMixerControlView
{
    std::string authoredId;
    std::string runtimeId;
    std::string sectionLabel;
    std::string controlLabel;
    std::string parameterLabel;
    std::string valueUnit;
    std::string accessibilityDescription;
    drs::engine::PublishedMacroControlKind controlKind = drs::engine::PublishedMacroControlKind::knob;
    std::size_t authoredOrder = 0;
    double minimum = 0.0;
    double maximum = 1.0;
    double displayMinimum = 0.0;
    double displayMaximum = 1.0;
    double value = 0.0;
};

class PerformanceMixer final : public juce::Component
{
public:
    using ValueChangedCallback = std::function<void(const std::string&, double)>;

    struct LayoutSnapshot
    {
        int columnCount = 0;
        int rowCount = 0;
        bool compact = false;
        bool viewportActive = false;
    };

    explicit PerformanceMixer(ValueChangedCallback onValueChanged = {});
    ~PerformanceMixer() override;

    void setControls(std::vector<PerformanceMixerControlView> controls);
    std::size_t getControlCount() const noexcept;
    LayoutSnapshot getLayoutSnapshot() const noexcept;
    void resized() override;

private:
    class Lane;
    void rebuildLanes();
    void updateLayout();

    ValueChangedCallback onValueChanged;
    std::vector<PerformanceMixerControlView> controls;
    std::vector<std::unique_ptr<Lane>> lanes;
    juce::Viewport viewport;
    juce::Component content;
    LayoutSnapshot layout;
};
} // namespace drs::app
