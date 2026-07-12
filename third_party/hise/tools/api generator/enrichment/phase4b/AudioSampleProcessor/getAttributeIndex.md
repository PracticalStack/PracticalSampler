AudioSampleProcessor::getAttributeIndex(String parameterId) -> Integer

Thread safety: WARNING -- String parameter involves atomic ref-count operations.
Returns the parameter index for the given parameter name string. Reverse lookup of
getAttributeId(). Returns -1 if the handle is invalid.
Dispatch/mechanics:
  audioSampleProcessor->getParameterIndexForIdentifier(parameterId)
Pair with:
  getAttributeId -- forward lookup (index to name)
  getAttribute/setAttribute -- use the returned index to get/set values
Source:
  ScriptingApiObjects.cpp:4763+  getAttributeIndex() -> Processor::getParameterIndexForIdentifier()
