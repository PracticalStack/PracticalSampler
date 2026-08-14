#include "shared/authoring/ZoneMapCanvas.h"

#include <iostream>
#include <stdexcept>

namespace
{
void require(const bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

float velocityToY(const juce::Rectangle<float>& inner, const int velocity)
{
    return inner.getY() + (1.0f - static_cast<float>(velocity) / 127.0f) * inner.getHeight();
}
} // namespace

int main()
{
    using namespace drs::app::authoring;
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        ZoneMapCanvas canvas;
        canvas.setSize(640, 360);
        canvas.setVisible(true);

        drs::engine::AuthoringZoneSummary lower;
        lower.id = "lower";
        lower.displayName = "Lower";
        lower.articulationId = "sustain";
        lower.rootKey = 60;
        lower.keyLow = 0;
        lower.keyHigh = 127;
        lower.velocityLow = 1;
        lower.velocityHigh = 70;
        lower.velocityCrossfade.fadeOutLowVelocity = 55;
        lower.velocityCrossfade.fadeOutHighVelocity = 70;
        lower.selected = true;

        auto upper = lower;
        upper.id = "upper";
        upper.displayName = "Upper";
        upper.velocityLow = 55;
        upper.velocityHigh = 127;
        upper.velocityCrossfade = {};
        upper.velocityCrossfade.fadeInLowVelocity = 55;
        upper.velocityCrossfade.fadeInHighVelocity = 70;
        upper.selected = false;

        canvas.setZoneSummaries({ lower, upper });
        canvas.setSelectionState({ { "lower" }, "lower" });

        int commitCount = 0;
        int committedLow = 0;
        int committedHigh = 0;
        canvas.setOnVelocityCrossfadeCommitRequested(
            [&](const std::string& lowerId, const std::string& upperId, int low, int high)
            {
                require(lowerId == "lower" && upperId == "upper",
                        "Map crossfade gesture must retain both relationship endpoints.");
                ++commitCount;
                committedLow = low;
                committedHigh = high;
            });

        juce::Image image(juce::Image::ARGB, canvas.getWidth(), canvas.getHeight(), true);
        juce::Graphics graphics(image);
        canvas.paintEntireComponent(graphics, true);

        const auto inner = canvas.getMapViewportBounds();
        const auto crossfadeHandleX = inner.getRight() - 9.0f;
        const auto lowHandle = juce::Point<float>(crossfadeHandleX, velocityToY(inner, 55));
        const auto lowTarget = juce::Point<float>(crossfadeHandleX, velocityToY(inner, 48));
        require(canvas.beginRangeGestureAt(lowHandle),
                "Crossfade handles should take hit-test priority over ordinary range handles.");
        require(canvas.isCrossfadeGestureActive(),
                "A crossfade diamond handle should create a relationship gesture.");
        require(canvas.updateActiveRangeGesture(lowTarget) && canvas.endActiveRangeGesture(lowTarget),
                "Dragging a crossfade boundary should preview and commit one relationship edit.");
        require(commitCount == 1 && committedLow == 48 && committedHigh == 70,
                "Dragging the low crossfade boundary must preserve high and update both partners through one callback.");
        require(canvas.keyPressed(juce::KeyPress(juce::KeyPress::upKey, {}, 0)) && commitCount == 2
                    && committedLow == 49 && committedHigh == 70,
                "A focused crossfade handle should support keyboard endpoint adjustment.");

        const auto highHandle = juce::Point<float>(crossfadeHandleX, velocityToY(inner, 70));
        const auto highTarget = juce::Point<float>(crossfadeHandleX, velocityToY(inner, 90));
        require(canvas.beginRangeGestureAt(highHandle) && canvas.updateActiveRangeGesture(highTarget),
                "The high crossfade boundary should support its own preview gesture.");
        require(canvas.cancelActiveRangeGesture() && !canvas.isCrossfadeGestureActive(),
                "Cancelling a crossfade gesture must discard its preview without a transaction.");
        require(commitCount == 2,
                "Cancelling a crossfade gesture must not create an undoable relationship update.");

        ZoneMapCanvas denseCanvas;
        denseCanvas.setSize(960, 420);
        denseCanvas.setVisible(true);

        std::vector<drs::engine::AuthoringZoneSummary> denseZones;
        denseZones.reserve(1024);
        for (int key = 0; key < 128; ++key)
        {
            for (int layer = 0; layer < 8; ++layer)
            {
                drs::engine::AuthoringZoneSummary zone;
                zone.id = "dense-" + std::to_string(key) + "-" + std::to_string(layer);
                zone.displayName = "Dense " + std::to_string(key) + "-" + std::to_string(layer);
                zone.articulationId = "sustain";
                zone.rootKey = key;
                zone.keyLow = key;
                zone.keyHigh = key;
                zone.velocityLow = layer == 0 ? 1 : (layer * 16);
                zone.velocityHigh = layer == 7 ? 127 : ((layer + 1) * 16 - 1);
                denseZones.push_back(std::move(zone));
            }
        }

        denseCanvas.setZoneSummaries(denseZones);

        std::string selectedDenseZoneId;
        int denseSelectionCount = 0;
        denseCanvas.setOnZoneSelectionStateRequested(
            [&](const ZoneMapCanvas::SelectionState& selectionState)
            {
                selectedDenseZoneId = selectionState.primaryZoneId;
                denseSelectionCount = static_cast<int>(selectionState.zoneIds.size());
            });

        const auto denseInner = denseCanvas.getMapViewportBounds();
        const auto targetKey = 60;
        const auto targetLayer = 3;
        const auto targetLow = targetLayer * 16;
        const auto targetHigh = (targetLayer + 1) * 16 - 1;
        const auto targetLeft = denseInner.getX()
            + denseInner.getWidth() * (static_cast<float>(targetKey) / 128.0f);
        const auto targetWidth = std::max(10.0f, denseInner.getWidth() / 128.0f);
        const auto targetTop = velocityToY(denseInner, targetHigh);
        const auto targetBottom = denseInner.getY()
            + (static_cast<float>(128 - targetLow) / 127.0f) * denseInner.getHeight();
        const auto targetHeight = std::max(
            14.0f,
            targetBottom - targetTop);
        const auto targetX = targetLeft + targetWidth * 0.5f;
        const auto targetY = targetTop + targetHeight * 0.5f;
        require(denseCanvas.requestSelectionAt({ targetX, targetY }),
                "Dense zone maps should still resolve a direct point selection.");
        require(selectedDenseZoneId == "dense-60-3" && denseSelectionCount == 1,
                "Dense zone map selection should preserve the targeted primary zone.");

        std::cout << "Velocity crossfade Zone Map tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Velocity crossfade Zone Map tests failed: " << exception.what() << '\n';
        return 1;
    }
}
