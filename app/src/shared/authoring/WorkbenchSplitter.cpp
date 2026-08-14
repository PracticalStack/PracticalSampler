#include "shared/authoring/WorkbenchSplitter.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

namespace drs::app::authoring
{
WorkbenchSplitter::WorkbenchSplitter()
{
    setComponentID("authoringWorkbenchSplitter");
    setTitle("Workbench size splitter");
    setDescription("Drag vertically to resize the authoring workbench.");
    setHelpText("Use Up and Down to resize. Hold Shift for larger steps. Press Return to switch between standard and focused sizes.");
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
}

void WorkbenchSplitter::setOnHeightRequested(std::function<void(int)> callback)
{
    onHeightRequested = std::move(callback);
}

void WorkbenchSplitter::setOnSizeToggleRequested(std::function<void()> callback)
{
    onSizeToggleRequested = std::move(callback);
}

void WorkbenchSplitter::requestHeight(const int height)
{
    if (onHeightRequested)
        onHeightRequested(height);
}

void WorkbenchSplitter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(visual::surfaceSubtle);
    g.fillRect(bounds);
    g.setColour(visual::borderStrong.withAlpha(0.72f));
    const auto centreY = bounds.getCentreY();
    g.drawHorizontalLine(static_cast<int>(centreY), bounds.getCentreX() - 24.0f, bounds.getCentreX() + 24.0f);
    if (hasKeyboardFocus(false))
    {
        g.setColour(visual::focus);
        g.drawRect(bounds.reduced(0.5f), 1.0f);
    }
}

void WorkbenchSplitter::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    dragStartHeight = currentHeight;
    dragStartScreenY = event.getScreenPosition().y;
}

void WorkbenchSplitter::mouseDrag(const juce::MouseEvent& event)
{
    requestHeight(dragStartHeight + dragStartScreenY - event.getScreenPosition().y);
}

void WorkbenchSplitter::mouseDoubleClick(const juce::MouseEvent&)
{
    if (onSizeToggleRequested)
        onSizeToggleRequested();
}

bool WorkbenchSplitter::keyPressed(const juce::KeyPress& key)
{
    const auto step = key.getModifiers().isShiftDown() ? 32 : 8;
    if (key == juce::KeyPress::upKey)
    {
        requestHeight(currentHeight + step);
        return true;
    }
    if (key == juce::KeyPress::downKey)
    {
        requestHeight(currentHeight - step);
        return true;
    }
    if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
    {
        if (onSizeToggleRequested)
            onSizeToggleRequested();
        return true;
    }
    return false;
}

void WorkbenchSplitter::focusGained(FocusChangeType)
{
    repaint();
}

void WorkbenchSplitter::focusLost(FocusChangeType)
{
    repaint();
}
} // namespace drs::app::authoring
