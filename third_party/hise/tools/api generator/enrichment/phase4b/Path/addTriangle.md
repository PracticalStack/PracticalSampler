Path::addTriangle(Array xy1, Array xy2, Array xy3) -> undefined

Thread safety: SAFE
Adds a triangle defined by three corner points. Each point is a [x, y] array.
Points are connected in order and closed automatically. For regular triangles
without explicit vertex control, use addPolygon with numSides=3.

Required setup:
  const var p = Content.createPath();

Source:
  ScriptingGraphics.cpp  PathObject::addTriangle()
    -> direct array indexing: xy1[0], xy1[1], xy2[0], ...
    -> p.addTriangle(x1,y1, x2,y2, x3,y3)
