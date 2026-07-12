MidiProcessor::getAttribute(int parameterIndex) -> Double

Thread safety: SAFE
Returns the current value of the parameter at the given index. Use dynamic
constants (mp.Intensity) instead of raw index numbers. Returns 0.0 if invalid.
Pair with:
  setAttribute -- set the parameter value
  getAttributeId/getAttributeIndex -- convert between name and index
Source:
  ScriptingApiObjects.cpp:4643  getAttribute() -> mp->getAttribute(parameterIndex)
