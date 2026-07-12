ComplexGroupManager::fadeOutGroupEvent(Number layerIndex, Number groupIndex, Number fadeOutTimeMs) -> undefined

Thread safety: SAFE
Fades out all currently playing voices matching the specified layer/group. Unlike
delayGroupEvent/fadeInGroupEvent/addGroupEventStartOffset/setFixedGroupEventLength
which set up start data for future voices, this operates on already-playing voices.

Dispatch/mechanics:
  bumpGroupIndexFromZeroBased(groupIndex)
    -> iterates sampler->activeVoices
    -> matches voices by bitmask layer value
    -> av->setVolumeFade(fadeOutTimeMs * 0.001, 0.0) on matching voices

Anti-patterns:
  - Do NOT call before any matching voices are playing -- has no effect on future voices.
    Use fadeInGroupEvent for controlling voice start behavior.

Pair with:
  fadeInGroupEvent -- set fade-in for future voice starts
  setGroupVolume -- set static gain for a layer/group

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::fadeOutGroupEvent()
    -> iterates sampler->activeVoices -> matches bitmask -> setVolumeFade(seconds, 0.0)
