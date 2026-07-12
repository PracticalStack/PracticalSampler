TransportHandler::setLocalGridBypassed(Integer shouldBeBypassed) -> undefined

Thread safety: SAFE
Bypasses grid callbacks for this TransportHandler instance. When bypassed, no grid callbacks fire. When unbypassed, the next grid callback will have `firstGridInPlayback` set to true. This is local to this instance -- other instances are unaffected.
Required setup:
  const var th = Engine.createTransportHandler();
  th.setLocalGridBypassed(true);
Dispatch/mechanics: Sets `localBypassed` flag. On unbypass, sets `nextLocalIsFirst = true` so the next grid tick signals a resync.
Pair with: setOnGridChange -- the callback being bypassed. setEnableGrid -- the grid must be enabled for bypass to matter.
Source:
  ScriptingApi.cpp:8658  setLocalGridBypassed() -> localBypassed = shouldBeBypassed; nextLocalIsFirst = true on unbypass
