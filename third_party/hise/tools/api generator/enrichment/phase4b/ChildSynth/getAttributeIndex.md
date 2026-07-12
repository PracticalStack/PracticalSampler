ChildSynth::getAttributeIndex(String parameterId) -> int

Thread safety: WARNING -- string involvement, atomic ref-count operations on parameter lookup
Returns the integer parameter index for the given string identifier. Reverse of
getAttributeId(). Returns -1 if the parameter identifier is not found.
Pair with:
  getAttributeId -- forward lookup (index to string)
  getAttribute / setAttribute -- use the returned index for value access
Source:
  ScriptingApiObjects.cpp  getAttributeIndex()
    -> synth->getParameterIndexForIdentifier(parameterId)
