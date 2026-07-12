Rectangle::constrainedWithin(var targetArea) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle moved to fit within the target area, keeping size unchanged.
If this rectangle is larger than the target in either dimension, the result is clamped.

Dispatch/mechanics:
  getRectangleArgs(targetArea) -> JUCE Rectangle::constrainedWithin(other)
  Accepts Rectangle object, [x,y,w,h] array, or 4 separate numbers.

Pair with:
  contains -- check if a rectangle is already within bounds
  getIntersection -- get the overlapping region instead of moving

Anti-patterns:
  - Passing an invalid argument (wrong type, wrong array length) silently returns
    a copy of the original rectangle unchanged instead of reporting an error.

Source:
  RectangleDynamicObject.cpp  FunctionMap constructor, ADD_FUNCTION(constrainedWithin, ...)
    -> getRectangleArgs(a, other) -> JUCE Rectangle::constrainedWithin(other)
