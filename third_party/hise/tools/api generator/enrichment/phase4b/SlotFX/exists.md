SlotFX::exists() -> Integer

Thread safety: SAFE
Returns whether the underlying processor reference is still valid. Checks the
internal WeakReference -- returns 1 if the processor exists, 0 if deleted or
never assigned.
Source:
  ScriptingApiObjects.h:2106  exists() -> checkValidObject()
    -> WeakReference<Processor>::get() != nullptr
