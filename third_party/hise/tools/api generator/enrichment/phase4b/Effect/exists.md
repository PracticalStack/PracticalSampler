Effect::exists() -> Integer

Thread safety: SAFE
Returns whether the effect module referenced by this handle still exists in the
module tree. Returns false if the underlying processor has been deleted.
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::exists()
    -> checkValidObject() tests WeakReference<Processor> validity
