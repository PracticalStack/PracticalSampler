ComplexGroupManager::setFixedGroupEventLength(Number layerIndex, Number groupIndex, Number numSamplesToPlayBeforeFadeout) -> undefined

Thread safety: SAFE
Sets a fixed playback length in samples for voice starts matching the specified
layer/group. On the next noteOn, matching voices are automatically faded out after the
specified number of samples regardless of note-off messages. Modifies the per-voice
StartData stack.

Dispatch/mechanics:
  bumpGroupIndexFromZeroBased(groupIndex) -> applyEventDataInternal()
    -> stores StartData with Type::FadeOutOffset in activeDelayLayers stack
    -> consumed by getSpecialSoundStart() during voice startup

Pair with:
  delayGroupEvent -- delay voice start for same layer/group
  fadeInGroupEvent -- fade in voice start for same layer/group
  addGroupEventStartOffset -- offset sample start position

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::setFixedGroupEventLength()
    -> bumpGroupIndexFromZeroBased() -> applyEventDataInternal(FadeOutOffset)
