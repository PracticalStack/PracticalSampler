Modulator::getAttributeId(Number parameterIndex) -> String

Thread safety: WARNING -- String return involves atomic ref-count operations.
Returns the name of the attribute at the given parameter index. Useful for
iterating all parameters by index to build dynamic UIs or preset systems.

Pair with:
  getAttributeIndex -- reverse lookup (name to index)
  getNumAttributes -- total parameter count for iteration

Source:
  ScriptingApiObjects.cpp  getAttributeId()
    -> mod->getIdentifierForParameterIndex(parameterIndex).toString()
