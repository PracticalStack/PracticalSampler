Rectangle::withWidth(Double newWidth) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the same position and height but a different width.
Top-left corner unchanged.

Pair with:
  withHeight -- equivalent for vertical dimension
  withSize -- change both dimensions at once
  withRight -- set width implicitly by specifying the right edge

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withWidth, ...)
    -> JUCE Rectangle::withWidth(newWidth)
