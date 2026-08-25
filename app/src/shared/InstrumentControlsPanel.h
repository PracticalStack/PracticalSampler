#pragma once

#include "drs/engine/EngineFacade.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace drs::app
{
class InstrumentControlsPanel final : public juce::Component
{
public:
    using ValueChangedCallback = std::function<bool(const std::string&, double)>;
    using DescriptorProvider = std::function<
        std::vector<drs::engine::EngineInstrumentControlDescriptor>()>;

    InstrumentControlsPanel(drs::engine::EngineFacade& engineFacade,
                            ValueChangedCallback valueChangedCallback = {},
                            DescriptorProvider descriptorProvider = {});

    void paint(juce::Graphics&) override;
    void resized() override;
    void refreshNow();

    bool hasControls() const noexcept { return !rows.empty(); }

private:
    struct ControlRow final : public juce::Component
    {
        explicit ControlRow(const drs::engine::EngineInstrumentControlDescriptor& descriptor);

        void resized() override;
        void update(const drs::engine::EngineInstrumentControlDescriptor& nextDescriptor,
                    bool sendNotification);
        static juce::String formatValue(
            const drs::engine::EngineInstrumentControlDescriptor& descriptor);

        drs::engine::EngineInstrumentControlDescriptor descriptor;
        juce::Label nameLabel;
        juce::Label valueLabel;
        juce::Slider slider;
    };

    void rebuild(const std::vector<drs::engine::EngineInstrumentControlDescriptor>& descriptors);
    static bool sameTopology(
        const std::vector<std::unique_ptr<ControlRow>>& currentRows,
        const std::vector<drs::engine::EngineInstrumentControlDescriptor>& descriptors);

    drs::engine::EngineFacade& engineFacade;
    ValueChangedCallback valueChangedCallback;
    DescriptorProvider descriptorProvider;
    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<ControlRow>> rows;
    bool refreshing = false;
};
} // namespace drs::app
