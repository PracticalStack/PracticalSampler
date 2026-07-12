Graphics::drawRoundedRectangle(Array area, NotUndefined cornerData, Double borderSize) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Draws a rounded rectangle outline using the current colour. cornerData accepts a
number (uniform radius) or JSON: {CornerSize: float, Rounded: [topLeft, topRight,
bottomLeft, bottomRight]}. borderSize controls stroke width.

Pair with:
  fillRoundedRectangle -- filled version
  setColour -- must set colour before drawing

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawRoundedRectangle()
    -> if JSON with Rounded array: Path::addRoundedRectangle with per-corner bools
    -> if all corners false: falls back to drawRect equivalent
    -> CornerSize SANITIZED
