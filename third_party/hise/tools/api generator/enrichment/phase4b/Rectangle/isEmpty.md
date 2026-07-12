Rectangle::isEmpty() -> Integer

Thread safety: SAFE
Returns true if the rectangle's width or height is zero or less.
Useful for checking the result of getIntersection() to determine actual overlap.

Source:
  RectangleDynamicObject.cpp  FunctionMap constructor, ADD_FUNCTION(isEmpty, ...)
    -> JUCE Rectangle::isEmpty()
