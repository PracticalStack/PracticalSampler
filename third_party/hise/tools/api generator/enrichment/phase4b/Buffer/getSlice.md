Buffer::getSlice(Integer offsetInBuffer, Integer numSamples) -> Buffer

Thread safety: UNSAFE
Returns a new Buffer object that references a subrange of the source buffer.
The slice shares backing memory with the original.
Dispatch/mechanics: Creates a VariantBuffer reference view via referToOtherBuffer semantics, retaining source lifetime through referencedBuffer.
Pair with: trim (copy-based extraction), resample (create transformed copy from slice), Buffer.referTo (factory-level aliasing)
Anti-patterns:
  - Do NOT treat the result as an independent copy -- writes to the slice also modify the source region.
  - Do NOT pass negative offsets -- current implementation does not clamp and can create invalid pointer offsets.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("getSlice", lambda)
