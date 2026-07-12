Buffer::trim(Integer trimFromStart, Integer trimFromEnd) -> Buffer

Thread safety: UNSAFE
Returns a new copied buffer with samples removed from start and end.
Source buffer remains unchanged.
Dispatch/mechanics: Computes clamped start/end bounds, allocates a new VariantBuffer, then copies the selected range into the destination buffer.
Pair with: getSlice (reference view alternative), resample (transform trimmed output)
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("trim", lambda)
