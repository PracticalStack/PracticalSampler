Rectangle::removeFromLeft(Double numToRemove) -> ScriptObject

Thread safety: SAFE
Removes a strip from the left of this rectangle, mutating it. Returns the removed
strip as a new Rectangle. The source's left edge moves rightward by numToRemove.

Pair with:
  removeFromRight/removeFromTop/removeFromBottom -- layout slicing pattern
  withTrimmedLeft -- non-mutating alternative (does not return the strip)

Anti-patterns:
  - Do NOT treat as non-mutating -- the source rectangle is permanently narrowed.
    The returned value is the removed strip, not the remaining area.

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(removeFromLeft, ...)
    -> JUCE Rectangle::removeFromLeft(numToRemove) mutates source, returns strip
