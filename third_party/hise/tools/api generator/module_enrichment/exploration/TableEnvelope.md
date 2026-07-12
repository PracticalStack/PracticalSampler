# Table Envelope - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/TableEnvelope.h`, `hi_core/hi_modules/modulators/mods/TableEnvelope.cpp`
**Base class:** `EnvelopeModulator` (in `hi_core/hi_dsp/modules/Modulators.h`)

## Signal Path

noteOn -> startVoice() initialises per-voice state, evaluates attack/release mod chains -> ATTACK phase: uptime increments by `attackUptimeDelta * attackModValue` per sample, table lookup via `attackTable->getInterpolatedValue(uptime / 512.0)` -> when uptime >= 512, transitions to SUSTAIN (holds last attack table value) -> noteOff triggers stopVoice(), captures `releaseGain = current_value`, resets uptime to 0, enters RELEASE phase -> RELEASE phase: uptime increments by `releaseUptimeDelta * releaseModValue` per sample, output = `releaseGain * releaseTable->getInterpolatedValue(uptime / 512.0)` -> when uptime >= 512, value = 0.0, state = IDLE.

Processing is **per-sample** within `calculateBlock()` -- a while loop calls `calculateNewValue()` for each sample and writes to `internalBuffer`.

## Gap Answers

### signal-path-table-lookup: How does the table lookup integrate with attack/release timing?

There are **two separate tables**: an attack table and a release table, each a `SampleLookupTable` with 512 entries.

**Attack phase:** An internal `uptime` counter starts at 0 and increments by `attackUptimeDelta * attackModValue` each sample. `attackUptimeDelta` is computed as `512.0 / (attack_ms * controlRate / 1000.0)`. The table is looked up at position `uptime / 512.0` (normalised 0-1). When `uptime >= 512`, the attack is complete.

**Release phase:** On noteOff, `releaseGain` captures the current envelope value and `uptime` resets to 0. Each sample, uptime increments by `releaseUptimeDelta * releaseModValue`. Output is `releaseGain * releaseTable->getInterpolatedValue(uptime / 512.0)`. The release table output is **multiplied** by the captured release gain, not used as an absolute value.

The attack sweeps left-to-right through the attack table. The release sweeps left-to-right through the release table (not reversed). The default release table is a line from (0, 1.0) to (1, 0.0), so it acts as a fade-out multiplier.

### table-processor-usage: How many tables does the TableProcessor provide?

Two tables via `LookupTableProcessor(mc, 2)`. Table 0 = attack shape, Table 1 = release shape. Both are `SampleLookupTable` with 512 float entries. X-axis = normalised time position (0-1 mapped to 0 ms through Attack/Release ms). Y-axis = amplitude (0-1). The X-axis display converter shows the time in ms for the current Attack/Release setting. The default attack table is whatever the base `SampleLookupTable` default is (typically a line from 0 to 1). The default release table is explicitly set in the constructor: a line from (0, 1.0) to (1, 0.0) with 0.5 curve.

### sustain-behavior-implicit: What happens between attack completion and note-off?

When attack completes (`uptime >= 512`), the code reads the last value of the attack table via `attackTable->getInterpolatedValue(1.0)` and stores it as `current_value`. State transitions to SUSTAIN. The SUSTAIN case in `calculateNewValue()` is simply `break;` -- it holds `current_value` unchanged until noteOff. So the sustain level equals the rightmost value of the attack table.

**Important detail:** If the attack table's last value (`getLastValue()`) is <= 0.01 in polyphonic mode, the envelope immediately calls `stopVoice()` instead of entering SUSTAIN, transitioning directly to RELEASE. This allows using the TableEnvelope as a one-shot shape (attack-only, no sustain).

### monophonic-retrigger-interaction: How do Monophonic and Retrigger interact?

The interaction follows the base `EnvelopeModulator` pattern with one table-specific addition: the **RETRIGGER state**.

- **Monophonic OFF:** Standard polyphonic behavior. Each voice has independent state.
- **Monophonic ON, Retrigger ON:** On subsequent keys (`!isFirstKey`), the state enters `RETRIGGER` instead of `ATTACK`. The RETRIGGER state glides `current_value` toward the attack table's starting value (`getInterpolatedValue(0.0)`) at a fixed rate of 0.005 per sample. Once the target is reached, it transitions to ATTACK. This provides a smooth transition rather than a hard restart.
- **Monophonic ON, Retrigger OFF:** `restartEnvelope` is false for subsequent keys; the envelope continues from its current position without resetting.
- **Monophonic ON, first key:** Always restarts from ATTACK (or SUSTAIN if attack time effectively zero).

**Bug in monophonic path:** `monoState->releaseModValue` is incorrectly set from `attackChain->getConstantVoiceValue()` instead of `releaseChain->getConstantVoiceValue()`. The polyphonic path correctly uses `releaseChain`. This means monophonic release time modulation uses the attack chain's value.

### table-shape-vs-timing: Does changing Attack time stretch the table lookup linearly?

Yes, the stretching is purely linear. `calculateTableDelta()` computes `512.0 / (ms * controlRate / 1000.0)`. The uptime delta is constant throughout the phase. So the same table shape is traversed at the same constant rate regardless of timing -- changing from 100ms to 1000ms simply means the same 512 entries are read over a longer period. No non-linear interpolation or resampling occurs. The table's internal `getInterpolatedValue()` uses linear interpolation between adjacent entries.

### performance-table-resolution: What is the table resolution and performance?

