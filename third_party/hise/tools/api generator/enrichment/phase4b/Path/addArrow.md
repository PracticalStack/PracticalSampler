Path::addArrow(Array start, Array end, Number thickness, Number headWidth, Number headLength) -> undefined

Thread safety: SAFE
Adds an arrow shape from start to end with specified shaft thickness and
arrowhead dimensions. The arrowhead appears at the end point.

Required setup:
  const var p = Content.createPath();

Source:
  ScriptingGraphics.cpp  PathObject::addArrow()
    -> ApiHelpers::getPointFromVar(start), getPointFromVar(end)
    -> p.addArrow(Line<float>(p1, p2), thickness, headWidth, headLength)
