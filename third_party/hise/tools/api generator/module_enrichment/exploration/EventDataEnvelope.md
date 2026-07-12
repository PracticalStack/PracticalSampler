# EventData Envelope - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/EventDataModulator.h`, `hi_core/hi_modules/modulators/mods/EventDataModulator.cpp`
**Base class:** `EnvelopeModulator` (which extends `Modulator`, `VoiceModulation`, `TimeModulation`)

## Signal Path

noteOn -> startVoice() reads event data slot once -> calculateBlock() re-reads the slot every audio block -> linear ramp smoothing -> internalBuffer -> applyTimeModulation() multiplies/adds to voice buffer

The module stores the original HiseEvent per voice. Each calculateBlock() call uses the stored event's ID to look up the current value from AdditionalEventStorage. If the target value has changed, a linear ramp is initiated. The ramp runs at control rate (sample rate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR, typically /8). When the ramp is not active, the buffer is filled with a constant value. When active, values are advanced sample-by-sample within the control-rate block.

## Gap Answers

### continuous-vs-voicestart-reading: How does continuous event data reading work in the EnvelopeModulator version vs. the VoiceStartModulator (EventDataModulator)?

EventDataModulator (VoiceStartModulator) calls `additionalEventStorage->getValue()` once in `calculateVoiceStartValue()` at note-on and returns a fixed value for the voice's lifetime.

EventDataEnvelope calls `additionalEventStorage->getValue()` in `calculateBlock()` which is invoked every audio rendering block (at control rate). This means it polls the slot continuously. If the value stored in the slot has been updated (e.g., by a script calling `GlobalRoutingManager.setEventData()`), the new value is picked up on the next block. The comparison `state->rampValue.targetValue != v` triggers a new ramp toward the updated target. This is a polling mechanism, not an event-driven callback.

### smoothing-algorithm: What smoothing algorithm is used?

The smoothing uses `sfloat` which is an alias for `pimpl::_ramp<float>` -- a **linear ramp** (not exponential/one-pole). The `prepare()` method calculates `numSteps = round(timeMs / msPerSample)` at the control rate (sample rate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR). The `set()` method computes a fixed `delta = (target - current) / numSteps` and counts down. The `advance()` method adds delta each sample.

When SmoothingTime is 0ms, `numSteps` is 0 and `set()` calls `reset()` which snaps directly to the target value. So yes, **default 0ms means no smoothing (instant jumps)**.

This is different from PitchWheel/MidiController which use a different smoother. EventDataEnvelope uses a simple linear ramp.

### event-data-write-mechanism: How are event data slots written for continuous modulation?

The same `AdditionalEventStorage` instance is used, obtained via `GlobalRoutingManager::Helpers::getOrCreate(mc)`. In the constructor, the pointer `additionalEventStorage` is cached. Writes happen externally via `AdditionalEventStorage::setValue(eventId, slotIndex, newValue, notification)`. Since calculateBlock() polls the storage every block, mid-note updates are picked up automatically. The storage is a shared flat array indexed by `eventId & 1023` (1024 event slots) and `slotIndex & 15` (16 data slots). This means event IDs that collide modulo 1024 will overwrite each other.

### slot-index-range-discrepancy: Description says '0-15' but range max is 16.0. Is slot 16 valid?

`NumDataSlots = 16` and `jlimit<uint8>(0, AdditionalEventStorage::NumDataSlots, ...)` clamps to 0..16 inclusive. However, `getValue()` masks the index with `slotIndex & (NumDataSlots - 1)` = `slotIndex & 15`, so slot index 16 wraps to slot 0. The metadata range `(0.0, 16.0, 1.0)` with discrete stepping allows values 0-16, but 16 is effectively an alias for 0. The description "0-15" is functionally correct since 16 wraps. The slider max of 16.0 is slightly misleading but harmless since discrete stepping rounds to integers and the masking handles it.

### envelope-lifecycle: How does the note-on/note-off lifecycle work?

- **startVoice():** Stores the HiseEvent in per-voice state, reads the slot once, returns the initial value. Does NOT initialize the ramp -- the ramp state carries over from the previous voice use (no explicit reset of `rampValue`).
- **stopVoice():** Delegates to `EnvelopeModulator::stopVoice()`. No special cleanup.
- **isPlaying():** Always returns `true` ("never kill" per the comment). This means the envelope never signals voice termination. The voice must be killed by another envelope in the chain (like an AHDSR). This is critical: if EventDataEnvelope is the only envelope, voices will never release.
- **reset():** Delegates to `EnvelopeModulator::reset()`. Does not reset the ramp.
- **calculateBlock():** Continues to poll the event data slot regardless of whether the voice is in attack/sustain/release phase. There is no concept of release freezing -- the value continues to update as long as the voice is alive.

