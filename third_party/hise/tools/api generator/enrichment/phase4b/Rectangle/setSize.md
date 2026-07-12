Rectangle::setSize(Double width, Double height) -> undefined

Thread safety: SAFE
Changes this rectangle's width and height, keeping the top-left corner unchanged.
Mutates in place, returns nothing.

Pair with:
  withSize -- non-mutating alternative (returns a new Rectangle)
  withSizeKeepingCentre -- resize around the centre instead of top-left

Source:
  RectangleDynamicObject.cpp  ADD_VOID_FUNCTION2(setSize, ...)
    -> JUCE Rectangle::setSize(width, height)
