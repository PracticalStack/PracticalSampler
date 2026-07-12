Rectangle::withBottom(Double newBottom) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with a different bottom edge but the same top edge.
Height adjusts to accommodate: newHeight = newBottom - top.

Pair with:
  withBottomY -- repositions by bottom edge instead of resizing
  withRight -- equivalent operation for the horizontal axis

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withBottom, ...)
    -> JUCE Rectangle::withBottom(newBottom)
