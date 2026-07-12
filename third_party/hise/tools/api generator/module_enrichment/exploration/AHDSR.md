# AHDSR Envelope - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/AhdsrEnvelope.h`, `hi_core/hi_modules/modulators/mods/AhdsrEnvelope.cpp`
**Base class:** `EnvelopeModulator` (voice lifecycle, monophonic mode), `ahdsr_base` (DSP state machine)
**DSP implementation:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h`, `hi_dsp_library/dsp_nodes/EnvelopeNodes.cpp`

## Signal Path

noteOn -> startVoice() [read mod chains, compute per-voice rates] -> calculateBlock() [per-sample tick() state machine] -> internalBuffer -> applyTimeModulation() [multiply into voice buffer]

noteOff -> stopVoice() [set state to RELEASE] -> tick() continues in RELEASE until silence -> IDLE (voice killed)

The envelope operates as a per-sample state machine at the **control rate** (audio sample rate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR, typically /8 for instruments, /1 for effect plugins). The `prepareToPlay()` method calls `setBaseSampleRate(getControlRate())`, so all time-to-coefficient calculations use the downsampled rate.

In `calculateBlock()`, there are two paths:
1. **SUSTAIN shortcut:** If the voice is in the SUSTAIN state, the buffer is filled with a linear ramp from `lastSustainValue` to `currentSustainValue` (or a flat fill if they match). This avoids per-sample state machine overhead during sustain.
2. **All other states:** A per-sample while loop calls `calculateNewValue()` which delegates to `state->tick()`.

## Gap Answers

### signal-path-stage-order: What is the exact stage calculation order in renderNextBlock/calculateBlock?

The `tick()` method in `state_base` (EnvelopeNodes.cpp:293-432) implements a classic state machine:

1. **IDLE:** No output, `active = false`.
2. **ATTACK:** If `attack != 0`, computes `current_value = attackBase + current_value * attackCoef` (exponential curve shaped by `attackBase` coefficient derived from the `attackCurve` parameter). Transition to HOLD when `current_value >= attackLevel` (if `attackLevel > sustain`) or to SUSTAIN directly if `attackLevel <= sustain` (when `current_value >= sustain`). If `attack == 0`, immediately sets `current_value = attackLevel` and moves to HOLD.
3. **HOLD:** Increments `holdCounter` each sample. When `holdCounter >= holdTimeSamples`, transitions to DECAY. Otherwise holds `current_value = attackLevel`. Note: HOLD falls through to DECAY when the counter expires (no `break` after transition).
4. **DECAY:** If `decay != 0`, computes `current_value = decayBase + current_value * decayCoef`. Transitions to SUSTAIN when `current_value - sustain` is below the silence threshold (`FloatSanitizers::isSilence`). If sustain is 0.0, transitions directly to IDLE. If `decay == 0`, immediately sets value to sustain.
5. **SUSTAIN:** `current_value = sustain * modValues[SustainLevelChain]`. (In `calculateBlock`, the SUSTAIN state is intercepted before `tick()` is called, using a smooth ramp instead.)
6. **RELEASE:** If `release != 0`, computes `current_value = releaseBase + current_value * releaseCoef`. When `current_value` falls below the silence threshold, sets to 0.0 and transitions to IDLE. If `release == 0`, immediately sets to 0.0 and IDLE.
7. **RETRIGGER:** (Monophonic only) By default (`HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO = 0`), immediately sets state to ATTACK and recursively calls `tick()`. With the flag enabled, ramps toward zero at a fixed step of 0.005 per sample before re-entering ATTACK.

### curve-shape-implementation: What do AttackCurve and DecayCurve values actually produce?

**AttackCurve** (0.0-1.0) controls the `attackBase` parameter used in `calculateCoefficients()`:
- `< 0.5`: `attackBase = 1.0 / ((1.0 - value*2.0) * 100.0)` -- produces a small base (0.01 to ~0.02), yielding a concave/logarithmic curve (fast initial rise, slow approach to peak).
- `= 0.5`: `attackBase = 1.2` -- nearly linear.
- `> 0.5`: `attackBase = (value - 0.5) * 2.0 * 100.0` -- produces a large base (up to 100), yielding a convex/exponential curve (slow initial rise, fast approach to peak).

The per-sample formula is `current_value = stateBase + current_value * stateCoef` where `stateCoef = pow(attackBase, 1/t)` and `stateBase = (stateCoef / (attackBase - 1) - 1/(attackBase-1)) * attackLevel`. This is a power-curve approach where the `attackBase` value controls how the exponential converges.

**DecayCurve** (0.0-1.0) controls `targetRatioDR` via `setDecayCurve()`:
- Formula: `targetRatioDR = decayCurve * 0.0001`
- At `decayCurve = 0.0`: `targetRatioDR = 0.0` (clamped to 0.0000001 in `setTargetRatioDR`), producing a very steep exponential decay.
- At `decayCurve = 1.0`: `targetRatioDR = 0.0001`, producing a gentler exponential decay closer to linear.
- This ratio is used in `calcCoef()`: `coef = exp(-log((1+ratio)/ratio) / (rate*sr*0.001))`. A smaller ratio makes the curve steeper/more exponential.

The DecayCurve applies identically to both decay and release phases since both use the same `targetRatioDR` value through `calcCoef()`.

### attack-level-decibel-scaling: How is the dB-to-linear conversion applied?

In `setInternalAttribute()`, `AttackLevel` is stored as `attackLevel = Decibels::decibelsToGain(newValue)` and `Sustain` as `sustain = Decibels::decibelsToGain(newValue)` (via `setSustainLevel`). The `getAttribute()` methods convert back. JUCE's `Decibels::decibelsToGain()` returns 0.0 for values at or below the default threshold of -100 dB. So -100 dB is treated as true silence (0.0), and 0 dB is unity gain (1.0).

All internal DSP operates on the linear (0.0-1.0) domain. The dB display is purely for the UI.

### monophonic-retrigger-interaction: How do Monophonic and Retrigger interact?

In `startVoice()` when `isMonophonic == true`:
- Uses the shared `monophonicState` (voice index -1) instead of per-voice states.
- `restartEnvelope = shouldRetrigger || getNumPressedKeys() == 1`
  - First note (`numPressedKeys == 1`): always restarts regardless of Retrigger setting. State set to ATTACK, `current_value = 0.0`.
  - Subsequent notes with `Retrigger = On`: enters RETRIGGER state (which by default immediately transitions to ATTACK via recursive `tick()`). The envelope restarts from its current value -- it does NOT reset `current_value` to 0.0 before entering RETRIGGER. The RETRIGGER state (default path, `HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO = 0`) immediately jumps to ATTACK and calculates from the existing value.
  - Subsequent notes with `Retrigger = Off`: `restartEnvelope` is false, so the envelope continues from its current position without restarting. Modulation chain values are NOT re-evaluated.

In `handleHiseEvent()`, when monophonic and `getNumPressedKeys() == 0` (last key released), the state is set to RELEASE. In `stopVoice()` for monophonic mode, only the base `EnvelopeModulator::stopVoice()` is called (which is a no-op), so voice stopping is handled by `handleHiseEvent` based on the keymap.

In `isPlaying()`, monophonic always returns `true` -- the voice is never killed by the envelope in monophonic mode. This confirms the forum insight that monophonic mode disables voice killing.

### decay-curve-shared: Does DecayCurve apply identically to both decay and release?

Yes. `setDecayCurve()` calls `setTargetRatioDR()` which updates both `decayBase` and `releaseBase`, then calls `setDecayRate()` and `setReleaseRate()` which both use `calcCoef(time, targetRatioDR)`. There is no separate release curve parameter -- both phases use the identical `targetRatioDR` value.

### ecomode-downsampling: How does EcoMode's 16x downsampling work?

EcoMode is **vestigial**. In `setInternalAttribute()`: `case EcoMode: break; // not needed anymore...`. In `getAttribute()`: `case EcoMode: return 1.0f; // not needed anymore...`. The parameter has no effect on processing.

