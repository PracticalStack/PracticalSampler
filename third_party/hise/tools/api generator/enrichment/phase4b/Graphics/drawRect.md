Graphics::drawRect(Array area, Double borderSize) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Draws a rectangle outline (stroke only) using the current colour. borderSize controls
stroke width. Border is drawn inward from the area edges. For filled rectangle, use fillRect.

Pair with:
  fillRect -- filled version
  setColour -- must set colour before drawing

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawRect()
    -> new draw action; borderSize SANITIZED
