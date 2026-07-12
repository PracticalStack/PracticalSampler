Rectangle::withSizeKeepingCentre(Double newWidth, Double newHeight) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the same centre but different dimensions.
Resizes symmetrically around the centre point.

Pair with:
  withSize -- resize anchored to top-left instead of centre
  withCentre -- reposition by centre without resizing

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION2(withSizeKeepingCentre, ...)
    -> JUCE Rectangle::withSizeKeepingCentre(newWidth, newHeight)
