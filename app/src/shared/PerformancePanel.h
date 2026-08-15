#pragma once

#include "drs/engine/EngineFacade.h"
#include "shared/PerformanceMixer.h"
#include "shared/StatusPanel.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace drs::app
{
class PerformancePanel final : public juce::Component,
                               private juce::Timer,
                               private juce::MidiKeyboardStateListener
{
public:
    using MacroValueChangedCallback = std::function<void(const std::string&, double)>;
    using PerformanceNoteOnCallback = std::function<void(int, float)>;
    using PerformanceNoteOffCallback = std::function<void(int)>;
    using PublishCommandCallback = StatusPanel::PublishCommandCallback;
    using PublishPresentationProvider = StatusPanel::PublishPresentationProvider;
    using AudioCallbackActiveProvider = std::function<bool()>;
    using InstrumentControlsExpandedProvider = std::function<std::optional<bool>()>;
    using InstrumentControlsExpandedChangedCallback = std::function<void(bool)>;
    using WorkspaceDisplayNameProvider = std::function<juce::String()>;

    struct LayoutSnapshot
    {
        bool compact = false;
        bool shortHeight = false;
        bool controlsBesideArtwork = false;
        bool diagnosticsVisible = false;
        juce::Rectangle<int> headerBounds;
        juce::Rectangle<int> controlsBounds;
        juce::Rectangle<int> artworkBounds;
        juce::Rectangle<int> keyboardBounds;
        juce::Rectangle<int> diagnosticsBounds;
    };

    explicit PerformancePanel(drs::engine::EngineFacade& engineFacade,
                              MacroValueChangedCallback onMacroValueChanged = {},
                              PerformanceNoteOnCallback onPerformanceNoteOn = {},
                              PerformanceNoteOffCallback onPerformanceNoteOff = {},
                              PublishCommandCallback onPublishCommand = {},
                              PublishPresentationProvider publishPresentationProvider = {},
                              AudioCallbackActiveProvider audioCallbackActiveProvider = {},
                              InstrumentControlsExpandedProvider instrumentControlsExpandedProvider = {},
                              InstrumentControlsExpandedChangedCallback onInstrumentControlsExpandedChanged = {},
                              WorkspaceDisplayNameProvider workspaceDisplayNameProvider = {});
    ~PerformancePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refreshNow();
    void refreshArtworkNow();
    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }
    LayoutSnapshot getLayoutSnapshot() const noexcept { return layoutSnapshot; }

private:
    class PerformanceControlLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        PerformanceControlLookAndFeel();

        void drawButtonBackground(juce::Graphics& graphics,
                                  juce::Button& button,
                                  const juce::Colour& backgroundColour,
                                  bool highlighted,
                                  bool down) override;
        void drawToggleButton(juce::Graphics& graphics,
                              juce::ToggleButton& button,
                              bool highlighted,
                              bool down) override;
        void drawLinearSliderOutline(juce::Graphics& graphics,
                                     int x,
                                     int y,
                                     int width,
                                     int height,
                                     juce::Slider::SliderStyle style,
                                     juce::Slider& slider) override;
        void drawRotarySlider(juce::Graphics& graphics,
                              int x,
                              int y,
                              int width,
                              int height,
                              float sliderPosition,
                              float rotaryStartAngle,
                              float rotaryEndAngle,
                              juce::Slider& slider) override;
    };

    class ArtworkPanel final : public juce::Component
    {
    public:
        void setArtwork(juce::Image nextArtwork, juce::String nextDescription);
        void paint(juce::Graphics& g) override;

    private:
        juce::Image artwork;
        juce::String description;
    };

    struct MacroControl
    {
        std::string id;
        juce::Label nameLabel;
        juce::Slider slider;
        juce::Label valueLabel;
        bool mixerControl = false;
    };

    void timerCallback() override;
    void handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void rebuildMacroControls(const std::vector<drs::engine::EngineMacroDescriptor>& macros,
                              bool mixerControl);
    void refreshArtwork();
    void refreshInstrumentName();
    void refreshSurface();
    void refreshMacroValues();
    void updateMacroValues(const std::vector<drs::engine::EngineMacroDescriptor>& macros);
    void setInstrumentControlsCollapsed(bool shouldCollapse);
    void setDiagnosticsVisible(bool shouldShow);
    void updateInstrumentControlsVisibility();
    void syncKeyboardPlayableRange();

    drs::engine::EngineFacade& engineFacade;
    MacroValueChangedCallback onMacroValueChanged;
    PerformanceNoteOnCallback onPerformanceNoteOn;
    PerformanceNoteOffCallback onPerformanceNoteOff;
    PublishPresentationProvider publishPresentationProvider;
    AudioCallbackActiveProvider audioCallbackActiveProvider;
    InstrumentControlsExpandedProvider instrumentControlsExpandedProvider;
    InstrumentControlsExpandedChangedCallback onInstrumentControlsExpandedChanged;
    WorkspaceDisplayNameProvider workspaceDisplayNameProvider;
    bool hasActivePublishedPerformance = false;
    juce::String publishedPerformanceStateLabel { "Idle" };
    juce::String publishedPerformanceGuidance;
    juce::String publishedPerformanceFindingCode;
    drs::engine::EnginePerformanceSnapshot performanceSnapshot;
    std::uint64_t lastObservedPublishLifecycleRevision = 0;
    std::uint64_t lastObservedMacroTopologyRevision = 0;
    std::uint64_t lastObservedMacroValueRevision = 0;
    bool initialRevisionCheckPending = true;
    bool showingPublishedMixer = false;
    bool instrumentControlsCollapsed = true;
    bool diagnosticsVisible = false;
    bool audioCallbackActive = true;
    std::optional<bool> userInstrumentControlsExpandedChoice;
    std::size_t hiddenPublishedMacroCount = 0;
    std::vector<std::string> visibleMacroIds;
    std::vector<std::unique_ptr<MacroControl>> macroControls;
    PerformanceMixer publishedMixer;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;
    StatusPanel diagnosticsPanel;
    juce::Viewport diagnosticsViewport;
    ArtworkPanel artworkPanel;
    std::string loadedArtworkSourceKey;

    PerformanceControlLookAndFeel performanceLookAndFeel;
    LayoutSnapshot layoutSnapshot;

    juce::Label instrumentNameLabel;
    juce::Label instrumentContextLabel;
    juce::Label performanceGuidanceLabel;
    juce::Label macroStripLabel;
    juce::TextButton macroStripToggleButton;
    juce::TextButton detailsToggleButton;
    juce::Label mixerEmptyStateLabel;
    juce::Label loadIndicatorLabel;
};
} // namespace drs::app
