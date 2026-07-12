Graphics::fillPath(ScriptObject path, Array area) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, copies the Path, optionally scales to area
Fills the interior of a Path using the current colour. area is optional -- if provided,
the path is scaled to fit via scaleToFit (non-uniform). If not an array, path draws at
original coordinates. Silently does nothing if path is empty or invalid.

Pair with:
  drawPath -- outline/stroke version
  setColour -- must set colour before drawing

Anti-patterns:
  - Invalid path silently produces no output -- no error reported
  - Path scaled with non-uniform scaling; shape may distort if aspect ratios differ
  - Unlike drawPath, fillPath always returns early when the path is empty

Source:
  ScriptingGraphics.cpp  GraphicsObject::fillPath()
    -> path.scaleToFit(area) if area is array, else draws at original coords
    -> returns early if path has no bounds
