Path::getYAt(Number xPos) -> Double

Thread safety: SAFE
Returns the Y coordinate of the first intersection between the path and a
vertical line at the given X position. Returns undefined if X is outside the
path's horizontal extent.

Dispatch/mechanics:
  Uses flex_ahdsr_base::Helpers::getYAt() with PathFlatteningIterator.
  Tolerance adapts to path size: min(size/100, JUCE default).
  Skips closing sub-path segments.
  Returns the first matching segment's interpolated Y.

Anti-patterns:
  - Only returns the Y value of the first matching segment. For self-intersecting
    paths with multiple Y values at the same X, only the first is returned.
  - Returns undefined (not a number) when no match -- check with isDefined()
    before using in arithmetic.

Source:
  ScriptingGraphics.cpp  PathObject::getYAt()
    -> flex_ahdsr_base::Helpers::getYAt(p, xPos)
    -> PathFlatteningIterator walk, interpolate Y from matching segment
    -> returns var() for no match, var(y) for match
