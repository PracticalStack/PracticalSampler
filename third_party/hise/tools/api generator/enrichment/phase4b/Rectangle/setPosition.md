Rectangle::setPosition(Double x, Double y) -> undefined

Thread safety: SAFE
Moves this rectangle's top-left corner to the given coordinates, keeping size unchanged.
Mutates in place, returns nothing.

Pair with:
  withX/withY -- non-mutating alternatives for single-axis repositioning
  translated -- non-mutating relative offset

Source:
  RectangleDynamicObject.cpp  ADD_VOID_FUNCTION2(setPosition, ...)
    -> JUCE Rectangle::setPosition(x, y)
