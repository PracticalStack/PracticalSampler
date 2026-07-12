# Simple Envelope - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/SimpleEnvelope.h`, `hi_core/hi_modules/modulators/mods/SimpleEnvelope.cpp`
**Base class:** `EnvelopeModulator` (which extends `Modulator`, `VoiceModulation`, `TimeModulation`)

## Signal Path

SimpleEnvelope is a minimal two-stage (attack + release) envelope modulator that runs at the control rate. Per-voice state is stored in `SimpleEnvelopeState` structs. The envelope transitions through five states: IDLE -> ATTACK -> SUSTAIN -> RELEASE -> IDLE (plus RETRIGGER for monophonic mode).

On `startVoice()`:
1. If the attack chain has voice modulators, evaluate them to get `attackModValue`
2. Compute `thisAttackTime = attack * attackModValue`
3. In linear mode: compute `attackDelta = 1.0 / (thisAttackTime_ms / 1000 * controlRate)`
4. In exponential mode: compute per-voice exponential coefficients (`expAttackCoef`, `expAttackBase`)
5. Set state to ATTACK (or RETRIGGER in monophonic retrigger mode)
6. Return 0.0 if attack > 0, else 1.0 (instant attack)

On `stopVoice()`:
1. Set state to RELEASE (in monophonic mode, only if all keys released)

In `calculateBlock()`:
- **SUSTAIN**: Fill buffer with 1.0 (fast path, no per-sample loop)
- **IDLE**: Fill buffer with 0.0 (fast path)
- **ATTACK/RELEASE/RETRIGGER**: Per-sample calculation via `calculateNewValue()` (linear) or `calculateNewExpValue()` (exponential)

Linear per-sample (`calculateNewValue()`):
- ATTACK: `current_value += attackDelta` until >= 1.0, then transition to SUSTAIN
- RELEASE: `current_value -= release_delta` until <= 0.0, then transition to IDLE
- RETRIGGER: Immediately jump to ATTACK state and continue from current value (default build; with `HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO`, ramps down at -0.005/sample first)

Exponential per-sample (`calculateNewExpValue()`):
- ATTACK: `current_value = expAttackBase + current_value * expAttackCoef` until >= 1.0
- RELEASE: `current_value = expReleaseBase + current_value * expReleaseCoef` until <= 0.0001
- RETRIGGER: Same as linear retrigger (immediate jump to ATTACK)

## Gap Answers

### signal-path-stage-order: What is the exact calculation in calculateBlock? With only Attack and Release stages, how does the envelope transition?

The envelope has an implicit sustain at 1.0 while the note is held. The state machine is: ATTACK -> SUSTAIN -> RELEASE -> IDLE.

During ATTACK, the value ramps from 0.0 to 1.0. When it reaches 1.0, the state transitions to SUSTAIN, where `calculateBlock()` fills the buffer with 1.0f using `FloatVectorOperations::fill` (no per-sample loop). The envelope stays in SUSTAIN indefinitely until `stopVoice()` is called (note-off), which sets the state to RELEASE. During RELEASE, the value ramps from 1.0 to 0.0, then transitions to IDLE.

There is no Decay or explicit Sustain parameter -- this is a pure AR envelope with unity sustain.

### linear-mode-curve-shape: What exactly does LinearMode toggle?

**Linear mode (LinearMode=1, default):**
- Attack: `current_value += attackDelta` per control-rate sample, where `attackDelta = 1.0 / (time_ms / 1000.0 * controlRate)`. This is a straight-line ramp.
- Release: `current_value -= release_delta` per sample, where `release_delta` is computed the same way. Straight-line ramp down.

**Exponential mode (LinearMode=0):**
- Attack uses a one-pole filter formula: `current_value = expAttackBase + current_value * expAttackCoef`
  - `expAttackCoef = exp(-log((1 + targetRatio) / targetRatio) / (time_ms * controlRate * 0.001))`
  - `expAttackBase = (1.0 + targetRatio) * (1.0 - expAttackCoef)`
  - `targetRatioA = 0.3` (controls the curvature -- larger values = more linear, smaller = more exponential)
