Buffer::getMagnitude(Integer startSample, Integer numSamples) -> Double

Thread safety: WARNING -- Performance-sensitive O(n) scan over the selected sample range.
Returns the absolute peak magnitude in the selected range.
Returns 0.0 for empty buffers.
Pair with: getRMSLevel (peak vs RMS comparison), getPeakRange (min/max bounds), indexOfPeak (peak location), detectPitch (gate pitch analysis)
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("getMagnitude", lambda)
