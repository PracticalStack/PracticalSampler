Graphics::drawInnerShadowFromPath(ScriptObject path, Array area, Colour colour, Number radius, Array offset) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, copies the Path, creates melatonin::InnerShadow
Draws an inner shadow inside a Path outline (inset/recessed effect). Path is scaled
to fit the area (non-uniform). offset controls light direction (converted to int).
Does not require a layer. Silently does nothing if path is empty or invalid.

Pair with:
  drawDropShadowFromPath -- for outward-extending shadows
  fillPath -- typically fill the path first, then add inner shadow on top

Anti-patterns:
  - Invalid path silently produces no output -- no error reported
  - Path scaled with non-uniform scaling; shape may distort
  - Offset converted to integer -- no sub-pixel shadow offsets

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawInnerShadowFromPath()
    -> drawDropShadowFromPath<melatonin::InnerShadow>() template
    -> path.scaleToFit(area, preserveProportions=false)
