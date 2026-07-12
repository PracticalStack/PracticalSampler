Rectangle::translated(Double deltaX, Double deltaY) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle moved by the given offsets. Size unchanged, position shifted.
Positive deltaX moves right, positive deltaY moves down.

Pair with:
  setPosition -- mutating absolute repositioning
  withX/withY -- non-mutating absolute repositioning on a single axis

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION2(translated, ...)
    -> JUCE Rectangle::translated(deltaX, deltaY)
