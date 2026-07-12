Sample::getRange(int propertyIndex) -> Array

Thread safety: UNSAFE -- allocates a two-element Array on each call; delegates to
ModulatorSamplerSound::getPropertyRange() which accesses ValueTree properties.
Returns [start, end] array with the valid value range for the specified property.
Ranges are dynamic -- they depend on current values of related properties (e.g.,
HiKey range starts at current LoKey, SampleStart range ends at SampleEnd or LoopStart).
Pair with:
  set -- validate or clamp values before writing (set auto-clips, but getRange
    lets you constrain UI controls)
  get -- read current value to interpret within the returned range
Anti-patterns:
  - Do NOT cache range results across property changes -- ranges are interdependent
    and stale after any set() call on related properties
Source:
  ScriptingApiObjects.cpp  getRange()
    -> sound->getPropertyRange(sampleIds[propertyIndex])
    -> returns Range<int> mapped to [start, end] Array
