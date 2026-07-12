# Synth -- Project Context

## Project Context

### Real-World Use Cases
- **Sample-based instrument interface**: A plugin that needs to load sample maps, apply effects, and provide a scripted UI builds its entire script layer around `Synth.getEffect`, `Synth.getSampler`, `Synth.getModulator` references cached in `onInit`. `Synth.deferCallbacks(true)` is used when the interface performs heavy UI work and doesn't need to modify MIDI events in real time.
- **Drum machine / step sequencer**: A pattern-based instrument uses `Synth.playNote` / `Synth.noteOffDelayedByEventId` to trigger one-shot notes from a sequencer grid, `Synth.addVolumeFade` with `-100` for fade-to-kill on loop restarts, and `Synth.createBuilder` to programmatically construct a channel-strip architecture with per-channel routing matrices.
- **Monophonic / legato synthesizer**: A synth or sampled instrument with monophonic mode uses `Synth.addNoteOn` / `Synth.noteOffByEventId` to replace the incoming note with an artificial one, then `Synth.addPitchFade` to glide between pitches. `Synth.isLegatoInterval()` detects held-key transitions.
- **Unison / voice-stacking synth**: A synth that needs multiple detuned voices per key uses `Synth.setShouldKillRetriggeredNote(false)` to prevent same-pitch kills, then generates N artificial notes per incoming note via `Synth.addNoteOn` with different MIDI channels.
- **Multi-output mixer**: A plugin with per-channel bus routing uses `Synth.getRoutingMatrix` to dynamically remap stereo pairs to output buses, enabling a multi-out configuration that the user can toggle at runtime.

### Complexity Tiers
1. **Module references only** (most common): `getEffect`, `getSampler`, `getModulator`, `getChildSynth`, `getMidiProcessor` cached in `onInit`, then `setAttribute` / `setBypassed` in callbacks. Covers basic instrument and FX plugin scripts.
2. **MIDI generation**: `playNote` / `noteOffByEventId` / `noteOffDelayedByEventId` / `addVolumeFade` for scripted note triggering from UI elements or sequencers. Requires understanding event IDs and artificial notes.
3. **Monophonic / glide**: `addNoteOn` / `addPitchFade` / `isLegatoInterval` / `setFixNoteOnAfterNoteOff` / `attachNote` for legato instruments. Requires careful event ID tracking and the two-phase pitch fade pattern.
4. **Builder API and routing**: `createBuilder` for programmatic module tree construction, `getRoutingMatrix` for multi-output routing. Used in complex architectures with many identical channel strips.

### Practical Defaults
- Use `Synth.deferCallbacks(true)` when the script processor is purely a UI controller for a sampler or effect chain. This moves MIDI callbacks off the audio thread so you can safely perform string operations, UI updates, and allocations.
- Use `Synth.deferCallbacks(false)` (the default) when the script needs to modify MIDI events in real time - legato scripts, arpeggiators, or any script that calls `Message.ignoreEvent`, `Synth.addNoteOn`, or `Synth.addPitchFade`.
- Always store event IDs returned by `playNote` and `addNoteOn` in `reg` variables for audio-thread access. Use `Synth.noteOffByEventId` (not the deprecated `noteOff`) to stop them.
- For one-shot note previews (triggered from UI buttons), use `Synth.playNote` + `Synth.noteOffDelayedByEventId` with a fixed sample count (e.g., 44100 for ~1 second).
- For custom on-screen keyboards built with ScriptPanels, use `Synth.playNoteFromUI` / `Synth.noteOffFromUI` instead of `playNote` / `noteOffByEventId`. This routes through the real MIDI pipeline and updates the keyboard state correctly.
- `Synth.startTimer(0.05)` (50ms) is a good default for UI-update timers. For sample-accurate timing, use the non-deferred timer system with smaller intervals.

### Integration Patterns
- `Synth.getEffect()` -> `Effect.setAttribute()` / `Effect.setBypassed()` -- the most common integration: caching processor references in `onInit`, then controlling them from control callbacks or broadcasters.
- `Synth.createBuilder()` -> `Builder.create()` -> `Builder.get()` -> `RoutingMatrix.addConnection()` -- programmatic module tree construction with per-channel routing. The builder script is commented out by default and activated only when the module tree needs to change.
- `Synth.playNote()` -> `Synth.addVolumeFade(id, fadeMs, -100)` -- fade-to-kill pattern for stopping artificial notes with a smooth volume ramp instead of an abrupt note-off.
- `Synth.addNoteOn()` -> `Synth.addPitchFade(id, 0, -delta, 0)` -> `Synth.addPitchFade(id, glideTime, 0, 0)` -- two-phase glide: instantly set a pitch offset matching the interval between old and new notes, then fade back to zero over the glide time.
- `Synth.getNumPressedKeys()` / `Synth.isLegatoInterval()` -> `Synth.startTimer()` / `Synth.stopTimer()` -- timer-based sustain release: start a delayed timer when the last key is released, cancel it if a new key arrives, and stop the background note on timer expiry.
- `Synth.getRoutingMatrix()` -> `RoutingMatrix.clear()` -> `RoutingMatrix.addConnection()` -- dynamic multi-output routing toggled by a UI button.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `Synth.getEffect("name")` inside `onNoteOn` | Cache `const var fx = Synth.getEffect("name")` in `onInit` | Most `get*()` methods allocate wrapper objects and are restricted to `onInit`. Calling them in MIDI callbacks causes audio-thread allocations or outright errors. |
| Using `Synth.playNote` for custom keyboard panels | Using `Synth.playNoteFromUI` / `Synth.noteOffFromUI` | `playNoteFromUI` routes through the real MIDI input pipeline, updating keyboard state (`isKeyDown`, `getNumPressedKeys`). `playNote` creates artificial events that bypass keyboard state tracking. |
| Calling `Synth.addPitchFade(id, glideTime, targetPitch, 0)` once | Two-phase pattern: `addPitchFade(id, 0, -delta, 0)` then `addPitchFade(id, glideTime, 0, 0)` | A single pitch fade sets the target pitch relative to the note's original pitch, not relative to the current glide position. The two-phase pattern starts with an instant offset and fades to neutral. |
| Forgetting to call `Synth.setShouldKillRetriggeredNote(false)` in a unison script | Call it in `onInit` before generating multiple voices per key | Without this, the synth kills existing voices when a new note arrives on the same pitch, defeating the purpose of unison voice stacking. |
