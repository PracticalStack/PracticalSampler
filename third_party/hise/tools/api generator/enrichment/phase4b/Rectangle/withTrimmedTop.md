Rectangle::withTrimmedTop(Double amountToRemove) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the given amount removed from the top edge.
Non-mutating -- the original rectangle is unchanged.
Top edge moves down, height decreases. Negative values extend upward.

Pair with:
  removeFromTop -- mutating version that also returns the removed strip
  withTrimmedBottom/withTrimmedLeft/withTrimmedRight -- trim other edges

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withTrimmedTop, ...)
    -> JUCE Rectangle::withTrimmedTop(amountToRemove)
