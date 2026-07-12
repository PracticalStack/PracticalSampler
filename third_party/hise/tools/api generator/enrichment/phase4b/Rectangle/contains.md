Rectangle::contains(var otherRectOrPoint) -> Integer

Thread safety: SAFE
Tests whether this rectangle fully contains another rectangle or contains a point.
Accepts rectangle (Rectangle object, [x,y,w,h] array, 4 numbers) or point ([x,y] array, 2 numbers).

Dispatch/mechanics:
  Tries getRectangleArgs first (4-arg or Rectangle object) -> JUCE Rectangle::contains(rect)
  Falls back to getPointArgs (2-arg or [x,y] array) -> JUCE Rectangle::contains(point)
  Returns false if argument cannot be parsed as either format.

Pair with:
  intersects -- test for any overlap rather than full containment
  constrainedWithin -- move a rectangle to fit within bounds

Anti-patterns:
  - Two-argument calls are always parsed as a point, not a rectangle.
    contains(50, 100) checks point (50,100), not a rectangle.
  - Invalid arguments silently return false instead of reporting an error.

Source:
  RectangleDynamicObject.cpp  FunctionMap constructor, ADD_FUNCTION(contains, ...)
    -> getRectangleArgs(a, other) -> JUCE Rectangle::contains(other)
    -> else getPointArgs(a, point) -> JUCE Rectangle::contains(point)
