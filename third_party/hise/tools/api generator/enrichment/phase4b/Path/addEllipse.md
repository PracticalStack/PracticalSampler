Path::addEllipse(Array area) -> undefined

Thread safety: SAFE
Adds a closed ellipse filling the bounding rectangle. For a circle, use a
square bounding rectangle (equal width and height).

Required setup:
  const var p = Content.createPath();

Source:
  ScriptingGraphics.cpp  PathObject::addEllipse()
    -> ApiHelpers::getRectangleFromVar(area)
    -> p.addEllipse(rect)
