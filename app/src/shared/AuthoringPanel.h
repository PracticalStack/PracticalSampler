#pragma once

#include "shared/AuthoringPreviewModel.h"
#include "drs/engine/AuthoringSession.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <functional>
#include <vector>

namespace drs::app
{
class AuthoringPanel final : public juce::Component
{
public:
    using NotePreviewStartedCallback = std::function<void(int, float)>;
    using NotePreviewEndedCallback = std::function<void(int)>;
    using WaveformPreviewProvider = std::function<AuthoringWaveformPreview()>;
    using ImportResponsivenessProvider = std::function<AuthoringImportResponsivenessSnapshot()>;

    explicit AuthoringPanel(drs::engine::AuthoringSession& authoringSession,
                            WaveformPreviewProvider waveformPreviewProvider = {},
                            ImportResponsivenessProvider importResponsivenessProvider = {},
                            NotePreviewStartedCallback onNotePreviewStarted = {},
                            NotePreviewEndedCallback onNotePreviewEnded = {});

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class ZoneMapComponent final : public juce::Component
    {
    public:
        void setZoneSummaries(std::vector<drs::engine::AuthoringZoneSummary> summaries);
        void paint(juce::Graphics& g) override;

    private:
        std::vector<drs::engine::AuthoringZoneSummary> zoneSummaries;
    };

    class WaveformPreviewComponent final : public juce::Component
    {
    public:
        void setPreview(AuthoringWaveformPreview preview);
        void paint(juce::Graphics& g) override;

    private:
        AuthoringWaveformPreview preview;
    };

    void rebuildZoneSelector();
    void refreshFromSession();
    void applySelectedZoneEdit(const juce::String& label);

    drs::engine::AuthoringSession& authoringSession;
    WaveformPreviewProvider waveformPreviewProvider;
    ImportResponsivenessProvider importResponsivenessProvider;
    NotePreviewStartedCallback onNotePreviewStarted;
    NotePreviewEndedCallback onNotePreviewEnded;
    bool isRefreshing = false;

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label sourceLabel;
    juce::Label articulationLabel;
    juce::Label waveformLabel;
    juce::Label waveformInfoLabel;
    juce::Label loopInfoLabel;
    juce::Label importMetricsLabel;
    juce::Label zoneLabel;
    juce::ComboBox zoneSelector;
    ZoneMapComponent zoneMap;
    WaveformPreviewComponent waveformPreview;

    juce::Slider rootKeySlider;
    juce::Slider keyLowSlider;
    juce::Slider keyHighSlider;
    juce::Slider velocityLowSlider;
    juce::Slider velocityHighSlider;
    juce::Slider gainSlider;
    juce::Slider panSlider;

    juce::Label rootKeyLabel;
    juce::Label keyLowLabel;
    juce::Label keyHighLabel;
    juce::Label velocityLowLabel;
    juce::Label velocityHighLabel;
    juce::Label gainLabel;
    juce::Label panLabel;

    juce::ToggleButton loopEnabledToggle;
    juce::TextButton previewButton;
    juce::TextButton undoButton;
    juce::TextButton redoButton;
    juce::TextButton saveCheckpointButton;
};
} // namespace drs::app
