DisplayBuffer::toBase64() -> String

Thread safety: UNSAFE -- string construction from PropertyObject::exportAsBase64() involves heap allocation.
Exports the display buffer state as a base64-encoded string. Only buffer types that
implement state serialization produce non-empty output (flex AHDSR envelopes). Most
buffer types return an empty string.
Pair with:
  fromBase64 -- restore the exported state
Anti-patterns:
  - Do NOT assume a non-empty return -- most buffer types (FFT, oscilloscope,
    goniometer, generic) return an empty string without error. There is no way to
    distinguish "empty state" from "unsupported operation".
Source:
  ScriptingApiObjects.cpp:1828  ScriptRingBuffer::toBase64()
    -> PropertyObject::exportAsBase64()
    -> returns String (empty for most PropertyObject types)
