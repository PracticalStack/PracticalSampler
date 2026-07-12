Rectangle::expanded(Double x, Double optionalY) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle enlarged by the given amount on all sides.
With one argument, expands uniformly (width/height increase by 2x the amount).
With two arguments, first controls horizontal, second controls vertical.

Pair with:
  reduced -- inverse operation (shrink instead of grow)

Anti-patterns:
  - Negative values shrink the rectangle (same effect as reduced). This is valid
    but potentially confusing.

Source:
  RectangleDynamicObject.cpp  FunctionMap constructor, ADD_FUNCTION(expanded, ...)
    -> checks numArguments for 1 vs 2 args
    -> JUCE Rectangle::expanded(x) or Rectangle::expanded(x, y)
