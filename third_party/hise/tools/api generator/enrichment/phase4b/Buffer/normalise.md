Buffer::normalise(Double gainInDecibels) -> undefined

Thread safety: WARNING -- In-place O(n) normalization over the full buffer.
Scales the buffer in place so peak magnitude reaches the normalization target.
Operation mutates the existing buffer data.
Pair with: getMagnitude (inspect pre/post peak), getRMSLevel (inspect perceived energy)
Anti-patterns:
  - Do NOT assume gainInDecibels is currently honored -- implementation bug normalizes to 0 dB peak regardless of argument.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("normalise", lambda)