The actual downsampling is handled globally: `prepareToPlay()` calls `setBaseSampleRate(getControlRate())`, and `getControlRate()` returns `sampleRate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`. The factor is 8 for instruments (HISE_EVENT_RASTER=8) and 1 for effect plugins (FRONTEND_IS_PLUGIN). The `calculateBlock()` method operates at the control rate -- it fills the internal buffer at the downsampled rate, then `applyTimeModulation()` (in the base `TimeModulation` class) handles upsampling/interpolation to audio rate.

### hold-no-modchain: Is Hold's lack of modulation chain intentional?

Hold has no modulation chain. The internal chains enum defines only 5 chains (AttackTime, AttackLevel, DecayTime, SustainLevel, ReleaseTime). The `holdTimeSamples` value is computed once from the `hold` parameter in `setHoldTime()` and is not modulated per-voice. The hold counter comparison in `tick()` uses `envelope->holdTimeSamples` (the shared value), not a per-voice modulated value. This appears intentional -- Hold is a simple timing gate that doesn't benefit from per-voice variation in most use cases.

### mod-chain-gain-mode-semantics: How does gain-mode modulation interact with time and level parameters?

All five modulation chains use `ScaleOnly` mode (gain mode) with VoiceStartModulator constraint. The modulation values are read once at `startVoice()` via `getConstantVoiceValue()` and stored in `state->modValues[]`.

