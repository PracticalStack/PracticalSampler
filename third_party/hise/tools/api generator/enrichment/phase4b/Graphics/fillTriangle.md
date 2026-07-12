Graphics::fillTriangle(Array area, Double angle) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, creates a Path with rotation
Fills a triangle within the area, rotated by angle in radians. Default (angle=0)
is upward-pointing isosceles: vertices at (0.5,0), (1,1), (0,1) in normalized coords.
For outline only, use drawTriangle.

Pair with:
  drawTriangle -- outline version
  setColour -- must set colour before drawing

Source:
  ScriptingGraphics.cpp  GraphicsObject::fillTriangle()
    -> Path with rotation transform -> scaleToFit(area)
    -> filled via fillPath internally
