Buffer::getPeakRange(Integer startSample, Integer numSamples) -> Array

Thread safety: UNSAFE
Returns [minValue, maxValue] for the selected sample region.
Useful for bidirectional activity checks where negative-only content matters.
Pair with: getMagnitude (single absolute peak), getRMSLevel (energy estimate), toCharString (range-constrained compact encoding), toBase64 (serialize only active data)
Anti-patterns:
  - Do NOT pass negative startSample values -- current implementation does not clamp and can read out of bounds.
  - Do NOT check only range[1] for activity gating -- negative-only lanes can be non-empty.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("getPeakRange", lambda)
