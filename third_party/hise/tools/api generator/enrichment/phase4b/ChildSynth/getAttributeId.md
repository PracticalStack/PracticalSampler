ChildSynth::getAttributeId(int parameterIndex) -> String

Thread safety: WARNING -- string involvement, atomic ref-count operations
Returns the string identifier of the attribute at the specified parameter index.
E.g., index 0 returns "Gain", index 1 returns "Balance".
Pair with:
  getAttributeIndex -- reverse lookup (string to index)
  getAttribute / setAttribute -- use the index for value access
Source:
  ScriptingApiObjects.cpp  getAttributeId()
    -> synth->getIdentifierForParameterIndex(parameterIndex).toString()
