Path::getBounds(Number scaleFactor) -> Array

Thread safety: SAFE
Returns the bounding rectangle [x, y, width, height] of the path with an
optional scale factor applied. Use scaleFactor=1.0 for unscaled bounds.
Returns a zero-area rectangle for an empty path.

Anti-patterns:
  - Return type changes to a Rectangle object when the project-level
    HISE_USE_SCRIPT_RECTANGLE_OBJECT preprocessor is enabled. Code that indexes
    the result (e.g., bounds[2] for width) only works with the default array format.

Source:
  ScriptingGraphics.cpp  PathObject::getBounds()
    -> p.getBoundsTransformed(AffineTransform::scale(scaleFactor))
    -> ApiHelpers::getVarRectangle(useRectangleClass, bounds)
