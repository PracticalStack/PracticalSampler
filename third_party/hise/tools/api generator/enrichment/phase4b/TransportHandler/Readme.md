TransportHandler (object)
Obtain via: Engine.createTransportHandler()

Registers callbacks for DAW transport events: tempo, playback, beats, grid,
time signature, and bypass. Provides an internal clock system with configurable
sync modes for standalone or DAW-independent operation, and a high-precision
grid timer for sample-accurate sequencing.

Constants:
  SyncModes:
    Inactive = 0           No syncing going on
    ExternalOnly = 1       Only reacts on external clock events
    InternalOnly = 2       Only reacts on internal clock events
    PreferInternal = 3     Override with internal clock when it is playing
    PreferExternal = 4     Override with external clock when it is playing
    SyncInternal = 5       Sync internal clock when external playback starts

Complexity tiers:
  1. DAW-only transport: setOnTransportChange, setOnTempoChange. React to host
     transport without an internal clock. Default sync mode works.
  2. Internal clock with host fallback: + setSyncMode(PreferInternal),
     startInternalClock, stopInternalClock, stopInternalClockOnExternalStop.
     Plugins with their own play/stop controls.
  3. Full transport system: + setEnableGrid, setOnGridChange,
     sendGridSyncOnNextCallback, setLocalGridMultiplier. MIDI-triggered clock
     start with sample-accurate timestamps, host-sync toggle, grid timing.

Practical defaults:
  - Use PreferInternal as the default sync mode for plugins with their own
    transport controls. Switch to PreferExternal only when the user enables
    host sync.
  - Tempo factor 8 (1/8 note) is a good grid resolution for drum sequencers --
    fast enough for detailed patterns, slow enough for manageable callback
    frequency.
  - Always enable stopInternalClockOnExternalStop(true) when using
    PreferInternal with a host-sync option.
  - Use AsyncNotification for the transport change callback when it updates UI.
    Bridge to a Broadcaster for multi-file state propagation.

Common mistakes:
  - Registering a grid callback without calling setEnableGrid() first --
    silently never fires, no error reported.
  - Using a regular function with SyncNotification -- must use inline function
    for synchronous callbacks (audio thread).
  - Calling startInternalClock(0) from a MIDI callback -- use
    startInternalClock(Message.getTimestamp()) for sample-accurate timing.
  - Updating UI components directly in the transport callback -- bridge to a
    Broadcaster for loose coupling across script files.
  - Not stopping the internal clock before loading a preset -- causes timing
    discontinuities. Stop first, sendGridSyncOnNextCallback, then restart.

Example:
  const var th = Engine.createTransportHandler();

  // Synchronous tempo callback (audio thread, requires inline function)
  inline function onTempoChange(newTempo)
  {
      // React to tempo changes on the audio thread
  }

  th.setOnTempoChange(SyncNotification, onTempoChange);

  // Asynchronous transport callback (UI thread)
  inline function onTransportChange(isPlaying)
  {
      Console.print(isPlaying ? "Playing" : "Stopped");
  }

  th.setOnTransportChange(AsyncNotification, onTransportChange);

  // High-precision grid for sample-accurate sequencing
  th.setEnableGrid(true, 7); // 1/8 note grid

  inline function onGridChange(gridIndex, timestamp, firstGrid)
  {
      if (firstGrid)
          Console.print("Grid restarted");
  }

  th.setOnGridChange(SyncNotification, onGridChange);

  // Set sync mode for internal/external clock interaction
  th.setSyncMode(th.PreferExternal);

Methods (19):
  getGridLengthInSamples      getGridPosition
  isNonRealtime               isPlaying
  sendGridSyncOnNextCallback  setEnableGrid
  setLinkBpmToSyncMode        setLocalGridBypassed
  setLocalGridMultiplier      setOnBeatChange
  setOnBypass                 setOnGridChange
  setOnSignatureChange        setOnTempoChange
  setOnTransportChange        setSyncMode
  startInternalClock          stopInternalClock
  stopInternalClockOnExternalStop
