Graphics::drawEllipse(Array area, Double lineThickness) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Draws an ellipse outline (stroke only) inscribed in the area. Square area = circle.
Uses the current colour. For filled ellipse, use fillEllipse.

Pair with:
  fillEllipse -- filled version
  setColour -- must set colour before drawing

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawEllipse()
    -> new draw action with area and lineThickness
