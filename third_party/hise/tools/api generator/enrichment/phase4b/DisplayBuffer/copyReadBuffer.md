DisplayBuffer::copyReadBuffer(AudioData targetBuffer) -> undefined

Thread safety: UNSAFE -- acquires SimpleReadWriteLock read lock on data lock, then CriticalSection on read buffer lock for the copy operation.
Copies the ring buffer's read data into a preallocated target buffer. Accepts a single
Buffer (copies channel 0) or an Array of Buffers (copies per-channel). Target buffer
size must match the ring buffer's sample count exactly.
Dispatch/mechanics:
  Single Buffer -> copies channel 0 of read buffer (size-checked)
  Array of Buffers -> iterates channels, copies each (channel count + size checked)
  Acquires getReadBufferLock() (CriticalSection) for thread-safe copy
Pair with:
  getReadBuffer -- use copyReadBuffer instead when you need to modify the data
  getResizedBuffer -- alternative when you need a different sample count
Anti-patterns:
  - Do NOT pass a buffer whose size does not match the ring buffer sample count --
    the copy will fail silently. Check the read buffer size first.
  - Do NOT pass an array with a different channel count than the ring buffer --
    channel count mismatch is checked and rejected.
Source:
  ScriptingApiObjects.cpp:1828  ScriptRingBuffer::copyReadBuffer()
    -> acquires getReadBufferLock() (CriticalSection)
    -> FloatVectorOperations::copy() per channel
