Modulator::getAttributeIndex(String parameterId) -> Integer

Thread safety: WARNING -- String parameter involves atomic ref-count operations.
Returns the parameter index for a given attribute name. Returns -1 if the name
is not found. Reverse lookup of getAttributeId().

Pair with:
  getAttributeId -- forward lookup (index to name)
  setAttribute -- use the returned index to set values

Source:
  ScriptingApiObjects.cpp  getAttributeIndex()
    -> mod->getParameterIndexForIdentifier(parameterId)
