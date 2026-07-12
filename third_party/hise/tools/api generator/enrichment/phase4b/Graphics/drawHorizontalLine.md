Graphics::drawHorizontalLine(Number y, Double x1, Double x2) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Draws a 1-pixel horizontal line at integer y from x1 to x2. Pixel-snapped (no
sub-pixel interpolation). Uses the current colour. For thicker lines, use drawLine.

Pair with:
  drawVerticalLine -- vertical counterpart
  drawLine -- for arbitrary thickness

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawHorizontalLine()
    -> new draw action; x1/x2 SANITIZED against NaN/Inf
