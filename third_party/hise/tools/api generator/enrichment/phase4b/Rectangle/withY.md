Rectangle::withY(Double newY) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the same size and x-position but a different y-position.
Moves the entire rectangle vertically, preserving height.

Pair with:
  withX -- equivalent for horizontal axis
  withBottomY -- positions by bottom edge instead of top edge
  setPosition -- mutating alternative for repositioning both axes

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withY, ...)
    -> JUCE Rectangle::withY(newY)
