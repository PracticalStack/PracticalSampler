Graphics::fillRoundedRectangle(Array area, NotUndefined cornerData) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Fills a rounded rectangle using the current colour. cornerData accepts a number
(uniform radius) or JSON: {CornerSize: float, Rounded: [topLeft, topRight,
bottomLeft, bottomRight]}. When all corners are false, falls back to fillRect.

Pair with:
  drawRoundedRectangle -- outline version
  setColour -- must set colour before drawing

Source:
  ScriptingGraphics.cpp  GraphicsObject::fillRoundedRectangle()
    -> if JSON with Rounded array: Path::addRoundedRectangle with per-corner bools
    -> CornerSize SANITIZED
