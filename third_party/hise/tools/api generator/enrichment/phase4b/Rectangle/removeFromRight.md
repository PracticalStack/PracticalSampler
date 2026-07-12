Rectangle::removeFromRight(Double numToRemove) -> ScriptObject

Thread safety: SAFE
Removes a strip from the right of this rectangle, mutating it. Returns the removed
strip as a new Rectangle. The source's right edge moves leftward by numToRemove.

Pair with:
  removeFromLeft/removeFromTop/removeFromBottom -- layout slicing pattern
  withTrimmedRight -- non-mutating alternative (does not return the strip)

Anti-patterns:
  - Do NOT treat as non-mutating -- the source rectangle is permanently narrowed.
    The returned value is the removed strip, not the remaining area.

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(removeFromRight, ...)
    -> JUCE Rectangle::removeFromRight(numToRemove) mutates source, returns strip
