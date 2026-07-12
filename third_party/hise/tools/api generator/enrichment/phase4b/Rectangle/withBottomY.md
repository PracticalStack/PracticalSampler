Rectangle::withBottomY(Double newBottomY) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the same size, moved vertically so its bottom edge
is at newBottomY. Top edge becomes newBottomY - height.

Pair with:
  withBottom -- changes height by moving bottom edge (keeps top fixed)
  withY -- positions by top edge instead of bottom edge

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withBottomY, ...)
    -> JUCE Rectangle::withBottomY(newBottomY)
