Path::contains(Array point) -> Integer

Thread safety: SAFE
Tests whether a point [x, y] lies within the path. Returns true if the point
is inside any closed sub-path (uses winding-number rule). Only meaningful for
closed paths -- open paths have no defined interior.

Source:
  ScriptingGraphics.cpp  PathObject::contains()
    -> ApiHelpers::getPointFromVar(point)
    -> p.contains(point)
