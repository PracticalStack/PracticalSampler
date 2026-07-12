Buffer::toCharString(Integer numChars, Array range) -> String

Thread safety: UNSAFE
Encodes buffer magnitudes into a compact two-character-per-bin string.
Each output pair represents one analysis segment.
Pair with: getPeakRange (range planning before encoding), getMagnitude (coarse level checks)
Anti-patterns:
  - Do NOT set numChars larger than buffer length -- current implementation can compute samplesPerChar = 0 and stall loop progress.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("toCharString", lambda)
