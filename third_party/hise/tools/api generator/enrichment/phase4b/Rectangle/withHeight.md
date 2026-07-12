Rectangle::withHeight(Double newHeight) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the same position and width but a different height.
Top-left corner unchanged.

Pair with:
  withWidth -- equivalent for horizontal dimension
  withSize -- change both dimensions at once

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withHeight, ...)
    -> JUCE Rectangle::withHeight(newHeight)