### monophonic-retrigger-behavior: In monophonic mode with Retrigger enabled, what happens?

The base class `EnvelopeModulator::render()` handles monophonic routing: only the first rendered voice actually calls `calculateBlock()`, all subsequent voices copy from `firstVoiceBuffer`. The `startVoice()` is still called per-voice and stores the new event's HiseEvent. In monophonic mode, the `monophonicState` is used (voice index -1). When retrigger fires, `startVoice()` updates `state->e` with the new event, so subsequent `calculateBlock()` calls will read from the new event's data slot. The ramp state is NOT reset on retrigger, so if a ramp was in progress it continues from its current position toward whatever value the new event's slot holds.

## Processing Chain Detail

1. **Voice start (startVoice)** - Per-voice. Stores HiseEvent, reads initial slot value. CPU: negligible.
2. **Block calculation (calculateBlock)** - Per-voice (or monophonic shared). Polls AdditionalEventStorage every block. Compares to current ramp target. If changed, sets new ramp. If ramp active: per-sample linear interpolation. If ramp inactive: constant fill. CPU: negligible to low (constant fill is SIMD-optimized via FloatVectorOperations::fill; ramp is a simple per-sample loop).
3. **Time modulation application (applyTimeModulation)** - Handled by base class. Multiplies or adds the internalBuffer values to the voice buffer depending on Modulation::Mode. CPU: negligible.

## Modulation Points

No modulation chains. `getNumChildProcessors()` returns 0 and `getNumInternalChains()` returns 0. The module itself IS the modulation source -- it produces values that modulate whatever it is connected to.

## Conditional Behavior

- **Monophonic mode (inherited from EnvelopeModulator):** When enabled, only the first rendered voice runs calculateBlock(). All other voices copy the result. The monophonicState is used instead of per-voice states.
- **Retrigger (inherited from EnvelopeModulator):** When enabled in monophonic mode, startVoice() is called for new notes, updating the stored event.
- **SmoothingTime = 0:** Causes instant value changes (no ramp). `numSteps` is 0, so `set()` calls `reset()` which snaps to target.
- **Slot not written (getValue returns false):** Falls back to `defaultValue`.

## Vestigial / Notable

- `isPlaying()` always returns `true`. This module can never kill a voice. If used as the sole envelope modulator, voices accumulate indefinitely. This is by design (it's a data reader, not a traditional attack-release envelope) but could surprise users.
- The `state` member is a raw pointer that gets reassigned every `calculateBlock()` call. It points into the `states` array or `monophonicState`. This is a code pattern, not a bug, but means the member is not a persistent reference.
- The ramp state is not reset in `startVoice()` or `reset()`. A voice reused after another note may start with a non-zero ramp delta if the previous ramp was interrupted. In practice this is harmless since calculateBlock() immediately sets the correct target.
- The editor shares the same `EventDataEditor` class with EventDataModulator, using a dynamic_cast to determine whether to show the SmoothingTime slider.

## CPU Assessment

- **Baseline tier:** negligible
- **Per-block poll of AdditionalEventStorage:** One array lookup per block -- negligible.
- **Ramp active path:** Per-sample loop with one addition per sample -- low. Only active during transitions.
- **Ramp inactive path:** Single FloatVectorOperations::fill -- negligible (SIMD optimized).
- **No parameters scale cost.** SmoothingTime only affects duration of the ramp-active state, not per-sample cost.

## UI Components

Uses the shared `EventDataEditor` class (not a FloatingTile). Contains three HiSlider knobs: SlotIndex, DefaultValue, and SmoothingTime (envelope-only). No FloatingTile content types discovered.

## Notes

- EventDataEnvelope and EventDataModulator share the same source files and editor class. They share the same slot mechanism via AdditionalEventStorage but differ fundamentally: EventDataModulator snapshots once at voice start; EventDataEnvelope polls every block.
- The control-rate downsampling factor (typically 8) means the ramp operates at sampleRate/8. For 44100 Hz, the ramp rate is ~5512 Hz. SmoothingTime of 100ms would be ~551 steps.
- The `isPlaying() -> true` pattern means this module must always be paired with another envelope that handles voice lifecycle (like AHDSR). This is the same pattern as other "utility" envelopes.
- Natural seeAlso: EventDataModulator (voice-start counterpart), GlobalRoutingManager (write-side API for setting event data slots).
