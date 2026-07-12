MidiProcessor::getAttributeId(int parameterIndex) -> String

Thread safety: WARNING -- String return involves atomic ref-count operations.
Returns the string identifier of the parameter at the given index. Useful for
debugging or building dynamic parameter UIs that enumerate parameters by name.
Pair with:
  getAttributeIndex -- inverse lookup (string to index)
Source:
  ScriptingApiObjects.cpp:4663  getAttributeId()
    -> mp->getIdentifierForParameterIndex(index).toString()
