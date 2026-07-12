Sampler::createSelectionWithFilter(Function filterFunction) -> Array

Thread safety: UNSAFE -- iterates all samples and calls filter function for each, allocates
Creates an array of Sample objects by evaluating a filter function for each sample.
The function is called with `this` set to a Sample object (no arguments). Return
non-zero to include the sample.
Callback signature: filterFunction(this: Sample) -> int
Required setup:
  inline function velocityFilter()
  {
      return this.get(Sampler.HiVel) > 64;
  };
  const var loudSamples = Sampler.createSelectionWithFilter(velocityFilter);
Dispatch/mechanics:
  engine->callExternalFunctionRaw(filterFunction, args) per sample
    -> filterFunction receives ScriptingSamplerSound as `this`
    -> non-zero return includes the sample in the result array
Pair with:
  createSelection -- simpler regex-based alternative
  createSelectionFromIndexes -- index-based alternative
Source:
  ScriptingApi.cpp  Sampler::createSelectionWithFilter()
    -> iterates all sounds via SoundIterator
    -> callExternalFunctionRaw for each sample
