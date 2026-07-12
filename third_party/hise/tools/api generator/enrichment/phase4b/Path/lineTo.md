Path::lineTo(Number x, Number y) -> undefined

Thread safety: SAFE
Adds a straight line from the current position to (x, y). If no sub-path
has been started, JUCE implicitly adds startNewSubPath(0, 0) first.
Coordinates are sanitized against NaN/Inf via SANITIZED().

Pair with:
  startNewSubPath -- set the starting position before lineTo
  closeSubPath -- close the shape after drawing line segments

Source:
  ScriptingGraphics.cpp  PathObject::lineTo()
    -> p.lineTo(SANITIZED(x), SANITIZED(y))
