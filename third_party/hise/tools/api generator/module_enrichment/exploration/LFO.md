# LFO Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/LFOModulator.h`, `hi_core/hi_modules/modulators/mods/LFOModulator.cpp`
**Base class:** `TimeVariantModulator`, `TempoListener`, `ProcessorWithStaticExternalData`

## Signal Path

The LFO runs at the control rate (audio rate downsampled by `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`). The per-sample processing in `calculateBlock()` calls `calculateNewValue()` for each control-rate sample, then applies the intensity modulation chain as a post-process.

The processing chain per control-rate sample:

1. **Waveform lookup** - Read value from the current waveform source based on `uptime` phase accumulator
2. **Fade-in envelope** - Apply exponential attack envelope to scale the waveform value
3. **Mode-dependent output mapping** - Transform the value based on modulation mode (Gain/Pitch/Pan/Global) and bipolar setting
4. **Smoothing** - Low-pass filter to reduce discontinuities
5. **Phase advance** - Increment `uptime` by `angleDelta`

After the per-sample loop, the block is post-processed:

6. **Frequency modulation** - Sample the FrequencyChain and recalculate `angleDelta` (downsampled further, every ~4096 control-rate samples)
7. **Intensity modulation** - Scale the output block by the IntensityChain values

## Gap Answers

### signal-path-order: Complete processing order

Per control-rate sample in `calculateNewValue()`:
1. Waveform lookup (table interpolation, random value, or step value)
2. Fade-in: `attackValue = attackBase + attackValue * attackCoef` (exponential envelope)
3. Mode-dependent combination of waveform value and fade-in
4. Smoothing: `currentValue = smoother.smooth(newValue)`
5. Phase advance: `uptime += angleDelta`

Per block in `calculateBlock()`:
6. FrequencyChain sampled (further downsampled via `frequencyUpdater`), recalculates `angleDelta`
7. IntensityChain applied to the entire output buffer

The frequency chain does NOT affect per-sample calculation within the current block - it only updates `angleDelta` for subsequent samples. The intensity chain is applied as a post-multiply on the whole block.

### waveform-generation: How waveforms are generated

All standard waveforms (Sine, Triangle, Saw, Square) use pre-computed lookup tables of `SAMPLE_LOOKUP_TABLE_SIZE` entries, initialised once at startup. The current phase (`uptime`) indexes into the table with linear interpolation between adjacent samples. Output is inverted: `1.0 - interpolated_value`.

- **Random**: Generates a new random value once per cycle (when the phase wraps). Holds the value constant within each cycle.
- **Custom**: Uses the user-editable table (TableProcessor interface). Same interpolated lookup as standard waveforms. Output is inverted: `1.0 - interpolated_value`.
- **Steps**: Reads from the SliderPackProcessor. On each cycle wrap, advances to the next step index (`thisCycleIndex % numSliders`). Values are inverted: `1.0 - sliderValue`. Between wraps, holds the current step value. There is a simple 50/50 blend between the previous and current step value at the transition point (not full interpolation).

Phase offset is applied at reset: `uptime = phaseOffset * TABLE_SIZE`.

### mode-dependent-output: Modulation mode affects output

After the raw waveform value is computed, the fade-in envelope is applied differently depending on mode:

- **GainMode**: `value = 1.0 - rawValue * fadeIn` (modulation reduces from 1.0)
- **PitchMode / PanMode / OffsetMode (bipolar)**: `value = (1.0 - fadeIn) * 0.5 + fadeIn * rawValue` (centres at 0.5 during fade-in)
- **PitchMode / PanMode / OffsetMode (unipolar)**: `value = rawValue * fadeIn`
- **GlobalMode (bipolar)**: same as PitchMode bipolar
- **GlobalMode (unipolar)**: same as GainMode

The mode is determined automatically by where the modulator is placed in the module tree (e.g., in a gain chain vs pitch chain). The bipolar flag is a separate property.

### tempo-sync-mechanism: How tempo sync works

When `TempoSync` is enabled:
- The `Frequency` parameter switches meaning from Hz to a tempo division enum (e.g., Quarter, Eighth, Sixteenth)
- `calcAngleDelta()` converts the tempo division to Hz using the current host BPM
- The FrequencyChain modulation multiplies the resulting Hz value

`SyncToMasterClock` is an additional feature that only activates when `TempoSync` is also enabled. When both are on, the LFO phase resyncs to the host transport position on play start and resync events. The phase is calculated as: `uptime = (ppqOffset / cycleLength) * TABLE_SIZE`, aligning the LFO cycle to the musical grid.

### note-trigger-behaviour: Legato and IgnoreNoteOn interaction

On noteOn:
- If `IgnoreNoteOn` is on: no phase reset, no fade-in reset (free-running mode)
- If `Legato` is on AND keys are already held: no phase reset (legato behaviour)
- Otherwise: reset phase to `phaseOffset * TABLE_SIZE`, reset fade-in to 0, restart modulation chains

On noteOff:
- Decrement key counter
- If `Legato` is off OR no keys remain: stop voice modulators in both chains

Setting `IgnoreNoteOn` to true also triggers an immediate phase reset (one-time), allowing it to be used as a manual resync mechanism.

The fade-in always restarts from 0 when the phase is reset. With `FadeIn = 0`, the fade-in is instant (attackValue jumps to 1.0).

### loop-behaviour: What happens when LoopEnabled is off

Non-looping behaviour differs by waveform type:

