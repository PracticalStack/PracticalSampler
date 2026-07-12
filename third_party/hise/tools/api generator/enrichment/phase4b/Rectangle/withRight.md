Rectangle::withRight(Double newRight) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with a different right edge but the same left edge.
Width adjusts to accommodate: newWidth = newRight - oldLeft.

Pair with:
  withLeft -- equivalent for the left edge
  withWidth -- set width directly (keeps left edge)

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withRight, ...)
    -> JUCE Rectangle::withRight(newRight)
