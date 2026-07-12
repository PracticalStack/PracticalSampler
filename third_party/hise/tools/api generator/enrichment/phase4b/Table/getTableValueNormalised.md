Table::getTableValueNormalised(Number normalisedInput) -> Double

Thread safety: WARNING -- reads from the internal 512-float lookup array (lock-free), but sends a display index notification as a side effect which involves async notification dispatch.
Returns the interpolated output value at the given normalized input position (0.0-1.0).
Uses linear interpolation between adjacent entries in the 512-element lookup array.
As a side effect, updates the ruler position and fires the display callback.

Dispatch/mechanics:
  dynamic_cast<SampleLookupTable*> -> getInterpolatedValue(input, sendNotificationAsync)
    -> input * 512 * coefficient -> linear interpolation between adjacent lookup entries
    -> sends DisplayIndex notification (fires displayCallback, updates ruler)

Pair with:
  setDisplayCallback -- fires as a side effect of this method
  getCurrentlyDisplayedIndex -- reads back the position last set by this method

Anti-patterns:
  - Do NOT pass raw MIDI velocity (0-127) -- any input above 1.0 returns the last table
    value, effectively flattening the response. Normalize first:
    table.getTableValueNormalised(Message.getVelocity() / 127.0)

Source:
  ScriptingApiObjects.cpp:2145  ScriptTableData::getTableValueNormalised()
    -> SampleLookupTable::getInterpolatedValue(normalisedInput, sendNotificationAsync)
  Tables.h:322  SampleLookupTable::getInterpolatedValue()
    -> index = sampleIndex * SAMPLE_LOOKUP_TABLE_SIZE * coefficient
    -> linear interpolation between data[index] and data[index+1]
