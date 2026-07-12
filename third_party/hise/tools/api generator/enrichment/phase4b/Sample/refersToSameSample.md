Sample::refersToSameSample(var otherSample) -> bool

Thread safety: SAFE
Returns true if both Sample objects refer to the same underlying
ModulatorSamplerSound instance. Pointer identity check, not property comparison.
Useful for detecting whether two selections contain overlapping samples.
Anti-patterns:
  - [BUG] Error message for non-Sample argument contains typo:
    "refersToSampleSample" instead of "refersToSameSample"
Source:
  ScriptingApiObjects.cpp  refersToSameSample()
    -> compares ModulatorSamplerSound::Ptr identity
