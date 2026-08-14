#pragma once

#include "shared/authoring/ZoneMapRenderPolicy.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <string>
#include <vector>

namespace drs::app::authoring
{
class ZoneMapOverview final : public juce::Component
{
public:
    struct Zone
    {
        std::string id;
        std::string groupId;
        juce::Rectangle<float> normalizedBounds;
    };

    ZoneMapOverview();

    void setZones(std::vector<Zone> nextZones);
    void setSelection(std::vector<std::string> zoneIds);
    void setViewport(juce::Rectangle<float> normalizedViewport);
    void setOnViewportOriginRequested(std::function<void(juce::Point<float>)> callback);

    juce::Rectangle<float> getViewportFrameBounds() const;
    std::size_t getGroupCount() const noexcept { return groups.size(); }
    std::size_t getSelectedZoneCount() const noexcept { return selectedZoneIds.size(); }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

private:
    struct Group
    {
        std::string id;
        juce::Rectangle<float> normalizedBounds;
    };

    juce::Rectangle<float> getPlotBounds() const;
    juce::Rectangle<float> contentToComponent(juce::Rectangle<float> content) const;
    juce::Point<float> componentToContent(juce::Point<float> position) const;
    void requestViewportAt(juce::Point<float> contentPosition);

    std::vector<Zone> zones;
    std::vector<Group> groups;
    std::vector<std::string> selectedZoneIds;
    juce::Rectangle<float> viewport { 0.0f, 0.0f, 1.0f, 1.0f };
    juce::Point<float> dragOffset;
    std::function<void(juce::Point<float>)> onViewportOriginRequested;
    bool draggingViewport = false;
};
} // namespace drs::app::authoring
