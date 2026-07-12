Rectangle::withLeft(Double newLeft) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with a different left edge but the same right edge.
Width adjusts to accommodate: newWidth = oldRight - newLeft.

Pair with:
  withX -- moves entire rectangle horizontally (preserves width)
  withRight -- equivalent for the right edge

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withLeft, ...)
    -> JUCE Rectangle::withLeft(newLeft)
