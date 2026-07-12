Path::startNewSubPath(Number x, Number y) -> undefined

Thread safety: SAFE
Begins a new sub-path at the given coordinates without drawing anything. Sets
the "current position" for subsequent lineTo/quadraticTo/cubicTo commands.
Does NOT clear existing geometry. Coordinates are sanitized against NaN/Inf.

Pair with:
  lineTo -- draw from the started position
  closeSubPath -- close the shape back to the start point
  clear -- to reset the path before starting fresh

Source:
  ScriptingGraphics.cpp  PathObject::startNewSubPath()
    -> p.startNewSubPath(SANITIZED(x), SANITIZED(y))
