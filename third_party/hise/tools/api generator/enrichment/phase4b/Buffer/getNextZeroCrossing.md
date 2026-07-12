Buffer::getNextZeroCrossing(Integer index) -> Integer

Thread safety: WARNING -- Performance-sensitive linear scan from index to the end of the buffer.
Searches forward for the next negative-to-positive sign transition.
Returns -1 when no crossing exists.
Pair with: indexOfPeak (find dominant region, then search local crossings)
Anti-patterns:
  - Do NOT pass negative indices -- current implementation does not clamp and can read out of bounds.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("getNextZeroCrossing", lambda)
