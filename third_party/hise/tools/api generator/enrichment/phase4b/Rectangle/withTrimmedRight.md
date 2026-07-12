Rectangle::withTrimmedRight(Double amountToRemove) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the given amount removed from the right edge.
Non-mutating -- the original rectangle is unchanged.
Width decreases by amountToRemove. Negative values extend rightward.

Pair with:
  removeFromRight -- mutating version that also returns the removed strip
  withTrimmedLeft/withTrimmedTop/withTrimmedBottom -- trim other edges

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withTrimmedRight, ...)
    -> JUCE Rectangle::withTrimmedRight(amountToRemove)
