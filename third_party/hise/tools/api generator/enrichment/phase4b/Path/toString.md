Path::toString() -> String

Thread safety: WARNING -- string involvement, atomic ref-count operations
Converts the path to a human-readable string using JUCE's internal format.
Commands: m (move), l (line), q (quadratic), c (cubic), z (close). Longer
but inspectable compared to toBase64(). Restore via fromString(). Returns
empty string for an empty path.

Pair with:
  fromString -- restores a path from the string format
  toBase64 -- alternative compact binary format

Source:
  ScriptingGraphics.cpp  PathObject::toString()
    -> juce::Path::toString()
