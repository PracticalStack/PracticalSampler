TransportHandler::setOnGridChange(Number sync, Function f) -> undefined

Thread safety: UNSAFE -- Allocates a new Callback object and registers/removes a MusicalUpdateListener.
Registers a callback that fires on each grid tick. The grid must be enabled first via `setEnableGrid()`. Receives grid index (adjusted for local multiplier), sample-accurate timestamp, and whether this is the first grid tick in the current playback session. Does NOT fire immediately upon registration. Pass `undefined` as `f` to remove the listener.
Callback signature: f(int gridIndex, int timestamp, bool firstGridInPlayback)
Required setup:
  const var th = Engine.createTransportHandler();
  th.setEnableGrid(true, 11); // enable 1/16 grid first
  inline function onGrid(gridIndex, timestamp, firstGrid) {}
  th.setOnGridChange(SyncNotification, onGrid);
Dispatch/mechanics: If `f` is undefined, calls `removeMusicalUpdateListener()`. Otherwise calls `addMusicalUpdateListener()` and creates a new Callback(numArgs=3). At runtime, `onGridChange()` applies the local multiplier bitmask filter and bit-shift before dispatching.
Pair with: setEnableGrid -- must be called first to enable the grid. setLocalGridMultiplier, setLocalGridBypassed -- per-instance grid rate control.
Anti-patterns:
  - Registering a grid callback without calling `setEnableGrid(true, tempoFactor)` first silently results in no callbacks ever firing -- no error is reported.
  - Do not expect an immediate callback on registration.
Source:
  ScriptingApi.cpp:8596  setOnGridChange() -> addMusicalUpdateListener() -> new Callback()
  ScriptingApi.cpp:8540  onGridChange() -> bitmask filter -> gridCallback->call() / gridCallbackAsync->call()