**Time parameters (Attack, Decay, Release):** The modulation value directly multiplies the time. In `state_base::setAttackRate()`: `attackTime = modValue * rate`. A modulator outputting 0.5 halves the time. A modulator outputting 0.0 makes the time instant (special-cased: `attackCoef = 0.0, attackBase = 1.0`, which means `current_value` jumps to `attackLevel` in one tick).

**Level parameters (AttackLevel, Sustain):** The modulation value multiplies the **linear gain** value (not the dB value). In `startVoice()`: `state->attackLevel = attackLevel * state->modValues[AttackLevelChain]`. Since `attackLevel` is already in linear domain (converted from dB in `setInternalAttribute`), the modulator operates on 0.0-1.0 linear scale. A velocity modulator outputting 0.5 effectively halves the linear gain (roughly -6 dB), NOT subtracting 50 from the dB value.

This confirms the forum insight: the attack level modulation uses a 0-100% (0.0-1.0) multiplier on the linear gain value, not a dB offset.

## Processing Chain Detail

1. **Voice Start / Modulation Capture** (per-voice, negligible)
   - Read all 5 mod chain values via `getConstantVoiceValue()`
   - Compute per-voice attack level (`attackLevel * modValue`), per-voice rates, per-voice sustain
   - Compute per-voice coefficients via `setAttackRate/setDecayRate/setReleaseRate`

2. **State Machine Tick** (per-voice, per-sample at control rate, low)
   - Called once per control-rate sample in `calculateBlock()`
   - SUSTAIN state has a fast path (linear ramp fill, no per-sample tick)
   - All other states: per-sample exponential curve computation (`base + value * coef`)

3. **Buffer Output** (per-voice, negligible)
   - Writes to `internalBuffer` in `calculateBlock()`
   - Base class `applyTimeModulation()` multiplies envelope values into the voice buffer

4. **UI Ball Update** (shared, negligible)
   - Rate-limited by `ballUpdater` (ExecutionLimiter)
   - Sends display index message for the animated ball position
   - Only runs for the active voice or in monophonic mode

## Modulation Points

| Chain | Index | Target | Application Point |
|-------|-------|--------|-------------------|
| AttackTimeModulation | 0 | Attack time (ms) | `startVoice()` -> `state->setAttackRate(attack)` multiplies time |
| AttackLevelModulation | 1 | Attack level (linear) | `startVoice()` -> `attackLevel * modValues[1]` |
| DecayTimeModulation | 2 | Decay time (ms) | `startVoice()` -> `state->setDecayRate(decay)` multiplies time |
| SustainLevelModulation | 3 | Sustain level (linear) | `startVoice()` for initial + `calculateBlock()` sustain ramp + `tick()` for transitions |
| ReleaseTimeModulation | 4 | Release time (ms) | `startVoice()` -> `state->setReleaseRate(release)` multiplies time |

All chains are VoiceStartModulator-constrained (evaluated once at note-on). The sustain modulation value is also used continuously during the SUSTAIN state in `calculateBlock()` (for smooth sustain level changes) and during DECAY transitions in `tick()`.

## Conditional Behavior

### Monophonic mode (`isMonophonic`)
- **On:** Uses shared `monophonicState` for all voices. First voice calculated saves to `firstVoiceBuffer`; subsequent voices copy from it. Voice release triggered by `handleHiseEvent` when `getNumPressedKeys() == 0`. `isPlaying()` always returns `true` (voices never killed). Retrigger behavior active.
- **Off:** Each voice has independent `AhdsrEnvelopeState`. `stopVoice()` sets state to RELEASE. `isPlaying()` returns false when IDLE, allowing voice kill.

