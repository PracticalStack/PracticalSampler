Sampler::purgeSampleSelection(Array selection) -> undefined

Thread safety: UNSAFE -- killAllVoicesAndCall, allocates arrays
Purges the specified Sample objects and unpurges all other samples. All samples
NOT in the array are unpurged; all samples IN the array are purged.
Anti-patterns:
  - Do NOT pass duplicate Sample objects -- reports an error
  - Do NOT pass samples from a different sampler -- reports an error
Pair with:
  createSelection -- create the Sample array to pass to this method
  createSelectionFromIndexes -- alternative way to create Sample arrays
Source:
  ScriptingApi.cpp  Sampler::purgeSampleSelection()
    -> s->killAllVoicesAndCall(f, true)
    -> refreshes preload sizes and memory usage
