GlobalCable::registerCallback(Function callbackFunction, Integer synchronous) -> undefined

Thread safety: UNSAFE -- allocates callback objects and modifies the cable target list
Registers a function to be called whenever a value is sent through the cable. The callback receives the cable value converted through the local input range. Multiple callbacks can be registered per reference.
Callback signature: f(double value)
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.setRange(0.0, 100.0);

inline function onValue(value) { Console.print(value); };
cable.registerCallback(onValue, AsyncNotification);
```
Dispatch/mechanics: Creates a `Callback` inner struct registered as a `CableTargetBase` on the cable. With `SyncNotification`, the callback executes immediately on the calling thread via `callSync()` -- the function must be realtime-safe (`inline function`), otherwise registration silently fails. With `AsyncNotification`, values are stored via `ModValue` and delivered by `PooledUIUpdater::SimpleTimer` polling on the UI thread; rapid changes are coalesced (only the latest value delivered).
Pair with: `deregisterCallback` (remove a registered callback), `registerDataCallback` (data channel equivalent)
Anti-patterns: Using a non-realtime-safe function with `SyncNotification` silently fails -- the callback never fires, with no error. Each call adds a new callback; call `deregisterCallback` first to avoid duplicates.
Source:
  ScriptingApiObjects.cpp:9261  registerCallback() -> ApiHelpers::isSynchronous() -> new Callback() -> Cable::addTarget()
  ScriptingApiObjects.cpp:9145  Callback constructor -> isRealtimeSafe() check -> SimpleTimer start/stop
