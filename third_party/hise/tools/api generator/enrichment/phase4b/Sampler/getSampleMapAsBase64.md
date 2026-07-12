Sampler::getSampleMapAsBase64() -> String

Thread safety: UNSAFE -- compresses ValueTree to zstd, base64 encodes, allocates
Returns the current sample map as a zstd-compressed, base64-encoded string.
Can be stored in a user preset and restored with loadSampleMapFromBase64().
Pair with:
  loadSampleMapFromBase64 -- restores the encoded sample map
  saveCurrentSampleMap -- alternative: saves to XML file instead
Source:
  ScriptingApi.cpp  Sampler::getSampleMapAsBase64()
    -> ValueTree zstd compress -> base64 encode
