Buffer::detectPitch(Double sampleRate, Integer startSample, Integer numSamples) -> Double

Thread safety: UNSAFE
Estimates fundamental frequency for the selected region and returns Hz.
If numSamples is omitted, analysis runs to the end of the buffer.
Dispatch/mechanics: Delegates to PitchDetection::detectPitch on the selected sample range; method registration exists only when HISE_INCLUDE_PITCH_DETECTION is enabled.
Pair with: getMagnitude (level gate before pitch detect), getRMSLevel (energy gate), decompose (pre-separate tonal/noise content)
Anti-patterns:
  - Do NOT rely on this method in builds without HISE_INCLUDE_PITCH_DETECTION -- the API method is not registered.
  - Do NOT pass negative startSample values -- current implementation does not clamp and can read out of bounds.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("detectPitch", lambda)
