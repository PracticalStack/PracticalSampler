ComplexGroupManager::setLayerProperty(Number layerIndex, String propertyId, var value) -> undefined

Thread safety: UNSAFE -- ValueTree modification with async listener notifications and type conversion
Sets a layer configuration property. Validates property ID against a fixed set; invalid
IDs produce a script error. Handles type conversion: "tokens" accepts a String array
(stored as comma-separated text), "matrixString" accepts an integer array (stored as
encoded string). Changes to structural properties like "tokens" trigger a full layer
and bitmask rebuild.

Valid property IDs:
  type, id, tokens, colour, folded, ignorable, cached, purgable,
  fader, slotIndex, sourceType, matrixString, isChromatic, matchGain,
  accuracy, fadeTime

Dispatch/mechanics:
  getManager()->getDataTree().getChild(layerIndex)
    -> Helpers::isValidId() validation
    -> Helpers::convertFromJS() for type conversion (String[] -> comma text, int[] -> encoded string)
    -> ValueTree property set with async listener notification
    -> structural changes trigger onRebuildPropertyChange() -> full layer rebuild

Pair with:
  getLayerProperty -- read layer configuration properties
  getLayerIndex -- resolve string layer ID to numeric index

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::setLayerProperty()
    -> getDataTree().getChild() -> Helpers::convertFromJS() -> ValueTree set
  ComplexGroupManager.cpp:1424  Helpers::isValidId()