- **Table size:** 512 entries (`SAMPLE_LOOKUP_TABLE_SIZE`).
- **Lookup frequency:** Per-sample. Every sample calls `calculateNewValue()` which does one `getInterpolatedValue()` call (linear interpolation between two adjacent float entries).
- **Interpolation:** Linear interpolation between the two nearest table entries. The index is `uptime / 512.0 * 512.0 * coefficient` where coefficient is typically 1.0 for SampleLookupTable.
- **Cost per sample:** One floating-point multiply (uptime delta), one comparison, one table lookup with linear interpolation (2 float reads + lerp). This is comparable to or slightly cheaper than AHDSR which computes exponential coefficients.
- **No downsampling.** Every sample is computed individually, unlike AHDSR which may benefit from HISE's global control rate downsampling factor. The `calculateBlock()` loop is sample-by-sample.

## Processing Chain Detail

1. **Voice start evaluation** (per-voice, negligible): Evaluate attack and release mod chains to get per-voice time multipliers. Compute initial state (ATTACK or SUSTAIN if attack~0).
2. **Attack phase table sweep** (per-voice, per-sample, low): Increment uptime, lookup attack table value via linear interpolation. Runs for `attack_ms * controlRate / 1000` samples.
3. **Sustain hold** (per-voice, negligible): No computation; `current_value` is unchanged.
4. **Retrigger glide** (per-voice, per-sample, negligible): Only in monophonic mode. Linear ramp at fixed 0.005/sample toward attack table start value.
5. **Release phase table sweep** (per-voice, per-sample, low): Increment uptime, lookup release table value, multiply by `releaseGain`. Runs for `release_ms * controlRate / 1000` samples.

## Modulation Points

- **AttackTimeModulation** (chain index 0): VoiceStartModulator constrainer. Evaluated once at note-on. Result stored as `attackModValue = 1.0 / chainValue`. This scales `attackUptimeDelta` -- a higher mod value means faster attack (shorter time). Applied multiplicatively each sample during ATTACK phase.
- **ReleaseTimeModulation** (chain index 1): VoiceStartModulator constrainer. Evaluated once at note-on. Result stored as `releaseModValue = 1.0 / chainValue`. Scales `releaseUptimeDelta` similarly. Applied multiplicatively each sample during RELEASE phase.

Note: The modulation inversion (`1.0 / value`) means the chain output multiplies the *time* (higher chain value = longer time is inverted to higher delta = faster sweep). A chain value of 0.5 doubles the delta, halving the time. A chain value approaching 0 causes `attackModValue > 998.0` which triggers the "skip attack" fast path.

## Conditional Behavior

- **Attack time effectively zero:** If `attack == 0.0f` or `attackModValue > 998.0`, the attack phase is skipped entirely -- `current_value` is set to 1.0 and state goes directly to SUSTAIN. The 998.0 threshold corresponds to a mod chain value near 0.001.
- **Attack table ends near zero:** If `attackTable->getLastValue() <= 0.01f` at attack completion in polyphonic mode, the voice enters RELEASE immediately instead of SUSTAIN. This is a one-shot envelope behavior. This check is NOT performed in monophonic mode.
- **Monophonic + first key vs subsequent:** First key always starts ATTACK. Subsequent keys enter RETRIGGER (if retrigger enabled) which glides to the attack table's start value before starting ATTACK.

## Interface Usage

**LookupTableProcessor** (TableProcessor): Provides 2 `SampleLookupTable` instances. Table 0 (attack) is swept during ATTACK phase with `getInterpolatedValue(normalised_position)`. Table 1 (release) is swept during RELEASE phase, with its output multiplied by `releaseGain`. Both tables have X-axis text converters that display time in ms based on the current Attack/Release parameter value. The `referenceShared()` method allows tables to be shared across modules via the external data system.

## Vestigial / Notable

- The monophonic path has a bug where `releaseModValue` is set from `attackChain` instead of `releaseChain` (line 158 of TableEnvelope.cpp). This means monophonic release time modulation is driven by the attack modulation chain value.
- The retrigger glide rate is hardcoded at 0.005 per sample (not configurable). At 44100 Hz, a full 0-to-1 glide takes ~200 samples (~4.5ms). This is independent of any parameter.
- The `isPlaying()` method returns `true` unconditionally in monophonic mode, meaning the monophonic voice is never killed by the voice management system.

## CPU Assessment

- **Baseline:** Low. Per-sample table lookup with linear interpolation is lightweight.
- **Scaling:** Cost scales linearly with active voices (each voice runs its own per-sample loop).
- **No expensive operations:** No transcendental functions, no FFT, no oversampling.
- **Comparison to AHDSR:** Similar cost. AHDSR uses exponential coefficient multiplication per sample; TableEnvelope uses table lookup with linear interpolation per sample. Both are per-sample, per-voice.
- **Overall tier:** Low.

## UI Components

The editor class is `TableEnvelopeEditorBody` (defined in `TableEnvelopeEditor.h`). It contains:
- `HiSlider` for Attack time
- `HiSlider` for Release time
- `TableEditor` for attack table
- `TableEditor` for release table

No FloatingTile content types were found. The editor is a custom `ProcessorEditorBody` subclass, not a FloatingTile panel.

## Notes

- The `getControlRate()` method (from Modulator base class) is used to compute the table delta. This is the downsampled control rate, not the audio sample rate. However, `calculateBlock()` processes every sample in a tight loop, so the table is actually evaluated at the full control rate (which may be downsampled from the audio rate by `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`).
- The release table default (0,1)->(1,0) with 0.5 curve provides a natural fade-out. The attack table default is the SampleLookupTable default (not explicitly set in the constructor).
- UI display index messages are rate-limited via `ExecutionLimiter<DummyCriticalSection> uiUpdater` to avoid excessive GUI updates.
