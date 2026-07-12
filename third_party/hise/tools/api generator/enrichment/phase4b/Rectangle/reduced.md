Rectangle::reduced(Double x, Double optionalY) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle shrunk by the given amount on all sides.
With one argument, reduces uniformly (width/height decrease by 2x the amount).
With two arguments, first controls horizontal, second controls vertical.

Pair with:
  expanded -- inverse operation (grow instead of shrink)

Anti-patterns:
  - Negative values expand the rectangle (same effect as expanded). This is valid
    but potentially confusing.

Source:
  RectangleDynamicObject.cpp  FunctionMap constructor, ADD_FUNCTION(reduced, ...)
    -> checks numArguments for 1 vs 2 args
    -> JUCE Rectangle::reduced(x) or Rectangle::reduced(x, y)