- **Custom**: When `uptime` exceeds `TABLE_SIZE`, the output freezes at the last table value. The `loopEndValue` is cached on first read.
- **Steps**: When the current step index reaches `numSliders - 1` on a cycle wrap, the output freezes at the last step's value.
- **Standard waveforms (Sine/Triangle/Saw/Square)**: The phase counter continues wrapping through the table regardless of `LoopEnabled`. Loop disable only affects Custom and Steps modes.

This means `LoopEnabled = Off` effectively creates a one-shot envelope from Custom table shapes or step sequences, but has no effect on the four standard waveforms.

### steps-mode-detail: Step sequencer SliderPack usage

In Steps mode:
- The step index advances on each cycle wrap: `currentSliderIndex = thisCycleIndex % numSliders`
- Values are read as `1.0 - data.getValue(index)` (inverted so slider-up = high modulation)
- At the transition point between steps, there is a simple 50/50 blend: `newValue = 0.5 * newStep + 0.5 * previousStep`. This is a single-sample crossfade, not a configurable interpolation.
- Between transitions, the value is held constant at the current step value
- The displayed index in the slider pack UI is updated on each transition
- `NumSteps` controls how many sliders are used from the pack

### frequency-chain-application: Frequency chain update rate

The frequency chain is calculated once per block at control rate, but the result is only applied to `angleDelta` when `frequencyUpdater.shouldUpdate()` returns true. The updater has a manual count limit of `4096 / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`, meaning the frequency is updated approximately every 4096 audio samples. This is a significant downsampling - frequency modulation is not per-sample precise.

### performance: CPU profile

The LFO runs at the control rate (not audio rate), downsampled by `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`. Per control-rate sample:
- One table lookup with linear interpolation (negligible)
- One exponential envelope step (negligible)
- One smoother step (negligible)
- One phase increment (negligible)

Per block:
- Intensity chain and frequency chain modulation (depends on what modulators are in the chains)

The LFO itself has a downsampling factor of 32 (`LFO_DOWNSAMPLING_FACTOR`) for some internal operations.

- **Baseline:** low (control-rate table lookups + smoothing + mod chain overhead)
- **Polyphonic:** false (monophonic)
- **Scaling factors:** none from the LFO itself; cost depends on modulators in the intensity/frequency chains

## Processing Chain Detail

1. **Waveform generation** - Table lookup with linear interpolation at control rate. Per control-rate sample, low CPU.
2. **Fade-in envelope** - Exponential coefficient calculation, per control-rate sample, negligible CPU.
3. **Mode-dependent output** - Conditional arithmetic based on modulation mode, negligible CPU.
4. **Smoothing** - Low-pass filter, per control-rate sample, negligible CPU.
5. **Frequency modulation** - Chain calculation + angleDelta update, downsampled further, negligible CPU.
6. **Intensity modulation** - Chain multiply on output buffer, per block, low CPU.

## Modulation Points

- **IntensityChain** (index 0): Applied as a post-multiply on the entire output block after per-sample calculation. Scales the LFO depth. In bipolar modes, modulates around the 0.5 centre point.
- **FrequencyChain** (index 1): Multiplies the base frequency to modulate the LFO speed. Updated every ~4096 audio samples, not per-sample.

## Conditional Behavior

- **WaveformType**: Switches between 7 entirely different waveform generation paths (Sine/Triangle/Saw/Square use lookup tables; Random uses per-cycle random; Custom uses the user table; Steps uses the slider pack).
- **TempoSync**: Changes Frequency parameter from Hz to tempo division. Affects angleDelta calculation.
- **SyncToMasterClock**: Only active when TempoSync is also on. Resyncs phase to host transport.
- **Legato**: Prevents phase reset on overlapping notes.
- **IgnoreNoteOn**: Prevents all note-triggered phase resets (free-running).
- **LoopEnabled**: Only affects Custom and Steps modes. Standard waveforms always loop.
- **Bipolar** (not a parameter - set by modulation mode): Changes the output mapping formula.

## Interface Usage

**TableProcessor (LookupTableProcessor):** Provides the Custom waveform. The table is a standard 0-1 lookup table used for interpolated waveform generation. The X axis text converter shows timing information (milliseconds or tempo fraction depending on sync mode). The table is shared (not per-voice).

**SliderPackProcessor:** Provides step values for the Steps waveform mode. The number of active sliders is controlled by the NumSteps parameter. Values are read sequentially with the step index advancing each cycle. The displayed index is updated for visual feedback.

## CPU Assessment

- **Baseline:** low
- **Polyphonic:** false (monophonic, shared across all voices)
- **Scaling factors:** none from the LFO itself

The LFO runs at control rate with simple table lookups and arithmetic. The main CPU cost comes from whatever modulators are placed in the intensity and frequency chains.

## UI Components

The editor is `LfoEditorBody`, a standard processor editor (not a FloatingTile). No dedicated FloatingTile content type exists for this module.

## Notes

The waveform values from lookup tables are inverted in the output (`1.0 - value`) so that the default Sine waveform starts at the top and dips down, matching the conventional LFO display where the modulated parameter is reduced from its base value in Gain mode.

The `loopEndValue` variable caches the last waveform value when looping is disabled, preventing repeated lookups. It is reset to -1.0 on phase reset.

The scaleFunction is set to `input * 2.0 - 1.0` in the constructor, suggesting the module supports bipolar output mapping for pitch/pan modes.
