# Message -- Project Context

## Project Context

### Real-World Use Cases
- **Sampler instrument with articulation switching**: A multi-articulation sampler (harp, woodwind, strings) uses `getNoteNumber()` and `ignoreEvent()` in `onNoteOn` to detect keyswitch notes, suppress them from playback, and switch articulation by toggling MidiMuter bypass states. Controller callbacks handle sustain pedal state and expression mapping.
- **Drum machine with humanization**: A drum sequencer uses `delayEvent()` to add per-note timing slop (randomized sample delays) to sequenced hits, combined with `isArtificial()` to only humanize sequencer-generated notes while leaving live input unmodified. `makeArtificial()` and `ignoreEvent()` manage note routing across multiple channel strips.
- **Piano sampler with timbre control**: A piano plugin uses `setTransposeAmount()` paired with `setCoarseDetune()` to shift which velocity layer's samples are triggered without changing the audible pitch -- achieving brightness/darkness control. `store()` with `MessageHolder` and `UnorderedStack` tracks held notes for release sample logic.
- **Monophonic legato synthesizer**: A synth with glide uses `ignoreEvent()` to suppress the incoming note, then `Synth.addNoteOn()` to create a new voice with `Synth.addPitchFade()` for portamento. `makeArtificial()` manages event IDs for per-voice operations.

### Complexity Tiers
1. **Basic MIDI routing** (most common): `getNoteNumber()`, `getVelocity()`, `getControllerNumber()`, `getControllerValue()`, `ignoreEvent()`. Sufficient for keyswitch detection, CC-driven parameter changes, note range filtering, and basic velocity modification.
2. **Event manipulation**: Adds `setTransposeAmount()` + `setCoarseDetune()` for timbre shifting, `setVelocity()` for velocity curves, `delayEvent()` for humanization, `setStartOffset()` for sample phase control. Required for instruments with musically meaningful MIDI processing.
3. **Artificial event system**: Adds `makeArtificial()`, `getEventId()`, `isArtificial()`, and integration with `Synth.addNoteOn()`/`Synth.addVolumeFade()`/`Synth.addPitchFade()`. Required for monophonic scripts, arpeggiators, note-splitting, and any logic that creates or replaces MIDI events.
4. **Persistent event storage**: Adds `store()` with `MessageHolder` for tracking held notes across callbacks, release triggering, and advanced sustain pedal logic.

### Practical Defaults
- Use `getControllerNumber()` with the constants `Message.PITCH_BEND_CC` (128) and `Message.AFTERTOUC_CC` (129) to handle all controller-type events in a single `onController` callback. This is cleaner than separate aftertouch/pitch handling.
- Pair `setTransposeAmount(-N)` with `setCoarseDetune(N)` for timbre shifting: the transpose selects a different sample, while coarse detune cancels the pitch change. Both values must have opposite signs.
- Use `isArtificial()` to guard `delayEvent()` so humanization only applies to sequencer-generated events, not live MIDI input.
- Use `ignoreEvent(true)` in `onNoteOn` for keyswitch notes rather than filtering in `onController` -- keyswitches are note events.
- Use `reg` variables (not `var`) for state that persists across callbacks in the same script processor, such as `lastNote`, `lastEventId`, and `pedalDown`.

### Integration Patterns
- `Message.store(holder)` -> `Synth.addMessageFromHolder(holder)` -- Store an incoming note for deferred replay (release triggering, sustain pedal logic). The holder preserves the full event including velocity, channel, and transpose.
- `Message.ignoreEvent(true)` -> `Synth.addNoteOn()` -- The ignore-then-resynthesize pattern for monophonic scripts: suppress the original event, create a new artificial one with controlled timing and pitch fade.
- `Message.makeArtificial()` -> `Message.getEventId()` -> `Synth.addVolumeFade(id, ...)` -- Convert to artificial to get a stable event ID, then use that ID for per-voice volume/pitch fades.
- `Message.getControllerNumber()` == 64 -> `Message.getControllerValue()` > 64 -- Sustain pedal detection pattern, used as the entry point for held-note release logic.
- `Message.delayEvent(samples)` + `Message.setTransposeAmount(delta)` -- Delay by 1 sample after sending a pitch bend controller, ensuring the pitch change arrives before the note. Used when remapping incoming notes to different pitches via controller+transpose.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Message.setTransposeAmount(-5); Message.setCoarseDetune(0);` | `Message.setTransposeAmount(-5); Message.setCoarseDetune(5);` | Coarse detune must cancel the transpose to keep the audible pitch unchanged. Without it, the note sounds 5 semitones lower. |
| Applying `delayEvent()` to all notes unconditionally | Guarding with `if (Message.isArtificial())` | Humanization delays should only apply to sequencer-generated notes. Delaying live MIDI input adds unwanted latency to the player's performance. |
| Using `var` for `lastEventId` across callbacks | Using `reg lastEventId` | `var` at file scope works but `reg` is the idiomatic choice for mutable state accessed from audio-thread callbacks. |
| Normalizing `getControllerValue()` by dividing by 127 for all events | Checking `getControllerNumber()` first to handle pitch bend's 0-16383 range | Pitch bend uses 14-bit values (0-16383), not 7-bit (0-127). Dividing by 127 produces values up to ~129 for pitch wheel. |
