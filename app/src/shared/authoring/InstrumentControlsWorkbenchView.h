#pragma once

#include "drs/engine/InstrumentControlContract.h"
#include "shared/authoring/MidiLearnCoordinator.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>
#include <functional>
#include <memory>
#include <cstdint>
#include <unordered_map>

namespace drs::app::authoring
{
// Compact, metadata-driven control surface. It deliberately renders as a
// responsive card grid so imported Tune/Decay/etc. controls do not compete
// with the fixed Performance macro row.
class InstrumentControlsWorkbenchView final : public juce::Component,
                                              private juce::KeyListener
{
public:
    InstrumentControlsWorkbenchView();

    enum class Surface : std::uint8_t
    {
        mixer,
        instrumentControls
    };

    using ValueChangedCallback = std::function<void(const std::string&, double)>;
    using ResetCallback = std::function<void(const std::string&)>;
    using BindingChangedCallback = std::function<void(const std::string&, int, std::uint8_t)>;
    using BindingClearCallback = std::function<void(const std::string&)>;
    using BindingRestoreCallback = std::function<void(const std::string&)>;
    using LearnRequestedCallback = std::function<void(const std::string&)>;
    void setValueChangedCallback(ValueChangedCallback callback) { valueChangedCallback = std::move(callback); }
    void setResetCallback(ResetCallback callback) { resetCallback = std::move(callback); }
    void setBindingChangedCallback(BindingChangedCallback callback) { bindingChangedCallback = std::move(callback); }
    void setBindingClearCallback(BindingClearCallback callback) { bindingClearCallback = std::move(callback); }
    void setBindingRestoreCallback(BindingRestoreCallback callback) { bindingRestoreCallback = std::move(callback); }
    void setLearnRequestedCallback(LearnRequestedCallback callback) { learnRequestedCallback = std::move(callback); }

    void observeMidiCc(std::uint8_t channel, std::uint8_t controllerNumber,
                       std::uint8_t value, std::uint64_t nowMs);
    void armMidiLearn(const std::string& controlId, std::uint64_t nowMs);
    void cancelMidiLearn() noexcept;
    void clearMidiAssignment(const std::string& controlId);
    void restoreMidiAssignment(const std::string& controlId);
    bool isAssignmentsDrawerOpen() const noexcept { return assignmentsDrawerOpen; }
    void setAssignmentsDrawerOpen(bool open);
    Surface surface() const noexcept { return activeSurface; }
    void setSurface(Surface surface);
    std::string midiSourceStatus(const std::string& controlId) const;

    void setControls(std::vector<drs::engine::RuntimeProjectInstrumentControlDefinition> controls,
                     std::vector<drs::engine::RuntimeProjectMidiControlBindingDefinition> bindings);
    int preferredContentHeight(int width) const noexcept;
    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    struct Row
    {
        juce::String title;
        juce::String detail;
        drs::engine::RuntimeInstrumentControlCategory category
            = drs::engine::RuntimeInstrumentControlCategory::hidden;
        std::string id;
        double value = 0.0;
        double displayMinimum = 0.0;
        double displayMaximum = 1.0;
        int displayPrecision = 2;
        drs::engine::RuntimeInstrumentControlUnit unit
            = drs::engine::RuntimeInstrumentControlUnit::generic;
    };

    std::vector<Row> rows;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::ComboBox>> controllerSelectors;
    std::vector<std::unique_ptr<juce::ComboBox>> channelSelectors;
    std::vector<std::unique_ptr<juce::TextButton>> learnButtons;
    std::vector<std::unique_ptr<juce::TextButton>> clearButtons;
    std::vector<std::unique_ptr<juce::TextButton>> restoreButtons;
    std::vector<std::unique_ptr<juce::TextButton>> replaceButtons;
    std::vector<std::unique_ptr<juce::TextButton>> cancelConflictButtons;
    std::vector<drs::engine::RuntimeProjectInstrumentControlDefinition> sourceControls;
    std::vector<drs::engine::RuntimeProjectMidiControlBindingDefinition> sourceBindings;
    std::unordered_map<std::string, std::string> midiSourceStatuses;
    juce::TextEditor filterEditor;
    juce::ComboBox sortSelector;
    juce::TextButton mixerSurfaceButton { "Mixer" };
    juce::TextButton instrumentSurfaceButton { "Parameters" };
    juce::TextButton assignmentsDrawerButton { "Assignments" };
    ValueChangedCallback valueChangedCallback;
    ResetCallback resetCallback;
    BindingChangedCallback bindingChangedCallback;
    BindingClearCallback bindingClearCallback;
    BindingRestoreCallback bindingRestoreCallback;
    LearnRequestedCallback learnRequestedCallback;
    MidiLearnCoordinator learnCoordinator;
    bool refreshing = false;
    bool assignmentsDrawerOpen = false;
    Surface activeSurface = Surface::instrumentControls;

    struct PendingBinding
    {
        bool active = false;
        int controller = -1;
        std::uint8_t channel = 0;
        int previousControllerSelection = 1;
        int previousChannelSelection = 1;
    };
    std::vector<PendingBinding> pendingBindings;

    bool matchesFilter(const Row& row) const;
    void requestBindingChange(std::size_t rowIndex,
                              int controller,
                              std::uint8_t channel,
                              juce::ComboBox& controllerSelector,
                              juce::ComboBox& channelSelector);
    void applyPendingBinding(std::size_t rowIndex);
    void cancelPendingBinding(std::size_t rowIndex);
    bool bindingSourceConflicts(const std::string& controlId,
                                int controller,
                                std::uint8_t channel) const;
    bool keyPressed(const juce::KeyPress& key,
                    juce::Component* originatingComponent) override;
};
} // namespace drs::app::authoring
