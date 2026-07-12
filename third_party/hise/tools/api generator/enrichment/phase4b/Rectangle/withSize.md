Rectangle::withSize(Double newWidth, Double newHeight) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the same top-left position but different dimensions.

Pair with:
  withSizeKeepingCentre -- resize around the centre instead of top-left
  setSize -- mutating alternative (modifies in place)

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION2(withSize, ...)
    -> JUCE Rectangle::withSize(newWidth, newHeight)
