Rectangle::removeFromBottom(Double numToRemove) -> ScriptObject

Thread safety: SAFE
Removes a strip from the bottom of this rectangle, mutating it. Returns the removed
strip as a new Rectangle. The source shrinks by numToRemove from the bottom.

Pair with:
  removeFromTop/removeFromLeft/removeFromRight -- layout slicing pattern
  withTrimmedBottom -- non-mutating alternative (does not return the strip)

Anti-patterns:
  - Do NOT treat as non-mutating -- the source rectangle is permanently shrunk.
    The returned value is the removed strip, not the remaining area.

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(removeFromBottom, ...)
    -> JUCE Rectangle::removeFromBottom(numToRemove) mutates source, returns strip
