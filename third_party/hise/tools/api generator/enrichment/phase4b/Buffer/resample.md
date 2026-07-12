Buffer::resample(Double ratio, String interpolationType, Integer wrapAround) -> Buffer

Thread safety: UNSAFE
Returns a new buffer resampled by ratio with the selected interpolation mode.
Output length is round(inputLength / ratio).
Dispatch/mechanics: Clamps ratio to 0.01..1000.0, resolves interpolationType string to one of five JUCE interpolator classes, then processes input into a newly allocated VariantBuffer.
Pair with: getSlice (select region before resampling), trim (post-resample cleanup)
Anti-patterns:
  - Do NOT pass unsupported interpolation names -- runtime throws and lists the accepted modes.
Source:
  VariantBuffer.cpp:172  VariantBuffer::addMethods() -> setMethod("resample", lambda)
