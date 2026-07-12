Modulator::exists() -> Integer

Thread safety: SAFE
Returns 1 if the modulator reference is valid (internal C++ pointer is non-null
and modulator has not been deleted from the module tree), 0 otherwise.

Source:
  ScriptingApiObjects.cpp  checkValidObject() via ConstScriptingObject
    -> checks WeakReference<Modulator> mod != nullptr
