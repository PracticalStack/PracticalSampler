# TransportHandler -- Class Analysis

## Brief
Registers callbacks for DAW transport events: tempo, playback, beats, grid, time signature, and bypass.

## Purpose
TransportHandler is an event-driven scripting API object that reacts to host transport state changes. It supports six event types -- tempo, transport (play/stop), beat position, time signature, high-precision grid, and plugin bypass -- each with independent synchronous (audio-thread) or asynchronous (UI-thread) callback dispatch. The class also provides an internal clock system with configurable sync modes for standalone or DAW-independent operation, and a high-precision grid timer for sample-accurate sequencing with per-instance rate division and bypass control.

## Details

### Callback Dispatch Model

Each event type (except bypass) supports two dispatch modes controlled by the `sync` parameter:

| Mode | sync value | Thread | Callback requirement |
|------|-----------|--------|---------------------|
| Synchronous | `SyncNotification` | Audio thread | Must be `inline function` |
| Asynchronous | `AsyncNotification` | UI thread (via PooledUIUpdater) | Any function |

Registering a sync callback clears the async slot for the same function (and vice versa). Both modes can coexist if they use different function references. The bypass callback is always asynchronous.

When a callback is registered, it fires immediately with the current state (tempo, play state, time signature). The beat and grid callbacks do NOT fire immediately -- they begin firing on the next musical position change.

Sync callbacks execute directly on the audio thread during the `TempoListener` virtual call. Async callbacks queue via `sendPooledChangeMessage()` and deliver on the UI thread at the next PooledUIUpdater tick. Async callbacks from TransportHandler use the high-priority path.

### Grid System

The grid provides sample-accurate timing events at a configurable musical rate. Setup:

1. Call `setEnableGrid(true, tempoFactor)` to activate the global grid at a specific tempo division (0-18, mapping to note values from 1/1 to 1/64T)
2. Register a grid callback via `setOnGridChange(sync, callback)` with 3 args: gridIndex, timestamp, firstGridInPlayback
3. Optionally adjust per-instance rate with `setLocalGridMultiplier(factor)` (power-of-two, 1-64)
4. Optionally bypass per-instance grid with `setLocalGridBypassed(true)`

The grid is a global clock resource -- `setEnableGrid` affects all TransportHandler instances. The local multiplier and bypass are per-instance, allowing different handlers to operate at different subdivisions of the same master grid.

The `timestamp` argument in the grid callback is a sample offset within the current audio block, enabling sample-accurate event scheduling (e.g., note placement in an arpeggiator).

### Clock Sync Modes

