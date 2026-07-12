Path::addRectangle(Array area) -> undefined

Thread safety: SAFE
Adds a closed rectangle to the path. For rounded corners, use
addRoundedRectangle or addRoundedRectangleCustomisable instead.

Required setup:
  const var p = Content.createPath();

Source:
  ScriptingGraphics.cpp  PathObject::addRectangle()
    -> ApiHelpers::getRectangleFromVar(area)
    -> p.addRectangle(rect)
