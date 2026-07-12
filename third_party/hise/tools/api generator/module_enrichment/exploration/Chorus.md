# Chorus - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/Chorus.h`, `hi_core/hi_modules/effects/fx/Chorus.cpp`
**Base class:** `MasterEffectProcessor`

## Signal Path

Audio input -> write to delay buffer (with feedback) -> read from modulated position (linear interpolation) -> subtractive mix with dry -> output.

The chorus uses a single delay buffer per channel (buffer for L, buffer2 for R). An internal parabolic LFO modulates the read position. The wet signal is subtracted from the dry signal (not added). The dry/wet ratio is hardcoded at 53% dry / 47% wet and is not user-adjustable.

## Gap Answers

### signal-path-order

Single modulated delay line per channel. In `processReplacing()`:

1. Write input + feedback to buffer: `buffer[bp] = input + feedback * lastDelayed`
2. Calculate modulated read position: `dpt = dm + dep * (1 - phi^2)`
3. Read from delay buffer with linear interpolation between two adjacent samples
4. Mix output: `output = dry * input - wet * delayed` (subtractive)

Both channels share the same LFO phase (no stereo offset between L and R).

### internal-lfo

The internal LFO uses a parabolic waveform: `1 - phi^2` where phi sweeps from -1 to +1. The phase `phi` increments by `rat` per sample and wraps at 1.0 back to -2.0 offset (effectively -1 to +1 range). This creates a smooth, rounded modulation shape similar to a cosine.

There is no phase offset between left and right channels - both channels use the same phi value and thus the same delay modulation. This means the chorus does not inherently widen the stereo image through phase differences.

### delay-range

The Delay parameter (0-1) controls the centre point of the modulation. In `calculateInternalValues()`:
- `dep = 2000 * depth^2` (max modulation depth in samples)
- `dem = dep - dep * delay` (minimum delay offset)
- `dep -= dem` (modulation range = dep * delay)

The actual delay in samples varies from `dem` to `dem + dep * (1 - phi^2)`. At Delay=1.0 (default), dem=0 and the full depth range is used. At Delay=0.5, the modulation is centred at half the depth.

The delay buffer is 2048 samples (BUFMAX). At 44.1kHz, the maximum modulation depth is 2000 samples = ~45ms.

### feedback-path

Feedback is applied before the delay buffer write: `buffer[bp] = input + fb * lastDelayed`. The feedback coefficient is `1.9 * feedback - 0.95`, mapping the 0-1 parameter range to -0.95 to +0.95. This allows both positive and negative feedback, with negative values producing a different tonal character (cancelling rather than reinforcing).

Feedback is applied per-channel using the last delayed value read from each channel's buffer.

### rate-range

In `calculateInternalValues()`: `rat = pow(10, 3 * rate - 2) * 2 / sampleRate`.

At 44.1kHz:
- Rate=0: `pow(10,-2) * 2 / 44100 = 0.000000454` (essentially static; code forces rat=0 and resets phase when rate < 0.01)
- Rate=0.33: ~0.046 Hz
- Rate=0.5: ~0.143 Hz
- Rate=0.67: ~0.455 Hz  
- Rate=1.0: ~0.454 Hz... let me recalculate.

Actually: `pow(10, 3*1 - 2) = pow(10, 1) = 10`. `10 * 2 / 44100 = 0.000454`. This is the per-sample phase increment. The LFO cycles from -1 to +1 (range of 2), so frequency = rat * sampleRate / 2 / 2... 

More precisely: phi wraps every 2.0 units of range, so cycles per sample = rat / 2, frequency = rat * sampleRate / 2. At rate=1.0: `10 * 2 / 44100 * 44100 / 2 = 10 Hz`. At rate=0.5: pow(10, 0.5) = 3.16, -> ~3.16 Hz. At rate=0.33: ~1 Hz. The usable range is roughly 0.01 to 10 Hz.

### performance

Linear interpolation between adjacent delay buffer samples: `f1 = tmpf * (buffer[tmpi] - buffer[tmp]) + buffer[tmp]`. Per-sample processing. The modulation calculation (phase increment, parabolic LFO, delay index) and interpolated read are done per sample. CPU cost is low - simpler than a biquad filter.

## Processing Chain Detail

1. **LFO phase update** (per-sample): `phi += rat`, wrap at 1.0
2. **Buffer write** (per-sample): input + feedback * lastDelayed -> circular buffer
3. **Delay position calculation** (per-sample): parabolic modulation `dm + dep * (1 - phi^2)`
4. **Buffer read with interpolation** (per-sample): linear interpolation between two buffer positions
5. **Output mix** (per-sample): `dry * input - wet * delayed` (subtractive, hardcoded 53/47 ratio)

## Conditional Behaviour

When Rate < 0.01, the rate is forced to 0 and the LFO phase is reset. This effectively freezes the chorus at a static delay.

## CPU Assessment

- **Baseline:** low
- Simple per-sample delay buffer read/write with linear interpolation
- No scaling factors - all parameters are cheap

## UI Components

Uses `ChorusEditor` - standard parameter editor with no FloatingTile content type.

## Notes

- The dry/wet mix is hardcoded at `parameterMix = 0.47` in the constructor. This value is never modified by any parameter. Users cannot adjust the wet/dry balance.
- The wet signal is subtracted (`dry * input - wet * delayed`), not added. This creates the characteristic chorus comb-filter cancellation pattern.
- Both channels share the same LFO phase - no stereo widening from phase offset. The stereo effect comes only from the different feedback history in each channel's buffer.
- The feedback range maps to -0.95 to +0.95, meaning at the default value of 0.3: `1.9 * 0.3 - 0.95 = -0.38` (negative feedback by default). At feedback=0.5: `1.9 * 0.5 - 0.95 = 0.0` (no feedback). Values above 0.5 give positive feedback.
- The delay buffer is allocated with `new float[BUFMAX]` in the constructor and never freed (memory leak on destruction). This is a minor code-smell.
