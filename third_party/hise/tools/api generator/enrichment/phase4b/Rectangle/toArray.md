Rectangle::toArray() -> Array

Thread safety: SAFE
Returns a [x, y, width, height] array representing this rectangle.
Useful for converting back to the traditional array format used by many HISE APIs.

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION(toArray, ...)
    -> constructs Array from rectangle.getX/Y/Width/Height()
