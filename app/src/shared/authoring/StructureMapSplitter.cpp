#include "shared/authoring/StructureMapSplitter.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

namespace drs::app::authoring
{
StructureMapSplitter::StructureMapSplitter()
{
    setComponentID("authoringStructureMapSplitter");
    setTitle("Structure Viewer width splitter");
    setDescription("Drag horizontally to resize the Structure Viewer and Zone Map.");
    setHelpText("Use Left and Right to resize. Hold Shift for larger steps. Press Home or Return to restore the default width.");
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void StructureMapSplitter::setOnWidthRequested(std::function<void(int)> callback)
{
    onWidthRequested = std::move(callback);
}

void StructureMapSplitter::setOnResetRequested(std::function<void()> callback)
{
    onResetRequested = std::move(callback);
}

void StructureMapSplitter::requestWidth(const int width)
{
    if (onWidthRequested)
        onWidthRequested(width);
}

void StructureMapSplitter::requestReset()
{
    if (onResetRequested)
        onResetRequested();
}

void StructureMapSplitter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(visual::surfaceSubtle);
    g.fillRect(bounds);
    g.setColour(visual::borderStrong.withAlpha(0.72f));
    const auto centreX = bounds.getCentreX();
    g.drawVerticalLine(static_cast<int>(centreX), bounds.getCentreY() - 24.0f,
                       bounds.getCentreY() + 24.0f);
    if (hasKeyboardFocus(false))
    {
        g.setColour(visual::focus);
        g.drawRect(bounds.reduced(0.5f), 1.0f);
    }
}

void StructureMapSplitter::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    dragStartWidth = currentStructureWidth;
    dragStartScreenX = event.getScreenPosition().x;
}

void StructureMapSplitter::mouseDrag(const juce::MouseEvent& event)
{
    requestWidth(dragStartWidth + event.getScreenPosition().x - dragStartScreenX);
}

void StructureMapSplitter::mouseDoubleClick(const juce::MouseEvent&)
{
    requestReset();
}

bool StructureMapSplitter::keyPressed(const juce::KeyPress& key)
{
    const auto step = key.getModifiers().isShiftDown() ? 32 : 8;
    if (key == juce::KeyPress::leftKey)
    {
        requestWidth(currentStructureWidth - step);
        return true;
    }
    if (key == juce::KeyPress::rightKey)
    {
        requestWidth(currentStructureWidth + step);
        return true;
    }
    if (key == juce::KeyPress::homeKey
        || key == juce::KeyPress::returnKey
        || key == juce::KeyPress::spaceKey)
    {
        requestReset();
        return true;
    }
    return false;
}

void StructureMapSplitter::focusGained(FocusChangeType)
{
    repaint();
}

void StructureMapSplitter::focusLost(FocusChangeType)
{
    repaint();
}
} // namespace drs::app::authoring
