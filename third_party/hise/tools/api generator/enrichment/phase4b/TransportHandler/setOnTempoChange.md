TransportHandler::setOnTempoChange(Number sync, Function f) -> undefined

Thread safety: UNSAFE -- Allocates a new Callback object (heap allocation via `new Callback(...)`) and stores it in a ScopedPointer.
Registers a callback that fires when the host tempo (BPM) changes. The `sync` parameter controls dispatch: `SyncNotification` executes on the audio thread (requires `inline function`), `AsyncNotification` dispatches to the UI thread. Fires immediately with current tempo upon registration.
Callback signature: f(double newTempo)
Required setup:
  const var th = Engine.createTransportHandler();
  inline function onTempo(newTempo) {}
  th.setOnTempoChange(SyncNotification, onTempo);
Dispatch/mechanics: Creates a new Callback(numArgs=1). Immediately fires with cached `bpm` value (forceSync=true regardless of dispatch mode). Clears the opposite-mode slot if it holds the same function reference. At runtime, `tempoChanged()` is called on the audio thread and dispatches to sync and/or async callback slots.
Pair with: setOnTransportChange, setOnSignatureChange -- together cover all transport state events.
Anti-patterns:
  - Do not assume the callback only fires on tempo changes -- it fires immediately upon registration with the current BPM value, always synchronously on that first call regardless of async mode.
  - Using a regular `function` with `SyncNotification` throws "Must use inline functions for synchronous callback" at registration.
Source:
  ScriptingApi.cpp:8426  setOnTempoChange() -> new Callback() -> call(bpm, forceSync=true)
  ScriptingApi.cpp:8492  tempoChanged() -> tempoChangeCallback->call() / tempoChangeCallbackAsync->call()
