ComplexGroupManager::getLayerIndex(var layerIdOrIndex) -> Integer

Thread safety: WARNING -- atomic ref-count operations during string comparison when a string layer ID is passed
Resolves a layer identifier to its zero-based numeric index. If a String is passed,
matches against layer IDs. If a Number is passed, returns it as-is (pass-through).

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::getLayerIndex()
    -> getLayerIndexInternal() -> string match against layer IDs or int pass-through
