DisplayBuffer::createPath(Array dstArea, Array sourceRange, Number normalisedStartValue) -> ScriptObject

Thread safety: UNSAFE -- allocates a PathObject and acquires SimpleReadWriteLock read lock on the buffer data lock.
Creates a Path object from the ring buffer data, scaled to the given destination
rectangle. Path generation is delegated to the active PropertyObject, so the visual
result depends on the buffer source type (oscilloscope, FFT, envelope, etc.).
Required setup:
  const var src = Synth.getDisplayBufferSource("Analyser1");
  const var db = src.getDisplayBuffer(0);
Dispatch/mechanics:
  Parses dstArea and sourceRange as rectangles via ApiHelpers::getRectangleFromVar()
  sourceRange encodes [minValue, maxValue, startSample, endSample]
  Acquires SimpleReadWriteLock::ScopedReadLock on buffer data lock
  Delegates to PropertyObject::createPath() for type-specific path generation
Pair with:
  setRingBufferProperties -- configure buffer type properties before creating paths
  getReadBuffer -- alternative for direct data access instead of path generation
Anti-patterns:
  - Do NOT call createPath() in the paint routine -- it acquires a lock on the ring
    buffer. Call it in the timer callback, store the path, then draw in paint.
  - Do NOT rely on sourceRange endSample (4th element) -- it is ignored due to a
    variable overwrite in the C++ implementation. The path always renders the full
    buffer length. Only startSample (3rd element) is respected.
  - Do NOT treat sourceRange as a spatial rectangle -- it uses non-standard packing:
    [minValue, maxValue, startSample, endSample].
Source:
  ScriptingApiObjects.cpp:1828  ScriptRingBuffer::createPath()
    -> ApiHelpers::getRectangleFromVar() for dstArea and sourceRange
    -> SimpleReadWriteLock::ScopedReadLock on data lock
    -> PropertyObject::createPath(sampleRange, valueRange, targetBounds, startValue)
    -> returns PathObject wrapping JUCE Path
