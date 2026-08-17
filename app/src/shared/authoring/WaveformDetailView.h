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
    using LoopRegionCommitCallback = std::function<void(std::uint64_t loopStartFrame,
                                                        std::uint64_t loopEndFrameExclusive,
                                                        const std::string& label)>;

    WaveformDetailView();
    void setPreview(AuthoringWaveformPreview nextPreview);
    void setDetailRequestCallback(DetailRequestCallback callback);
    void setLoopRegionCommitCallback(LoopRegionCommitCallback callback);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusLost(FocusChangeType cause) override;

    drs::engine::WaveformFrameRange getViewportFrames() const noexcept { return viewportFrames; }
    drs::engine::WaveformFrameRange getSelectionFrames() const noexcept;
    bool hasSelection() const noexcept { return preview.selectionActive; }
    void clearSelection();

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
    std::uint64_t frameAtX(float x) const noexcept;
    void cancelRegionGesture();

    enum class Gesture
    {
        none,
        pan,
        selection,
        loopStart,
        loopEnd
    };

    AuthoringWaveformPreview preview;
    DetailRequestCallback detailRequestCallback;
    LoopRegionCommitCallback loopRegionCommitCallback;
    drs::engine::WaveformFrameRange viewportFrames;
    drs::engine::WaveformFrameRange dragStartViewport;
    juce::Point<float> dragStartPosition;
    bool dragging = false;
    Gesture gesture = Gesture::none;
    std::uint64_t gestureAnchorFrame = 0;
    std::uint64_t originalLoopStartFrame = 0;
    std::uint64_t originalLoopEndFrame = 0;
    Gesture selectedBoundary = Gesture::none;
};
} // namespace drs::app::authoring