- Release uses the same formula structure with `targetRatioR = 0.0001` (very small, giving a steep initial drop then long tail -- classic exponential release decay).
  - Release threshold is 0.0001 (vs 0.0 for linear), so exponential release cuts off at -80 dB.

The exponential formula is a discrete-time approximation of an RC circuit charging/discharging curve. The `targetRatio` parameter controls how close to the asymptote the curve reaches in the given time.

### monophonic-retrigger-interaction: How do Monophonic and Retrigger interact?

In monophonic mode (`isMonophonic=true`), all voices share the `monophonicState` struct. On `startVoice()`:
- If `shouldRetrigger=true` OR this is the first key (`getNumPressedKeys() == 1`), the envelope restarts.
  - With `shouldRetrigger=true`: state is set to RETRIGGER, which (in the default build) immediately transitions to ATTACK and continues from the current value. The envelope does NOT reset to zero -- it attacks from whatever value it currently has.
  - With first key (no retrigger needed): state is set directly to ATTACK.
- If `shouldRetrigger=false` AND keys are already held, `startVoice()` returns `state->current_value` unchanged -- the envelope continues wherever it was (legato behavior).

On `stopVoice()` in monophonic mode: the envelope only transitions to RELEASE when `getNumPressedKeys() == 0` (all keys released). Individual key releases while other keys are still held are ignored.

The `reset()` method is a no-op in monophonic mode (returns immediately), preventing voice stealing from resetting the shared envelope state.

### release-no-modchain: Release has no modulation chain - is this intentional?

Yes, this is by design. The header has a commented-out `//ReleaseChain` in the `InternalChains` enum, confirming it was considered but deliberately excluded. Only `AttackChain` (index 0) exists, constrained to VoiceStartModulators.

The release time is set globally via `setReleaseRate()` which stores coefficients in shared member variables (`release_delta`, `expReleaseCoef`, `expReleaseBase`). These are NOT per-voice state. In contrast, attack coefficients are stored both globally (for default) and per-voice (in `SimpleEnvelopeState::attackDelta`, `expAttackCoef`, `expAttackBase`) to support per-voice modulation.

This means: (1) Release time cannot be modulated per-voice, and (2) changing the Release parameter affects all currently releasing voices immediately (they share the same `release_delta`/`expReleaseCoef`).

### performance-vs-ahdsr: How lightweight is SimpleEnvelope compared to AHDSR?

SimpleEnvelope is per-sample at the control rate (same as AHDSR). Key efficiency advantages:

1. **Fast-path for SUSTAIN and IDLE**: When in SUSTAIN, `calculateBlock()` uses `FloatVectorOperations::fill(1.0f)` (SIMD-optimized) instead of a per-sample loop. Same for IDLE with 0.0f. Since most of a note's lifetime is in SUSTAIN, this is a significant optimization.
2. **Simpler per-sample math**: Linear mode is one addition and comparison. Exponential mode is one multiply-add and comparison. AHDSR has more complex state machine logic with more branches per sample.
3. **Fewer modulation chains**: Only one child chain (Attack) vs AHDSR's five. Fewer chains = less overhead in voice start and block processing.
4. **No curve table lookups**: AHDSR with AhdsrCurves uses table lookups per sample; SimpleEnvelope's exponential mode is purely arithmetic.

The simplicity translates to measurable CPU savings, particularly for high voice counts where the SUSTAIN fast-path avoids per-sample work entirely.

## Processing Chain Detail

1. **Voice Start** (per-voice, negligible CPU)
   - Evaluate AttackChain if present -> `attackModValue`
   - Compute per-voice attack time: `attack * attackModValue`
   - Compute per-voice attack coefficients (linear delta or exponential coef/base)
   - Set initial state to ATTACK

