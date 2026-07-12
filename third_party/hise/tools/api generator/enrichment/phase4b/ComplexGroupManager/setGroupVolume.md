ComplexGroupManager::setGroupVolume(Number layerIndex, Number groupIndex, Number gainFactor) -> undefined

Thread safety: SAFE
Sets the per-group linear gain multiplier for a specific layer/group. Only works with
Custom LogicType layers. On non-Custom layers, the internal cast to CustomLayer fails
silently with no error message.

Dispatch/mechanics:
  bumpGroupIndexFromZeroBased(groupIndex) -> gm->setGroupVolume()
    -> casts layer to CustomLayer* -> sets layerGains[value-1] = gainFactor

Anti-patterns:
  - Do NOT call on a non-Custom LogicType layer -- silently does nothing (internal
    CustomLayer cast returns null, no error produced). Check the layer type with
    getLayerProperty(layerIndex, "type") first if uncertain.

Pair with:
  fadeOutGroupEvent -- fade out already-playing voices
  getLayerProperty -- verify layer is Custom type before calling

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::setGroupVolume()
    -> bumpGroupIndexFromZeroBased() -> gm->setGroupVolume() -> CustomLayer cast -> layerGains[value-1]
