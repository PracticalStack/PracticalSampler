Graphics::drawDropShadow(Array area, Colour colour, Number radius) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, creates melatonin::DropShadow, copies a Path
Draws a rectangular drop shadow around the specified area using the melatonin blur library.
Shadow extends equally in all directions (offset is fixed at 0,0). Does not require a layer.

Pair with:
  drawDropShadowFromPath -- for non-rectangular shadows or shadows with custom offset
  drawInnerShadowFromPath -- for inset shadows

Anti-patterns:
  - Area is converted to integer coordinates -- sub-pixel positioning is lost.
    Use drawDropShadowFromPath with a rectangular path for float precision.
  - Shadow offset is always (0,0). For directional shadows, use drawDropShadowFromPath.

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawDropShadow()
    -> rectangle converted to Path
    -> melatonin::DropShadow::render() with zero offset
