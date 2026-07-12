MidiProcessor::getAttributeIndex(String parameterName) -> Integer

Thread safety: WARNING -- String parameter involves atomic ref-count operations.
Returns the integer index of the parameter with the given string identifier.
Inverse of getAttributeId(). Returns -1 if no parameter matches.
Pair with:
  getAttributeId -- inverse lookup (index to string)
  getAttribute/setAttribute -- use the returned index
Source:
  ScriptingApiObjects.cpp:4670  getAttributeIndex()
    -> mp->getParameterIndexForIdentifier(id)
