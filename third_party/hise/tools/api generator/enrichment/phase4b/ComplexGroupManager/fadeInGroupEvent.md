ComplexGroupManager::fadeInGroupEvent(Number layerIndex, Number groupIndex, Number fadeInTimeMs, Number targetGainDb) -> undefined

Thread safety: SAFE
Sets a fade-in time and target gain for voice starts matching the specified layer/group.
On the next noteOn, matching voices fade in over the specified duration to the target
gain level. Converts ms to seconds and dB to linear gain before storing in StartData.

Dispatch/mechanics:
  bumpGroupIndexFromZeroBased(groupIndex)
    -> fadeInTimeMs * 0.001 -> Decibels::decibelsToGain(targetGainDb)
    -> applyEventDataInternal() with FadeTimeIndex + TargetVolume
    -> consumed by getSpecialSoundStart() during voice startup

Pair with:
  fadeOutGroupEvent -- fade out already-playing voices
  delayGroupEvent -- delay voice start for same layer/group
  addGroupEventStartOffset -- offset sample start position
  setFixedGroupEventLength -- set fixed playback length

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::fadeInGroupEvent()
    -> bumpGroupIndexFromZeroBased() -> converts ms/dB -> applyEventDataInternal(FadeTimeIndex, TargetVolume)
