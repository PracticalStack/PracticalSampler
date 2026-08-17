#pragma once

#include "shared/AuthoringPreviewModel.h"
#include "drs/engine/WaveformRegionPolicy.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace drs::app::authoring
{
class WaveformDetailView final : public juce::Component
{
public:
    using DetailRequestCallback = std::function<void(std::uint64_t startFrame,
                                                     std::uint64_t endFrameExclusive,
                                                     std::size_t displayPointCount)>;

    WaveformDetailView();
    void setPreview(AuthoringWaveformPreview nextPreview);
    void setDetailRequestCallback(DetailRequestCallback callback);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;

    drs::engine::WaveformFrameRange getViewportFrames() const noexcept { return viewportFrames; }

private:
    juce::Rectangle<float> getCanvasBounds() const;
    void fitToSource(bool requestDetail);
    void publishDetailRequest();
    void panByFrames(std::int64_t frames, bool publishRequest);
    void drawPeaks(juce::Graphics& g,
                   const std::vector<AuthoringWaveformPreviewPoint>& points,
                   drs::engine::WaveformFrameRange coverage,
                   juce::Colour colour,
                   float thickness) const;

    AuthoringWaveformPreview preview;
    DetailRequestCallback detailRequestCallback;
    drs::engine::WaveformFrameRange viewportFrames;
    drs::engine::WaveformFrameRange dragStartViewport;
    juce::Point<float> dragStartPosition;
    bool dragging = false;
};
} // namespace drs::app::authoring
