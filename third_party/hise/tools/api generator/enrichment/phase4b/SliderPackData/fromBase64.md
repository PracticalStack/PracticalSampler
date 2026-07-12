SliderPackData::fromBase64(String b64) -> undefined

Thread safety: UNSAFE -- allocates a new VariantBuffer and swaps the internal data buffer.
Restores slider pack data from a Base64-encoded string containing raw float data.
The slider count changes to match the decoded data size. Replaces the buffer entirely.
Required setup:
  const var spd = Engine.createAndRegisterSliderPackData(0);
Dispatch/mechanics:
  Decodes Base64 to MemoryBlock -> creates new VariantBuffer from float data
  -> swaps internal dataBuffer -> fires ContentRedirected event
Pair with:
  toBase64 -- encode/decode pair for serialization
  getNumSliders -- check slider count after restore (determined by decoded data)
Anti-patterns:
  - Do NOT pass an empty string expecting it to clear the data -- silently ignored,
    data remains unchanged with no error
Source:
  ScriptingApiObjects.cpp  ScriptSliderPackData::fromBase64()
    -> MemoryBlock::fromBase64Encoding(b64)
    -> new VariantBuffer from decoded float data
