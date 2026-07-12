Graphics::drawTriangle(Array area, Double angle, Double lineThickness) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, creates a Path with rotation
Draws a triangle outline (stroke only) within the area, rotated by angle in radians.
Default (angle=0) is upward-pointing isosceles: vertices at (0.5,0), (1,1), (0,1)
in normalized coords. For filled triangle, use fillTriangle.

Pair with:
  fillTriangle -- filled version
  setColour -- must set colour before drawing

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawTriangle()
    -> Path with rotation transform -> scaleToFit(area)
    -> stroked with PathStrokeType(lineThickness)
