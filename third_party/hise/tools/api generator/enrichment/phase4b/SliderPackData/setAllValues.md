SliderPackData::setAllValues(var value) -> undefined

Thread safety: UNSAFE -- allocates an Array<float> and copies data into the internal buffer.
Sets slider values from a single number, an Array, or a Buffer. A single number sets
all sliders to that value. An Array or Buffer sets sliders from elements -- only the
first N sliders are updated where N is the source length.
Dispatch/mechanics:
  Detects scalar vs multi-value (isBuffer()/isArray())
  -> builds float array -> SliderPackData::setFromFloatArray(arr, sendNotificationAsync)
  -> fires ContentChange event with index -1 (bulk)
Pair with:
  setAllValuesWithUndo -- same behavior but registers with undo manager
Anti-patterns:
  - Do NOT assume passing a short Array fills remaining sliders with 0 -- only the
    first N sliders are updated, remaining keep their previous values
Source:
  ScriptingApiObjects.cpp:2336  ScriptSliderPackData::setAllValues()
    -> iterates to maxIndex, reads value[i] or (float)value
    -> SliderPackData::setFromFloatArray()
