AudioSampleProcessor::getAttributeId(Number parameterIndex) -> String

Thread safety: WARNING -- String return value involves atomic ref-count operations.
Returns the name of the module parameter at the given index as a string. Use to discover
parameter names at runtime when dynamic constants are not known ahead of time.
Dispatch/mechanics:
  audioSampleProcessor->getIdentifierForParameterIndex(index).toString()
Pair with:
  getAttributeIndex -- reverse lookup (name to index)
  getNumAttributes -- total parameter count for iteration
Source:
  ScriptingApiObjects.cpp:4763+  getAttributeId() -> Processor::getIdentifierForParameterIndex()
