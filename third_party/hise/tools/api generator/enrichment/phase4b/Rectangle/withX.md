Rectangle::withX(Double newX) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the same size and y-position but a different x-position.
Moves the entire rectangle horizontally, preserving width.

Pair with:
  withY -- equivalent for vertical axis
  withLeft -- changes width by moving left edge (keeps right edge fixed)
  setPosition -- mutating alternative for repositioning both axes

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withX, ...)
    -> JUCE Rectangle::withX(newX)
