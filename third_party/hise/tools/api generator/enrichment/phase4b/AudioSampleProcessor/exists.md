AudioSampleProcessor::exists() -> Integer

Thread safety: SAFE
Returns whether the underlying processor reference is still valid. Returns false if the
processor has been deleted or the handle was constructed with a null reference.
Dispatch/mechanics:
  objectExists() -> checks WeakReference<Processor> != nullptr
Source:
  ScriptingApiObjects.h:2437  objectExists() -> WeakReference null check
