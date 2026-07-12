AudioFile::setRange(Integer min, Integer max) -> undefined

Thread safety: UNSAFE -- acquires write lock, copies buffer data for the sub-range.
Sets the active sample range within the loaded audio file. Range is clamped to
valid bounds (0 to total length). After calling, getContent and getNumSamples
reflect only the selected range. Zero-length range or empty buffer clears data.

Dispatch/mechanics:
  clamps min to >= 0, max to <= buffer->getBuffer().getNumSamples()
  zero size -> clear()
  otherwise -> buffer->setRange({min, max})
    -> extracts sub-range from originalBuffer into currentData

Pair with:
  getRange -- read back the current range
  getNumSamples -- returns sub-range size after setRange
  getTotalLengthInSamples -- returns original file length (unaffected)
  getContent -- returns data for the current range only

Source:
  ScriptingApiObjects.cpp:1643  ScriptAudioFile::setRange()
    -> jmax(0, min), jmin(totalSamples, max)
    -> buffer->setRange({min, max})
