DisplayBufferSource::getDisplayBuffer(int index) -> ScriptObject

Thread safety: UNSAFE -- allocates a new ScriptRingBuffer (DisplayBuffer) object on each call.
Returns a DisplayBuffer reference for the display buffer at the given index on the source
processor. Each call creates a new wrapper object pointing to the same underlying ring buffer.

Required setup:
  const var dbs = Synth.getDisplayBufferSource("MyProcessor");

Dispatch/mechanics:
  objectExists() check via WeakReference -> getNumDataObjects(DisplayBuffer)
    -> validates index with isPositiveAndBelow
    -> new ScriptRingBuffer(scriptProcessor, index, source)

Pair with:
  Synth.getDisplayBufferSource -- factory method to obtain the source object
  DisplayBuffer.createPath -- standard visualization pipeline from the returned buffer
  DisplayBuffer.getReadBuffer -- raw sample access from the returned buffer

Anti-patterns:
  - Do NOT call with an out-of-range index -- throws a script error
    ("Can't find buffer at index N")
  - Do NOT rely on the returned object if the source processor has been deleted --
    weak reference expires silently, subsequent calls on the buffer will fail

Source:
  ScriptingApiObjects.cpp:7309  ScriptDisplayBufferSource::getDisplayBuffer()
    -> source->getNumDataObjects(ExternalData::DataType::DisplayBuffer)
    -> new ScriptRingBuffer(getScriptProcessor(), index, source)
