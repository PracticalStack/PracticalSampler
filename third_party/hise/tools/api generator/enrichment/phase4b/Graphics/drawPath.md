Graphics::drawPath(ScriptObject path, Array area, NotUndefined strokeStyle) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, copies the Path, creates PathStrokeType
Draws the outline (stroke) of a Path. area is optional -- if provided, the path is
scaled to fit via scaleToFit (non-uniform). strokeStyle accepts a number (thickness)
or JSON: {Thickness, EndCapStyle: "butt"/"square"/"rounded", JointStyle: "mitered"/"curved"/"beveled"}.

Pair with:
  fillPath -- filled version of the same path
  setColour -- must set colour before drawing

Anti-patterns:
  - Invalid path silently produces no output -- no error reported
  - Unrecognized EndCapStyle/JointStyle strings produce a default stroke type
    instead of an error (indexOf returns -1, cast to enum)

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawPath()
    -> ApiHelpers::createPathStrokeType() parses strokeStyle
    -> path.scaleToFit(area) if area is an array, else draws at original coords
