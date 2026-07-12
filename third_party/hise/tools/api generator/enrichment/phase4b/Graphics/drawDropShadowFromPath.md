Graphics::drawDropShadowFromPath(ScriptObject path, Array area, Colour colour, Number radius, Array offset) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, copies the Path, creates melatonin::DropShadow
Draws a drop shadow following a Path outline. Path is scaled to fit the area (non-uniform).
offset is [x, y] controlling shadow direction (converted to int). Does not require a layer.
Silently does nothing if path is empty or not a valid Path object.

Pair with:
  drawInnerShadowFromPath -- for inset version of the same shadow
  fillPath -- typically draw shadow first, then fill the path on top

Anti-patterns:
  - Invalid path silently produces no output -- no error reported
  - Path is scaled with non-uniform scaling; shadow shape may distort if path and
    area aspect ratios differ
  - Offset converted to integer -- no sub-pixel shadow offsets

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawDropShadowFromPath()
    -> drawDropShadowFromPath<melatonin::DropShadow>() template
    -> path.scaleToFit(area, preserveProportions=false)
    -> melatonin::DropShadow::render() with offset
