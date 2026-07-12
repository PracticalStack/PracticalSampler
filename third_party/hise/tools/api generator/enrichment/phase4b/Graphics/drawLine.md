Graphics::drawLine(Double x1, Double x2, Double y1, Double y2, Double lineThickness) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Draws a line from (x1, y1) to (x2, y2) with the specified thickness. Uses current colour.
IMPORTANT: Parameter order is (x1, x2, y1, y2) -- x-coordinates grouped before y-coordinates.
All parameters SANITIZED against NaN/Inf.

Anti-patterns:
  - Do NOT assume point-by-point order (x1, y1, x2, y2) -- the actual order is
    (x1, x2, y1, y2). Despite the unusual order, the line draws correctly from
    point (x1, y1) to (x2, y2) due to internal reordering.

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawLine()
    -> creates action with reordered params: (x1, y1, x2, y2)
    -> JUCE g.drawLine(startX=x1, startY=y1, endX=x2, endY=y2, thickness)
