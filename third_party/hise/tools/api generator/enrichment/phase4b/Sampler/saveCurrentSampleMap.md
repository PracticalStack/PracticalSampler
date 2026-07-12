Sampler::saveCurrentSampleMap(String relativePathWithoutXml) -> Integer

Thread safety: UNSAFE -- file I/O operations
Saves the current sample map to an XML file in the SampleMaps directory. The
path is relative to SampleMaps; .xml extension is added automatically. Returns
true on success, false if the sampler has no sounds or the save fails. Overwrites
existing files.
Pair with:
  getSampleMapAsBase64 -- alternative: serialize to base64 string instead of file
  loadSampleMap -- load a saved map by reference
  getCurrentSampleMapId -- get the current map ID
Source:
  ScriptingApi.cpp  Sampler::saveCurrentSampleMap()
