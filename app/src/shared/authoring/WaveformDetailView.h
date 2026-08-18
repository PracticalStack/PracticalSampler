#pragma once

#include "shared/AuthoringPreviewModel.h"
#include "shared/WaveformSnapService.h"
#include "drs/engine/WaveformRegionPolicy.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace drs::app::authoring
{
class WaveformDetailView final : public juce::Component,
                                 private juce::Timer
{
public:
    using DetailRequestCallback = std::function<void(std::uint64_t startFrame,
                                                     std::uint64_t endFrameExclusive,
                                                     std::size_t displayPointCount)>;
    using LoopRegionCommitCallback = std::function<void(std::uint64_t loopStartFrame,
                                                        std::uint64_t loopEndFrameExclusive,
                                                        const std::string& label)>;
    using PlaybackRegionCommitCallback = std::function<void(std::uint64_t startFrame,
                                                            std::uint64_t endFrameExclusive,
                                                            const std::string& label)>;

    WaveformDetailView();
    void setPreview(AuthoringWaveformPreview nextPreview);
    void setDetailRequestCallback(DetailRequestCallback callback);
    void setLoopRegionCommitCallback(LoopRegionCommitCallback callback);
    void setPlaybackRegionCommitCallback(PlaybackRegionCommitCallback callback);
    void setZeroCrossingSnapEnabled(bool enabled);
    bool isZeroCrossingSnapEnabled() const noexcept { return zeroCrossingSnapEnabled; }
    const std::string& getSnapStatus() const noexcept { return snapStatus; }
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
    void setSelectionFrames(drs::engine::WaveformFrameRange selection);

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
    void applyBoundaryCandidate(std::uint64_t candidateFrame, bool snapped);
    void submitSnapCandidate(std::uint64_t candidateFrame);
    drs::engine::WaveformEditableRegions currentRegions() const noexcept;
    void timerCallback() override;

    enum class Gesture
    {
        none,
        pan,
        selection,
        playbackStart,
        playbackEnd,
        playbackMove,
        loopStart,
        loopEnd
    };

    AuthoringWaveformPreview preview;
    DetailRequestCallback detailRequestCallback;
    LoopRegionCommitCallback loopRegionCommitCallback;
    PlaybackRegionCommitCallback playbackRegionCommitCallback;
    WaveformSnapService snapService;
    drs::engine::WaveformFrameRange viewportFrames;
    drs::engine::WaveformFrameRange dragStartViewport;
    juce::Point<float> dragStartPosition;
    bool dragging = false;
    Gesture gesture = Gesture::none;
    std::uint64_t gestureAnchorFrame = 0;
    std::uint64_t originalLoopStartFrame = 0;
    std::uint64_t originalLoopEndFrame = 0;
    std::uint64_t originalPlaybackStartFrame = 0;
    std::uint64_t originalPlaybackEndFrame = 0;
    std::uint64_t latestRawCandidateFrame = 0;
    std::uint64_t pendingSnapGeneration = 0;
    bool zeroCrossingSnapEnabled = false;
    bool snapApplied = false;
    std::string snapStatus = "Frame snap";
    Gesture selectedBoundary = Gesture::none;
};
} // namespace drs::app::authoring
