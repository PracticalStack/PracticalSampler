Parameter::setValue(Double newValue) -> undefined

Thread safety: WARNING -- context-dependent. With externalConnection enabled, dispatches
to setValueAsync() (safe, direct DSP). Without it, dispatches to setValueSync() (unsafe,
ValueTree with undo). In backend builds, throws a script error if called from the audio
thread without external connection enabled.

Unified value setter that replaces the need to choose between setValueAsync() and
setValueSync() manually. Configure the mode once with setUseExternalConnection(), then
use setValue() throughout.

Dispatch/mechanics:
  externalConnection == true:  -> setValueAsync(newValue) -> dynamicParameter->call()
  externalConnection == false: -> setValueSync(newValue) -> data.setProperty(Value, ...)
    -> valuePropertyUpdater listener -> setValueAsync(newValue)
  Backend audio-thread guard: throws script error if !externalConnection on audio thread
    (compiled out in exported plugins -- silently falls through to setValueSync)

Pair with:
  setUseExternalConnection -- must configure dispatch mode before calling setValue
  getValue -- read back the current value

Anti-patterns:
  - Do NOT call from the audio thread without first enabling external connection via
    setUseExternalConnection(true) -- in backend builds this throws a script error; in
    exported plugins the check is compiled out and the call silently falls through to
    setValueSync() which performs unsafe ValueTree operations on the audio thread.

Source:
  NodeBase.cpp  Parameter::setValue()
    -> branches on externalConnection flag
    -> true path: setValueAsync(newValue)
    -> false path: setValueSync(newValue)
