Path::fromString(String stringPath) -> undefined

Thread safety: UNSAFE -- JUCE's Path::restoreFromString performs string parsing with heap allocations
Restores path geometry from a human-readable string previously created by
toString(). Replaces the current path contents entirely. The format uses
single-character commands: m (move), l (line), c (cubic), q (quadratic),
z (close).

Pair with:
  toString -- produces the string format that fromString consumes
  loadFromData -- alternative for base64 or binary format

Source:
  ScriptingGraphics.cpp  PathObject::fromString()
    -> juce::Path::restoreFromString(stringPath)
