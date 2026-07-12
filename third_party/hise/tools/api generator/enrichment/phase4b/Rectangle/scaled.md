Rectangle::scaled(Double factorX, Double optionalFactorY) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with both position and size scaled by the given factor(s).
With one argument, both axes scale uniformly. With two, first is horizontal, second vertical.

Dispatch/mechanics:
  Uses AffineTransform::scale(factorX, factorY) to transform all coordinates.
  Position is also scaled -- a rect at (100,50) scaled by 0.5 moves to (50,25).

Pair with:
  withSizeKeepingCentre -- to scale size only while keeping the centre fixed

Anti-patterns:
  - Scaling transforms both position and size. A rectangle not at the origin will
    move toward/away from (0,0). Use withSizeKeepingCentre if you want to resize
    around the centre.

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION(scaled, ...)
    -> checks numArguments: 1 arg uses same factor for both axes
    -> JUCE Rectangle::transformedBy(AffineTransform::scale(fx, fy))
