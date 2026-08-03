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

        const auto inner = canvas.getLocalBounds().toFloat().reduced(12.0f);
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

        std::cout << "Velocity crossfade Zone Map tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Velocity crossfade Zone Map tests failed: " << exception.what() << '\n';
        return 1;
    }
}
