Path::quadraticTo(Number cx, Number cy, Number x, Number y) -> undefined

Thread safety: SAFE
Adds a quadratic Bezier curve from the current position to endpoint (x, y)
using (cx, cy) as the single control point. All four parameters are individual
scalars. A sub-path must be active before calling.

Pair with:
  startNewSubPath -- must have an active sub-path before calling
  cubicTo -- for smoother curves with two control points
  lineTo -- for straight segments between curves

Anti-patterns:
  - Do NOT pass NaN or Inf values -- coordinates are NOT sanitized (unlike
    startNewSubPath/lineTo which use SANITIZED()). Corrupt values silently
    damage path geometry.

Source:
  ScriptingGraphics.cpp  PathObject::quadraticTo()
    -> p.quadraticTo(cx, cy, x, y)
    No SANITIZED() macro applied.
