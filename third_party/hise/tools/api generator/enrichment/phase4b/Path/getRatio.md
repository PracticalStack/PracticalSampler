Path::getRatio() -> Double

Thread safety: SAFE
Returns the width-to-height aspect ratio of the path's bounding box.
A ratio of 1.0 = square, >1.0 = wider than tall, <1.0 = taller than wide.

Anti-patterns:
  - Returns Infinity or NaN if the path has zero height (e.g., a purely
    horizontal line). No validation on the division result.

Source:
  ScriptingGraphics.cpp  PathObject::getRatio()
    -> p.getBounds().getWidth() / p.getBounds().getHeight()
