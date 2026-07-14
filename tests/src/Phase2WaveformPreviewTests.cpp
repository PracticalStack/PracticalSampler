#include "plugin/PluginProcessor.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool hasVisibleWaveform(const drs::app::AuthoringWaveformPreview& preview)
{
    constexpr auto amplitudeEpsilon = 1.0e-4f;

    return std::any_of(preview.points.begin(),
                       preview.points.end(),
                       [&](const drs::app::AuthoringWaveformPreviewPoint& point)
                       {
                           return std::abs(point.minValue) > amplitudeEpsilon
                               || std::abs(point.maxValue) > amplitudeEpsilon
                               || std::abs(point.maxValue - point.minValue) > amplitudeEpsilon;
                       });
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        drs::plugin::Processor processor;

        const auto metrics = processor.getAuthoringImportResponsivenessSnapshot();
        require(metrics.available, "Authoring import responsiveness snapshot should be available.");
        require(metrics.totalItemCount == 2, "Phase 2 reference project sample-source count changed unexpectedly.");
        require(metrics.pendingCount == 0, "Phase 2 authoring import metrics should not report pending items.");
        require(metrics.processedCount == 2, "Phase 2 authoring import metrics should process both reference items.");
        require(metrics.failedItemCount == 0, "Phase 2 authoring import metrics should not report failed items.");
        require(metrics.maxProcessDurationMicros >= metrics.averageProcessDurationMicros,
                "Phase 2 authoring import max duration should be at least the average duration.");
        require(!metrics.lastProcessedItemId.empty(),
                "Phase 2 authoring import metrics should report the last processed item id.");

        const auto defaultPreview = processor.getAuthoringWaveformPreview();
        require(defaultPreview.available, "Default Phase 2 selected zone should expose a waveform preview.");
        require(defaultPreview.formatName == "WAV file", "Phase 2 default waveform fixture format changed unexpectedly.");
        require(defaultPreview.sampleRate == 44100.0, "Phase 2 default waveform fixture sample rate changed unexpectedly.");
        require(defaultPreview.channelCount == 2, "Phase 2 default waveform fixture channel count changed unexpectedly.");
        require(defaultPreview.frameCount == 44100, "Phase 2 default waveform fixture frame count changed unexpectedly.");
        require(!defaultPreview.points.empty(), "Phase 2 default waveform fixture should include preview points.");
        require(hasVisibleWaveform(defaultPreview),
                "Phase 2 default waveform fixture should render visible waveform amplitude data.");
        require(!defaultPreview.loopEnabled, "Lead waveform preview should begin with looping disabled.");
        require(defaultPreview.loopStartFrame == 0 && defaultPreview.loopEndFrame == 0,
                "Lead waveform preview loop markers changed unexpectedly.");

        const auto selectionResult = processor.getAuthoringSession().selectZone("pad-a3-high");
        require(selectionResult.applied, "Selecting the looping pad zone should succeed before preview validation.");

        const auto loopPreview = processor.getAuthoringWaveformPreview();
        require(loopPreview.available, "Looping Phase 2 selected zone should expose a waveform preview.");
        require(loopPreview.sampleRate == 44100.0, "Looping Phase 2 waveform sample rate changed unexpectedly.");
        require(loopPreview.channelCount == 1, "Looping Phase 2 waveform channel count changed unexpectedly.");
        require(loopPreview.frameCount == 88200, "Looping Phase 2 waveform frame count changed unexpectedly.");
        require(!loopPreview.points.empty(), "Looping Phase 2 waveform preview should include preview points.");
        require(hasVisibleWaveform(loopPreview),
                "Looping Phase 2 waveform preview should render visible waveform amplitude data.");
        require(loopPreview.loopEnabled, "Looping Phase 2 zone should report loop-enabled preview metadata.");
        require(loopPreview.loopStartFrame == 512, "Looping Phase 2 zone loop start changed unexpectedly.");
        require(loopPreview.loopEndFrame == 22016, "Looping Phase 2 zone loop end changed unexpectedly.");

        std::cout << "Phase 2 waveform preview tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 waveform preview tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
