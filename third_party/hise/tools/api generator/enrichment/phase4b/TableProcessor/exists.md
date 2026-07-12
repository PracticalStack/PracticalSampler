TableProcessor::exists() -> int

Thread safety: SAFE
Returns whether the underlying processor reference is still valid. Returns
false if the processor has been deleted or if Synth.getTableProcessor() could
not find the specified module.

Dispatch/mechanics:
  checkValidObject() -> objectExists() -> WeakReference<Processor> null check

Source:
  ScriptingApiObjects.h:2554  objectExists() checks WeakReference for null
