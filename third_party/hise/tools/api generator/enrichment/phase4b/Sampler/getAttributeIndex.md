Sampler::getAttributeIndex(String parameterId) -> Integer

Thread safety: WARNING -- string involvement, atomic ref-count operations
Returns the parameter index for a given parameter identifier string. Returns -1
if not found.
Pair with:
  getAttributeId -- reverse lookup (index to name)
  getAttribute -- get the parameter value by index
Source:
  ScriptingApi.cpp  Sampler::getAttributeIndex()
