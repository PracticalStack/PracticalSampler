Graphics::rotate(Double angleInRadian, Array center) -> undefined

Thread safety: UNSAFE -- allocates a new draw action containing an AffineTransform
Rotates all subsequent drawing by angleInRadian around the center point [x, y].
Positive = clockwise. Cumulative with existing transforms. Persists for the rest
of the paint callback. angle is SANITIZED against NaN/Inf.

Pair with:
  flip -- for mirror transforms

Source:
  ScriptingGraphics.cpp  GraphicsObject::rotate()
    -> AffineTransform::rotation(angle, centerX, centerY)
    -> added as addTransform draw action
