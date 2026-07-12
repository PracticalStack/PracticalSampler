ComplexGroupManager::getLayerProperty(Number layerIndex, String propertyId) -> NotUndefined

Thread safety: UNSAFE -- ValueTree access with String property lookups and type conversion
Returns the value of a layer configuration property. Return type depends on the property:
most return String or Number, but "tokens" returns a String array and "matrixString"
returns an integer array. Invalid property IDs produce a script error.

Valid property IDs:
  type, id, tokens, colour, folded, ignorable, cached, purgable,
  fader, slotIndex, sourceType, matrixString, isChromatic, matchGain,
  accuracy, fadeTime

Dispatch/mechanics:
  getManager()->getDataTree().getChild(layerIndex)
    -> Helpers::isValidId() validation
    -> Helpers::convertToJS() for type conversion (matrixString -> int[], tokens -> String[])

Pair with:
  setLayerProperty -- set layer configuration properties
  getLayerIndex -- resolve string layer ID to numeric index

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::getLayerProperty()
    -> getDataTree().getChild() -> Helpers::convertToJS()
  ComplexGroupManager.cpp:1424  Helpers::isValidId()
