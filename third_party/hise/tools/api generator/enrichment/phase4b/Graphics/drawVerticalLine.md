Graphics::drawVerticalLine(Number x, Double y1, Double y2) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Draws a 1-pixel vertical line at integer x from y1 to y2. Pixel-snapped (no
sub-pixel interpolation). Uses the current colour. For thicker lines, use drawLine.

Pair with:
  drawHorizontalLine -- horizontal counterpart
  drawLine -- for arbitrary thickness

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawVerticalLine()
    -> new draw action; y1/y2 SANITIZED against NaN/Inf
