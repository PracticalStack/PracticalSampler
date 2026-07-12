# MidiList -- Project Context

## Project Context

### Real-World Use Cases
- **Velocity recall for release triggers**: A sampler plugin stores each note's velocity on note-on and recalls it on note-off to trigger release samples at the correct dynamic level. This is the most common MidiList use case - it exploits the 128-slot layout matching MIDI note numbers exactly.
- **Note state tracking for visual feedback**: A piano or keyboard display stores a flag (0 or 1) per note to track which keys are currently held, then reads the MidiList inside a paint routine to highlight active keys. `fill(0)` initializes the display state; `setValue(noteNumber, 1)` on note-on and `setValue(noteNumber, 0)` on note-off keeps it current.
- **Per-note timing for round-robin resets**: A round-robin system stores the last trigger timestamp per note using `Engine.getUptime()`. On the next note-on, the elapsed time determines whether to reset the round-robin counter. This pattern relies on MidiList accepting arbitrary integer values (not just 0-127).
- **MIDI-to-channel routing table**: A drum machine maps each MIDI note to a channel index. The MidiList stores which channel (0-11) each note triggers, enabling instant lookup in both MIDI routing and keyboard display code. `fill(-1)` marks all notes as unassigned; `setValue(noteNumber, channelIndex)` creates the mapping.
- **Pedal-off deduplication**: When a sustain pedal releases, all held notes must trigger release samples - but only once each. A MidiList is cleared, then used as a "seen set" while iterating the held notes: check `getValue(noteNumber) == -1` before processing, then `setValue(noteNumber, 1)` to mark as handled.

### Complexity Tiers
1. **Basic storage** (most common): `fill`, `setValue`, `getValue`, `clear`. Covers velocity tracking, note state flags, and simple lookup tables. This tier handles the majority of real-world MidiList use cases.
2. **Search and count**: `getIndex`, `getValueAmount`, `getNumSetValues`, `isEmpty`. Used when the script needs to find notes matching a condition or check whether any notes are active. Useful for display logic and state validation.
3. **Serialization**: `getBase64String`, `restoreFromBase64String`. Used for saving/restoring custom MIDI mappings or velocity curves in user presets.

### Practical Defaults
- Use `fill(0)` (not `clear()`) when the MidiList represents a state where zero is the natural "off" value (key state flags, counters). Reserve `clear()` (which sets all slots to `-1`) for scenarios where `-1` represents "unset" or "no mapping."
- Store MidiList references in `const var` at the top of the script or namespace. MidiLists are typically created once and reused throughout the plugin's lifetime.
- For per-note timing, store the result of `Engine.getUptime()` directly. The value is a float but MidiList truncates to integer - for sub-second precision, multiply by 1000 before storing.

### Integration Patterns
- `MidiList.setValue()` + `Message.getNoteNumber()` - The standard pattern: use the incoming MIDI note number as the MidiList index to store per-note data (velocity, state flags, timestamps).
- `MidiList.getValue()` + `Synth.playNote()` - Recall stored velocity or state when generating synthetic notes (release triggers, retrigger-on-sustain-release).
- `MidiList.clear()` + iteration loop - The "seen set" pattern: clear the list, then iterate a collection of notes while using the MidiList to deduplicate processing.
- `MidiList.getValue()` inside `setPaintRoutine()` - Read per-note state in a panel's paint callback to visualize keyboard state, active ranges, or channel assignments.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using a JavaScript array (`var states = []; states.reserve(128);`) for per-note tracking | Use `Engine.createMidiList()` | MidiList is purpose-built for 128-slot MIDI data. Its native `fill`, `clear`, and search operations avoid script-level loops entirely. |
| Storing `Engine.getUptime()` directly for sub-second timing comparisons | Multiply by 1000 before storing: `list.setValue(n, Math.round(Engine.getUptime() * 1000))` | MidiList stores integers. `Engine.getUptime()` returns seconds as a float, so small durations collapse to the same integer. Scale to milliseconds for usable precision. |
| Using `fill(0)` when the intent is "no value assigned" | Use `clear()` or `fill(-1)` | The sentinel value `-1` is what `isEmpty()`, `getNumSetValues()`, and `getIndex()` treat as "empty." Using 0 as "empty" causes these methods to return incorrect results. |
