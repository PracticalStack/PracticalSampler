SliderPackData::getValue(int index) -> Double

Thread safety: SAFE -- acquires a lightweight read lock (atomic) for bounds check and sample read.
Returns the slider value at the given index. Out-of-range indices return the default
value (1.0) instead of throwing an error.
Anti-patterns:
  - Do NOT rely on out-of-range access returning a specific value for logic -- it
    silently returns 1.0 (the default), masking off-by-one bugs with a plausible float
Source:
  ScriptingApiObjects.cpp  ScriptSliderPackData::getValue()
    -> SliderPackData::getValue(index) -> isPositiveAndBelow check
    -> returns dataBuffer[index] or defaultValue (1.0f)
