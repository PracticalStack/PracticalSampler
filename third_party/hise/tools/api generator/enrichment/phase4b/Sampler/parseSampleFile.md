Sampler::parseSampleFile(var sampleFile) -> JSON

Thread safety: UNSAFE -- file I/O, allocates DynamicObject
Parses an audio file and returns its metadata as a JSON object compatible with
loadSampleMapFromJSON(). Accepts a ScriptFile object or an absolute path string.
Returns undefined if parsing fails.
Pair with:
  loadSampleMapFromJSON -- load the parsed metadata as a sample map
  getAudioWaveformContentAsBase64 -- higher-level converter for AudioWaveform presets
Source:
  ScriptingApi.cpp  Sampler::parseSampleFile()
    -> s->parseMetadata(f) -> converts ValueTree to DynamicObject
