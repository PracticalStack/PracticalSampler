TransportHandler::setOnBypass(Function f) -> undefined

Thread safety: UNSAFE -- Allocates a new Callback object and registers with the plugin bypass handler.
Registers a callback that fires when the plugin's bypass state changes. Always asynchronous (no sync parameter). Fires immediately with the current bypass state upon registration.
Callback signature: f(bool isBypassed)
Required setup:
  const var th = Engine.createTransportHandler();
  inline function onBypass(isBypassed) {}
  th.setOnBypass(onBypass);
Dispatch/mechanics: Creates a new Callback(numArgs=1, sync=false). Registers with `PluginBypassHandler.listeners.addListener()` with immediate-fire flag. The static `onBypassUpdate()` callback dispatches with forceSync=true.
Source:
  ScriptingApi.cpp:8619  setOnBypass() -> new Callback(sync=false) -> addListener(immediate=true)
  ScriptingApi.cpp:8741  onBypassUpdate() -> bypassCallback->call(state, forceSync=true)
