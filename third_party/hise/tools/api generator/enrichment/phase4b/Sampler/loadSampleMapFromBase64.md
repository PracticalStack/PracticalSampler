Sampler::loadSampleMapFromBase64(String b64) -> undefined

Thread safety: UNSAFE -- killAllVoicesAndCall, decompression, allocation
Loads a sample map from a zstd-compressed, base64-encoded string (as produced
by getSampleMapAsBase64()). Kills all active voices before loading.
Pair with:
  getSampleMapAsBase64 -- produces the base64 string this method consumes
  loadSampleMap -- alternative: load by pool reference ID
  loadSampleMapFromJSON -- alternative: load from JSON array
Source:
  ScriptingApi.cpp  Sampler::loadSampleMapFromBase64()
    -> base64 decode -> zstd decompress -> ValueTree
    -> s->killAllVoicesAndCall(..., true)
