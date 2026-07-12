DisplayBuffer::getReadBuffer() -> Buffer

Thread safety: UNSAFE -- allocates a new VariantBuffer wrapper object on the heap.
Returns a Buffer wrapping channel 0 of the internal read buffer. The returned Buffer
shares memory with the ring buffer -- it is a direct pointer reference, not a copy.
Useful for quick size checks or read-only inspection.
Pair with:
  copyReadBuffer -- use instead when you need a safe independent copy to modify
  getResizedBuffer -- use instead when you need a different sample count
Anti-patterns:
  - Do NOT write to the returned buffer -- it is a direct memory reference to the
    ring buffer internals. Writing corrupts the shared data visible to other
    consumers. Use copyReadBuffer() for a modifiable copy.
Source:
  ScriptingApiObjects.cpp:1828  ScriptRingBuffer::getReadBuffer()
    -> getRingBuffer()->getReadBuffer() returns const AudioSampleBuffer&
    -> wraps channel 0 in new VariantBuffer (shared pointer, not copy)
