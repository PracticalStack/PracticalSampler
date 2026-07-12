Path::getPointOnPath(Number distanceFromStart) -> Array

Thread safety: UNSAFE -- allocates a JUCE Array for the return value
Returns [x, y] coordinates of a point at the given pixel distance along the
path from the start. If distance exceeds path length, returns the endpoint.
Returns [0, 0] for an empty path.

Pair with:
  getLength -- to determine valid distance range

Source:
  ScriptingGraphics.cpp  PathObject::getPointOnPath()
    -> p.getPointAlongPath(distanceFromStart) via PathFlatteningIterator
