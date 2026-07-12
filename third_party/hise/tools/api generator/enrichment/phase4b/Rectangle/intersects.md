Rectangle::intersects(var otherRect) -> bool

Thread safety: SAFE
Returns true if any part of another rectangle overlaps this one.

Dispatch/mechanics:
  getRectangleArgs(otherRect) -> JUCE Rectangle::intersects(other)
  Accepts Rectangle object, [x,y,w,h] array, or 4 separate numbers.

Pair with:
  getIntersection -- compute the actual overlapping region
  contains -- test for full containment rather than any overlap

Anti-patterns:
  - Invalid argument silently returns false instead of reporting an error.

Source:
  RectangleDynamicObject.cpp  FunctionMap constructor, ADD_FUNCTION(intersects, ...)
    -> getRectangleArgs(a, other) -> JUCE Rectangle::intersects(other)
