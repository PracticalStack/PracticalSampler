Buffer::applyMedianFilter(Integer windowSize) -> Buffer

Thread safety: UNSAFE
Applies a median filter and returns a new buffer with filtered samples.
The source buffer is not modified.
Dispatch/mechanics: Uses MedianFilter backend selected by USE_IPP_MEDIAN_FILTER and writes into a newly allocated VariantBuffer.
Pair with: decompose (additional offline spectral split), trim (post-filter length cleanup), getRMSLevel (measure result level)
Anti-patterns:
  - Do NOT omit windowSize -- current implementation returns undefined instead of reporting a parameter error.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("applyMedianFilter", lambda)
