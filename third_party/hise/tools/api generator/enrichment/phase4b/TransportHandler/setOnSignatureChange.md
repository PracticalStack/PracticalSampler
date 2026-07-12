TransportHandler::setOnSignatureChange(Number sync, Function f) -> undefined

Thread safety: UNSAFE -- Allocates a new Callback object (heap allocation via `new Callback(...)`).
Registers a callback that fires when the time signature changes. Fires immediately with the current time signature upon registration.
Callback signature: f(int nominator, int denominator)
Required setup:
  const var th = Engine.createTransportHandler();
  inline function onSig(nom, denom) {}
  th.setOnSignatureChange(SyncNotification, onSig);
Dispatch/mechanics: Creates a new Callback(numArgs=2). Immediately fires with cached `nom`/`denom` values (forceSync=true). Clears the opposite-mode slot if it holds the same function reference.
Pair with: setOnBeatChange -- beat callback behavior depends on time signature denominator.
Source:
  ScriptingApi.cpp:8472  setOnSignatureChange() -> new Callback() -> call(nom, denom, forceSync=true)
  ScriptingApi.cpp:8528  onSignatureChange() -> timeSignatureCallback->call() / timeSignatureCallbackAsync->call()