2. **Per-block calculation** (per-voice, low CPU)
   - SUSTAIN path: `FloatVectorOperations::fill(1.0f)` -- negligible
   - IDLE path: `FloatVectorOperations::fill(0.0f)` -- negligible
   - ATTACK path: per-sample ramp (linear: 1 add + 1 compare; exponential: 1 multiply-add + 1 compare) -- low
   - RELEASE path: per-sample ramp (same as attack but decrementing) -- low
   - RETRIGGER path: immediate state change then attack processing -- negligible overhead

3. **Note Off** (per-voice, negligible CPU)
   - Set state to RELEASE

## Modulation Points

- **AttackChain** (index 0): Evaluated at voice start via `attackChain->startVoice()` and `attackChain->getConstantVoiceValue()`. Multiplies the Attack parameter to produce the per-voice attack time. Constrained to VoiceStartModulators only (evaluated once at note-on, not continuously). The chain is also forwarded MIDI events via `handleHiseEvent()`.

## Conditional Behavior

### LinearMode (toggle)
- **LinearMode=1 (default)**: Attack and release use linear ramps (`current_value += delta` / `current_value -= delta`). `calcCoefficient()` returns `1.0 / (time_ms / 1000 * controlRate)`.
- **LinearMode=0**: Attack and release use exponential curves (one-pole filter formula). Attack uses `targetRatio=0.3` (moderate curvature). Release uses `targetRatio=0.0001` (steep initial decay, long tail). Release threshold changes from 0.0 (linear) to 0.0001 (exponential).

When LinearMode is toggled, `setInternalAttribute` recalculates both attack and release coefficients via `setAttackRate()` and `setReleaseRate()`.

### Monophonic mode
- **Monophonic=0 (default for multi-voice)**: Each voice has independent state. `isPlaying()` checks per-voice state. `reset()` clears per-voice state.
- **Monophonic=1**: All voices share `monophonicState`. `isPlaying()` always returns true (monophonic envelopes cannot kill voices). `reset()` is a no-op. `stopVoice()` only transitions to RELEASE when all keys are released.

### Retrigger (monophonic only)
- **Retrigger=1 (default)**: On new note, envelope enters RETRIGGER state which immediately jumps to ATTACK from current value (no ramp to zero in default build).
- **Retrigger=0**: On new note while keys are held, envelope continues at current value (legato). Only restarts for the very first key press.

## CPU Assessment

- **SUSTAIN/IDLE fast path**: negligible (SIMD fill)
- **ATTACK/RELEASE per-sample**: low (1-2 arithmetic ops per control-rate sample)
- **Voice start**: negligible (chain evaluation + coefficient computation)
- **Overall baseline**: **low** -- significantly lighter than AHDSR due to the SUSTAIN fast-path optimization and simpler state machine

No parameters scale the cost. The control rate is determined by `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR` (8x for instruments, 1x for effects).

## UI Components

The editor is `SimpleEnvelopeEditorBody` (defined in `SimpleEnvelopeEditor.h`). It contains:
- `HiSlider` for Attack
- `HiSlider` for Release
- `HiToggleButton` for LinearMode (labeled "useLinearMode")

No FloatingTile content types are registered. The editor uses a timer to display the AttackChain modulation output on the attack slider.

## Notes

- The release coefficients (`release_delta`, `expReleaseCoef`, `expReleaseBase`) are shared across all voices as class members, not per-voice state. This means changing the Release parameter during playback instantly affects all currently releasing voices. Attack coefficients are duplicated in per-voice state to support the attack modulation chain.
- The `ReleaseChain` is explicitly commented out in the InternalChains enum, confirming the intentional asymmetry between attack (modulatable) and release (not modulatable).
- There is no loop stage. The envelope states are strictly IDLE -> ATTACK -> SUSTAIN -> RELEASE -> IDLE with no cycling.
- The forum insight about release not triggering with `Synth.playNote()` is an event-handling issue (missing note-off via `Synth.noteOffByEventId()`), not a module-level bug. The `stopVoice()` method correctly transitions to RELEASE when called.
- The exponential release threshold (0.0001) vs linear release threshold (0.0) means exponential mode produces a slightly shorter effective release (cuts off at ~-80 dB) while linear mode ramps all the way to true zero.
