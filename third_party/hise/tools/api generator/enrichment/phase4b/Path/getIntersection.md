Path::getIntersection(Array start, Array end, Integer keepSectionOutsidePath) -> Array

Thread safety: UNSAFE -- allocates a JUCE Array for the return value
Tests whether a line segment intersects the path. Returns [x, y] of the
intersection point, or false if no intersection. The keepSectionOutsidePath
flag controls which clipped endpoint is returned: true = outer, false = inner.

Dispatch/mechanics:
  ApiHelpers::getPointFromVar(start/end)
  Start Y offset by -0.001 internally (edge case workaround)
  -> Path::intersectsLine(line) to check intersection
  -> Path::getClippedLine(line, keepSectionOutsidePath) to get the point

Anti-patterns:
  - Returns false (not an array) when no intersection -- callers must check
    the return type before indexing the result.
  - Start Y is silently offset by -0.001px internally. Intersection results may
    be very slightly imprecise for lines starting at exact path coordinates.

Source:
  ScriptingGraphics.cpp  PathObject::getIntersection()
    -> Line<float> l(p1.x, p1.y - 0.001f, p2.x, p2.y)
    -> p.intersectsLine(l) ? getClippedLine() : return false
