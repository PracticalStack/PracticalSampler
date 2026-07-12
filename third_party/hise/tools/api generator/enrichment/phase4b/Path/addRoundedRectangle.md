Path::addRoundedRectangle(Array area, Number cornerSize) -> undefined

Thread safety: SAFE
Adds a closed rectangle with uniformly rounded corners. cornerSize controls
the radius of all four corners. A cornerSize of 0.0 produces sharp corners
identical to addRectangle. For per-corner control, use
addRoundedRectangleCustomisable.

Required setup:
  const var p = Content.createPath();

Source:
  ScriptingGraphics.cpp  PathObject::addRoundedRectangle()
    -> ApiHelpers::getRectangleFromVar(area)
    -> p.addRoundedRectangle(rect, cornerSize)
