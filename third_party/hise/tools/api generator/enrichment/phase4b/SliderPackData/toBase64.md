SliderPackData::toBase64() -> String

Thread safety: UNSAFE -- allocates a MemoryBlock and constructs a String from the encoded data.
Exports all slider values as a Base64-encoded string containing the raw float array.
Returns an empty string if the underlying data is invalid.
Pair with:
  fromBase64 -- encode/decode pair for serialization
Source:
  ScriptingApiObjects.cpp  ScriptSliderPackData::toBase64()
    -> MemoryBlock from float data -> toBase64Encoding()
