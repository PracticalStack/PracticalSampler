Engine::extendTimeOut(int additionalMilliseconds) -> undefined

Thread safety: SAFE -- adds integer to a Time member variable
Extends the compilation timeout during onInit for long-running initialization.
No practical effect in compiled plugins (no active timeout).
Source:
  ScriptingApi.cpp  Engine::extendTimeOut()
    -> adds milliseconds to compilation timeout
