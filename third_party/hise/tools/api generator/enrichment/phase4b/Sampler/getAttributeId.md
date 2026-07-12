Sampler::getAttributeId(Number parameterIndex) -> String

Thread safety: WARNING -- string involvement, atomic ref-count operations
Returns the string identifier of a sampler parameter by its index.
Pair with:
  getAttributeIndex -- reverse lookup (name to index)
  getAttribute -- get the parameter value by index
Source:
  ScriptingApi.cpp  Sampler::getAttributeId()
    -> Processor::getIdentifierForParameterIndex(parameterIndex)