### Retrigger (`shouldRetrigger`, only in monophonic)
- **On + not first note:** State set to RETRIGGER. Default behavior (`HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO=0`): immediately transitions to ATTACK from current value. Mod chains re-evaluated.
- **Off + not first note:** Envelope continues from current state. Mod chains NOT re-evaluated.
- **First note (any retrigger setting):** Always starts fresh (ATTACK state, `current_value = 0.0`).

### Attack level vs sustain comparison (in tick ATTACK state)
- **attackLevel > sustain:** Normal path. Attack rises to attackLevel, then HOLD, then DECAY to sustain.
- **attackLevel <= sustain:** Attack phase transitions directly to SUSTAIN when `current_value >= sustain`, skipping HOLD and DECAY entirely.

### Zero-time parameters
- **attack == 0:** Skips attack phase entirely, sets `current_value = attackLevel`, enters HOLD.
- **decay == 0:** Skips decay, sets `current_value = sustain`, enters SUSTAIN (or IDLE if sustain == 0).
- **release == 0:** Sets `current_value = 0.0`, enters IDLE immediately.

## Vestigial / Notable

- **EcoMode:** Defined in enum, serialized, exposed in metadata with description "Enables 16x downsampling for reduced CPU usage". In `setInternalAttribute`: `case EcoMode: break;`. In `getAttribute`: returns hardcoded `1.0f`. The parameter is vestigial. Downsampling is now globally controlled by `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR` (default 8). Already tracked in issues.md.

## CPU Assessment

- **Per-voice envelope calculation:** Low. One multiply + one add per control-rate sample. The control rate is typically 1/8th of audio rate.
- **SUSTAIN fast path:** Negligible. Uses `FloatVectorOperations::fill()` or a simple ramp instead of per-sample tick.
- **Coefficient computation:** Negligible. Done once per voice at `startVoice()` (involves `exp()` and `log()` calls, but only once).
- **UI ball update:** Negligible. Rate-limited by `ExecutionLimiter`.
- **Overall baseline:** Low. Slightly more CPU than SimpleEnvelope due to more stages and per-sample branching, but fundamentally just one multiply-add per control-rate sample per voice.
- **No parameters scale the cost** (EcoMode is vestigial; downsampling is global and fixed).

## UI Components

- **FloatingTile content type:** `AHDSRGraph` (registered via `Panel` inner class with `SET_PANEL_NAME("AHDSRGraph")`)
- The panel creates an `AhdsrGraph` component connected to a `SimpleRingBuffer` display buffer.
- Supports Look and Feel overrides: `drawAhdsrBackground`, `drawAhdsrPath`, `drawAhdsrBall` (via `AhdsrGraph::LookAndFeelMethods`).
- Colours: `bgColour`, `fillColour` (itemColour1), `lineColour` (itemColour2), `outlineColour` (itemColour3).
- The display buffer holds 9 samples (one per parameter for the UI path rendering).
- Backend editor: `AhdsrEnvelopeEditor` (standard processor editor, not a FloatingTile).

## Notes

- The `ahdsr_base` class is shared between the module-tree `AhdsrEnvelope` and the scriptnode `envelope.ahdsr` node. The DSP logic (state machine, coefficient calculation) is identical; only the wrapper differs.
- The RETRIGGER state's default behavior (immediate jump to ATTACK) can be changed to a ramp-to-zero approach via the compile-time flag `HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO`. Default is 0 (immediate jump).
- The `calculateBlock()` SUSTAIN fast path recalculates sustain each block as `sustain * modValues[SustainLevelChain]`, allowing real-time sustain changes even though the mod chain is VoiceStartModulator-constrained. The parameter value can be changed via `setAttribute` during sustain and the envelope will smoothly ramp to the new value.
- When `attackLevel <= sustain`, the HOLD and DECAY phases are skipped entirely -- the envelope goes directly from ATTACK to SUSTAIN. This can be surprising if users set AttackLevel below Sustain and expect to hear a hold phase.
- In monophonic mode, `isPlaying()` always returns `true`, which means the envelope cannot kill voices. This confirms the forum insight about monophonic mode disabling voice killing. A workaround is to add a second polyphonic envelope for voice management.
- The DecayCurve parameter description ("Controls the curvature of the decay and release phases") is accurate -- it truly affects both identically. Users wanting independent release curve control should use FlexAHDSR.
