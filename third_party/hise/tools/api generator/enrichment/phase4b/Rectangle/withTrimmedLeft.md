Rectangle::withTrimmedLeft(Double amountToRemove) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the given amount removed from the left edge.
Non-mutating -- the original rectangle is unchanged.
Left edge moves right, width decreases. Negative values extend leftward.

Pair with:
  removeFromLeft -- mutating version that also returns the removed strip
  withTrimmedRight/withTrimmedTop/withTrimmedBottom -- trim other edges

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withTrimmedLeft, ...)
    -> JUCE Rectangle::withTrimmedLeft(amountToRemove)
