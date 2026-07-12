DisplayBuffer::getResizedBuffer(Integer numDestSamples, Integer resampleMode) -> Buffer

Thread safety: UNSAFE -- allocates a new VariantBuffer on the heap. When source and destination sizes match, delegates to getReadBuffer() which also allocates.
Creates a new Buffer resampled from the read buffer to the specified number of samples.
Uses point sampling when stride < 2.0, min/max midpoint when stride >= 2.0. Returns
an empty zero-length buffer if numDestSamples is 0 or negative.
Dispatch/mechanics:
  stride = sourceSize / destSize
  stride < 2.0: point sampling from source
  stride >= 2.0: finds min/max in each stride window, takes midpoint
  numDestSamples == sourceSize: delegates to getReadBuffer() (shared reference)
Pair with:
  copyReadBuffer -- use instead when you want an exact-size thread-safe copy
  getReadBuffer -- returned directly when sizes match (see anti-patterns)
Anti-patterns:
  - The resampleMode parameter is accepted but completely ignored. The resampling
    algorithm is determined solely by the stride ratio, not by this parameter.
  - When numDestSamples matches the read buffer size exactly, delegates to
    getReadBuffer(), returning a shared memory reference instead of a copy. For all
    other sizes, an independent buffer is returned. This inconsistency means the
    result may or may not be safe to modify depending on the size match.
Source:
  ScriptingApiObjects.cpp:1828  ScriptRingBuffer::getResizedBuffer()
    -> getRingBuffer()->getReadBuffer() for source data
    -> allocates new VariantBuffer(numDestSamples)
    -> point sample or min/max midpoint based on stride ratio
