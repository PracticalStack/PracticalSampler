# TransportHandler -- Project Context

## Project Context

### Real-World Use Cases
- **Drum machine sequencer transport**: A multi-channel drum sequencer builds a full transport system around TransportHandler: play/stop buttons that start/stop the internal clock, a host-sync toggle that switches between `PreferInternal` and `PreferExternal` sync modes, and MIDI note triggers that start/stop the clock with sample-accurate timestamps via `Message.getTimestamp()`. The transport change callback is bridged to a Broadcaster, creating a reactive chain where the play state propagates to UI buttons and sequencer state across multiple script files. MidiPlayers are synced to the master clock via `setSyncToMasterClock(true)`, making the TransportHandler the central timing authority. A second TransportHandler instance in a connected MIDI processor script provides a simplified alternative interface for external vs. internal clock selection.

### Complexity Tiers
1. **DAW-only transport** (simplest): Register `setOnTransportChange` and `setOnTempoChange` callbacks to react to the host. No sync mode setup needed -- the default works. Suitable for plugins that follow the DAW without an internal transport.
2. **Internal clock with host fallback**: Configure `setSyncMode(th.PreferInternal)`, use `startInternalClock(0)` / `stopInternalClock(0)` from UI buttons, and enable `stopInternalClockOnExternalStop(true)`. This covers plugins with their own play/stop controls.
3. **Full transport system with MIDI triggers**: All Tier 2 features, plus: MIDI note handlers that call `startInternalClock(Message.getTimestamp())` for sample-accurate clock start, a host-sync toggle that dynamically switches between `PreferInternal` and `PreferExternal`, `sendGridSyncOnNextCallback()` for resync after preset changes, `setEnableGrid()` for high-precision grid timing, and bridging the transport callback to a Broadcaster for multi-file state propagation.

### Practical Defaults
- Use `PreferInternal` as the default sync mode for plugins with their own transport controls, switching to `PreferExternal` only when the user explicitly enables host sync. A standalone-capable sequencer should work without a DAW by default.
- Tempo factor 8 (1/8 note) is a good grid resolution for drum sequencers -- fast enough for detailed patterns, slow enough to keep callback frequency manageable. Sync MidiPlayers to this grid via `setSyncToMasterClock(true)`.
- Always enable `stopInternalClockOnExternalStop(true)` when using `PreferInternal` with a host-sync option. Without it, stopping the DAW while host-synced leaves the internal clock in an ambiguous state.
- Use `AsyncNotification` for the transport change callback when the callback updates UI. Bridge it to a Broadcaster rather than updating components directly -- this enables multiple listeners across script files without tight coupling.

### Integration Patterns
- `TransportHandler.setOnTransportChange()` -> `Broadcaster` -- Pass a Broadcaster object as the callback function to `setOnTransportChange`. Every transport state change is automatically dispatched through the Broadcaster's listener network, eliminating manual forwarding of play state to UI buttons, sequencer logic, and preset preview systems.
- `TransportHandler.startInternalClock(Message.getTimestamp())` -> `MidiPlayer.setSyncToMasterClock(true)` -- Starting the clock with the MIDI event's timestamp provides sample-accurate alignment. MidiPlayers configured to sync to the master clock automatically begin playback at the correct position.
- `TransportHandler.stopInternalClock(0)` -> preset load -> `startInternalClock(0)` -- Stop the clock before loading a preset, then restart it after load completes. Call `sendGridSyncOnNextCallback()` before restarting to ensure the grid resyncs cleanly.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `startInternalClock(0)` from a MIDI callback | Call `startInternalClock(Message.getTimestamp())` from MIDI callbacks | The timestamp parameter provides sample-accurate positioning within the audio block. Using 0 always starts at the block boundary, which can cause timing jitter of up to one block size. |
| Updating UI components directly in the transport callback | Bridge the transport callback to a Broadcaster | When multiple script files need to react to transport changes, direct component updates create tight coupling. Passing a Broadcaster as the callback function enables loose coupling across many listener sites. |
| Not stopping the internal clock before loading a preset | Call `stopInternalClock(0)` before `Engine.loadUserPreset()` | Loading a preset while the clock is running can cause timing discontinuities. Stop playback first, then call `sendGridSyncOnNextCallback()` and restart the clock after the preset loads. |
