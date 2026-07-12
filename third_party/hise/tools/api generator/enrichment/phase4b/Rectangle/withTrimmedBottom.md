Rectangle::withTrimmedBottom(Double amountToRemove) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the given amount removed from the bottom edge.
Non-mutating -- the original rectangle is unchanged.
Negative values extend the rectangle downward.

Pair with:
  removeFromBottom -- mutating version that also returns the removed strip
  withTrimmedTop/withTrimmedLeft/withTrimmedRight -- trim other edges

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(withTrimmedBottom, ...)
    -> JUCE Rectangle::withTrimmedBottom(amountToRemove)
