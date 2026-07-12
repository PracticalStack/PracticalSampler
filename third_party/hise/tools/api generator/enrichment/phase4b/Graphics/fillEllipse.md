Graphics::fillEllipse(Array area) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Fills an ellipse inscribed in the area using the current colour. Square area = filled circle.
For outline only, use drawEllipse.

Pair with:
  drawEllipse -- outline version
  setColour -- must set colour before drawing

Source:
  ScriptingGraphics.cpp  GraphicsObject::fillEllipse()
    -> new draw action with area
