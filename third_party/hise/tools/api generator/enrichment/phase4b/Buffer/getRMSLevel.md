Buffer::getRMSLevel(Integer startSample, Integer numSamples) -> Double

Thread safety: WARNING -- Performance-sensitive O(n) analysis over the selected range.
Returns RMS level for the selected sample region.
Use when average energy is more meaningful than single-sample peaks.
Pair with: getMagnitude (RMS vs peak), getPeakRange (signed extrema), applyMedianFilter (measure post-filter energy)
Anti-patterns:
  - Do NOT pass negative startSample values -- current implementation does not clamp and can read out of bounds.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("getRMSLevel", lambda)
