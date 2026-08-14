#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace drs::app::authoring
{
class WorkbenchSplitter final : public juce::Component
{
public:
    WorkbenchSplitter();

    void setCurrentHeight(int height) noexcept { currentHeight = height; }
    int getCurrentHeight() const noexcept { return currentHeight; }
    void setOnHeightRequested(std::function<void(int)> callback);
    void setOnSizeToggleRequested(std::function<void()> callback);
    void requestHeight(int height);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

private:
    std::function<void(int)> onHeightRequested;
    std::function<void()> onSizeToggleRequested;
    int currentHeight = 232;
    int dragStartHeight = 232;
    int dragStartScreenY = 0;
};
} // namespace drs::app::authoring
