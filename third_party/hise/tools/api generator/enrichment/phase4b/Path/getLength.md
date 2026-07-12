Path::getLength() -> Double

Thread safety: SAFE
Returns the total length of the path in pixels. Computed by flattening curves
into line segments and summing lengths. Returns 0.0 for an empty path.

Source:
  ScriptingGraphics.cpp  PathObject::getLength()
    -> p.getLength(AffineTransform::scale(1.0f), 1.0f)
