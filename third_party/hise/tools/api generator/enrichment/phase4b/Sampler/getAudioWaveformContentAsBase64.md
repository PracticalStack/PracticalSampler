Sampler::getAudioWaveformContentAsBase64(JSON presetObj) -> String

Thread safety: UNSAFE -- parses audio file, allocates, compresses to base64
Converts an AudioWaveform user preset object into a base64-encoded sample map.
Reads `data` (file path), `rangeStart`, and `rangeEnd` properties from the
preset object.
Dispatch/mechanics:
  Reads preset object properties -> parseSampleFile internally
    -> compresses resulting sample map to base64 format
Pair with:
  parseSampleFile -- lower-level file metadata extraction
  loadSampleMapFromBase64 -- loads the base64 result back into a sampler
Source:
  ScriptingApi.cpp  Sampler::getAudioWaveformContentAsBase64()
    -> reads data/rangeStart/rangeEnd from preset object
    -> calls parseMetadata internally -> compresses to base64
