Rectangle::getIntersection(var otherRect) -> ScriptObject

Thread safety: SAFE
Returns the largest rectangle fitting within both this and the given rectangle.
If the rectangles do not overlap, returns an empty rectangle (zero width/height).

Dispatch/mechanics:
  getRectangleArgs(otherRect) -> copy of this -> JUCE intersectRectangle(other)
  intersectRectangle is a mutating JUCE method; the copy is returned as a new Rectangle.

Pair with:
  intersects -- boolean check before computing the intersection
  getUnion -- bounding box of both rectangles (opposite operation)

Anti-patterns:
  - Invalid argument silently returns a copy of the original rectangle unchanged,
    not an empty rectangle. Always validate input format.

Source:
  RectangleDynamicObject.cpp  FunctionMap constructor, ADD_FUNCTION(getIntersection, ...)
    -> getRectangleArgs(a, other) -> r.intersectRectangle(other) -> create(a, r)
