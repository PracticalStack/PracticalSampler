Sampler::loadSampleMapFromJSON(Array jsonSampleList) -> undefined

Thread safety: UNSAFE -- killAllVoicesAndCall, ValueTree construction, allocation
Loads a sample map from a JSON array of sample descriptor objects. Each object
should have at least FileName. Missing properties get defaults: LoVel=0,
HiVel=127, LoKey=0, HiKey=127, Root=64, RRGroup=1.
Dispatch/mechanics:
  convertJSONListToValueTree(jsonSampleList) -> ValueTree with defaults applied
    -> s->killAllVoicesAndCall(..., true) -> loads the constructed ValueTree
Pair with:
  parseSampleFile -- parse audio file metadata into JSON compatible with this method
  getSampleMapAsBase64 -- serialize the loaded map for preset storage
  loadSampleMap -- alternative: load by pool reference ID
Source:
  ScriptingApi.cpp  Sampler::loadSampleMapFromJSON()
    -> convertJSONListToValueTree() (ScriptingApi.cpp:5240)
    -> s->killAllVoicesAndCall(..., true)
