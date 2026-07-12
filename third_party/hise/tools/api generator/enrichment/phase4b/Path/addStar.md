Path::addStar(Array center, Number numPoints, Number innerRadius, Number outerRadius, Number angle) -> undefined

Thread safety: SAFE
Adds a star shape centered at the given point. numPoints is the number of tips
(e.g., 5 for a five-pointed star). innerRadius and outerRadius control the
concave and tip vertices. The angle parameter rotates the star in radians.

Required setup:
  const var p = Content.createPath();

Pair with:
  addPolygon -- for regular polygons (all vertices at same radius)

Source:
  ScriptingGraphics.cpp  PathObject::addStar()
    -> ApiHelpers::getPointFromVar(center)
    -> p.addStar(point, numPoints, innerRadius, outerRadius, angle)
