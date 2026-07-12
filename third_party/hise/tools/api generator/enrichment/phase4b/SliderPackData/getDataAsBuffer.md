SliderPackData::getDataAsBuffer() -> Buffer

Thread safety: SAFE -- returns a reference wrapper around the existing internal buffer without allocation.
Returns the internal float data as a Buffer object. The returned Buffer is a direct
reference -- not a copy. Writing to the Buffer modifies slider pack values directly.
Enables efficient DSP-style processing using Buffer operations.
Dispatch/mechanics:
  getSliderPackData()->getDataArray() returns var(dataBuffer.get())
  -> VariantBuffer returned BY REFERENCE, not copied
Anti-patterns:
  - Do NOT modify the Buffer and expect change notifications -- direct buffer writes
    bypass the listener system. Use setValue() or setAllValues() if listeners must
    be notified.
Source:
  ScriptingApiObjects.h:1344  ScriptSliderPackData::getDataAsBuffer()
    -> SliderPackData::getDataArray() -> var(dataBuffer.get())
