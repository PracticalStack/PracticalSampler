TransportHandler::setOnTransportChange(Number sync, Function f) -> undefined

Thread safety: UNSAFE -- Allocates a new Callback object (heap allocation via `new Callback(...)`).
Registers a callback that fires when the transport state changes (play/stop). Fires immediately with the current play state upon registration.
Callback signature: f(bool isPlaying)
Required setup:
  const var th = Engine.createTransportHandler();
  inline function onTransport(isPlaying) {}
  th.setOnTransportChange(SyncNotification, onTransport);
Dispatch/mechanics: Creates a new Callback(numArgs=1). Immediately fires with cached `play` value (forceSync=true). Clears the opposite-mode slot if it holds the same function reference. At runtime, `onTransportChange()` is called on the audio thread and dispatches to both callback slots.
Pair with: isPlaying -- query current state without callback. setOnTempoChange -- cover all transport state events.
Anti-patterns:
  - Using a regular `function` with `SyncNotification` throws "Must use inline functions for synchronous callback" at registration.
Source:
  ScriptingApi.cpp:8446  setOnTransportChange() -> new Callback() -> call(play, forceSync=true)
  ScriptingApi.cpp:8505  onTransportChange() -> transportChangeCallback->call() / transportChangeCallbackAsync->call()
