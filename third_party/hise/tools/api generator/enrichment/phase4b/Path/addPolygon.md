Path::addPolygon(Array center, Number numSides, Number radius, Number angle) -> undefined

Thread safety: SAFE
Adds a regular polygon centered at the given point. numSides controls the
shape: 3 = triangle, 5 = pentagon, 6 = hexagon, etc. The angle parameter
rotates the polygon in radians.

Required setup:
  const var p = Content.createPath();

Pair with:
  addTriangle -- for triangles with explicit vertex positions
  addStar -- for star shapes with inner/outer radii

Source:
  ScriptingGraphics.cpp  PathObject::addPolygon()
    -> ApiHelpers::getPointFromVar(center)
    -> p.addPolygon(point, numSides, radius, angle)
