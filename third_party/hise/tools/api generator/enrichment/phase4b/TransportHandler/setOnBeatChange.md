TransportHandler::setOnBeatChange(Number sync, Function f) -> undefined

Thread safety: UNSAFE -- Allocates a new Callback object and registers/removes a MusicalUpdateListener.
Registers a callback that fires on each musical beat. Receives beat index and whether it is the first beat of a new bar. Accounts for time signature denominator (e.g. fires twice as often in 6/8 vs 3/4). Does NOT fire immediately upon registration. Pass `undefined` as `f` to remove the listener.
Callback signature: f(int beatIndex, bool isNewBar)
Required setup:
  const var th = Engine.createTransportHandler();
  inline function onBeat(beatIndex, isNewBar) {}
  th.setOnBeatChange(SyncNotification, onBeat);
Dispatch/mechanics: If `f` is undefined, calls `removeMusicalUpdateListener()`. Otherwise calls `addMusicalUpdateListener()` and creates a new Callback(numArgs=2). Does not fire immediately -- first callback comes at the next beat boundary.
Pair with: setOnGridChange -- finer-grained musical timing. setOnSignatureChange -- beat behavior depends on time signature.
Anti-patterns:
  - Do not expect an immediate callback on registration (unlike setOnTempoChange/setOnTransportChange). The first call comes at the next beat boundary.
Source:
  ScriptingApi.cpp:8573  setOnBeatChange() -> addMusicalUpdateListener() -> new Callback()
  ScriptingApi.cpp:8516  onBeatChange() -> beatCallback->call() / beatCallbackAsync->call()
