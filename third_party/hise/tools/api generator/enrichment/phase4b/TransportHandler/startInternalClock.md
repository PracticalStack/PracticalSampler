TransportHandler::startInternalClock(Integer timestamp) -> undefined

Thread safety: SAFE
Starts the internal master clock at the given sample timestamp offset within the current audio block. This is a global operation affecting the shared MasterClock. If called during audio rendering, immediately processes grid and transport callbacks for the current block.
Required setup:
  const var th = Engine.createTransportHandler();
  th.setSyncMode(th.InternalOnly);
  th.startInternalClock(0);
Dispatch/mechanics: Calls `MasterClock::changeState(timestamp, internalClock=true, startPlayback=true)`. If inside audio rendering and state changed, calls `processAndCheckGrid()`, creates internal playhead, and calls `handleTransportCallbacks()`.
Pair with: stopInternalClock -- stop counterpart. setSyncMode -- must be in a mode that allows internal clock. setOnTransportChange -- receives the resulting play state change.
Source:
  ScriptingApi.cpp:8669  startInternalClock() -> MasterClock::changeState() -> processAndCheckGrid() -> handleTransportCallbacks()
