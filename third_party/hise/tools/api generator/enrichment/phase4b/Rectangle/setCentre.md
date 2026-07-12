Rectangle::setCentre(Double centerX, Double centerY) -> undefined

Thread safety: SAFE
Moves this rectangle so its centre is at the given position, keeping size unchanged.
Mutates in place, returns nothing.

Pair with:
  withCentre -- non-mutating alternative (returns a new Rectangle)

Source:
  RectangleDynamicObject.cpp  ADD_VOID_FUNCTION2(setCentre, ...)
    -> JUCE Rectangle::setCentre(centerX, centerY)
