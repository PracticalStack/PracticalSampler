Path::cubicTo(Array cxy1, Array cxy2, Number x, Number y) -> undefined

Thread safety: SAFE
Adds a cubic Bezier curve from the current position to endpoint (x, y) using
two control points. cxy1 and cxy2 are [cx, cy] arrays; x and y are separate
scalars. A sub-path must be active before calling.

Pair with:
  startNewSubPath -- must have an active sub-path before calling
  quadraticTo -- simpler single-control-point alternative
  lineTo -- for straight segments between curves

Anti-patterns:
  - Do NOT pass NaN or Inf values -- coordinates are NOT sanitized (unlike
    startNewSubPath/lineTo which use SANITIZED()). Corrupt values silently
    damage path geometry.
  - Note the mixed parameter convention: control points are [x,y] arrays,
    endpoint is two separate scalars. Inconsistent with quadraticTo (4 scalars)
    and addTriangle (3 arrays).

Source:
  ScriptingGraphics.cpp  PathObject::cubicTo()
    -> p.cubicTo(cxy1[0], cxy1[1], cxy2[0], cxy2[1], x, y)
    No SANITIZED() macro applied.
