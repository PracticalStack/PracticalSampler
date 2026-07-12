Rectangle::getUnion(var otherRect) -> ScriptObject

Thread safety: SAFE
Returns the smallest rectangle containing both this and the given rectangle (bounding box).

Dispatch/mechanics:
  getRectangleArgs(otherRect) -> JUCE Rectangle::getUnion(other)
  Accepts Rectangle object, [x,y,w,h] array, or 4 separate numbers.

Pair with:
  getIntersection -- overlapping region (opposite operation)

Anti-patterns:
  - Invalid argument silently returns a copy of the original rectangle unchanged
    instead of reporting an error.

Source:
  RectangleDynamicObject.cpp  FunctionMap constructor, ADD_FUNCTION(getUnion, ...)
    -> getRectangleArgs(a, other) -> JUCE Rectangle::getUnion(other)
