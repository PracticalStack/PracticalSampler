Path::addRoundedRectangleCustomisable(Array area, Array cornerSizeXY, Array boolCurves) -> undefined

Thread safety: SAFE
Adds a rectangle with per-corner rounding control. cornerSizeXY is [radiusX,
radiusY] for elliptical corners. boolCurves is [topLeft, topRight, bottomLeft,
bottomRight] -- true rounds the corner, false keeps it sharp.

Required setup:
  const var p = Content.createPath();

Pair with:
  addRoundedRectangle -- simpler alternative with uniform corner radius

Source:
  ScriptingGraphics.cpp  PathObject::addRoundedRectangleCustomisable()
    -> ApiHelpers::getRectangleFromVar(area)
    -> direct array indexing on cornerSizeXY and boolCurves
    -> p.addRoundedRectangle(x, y, w, h, radiusX, radiusY, tl, tr, bl, br)
