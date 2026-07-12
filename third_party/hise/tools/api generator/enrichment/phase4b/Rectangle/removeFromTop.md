Rectangle::removeFromTop(Double numToRemove) -> ScriptObject

Thread safety: SAFE
Removes a strip from the top of this rectangle, mutating it. Returns the removed
strip as a new Rectangle. The source's top edge moves downward by numToRemove.

Pair with:
  removeFromBottom/removeFromLeft/removeFromRight -- layout slicing pattern
  withTrimmedTop -- non-mutating alternative (does not return the strip)

Anti-patterns:
  - Do NOT treat as non-mutating -- the source rectangle is permanently shrunk.
    The returned value is the removed strip, not the remaining area.

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION1(removeFromTop, ...)
    -> JUCE Rectangle::removeFromTop(numToRemove) mutates source, returns strip
