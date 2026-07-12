DisplayBuffer::setRingBufferProperties(JSON propertyData) -> undefined

Thread safety: UNSAFE -- delegates to PropertyObject::setProperty() which may trigger buffer resizing or lock acquisition.
Configures the ring buffer by passing a JSON object of key-value pairs to the active
PropertyObject. Available properties depend on the buffer source type. Common
properties: BufferLength, NumChannels. FFT adds: WindowType, Overlap, DecibelRange,
UsePeakDecay, UseDecibelScale, YGamma, Decay, UseLogarithmicFreqAxis.
Required setup:
  const var src = Synth.getDisplayBufferSource("Analyser1");
  const var db = src.getDisplayBuffer(0);
Dispatch/mechanics:
  Iterates JSON key-value pairs
  Calls PropertyObject::setProperty(id, value) for each pair
  PropertyObject subclass determines which keys are valid
Anti-patterns:
  - Unknown property keys are silently ignored -- no error or warning. Typos in
    property names (e.g., "bufferLength" instead of "BufferLength") produce no
    feedback. Property keys are case-sensitive.
Source:
  ScriptingApiObjects.cpp:1828  ScriptRingBuffer::setRingBufferProperties()
    -> iterates JSON properties
    -> SimpleRingBuffer::setProperty(id, value)
    -> delegates to PropertyObject::setProperty()
