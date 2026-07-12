ComplexGroupManager::delayGroupEvent(Number layerIndex, Number groupIndex, Number delayInSamples) -> undefined

Thread safety: SAFE
Sets a delay in samples for voice starts matching the specified layer/group. On the next
noteOn, matching voices are delayed by the specified number of samples. Modifies the
per-voice StartData stack.

Dispatch/mechanics:
  bumpGroupIndexFromZeroBased(groupIndex) -> applyEventDataInternal()
    -> stores StartData with Type::DelayTimeIndex in activeDelayLayers stack
    -> consumed by getSpecialSoundStart() during voice startup

Pair with:
  addGroupEventStartOffset -- offset sample start position for same layer/group
  fadeInGroupEvent -- fade in voice start for same layer/group
  setFixedGroupEventLength -- set fixed playback length for same layer/group

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::delayGroupEvent()
    -> bumpGroupIndexFromZeroBased() -> applyEventDataInternal(DelayTimeIndex)
