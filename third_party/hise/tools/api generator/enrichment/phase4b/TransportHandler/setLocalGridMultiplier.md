TransportHandler::setLocalGridMultiplier(Integer factor) -> undefined

Thread safety: SAFE
Sets a per-instance multiplier that slows down grid callbacks for this TransportHandler. Only every Nth grid tick passes through, where N is the factor. The grid index in the callback is divided by the factor. This is local to this instance -- other instances are unaffected.
Required setup:
  const var th = Engine.createTransportHandler();
  th.setEnableGrid(true, 11);
  th.setLocalGridMultiplier(4); // receive every 4th grid tick
Dispatch/mechanics: Validates factor is 1 or power of two, clamps to [1, 64]. Stores `localGridMultiplier` and precomputes `localBitShift = log2(factor)` for bitmask filtering in `onGridChange()`.
Pair with: setEnableGrid -- sets the base grid rate this multiplier divides. setOnGridChange -- the callback affected by this multiplier. getGridLengthInSamples -- returned length accounts for the multiplier.
Anti-patterns:
  - Factor must be 1 or a power of two (2, 4, 8, 16, 32, 64). Non-power-of-two values report a script error.
Source:
  ScriptingApi.cpp:8639  setLocalGridMultiplier() -> jlimit(1, 64) -> localBitShift = log2(factor)
