#pragma once

#include "drs/engine/AuthoringSession.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <optional>
#include <vector>

namespace drs::app::authoring
{
class ZoneMapCanvas final : public juce::Component,
                            public juce::FileDragAndDropTarget
{
public:
    enum class RangeHandle
    {
        none,
        keyLow,
        keyHigh,
        velocityHigh,
        velocityLow
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
        std::function<void(const drs::engine::AuthoringZoneSummary& zone, const std::string& label)> nextCallback);
    void setOnZoneAuditionRequested(
        std::function<void(const std::string& zoneId, int midiNote, int velocity)> nextCallback);
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
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

private:
    struct ZoneLayout
    {
        std::size_t index = 0;
        juce::Rectangle<float> bounds;
    };

    struct RangeGesture
    {
        RangeHandle handle = RangeHandle::none;
        std::size_t zoneIndex = 0;
        drs::engine::AuthoringZoneSummary originalZone;
        drs::engine::AuthoringZoneSummary previewZone;
    };

    struct MarqueeGesture
    {
        juce::Point<float> start;
        juce::Point<float> current;
        bool ctrlDown = false;
        bool dragged = false;
    };

    juce::Rectangle<float> getInnerBounds() const;
    drs::engine::AuthoringZoneSummary getDisplayZoneSummary(std::size_t index) const;
    juce::Rectangle<float> computeZoneBounds(const drs::engine::AuthoringZoneSummary& zone) const;
    std::vector<ZoneLayout> buildZoneLayouts() const;
    std::vector<std::size_t> buildPaintOrder() const;
    std::optional<std::size_t> findZoneIndexAt(juce::Point<float> position) const;
    std::optional<std::size_t> findSelectedZoneIndex() const;
    std::vector<std::size_t> findSecondarySelectedZoneIndices() const;
    std::vector<std::pair<RangeHandle, juce::Point<float>>> buildHandleCenters(const juce::Rectangle<float>& zoneBounds) const;
    RangeHandle findRangeHandleAt(juce::Point<float> position, std::size_t& zoneIndex) const;
    drs::engine::AuthoringZoneSummary buildRangePreview(const RangeGesture& gesture, juce::Point<float> position) const;
    int positionToMidiKey(juce::Point<float> position) const;
    int positionToMidiVelocity(juce::Point<float> position) const;
    SelectionState buildSelectionStateForZoneIndex(std::size_t index, SelectionMode mode) const;
    SelectionState buildSelectionStateForBounds(juce::Rectangle<float> bounds, SelectionMode mode) const;
    bool requestSelectionByIndex(std::size_t index, SelectionMode mode = SelectionMode::replace);
    bool requestSelectionState(const SelectionState& selectionState);
    void showContextMenuAt(juce::Point<int> screenPosition);

    std::vector<drs::engine::AuthoringZoneSummary> zoneSummaries;
    SelectionState selectionState;
    std::function<void(const std::string& zoneId)> onZoneSelectionRequested;
    std::function<void(const SelectionState& selectionState)> onZoneSelectionStateRequested;
    std::function<void(const drs::engine::AuthoringZoneSummary& zone, const std::string& label)> onZoneRangeCommitRequested;
    std::function<void(const std::string& zoneId, int midiNote, int velocity)> onZoneAuditionRequested;
    std::function<void(std::vector<juce::File>)> onSampleFilesDropped;
    std::function<void()> onDeleteSelectedSampleRequested;
    std::optional<RangeGesture> activeGesture;
    std::optional<MarqueeGesture> activeMarqueeGesture;
    bool sampleFileDragActive = false;
};
} // namespace drs::app::authoring