The `setSyncMode()` method configures how the internal clock interacts with the external (DAW) clock. The MasterClock maintains two independent clock sources -- an internal clock (started/stopped by `startInternalClock`/`stopInternalClock`) and an external clock (driven by the DAW's AudioPlayHead). The sync mode determines which source drives transport callbacks, grid timing, BPM, and PPQ position.

The internal clock is started/stopped via `startInternalClock(timestamp)` / `stopInternalClock(timestamp)`. The timestamp is a sample offset for sub-block accuracy.

#### Mode Behaviors

**Inactive (0)** -- All clock processing is disabled. `changeState()` returns false, `processAndCheckGrid()` returns empty, `getPPQPos()` returns 0.0. No transport, beat, or grid callbacks fire. Use this to completely disable the TransportHandler's clock system.

**ExternalOnly (1)** -- The MasterClock only responds to DAW transport events. Internal clock start/stop calls are accepted but have no effect because `shouldCreateInternalInfo()` returns false -- the internal playhead is never created. `allowExternalSync()` returns true, so `updateFromExternalPlayHead()` processes DAW play state changes and drives grid timing from the DAW's PPQ position. Use this when the plugin should follow the DAW transport exclusively.

**InternalOnly (2)** -- The MasterClock only responds to script-driven clock events. `shouldCreateInternalInfo()` returns true unconditionally. `allowExternalSync()` returns false, so the DAW's play state is never processed. `shouldPreferInternal()` returns true, so any external clock events that somehow arrive are ignored by `changeState()` when the internal clock is playing. Grid timing and PPQ position are calculated from the internal uptime counter. Use this for standalone operation or when the plugin should ignore DAW transport entirely.

**PreferInternal (3)** -- Favors the internal clock but accepts external events as fallback. `shouldPreferInternal()` returns true, so when the internal clock is playing, external play/stop events are ignored by `changeState()`. When the internal clock is not playing, external events drive transport normally. `shouldCreateInternalInfo()` returns true (default fallthrough), so the internal playhead is always available. `allowExternalSync()` returns true. Use this when the plugin has its own transport but should fall back to the DAW when the internal clock is inactive.

**PreferExternal (4)** -- Favors the DAW clock but accepts internal events as fallback. `shouldPreferInternal()` returns false, so when the DAW is playing, internal start/stop events are ignored by `changeState()`. `shouldCreateInternalInfo()` returns false when the DAW is playing or the state is `ExternalClockPlay`. When the DAW starts playing and the internal clock was active, `updateFromExternalPlayHead()` performs a handoff: it transitions the state from `InternalClockPlay` to `ExternalClockPlay` and fires a first-grid resync event. When the DAW stops, the internal clock resumes if it was running (unless `stopInternalClockOnExternalStop` is enabled). Use this when the plugin should follow the DAW when available but maintain its own transport for standalone or when the DAW is stopped.

**SyncInternal (5)** -- Runs the internal clock but continuously syncs its PPQ position to the external playback position. `shouldPreferInternal()` returns true and `shouldCreateInternalInfo()` returns true, so the internal playhead is always used. The key difference: `processAndCheckGrid()` overrides the internal `uptime` with the external PPQ position whenever the DAW is playing (line 2289-2298), keeping the internal grid aligned with the DAW's musical position. Critically, `changeState()` ignores external stop commands (`!startPlayback && !internalClock` returns false), so the internal clock keeps running even when the DAW stops. Use this when you need internal clock control (start/stop from script) but want the grid and PPQ position to stay locked to the DAW's musical timeline.

#### Decision Function Summary

The sync mode is checked in four key MasterClock methods:

| Function | What it decides | Modes that return true |
|----------|----------------|----------------------|
| `shouldPreferInternal()` | Whether internal clock wins over external in conflicts | InternalOnly, PreferInternal, SyncInternal |
| `shouldCreateInternalInfo()` | Whether to create an internal playhead each audio block | InternalOnly, PreferInternal, SyncInternal (+ PreferExternal when DAW is not playing) |
| `allowExternalSync()` | Whether `updateFromExternalPlayHead()` processes DAW events | All except InternalOnly |
| `changeState()` early exits | Which clock source's events are dropped | Inactive drops all; others drop the non-preferred source when the preferred source is playing |

#### BPM Source Selection

When `setLinkBpmToSyncMode(true)` is active, `getBpmToUse()` selects BPM based on `shouldPreferInternal()`: modes that prefer internal (InternalOnly, PreferInternal, SyncInternal) use the internal BPM, while ExternalOnly and PreferExternal use the host BPM. When `linkBpmToSync` is false, the internal BPM is always used (with fallback to host BPM if internal is 0).

#### External Clock Source

The "external clock" referenced by the sync modes comes from different sources depending on the build target:

- **Exported plugin** (`USE_FRONTEND`): The real DAW provides the `AudioPlayHead`. JUCE's `AudioProcessor::getPlayHead()` returns the host's playhead directly, and `MasterClock::updateFromExternalPlayHead()` consumes its `CurrentPositionInfo` (BPM, PPQ position, isPlaying, time signature, loop points).

- **HISE IDE standalone** (`USE_BACKEND`, `IS_STANDALONE_APP`): There is no DAW, so `BackendProcessor` owns an `ExternalClockSimulator` instance (`BackendProcessor.h:557`). This class (`hi_core/hi_components/floating_layout/BackendPanelTypes.h:116`) subclasses `juce::AudioPlayHead` and simulates a DAW transport. `BackendProcessor::processBlock()` calls `setPlayHead(&externalClockSim)` to install it as the audio engine's playhead, then calls `externalClockSim.process(numSamples)` each block to advance the PPQ position. The `ExternalClockSimulator` maintains BPM, PPQ position, time signature (nom/denom), loop range, and isPlaying state. It is controlled by the `DAWClockController` UI component (the transport bar at the top of the HISE IDE), which sets `isPlaying`, `bpm`, loop points, etc. directly on the simulator's public members.

- **Plugin build** (`USE_BACKEND` as plugin, not standalone): `setPlayHead(&externalClockSim)` is guarded by `#if IS_STANDALONE_APP`, so the ExternalClockSimulator is unused -- the host DAW's playhead is used directly.

This means that when developing with sync modes in the HISE IDE, the "external clock" is the IDE's built-in transport bar. The same sync mode code runs identically in an exported plugin, but the external clock source switches to the real DAW transport automatically.

### Musical Update Listener Lifecycle

Beat and grid callbacks require the TransportHandler to be registered as a `MusicalUpdateListener` on the MainController. This registration happens automatically when you call `setOnBeatChange` or `setOnGridChange` with a function. Passing `undefined` as the function removes the listener. The destructor also removes the listener.

## obtainedVia
`Engine.createTransportHandler()`

## minimalObjectToken
th

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| Inactive | 0 | int | No syncing going on | SyncModes |
| ExternalOnly | 1 | int | Only reacts on external clock events | SyncModes |
| InternalOnly | 2 | int | Only reacts on internal clock events | SyncModes |
| PreferInternal | 3 | int | Override with internal clock when it is playing | SyncModes |
| PreferExternal | 4 | int | Override with external clock when it is playing | SyncModes |
| SyncInternal | 5 | int | Sync internal clock when external playback starts | SyncModes |

## Dynamic Constants
(None)

| Name | Type | Description |
|------|------|-------------|

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `th.setOnGridChange(SyncNotification, onGrid);` without calling `th.setEnableGrid(true, tempoFactor)` | Call `th.setEnableGrid(true, 7)` before registering the grid callback | The grid must be enabled globally before grid callbacks fire. Without it, the grid callback is registered but never triggered. |
| Using a regular `function` with `SyncNotification` | Use `inline function` for synchronous callbacks | Synchronous callbacks run on the audio thread and require `inline function`. A regular function will throw "Must use inline functions for synchronous callback" at registration. |

## codeExample
```javascript
const var th = Engine.createTransportHandler();

// Synchronous tempo callback (audio thread, requires inline function)
inline function onTempoChange(newTempo)
{
    // React to tempo changes on the audio thread
}

th.setOnTempoChange(SyncNotification, onTempoChange);

// Asynchronous transport callback (UI thread, any function works)
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
```

## Alternatives
- `Timer` -- fires at a fixed millisecond interval independent of the DAW transport. Use when you need periodic polling unrelated to musical timing.
- `Broadcaster` -- attaches to arbitrary component, module, or data sources. Use when monitoring UI/module state rather than transport events.

## Related Preprocessors
None.

## Diagrams

### sync-async-dispatch
- **Brief:** Sync vs Async Callback Dispatch
- **Type:** timing
- **Description:** When a transport event fires (e.g. tempo change), the TempoListener virtual is called on the audio thread. If a sync callback is registered, it executes immediately via callSync() on the audio thread. If an async callback is registered, it calls sendPooledChangeMessage() which queues a message. On the next PooledUIUpdater timer tick (UI thread), handlePooledMessage() dispatches callAsync() which executes the user function on the UI thread. Both paths can fire for the same event if different functions are registered in each slot.

### clock-sync-modes
- **Brief:** Clock Sync Mode State Machine
- **Type:** state
- **Description:** The MasterClock has two clock sources -- internal (started/stopped by script) and external (driven by the DAW). The SyncModes enum controls which source drives the transport callbacks. In Inactive mode, neither source produces events. ExternalOnly ignores the internal clock. InternalOnly ignores the DAW. PreferInternal uses the internal clock when playing, falling back to external. PreferExternal does the opposite. SyncInternal syncs the internal clock start position to external playback, combining both sources.
