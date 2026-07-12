Path::addQuadrilateral(Array xy1, Array xy2, Array xy3, Array xy4) -> undefined

Thread safety: SAFE
Adds a four-sided polygon defined by four corner points. Unlike addRectangle,
this allows non-rectangular shapes (trapezoids, parallelograms). Points are
connected in order and closed automatically.

Required setup:
  const var p = Content.createPath();

Source:
  ScriptingGraphics.cpp  PathObject::addQuadrilateral()
    -> direct array indexing: xy1[0], xy1[1], xy2[0], ...
    -> p.addQuadrilateral(x1,y1, x2,y2, x3,y3, x4,y4)
