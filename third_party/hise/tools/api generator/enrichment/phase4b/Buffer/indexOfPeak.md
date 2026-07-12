Buffer::indexOfPeak(Integer startSample, Integer numSamples) -> Integer

Thread safety: WARNING -- Linear scan over the selected range.
Finds the index of the highest absolute-value sample in the selected window.
Useful for transient anchoring before local processing.
Pair with: getMagnitude (peak value), getNextZeroCrossing (phase-aligned boundaries)
Anti-patterns:
  - Do NOT pass negative startSample values -- current implementation does not clamp and can read out of bounds.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("indexOfPeak", lambda)
