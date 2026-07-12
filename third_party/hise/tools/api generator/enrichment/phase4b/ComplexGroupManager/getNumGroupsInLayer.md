ComplexGroupManager::getNumGroupsInLayer(Number layerIndex) -> Integer

Thread safety: SAFE
Returns the number of groups (tokens) defined in the specified layer. Corresponds to
the number of entries in the layer's "tokens" property.

Pair with:
  setActiveGroup -- use returned count to validate group index bounds
  getLayerProperty -- retrieve layer properties including tokens list

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::getNumGroupsInLayer()
