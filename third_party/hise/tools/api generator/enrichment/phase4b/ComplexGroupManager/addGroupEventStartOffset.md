ComplexGroupManager::addGroupEventStartOffset(Number layerIndex, Number groupIndex, Number offsetSamples) -> undefined

Thread safety: SAFE
Adds a sample start offset for voices matching the specified layer/group. On the next
noteOn, matching voices begin playback from the given sample position instead of the
sample's default start point. Modifies the per-voice StartData stack.

Dispatch/mechanics:
  bumpGroupIndexFromZeroBased(groupIndex) -> applyEventDataInternal()
    -> stores StartData with Type::StartOffsetIndex in activeDelayLayers stack
    -> consumed by getSpecialSoundStart() during voice startup

Pair with:
  delayGroupEvent -- delay voice start for same layer/group
  fadeInGroupEvent -- fade in voice start for same layer/group
  setFixedGroupEventLength -- set fixed playback length for same layer/group

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::addGroupEventStartOffset()
    -> bumpGroupIndexFromZeroBased() -> applyEventDataInternal(StartOffsetIndex)
