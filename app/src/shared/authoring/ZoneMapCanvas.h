#pragma once

#include "drs/engine/AuthoringSession.h"
#include "shared/authoring/ZoneMapOverview.h"
#include "shared/authoring/ZoneMapRenderPolicy.h"
#include "shared/authoring/ZoneMapViewState.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <optional>
#include <vector>

namespace drs::app::authoring
{
class ZoneMapCanvas final : public juce::Component,
                            public juce::FileDragAndDropTarget,
                            private juce::Timer
{
public:
    enum class RangeHandle
    {
        none,
        keyLow,
        keyHigh,
        velocityHigh,
        velocityLow,
        crossfadeLow,
        crossfadeHigh
    };

    enum class SelectionMode
    {
        replace,
        toggle,
        additive
    };

    struct SelectionState
    {
        std::vector<std::string> zoneIds;
        std::string primaryZoneId;
    };

    ZoneMapCanvas();

    void setZoneSummaries(std::vector<drs::engine::AuthoringZoneSummary> summaries);
    void setSelectionState(SelectionState nextSelectionState);
    SelectionState getSelectionState() const;
    void setOnZoneSelectionRequested(std::function<void(const std::string& zoneId)> nextCallback);
    void setOnZoneSelectionStateRequested(std::function<void(const SelectionState& selectionState)> nextCallback);
    void setOnZoneRangeCommitRequested(
        std::function<void(const std::vector<drs::engine::AuthoringZoneSummary>& zones,
                           const std::string& label)> nextCallback);
    void setOnVelocityCrossfadeCommitRequested(
        std::function<void(const std::string& lowerZoneId,
                           const std::string& upperZoneId,
                           int overlapLow,
                           int overlapHigh)> nextCallback);
    void setOnZoneAuditionRequested(
        std::function<void(const std::string& zoneId, int midiNote, int velocity)> nextCallback);
    void setOnShowInStructureRequested(
        std::function<void(const std::vector<std::string>& zoneIds,
                           const std::string& primaryZoneId)> nextCallback);
    void setOnSampleFilesDropped(std::function<void(std::vector<juce::File>)> nextCallback);
    void setOnDeleteSelectedSampleRequested(std::function<void()> nextCallback);
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    bool requestSelectionAt(juce::Point<float> position, SelectionMode mode = SelectionMode::replace);
    bool requestSelectionInBounds(juce::Rectangle<float> bounds, SelectionMode mode = SelectionMode::replace);
    bool requestAuditionAt(juce::Point<float> position);
    bool requestDeleteSelectedSample();
    bool moveSelection(int direction);
    bool beginRangeGestureAt(juce::Point<float> position);
    bool updateActiveRangeGesture(juce::Point<float> position);
    bool endActiveRangeGesture(juce::Point<float> position);
    bool cancelActiveRangeGesture();
    bool isRangeGestureActive() const { return activeGesture.has_value(); }
    bool isCrossfadeGestureActive() const { return activeCrossfadeGesture.has_value(); }
    bool requestZoomAt(juce::Point<float> position, float wheelDelta);
    bool requestPanBy(juce::Point<float> pixelDelta);
    bool zoomBy(float wheelDelta);
    bool fitSelected(float paddingProportion = 0.04f);
    void resetViewport();
    float getZoomFactor() const noexcept { return viewport.getZoom(); }
    int getDisplayedZoomPercentage() const noexcept { return viewport.getDisplayedZoomPercentage(); }
    juce::Point<float> getViewportOrigin() const noexcept { return viewport.getOrigin(); }
    juce::Rectangle<float> getMapViewportBounds() const { return getInnerBounds(); }
    juce::Rectangle<int> getMinimapBounds() const;
    ZoneMapDetailLevel getDetailLevel() const noexcept;
    std::size_t getCachedGeometryCount() const noexcept { return cachedZoneGeometry.size(); }
    std::size_t getLastVisibleZoneCount() const noexcept { return lastVisibleZoneCount; }
    ZoneMapOverview& getOverview() noexcept { return overview; }
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

private:
    struct ZoneLayout
    {
        std::size_t index = 0;
        juce::Rectangle<float> bounds;
    };

    struct CachedZoneGeometry
    {
        std::size_t index = 0;
        juce::Rectangle<float> normalizedBounds;
        juce::Colour groupTint;
    };

    struct RangeGesture
    {
        RangeHandle handle = RangeHandle::none;
        std::size_t zoneIndex = 0;
        std::vector<std::size_t> zoneIndices;
        std::vector<drs::engine::AuthoringZoneSummary> originalZones;
        std::vector<drs::engine::AuthoringZoneSummary> previewZones;
    };

    struct MarqueeGesture
    {
        juce::Point<float> start;
        juce::Point<float> current;
        bool ctrlDown = false;
        bool dragged = false;
    };

    struct CrossfadeGesture
    {
        RangeHandle handle = RangeHandle::none;
        std::size_t lowerZoneIndex = 0;
        std::size_t upperZoneIndex = 0;
        int originalLow = 1;
        int originalHigh = 2;
        int previewLow = 1;
        int previewHigh = 2;
    };

    struct PanGesture
    {
        juce::Point<float> start;
        ZoneMapViewState initialViewport;
    };

    juce::Rectangle<float> getInnerBounds() const;
    juce::Rectangle<int> getNavigationToolbarBounds() const;
    juce::Rectangle<float> computeNormalizedZoneBounds(
        const drs::engine::AuthoringZoneSummary& zone) const;
    juce::Point<float> normalizedContentToCanvas(juce::Point<float> position) const;
    drs::engine::AuthoringZoneSummary getDisplayZoneSummary(std::size_t index) const;
    juce::Rectangle<float> computeZoneBounds(const drs::engine::AuthoringZoneSummary& zone) const;
    juce::Rectangle<float> computeZoneBounds(juce::Rectangle<float> normalizedBounds) const;
    std::vector<ZoneLayout> buildZoneLayouts() const;
    std::vector<std::size_t> buildPaintOrder() const;
    std::optional<std::size_t> findZoneIndexAt(juce::Point<float> position) const;
    std::optional<std::size_t> findSelectedZoneIndex() const;
    std::vector<std::size_t> findSecondarySelectedZoneIndices() const;
    std::vector<std::pair<RangeHandle, juce::Point<float>>> buildHandleCenters(const juce::Rectangle<float>& zoneBounds) const;
    std::vector<std::pair<RangeHandle, juce::Point<float>>> buildCrossfadeHandleCenters(
        const drs::engine::AuthoringZoneSummary& zone,
        const juce::Rectangle<float>& zoneBounds) const;
    RangeHandle findRangeHandleAt(juce::Point<float> position, std::size_t& zoneIndex) const;
    RangeHandle findCrossfadeHandleAt(juce::Point<float> position,
                                      std::size_t& lowerZoneIndex,
                                      std::size_t& upperZoneIndex) const;
    std::optional<std::pair<std::size_t, std::size_t>> findCrossfadePairForZone(std::size_t zoneIndex) const;
    std::vector<drs::engine::AuthoringZoneSummary> buildRangePreviews(
        const RangeGesture& gesture,
        juce::Point<float> position) const;
    int positionToMidiKey(juce::Point<float> position) const;
    int positionToMidiVelocity(juce::Point<float> position) const;
    float velocityToCanvasY(int velocity) const;
    SelectionState buildSelectionStateForZoneIndex(std::size_t index, SelectionMode mode) const;
    SelectionState buildSelectionStateForBounds(juce::Rectangle<float> bounds, SelectionMode mode) const;
    bool requestSelectionByIndex(std::size_t index, SelectionMode mode = SelectionMode::replace);
    bool requestSelectionState(const SelectionState& selectionState);
    void rebuildGeometryCache();
    void refreshViewportUi();
    void showContextMenuAt(juce::Point<int> screenPosition);
    void timerCallback() override;

    std::vector<drs::engine::AuthoringZoneSummary> zoneSummaries;
    std::vector<CachedZoneGeometry> cachedZoneGeometry;
    SelectionState selectionState;
    std::function<void(const std::string& zoneId)> onZoneSelectionRequested;
    std::function<void(const SelectionState& selectionState)> onZoneSelectionStateRequested;
    std::function<void(const std::vector<drs::engine::AuthoringZoneSummary>& zones,
                       const std::string& label)> onZoneRangeCommitRequested;
    std::function<void(const std::string& lowerZoneId,
                       const std::string& upperZoneId,
                       int overlapLow,
                       int overlapHigh)> onVelocityCrossfadeCommitRequested;
    std::function<void(const std::string& zoneId, int midiNote, int velocity)> onZoneAuditionRequested;
    std::function<void(const std::vector<std::string>& zoneIds,
                       const std::string& primaryZoneId)> onShowInStructureRequested;
    std::function<void(std::vector<juce::File>)> onSampleFilesDropped;
    std::function<void()> onDeleteSelectedSampleRequested;
    std::optional<RangeGesture> activeGesture;
    std::optional<CrossfadeGesture> activeCrossfadeGesture;
    std::optional<CrossfadeGesture> focusedCrossfadeGesture;
    std::optional<MarqueeGesture> activeMarqueeGesture;
    std::optional<PanGesture> activePanGesture;
    ZoneMapViewState viewport;
    juce::Component navigationToolbar;
    juce::TextButton fitAllButton;
    juce::TextButton fitSelectedButton;
    juce::TextButton zoomOutButton;
    juce::TextButton zoomInButton;
    juce::Label zoomValueLabel;
    ZoneMapOverview overview;
    std::optional<std::size_t> pendingHoverZoneIndex;
    std::optional<std::size_t> hoveredZoneIndex;
    mutable std::size_t lastVisibleZoneCount = 0;
    bool temporaryPanKeyDown = false;
    bool sampleFileDragActive = false;
};
} // namespace drs::app::authoring
