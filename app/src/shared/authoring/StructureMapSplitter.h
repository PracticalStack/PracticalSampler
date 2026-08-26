#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace drs::app::authoring
{
class StructureMapSplitter final : public juce::Component
{
public:
    StructureMapSplitter();

    void setCurrentStructureWidth(int width) noexcept { currentStructureWidth = width; }
    int getCurrentStructureWidth() const noexcept { return currentStructureWidth; }
    void setOnWidthRequested(std::function<void(int)> callback);
    void setOnResetRequested(std::function<void()> callback);
    void requestWidth(int width);
    void requestReset();

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

private:
    std::function<void(int)> onWidthRequested;
    std::function<void()> onResetRequested;
    int currentStructureWidth = 200;
    int dragStartWidth = 200;
    int dragStartScreenX = 0;
};
} // namespace drs::app::authoring
