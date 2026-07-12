Sample::set(int propertyIndex, var newValue) -> undefined

Thread safety: UNSAFE -- delegates to setSampleProperty which operates on ValueTree data with notifications, string lookups, and potential cascading property adjustments.
Sets the specified sample property. Value is automatically clipped to the valid
range. Setting certain properties triggers cascading adjustments to dependent
properties (e.g., SampleStart adjusts LoopXFade, LoopStart, SampleStartMod).
Pair with:
  get -- read back the property value
  getRange -- query valid bounds before setting loop properties
Anti-patterns:
  - Do NOT assume set order is irrelevant for LoVel/HiVel -- auto-clipping
    clamps against the current opposite bound. Set HiVel first when widening,
    LoVel first when narrowing.
Source:
  ScriptingApiObjects.cpp  set()
    -> sound->setSampleProperty(sampleIds[propertyIndex], newValue)
    -> clips to getPropertyRange()
    -> clipRangeProperties() may adjust dependent properties
