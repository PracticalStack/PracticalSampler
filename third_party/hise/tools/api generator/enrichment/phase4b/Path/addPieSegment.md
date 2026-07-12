Path::addPieSegment(Array area, Number fromRadians, Number toRadians, Number innerCircleProportionalSize) -> undefined

Thread safety: SAFE
Adds a pie segment (wedge shape) with optional inner circle cutout. The
innerCircleProportionalSize controls the donut hole: 0.0 = solid wedge from
center, 0.5 = half-radius cutout (ring segment), 1.0 = zero-width ring.
Angles are clockwise from 3 o'clock.

Required setup:
  const var p = Content.createPath();

Pair with:
  addArc -- for arc outlines without the wedge fill
  addEllipse -- for complete ellipses

Source:
  ScriptingGraphics.cpp  PathObject::addPieSegment()
    -> SANITIZED() on fromRadians, toRadians
    -> ApiHelpers::getRectangleFromVar(area)
    -> p.addPieSegment(rect, from, to, innerCircleProportionalSize)
