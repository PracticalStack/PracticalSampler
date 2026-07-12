SliderPackData::setValue(int sliderIndex, Double value) -> undefined

Thread safety: WARNING -- acquires a read lock (atomic compare-exchange) and dispatches a synchronous content change notification.
Sets the value of a single slider at the given index. Out-of-range indices are silently
ignored. The value is sanitized for non-finite numbers (NaN, infinity) before storage.
Dispatch/mechanics:
  isPositiveAndBelow bounds check -> sanitizeFloatNumber(value)
  -> writes to dataBuffer[index] under read lock
  -> fires ContentChange event with the slider index
Pair with:
  getValue -- read back the value this method sets
  setValueWithUndo -- undoable variant for user-initiated edits
Anti-patterns:
  - Do NOT use for user-initiated edits (click, record, randomize) -- use
    setValueWithUndo() so the user can undo
  - Do NOT rely on out-of-range indices throwing -- they are silently ignored,
    masking off-by-one bugs
Source:
  SliderPack.cpp  SliderPackData::setValue()
    -> isPositiveAndBelow(index, getNumSliders())
    -> dataBuffer[index] = sanitizeFloatNumber(value)
    -> getUpdater().sendContentChangeMessage(sendNotificationAsync, index)
