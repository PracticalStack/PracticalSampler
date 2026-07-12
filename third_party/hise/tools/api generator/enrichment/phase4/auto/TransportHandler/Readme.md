# TransportHandler

TransportHandler registers callbacks that fire when the DAW transport state changes:

- Tempo changes
- Playback (transport start / stop)
- Beats and time signature
- High-precision grid events
- Plugin bypass

Each callback (except bypass) supports synchronous (audio-thread) or asynchronous (UI-thread) dispatch, allowing you to react to transport events in real-time audio logic or deferred UI updates.

The grid system provides sample-accurate timing events at a configurable musical rate (whole note down to 64th triplet). You can set per-instance rate divisions so that multiple TransportHandler instances operate at different subdivisions of the same master grid.

The class also provides an internal clock with configurable sync modes - you can follow the DAW exclusively, run your own independent clock, or blend the two with fallback behaviour.

> [!Tip:Most operations are global across instances] Most operations are global and affect the shared master clock across all TransportHandler instances. Callbacks and local grid settings are per-instance.

| Mode | Index | External (DAW) | Internal (script) | Behaviour |
|------|-------|----------------|-------------------|----------|
| Inactive | 0 | ignored | ignored | No clock processing, `getPPQPos()` returns 0 |
| ExternalOnly | 1 | active | ignored | DAW transport only; no internal playhead |
| InternalOnly | 2 | ignored | active | Script-driven clock; `allowExternalSync()` returns false |
| PreferInternal | 3 | fallback | wins | Internal clock wins conflicts; external used when idle |
| PreferExternal | 4 | wins | fallback | DAW wins conflicts; internal used when DAW idle |
| SyncInternal | 5 | PPQ sync | start/stop | Internal controls playback; PPQ syncs to DAW position |

## Common Mistakes

- **Enable grid before registering callback**
  **Wrong:** `th.setOnGridChange(SyncNotification, onGrid);` without calling `th.setEnableGrid(true, tempoFactor)`  
  **Right:** Call `th.setEnableGrid(true, 7)` before registering the grid callback  
  *The grid must be enabled globally before grid callbacks fire. Without it, the grid callback is registered but never triggered.*

- **Use inline function for sync callbacks**
  **Wrong:** Using a regular `function` with `SyncNotification`  
  **Right:** Use `inline function` for synchronous callbacks  
  *Synchronous callbacks run on the audio thread and require `inline function`. A regular function will throw "Must use inline functions for synchronous callback" at registration.*

- **Pass Message.getTimestamp for accuracy**
  **Wrong:** Calling `startInternalClock(0)` from a MIDI callback  
  **Right:** Call `startInternalClock(Message.getTimestamp())` from MIDI callbacks  
  *The timestamp parameter provides sample-accurate positioning within the audio block. Using 0 always starts at the block boundary, which can cause timing jitter of up to one block size.*

- **Defer UI updates to timer or async**
  **Wrong:** Updating UI components directly in the transport callback  
  **Right:** Bridge the transport callback to a Broadcaster  
  *When multiple script files need to react to transport changes, direct component updates create tight coupling. Passing a Broadcaster as the callback function enables loose coupling across many listener sites.*

- **Stop clock before loading new preset**
  **Wrong:** Not stopping the internal clock before loading a preset  
  **Right:** Call `stopInternalClock(0)` before `Engine.loadUserPreset()`  
  *Loading a preset while the clock is running can cause timing discontinuities. Stop playback first, then call `sendGridSyncOnNextCallback()` and restart the clock after the preset loads.*
