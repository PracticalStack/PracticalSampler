# PolyshapeFX - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/ShapeFX.h` (line 351-510), `hi_core/hi_modules/effects/fx/ShapeFX.cpp` (line 677-990), `hi_core/hi_modules/effects/fx/WaveShapers.cpp` (line 84-172, 422-440)
**Base class:** `VoiceEffectProcessor`

## Signal Path

The signal path in `applyEffect()` (ShapeFX.cpp:857-948) is a per-voice stereo chain:

1. Build per-sample drive buffer: modulation values * smoothed(drive-1), then add 1.0 -- result is `scratch[]`
2. Apply drive and bias to audio (mode-dependent order, see Conditional Behavior)
3. If oversampling enabled: upsample 4x -> waveshape -> downsample
4. If oversampling disabled: waveshape directly
5. Post-shaping attenuation: divide by `(0.03162 * scratch + 1)` per sample
6. Conditional DC removal: 20 Hz high-pass if bias != 0 or mode == AsymetricalCurve

input -> drive+bias -> [oversample up ->] waveshaper [-> oversample down] -> attenuation -> DC removal -> output

## Gap Answers

### signal-path-order: What is the processing order in applyEffect() / renderVoice()?

The per-voice processing in `applyEffect()` (line 857) follows this order:

1. **Drive buffer construction** (lines 859-883): The drive modulation chain values are read into a scratch buffer (audio-rate or constant). The `LinearSmoothedValue` smoother (target = `drive - 1.0`) is applied as a gain to the modulation values via `smoother->applyGain()`. Then 1.0 is added, giving: `scratch = 1 + modValue * smoothed(drive - 1)`.

2. **Drive and bias application** (lines 888-910): Mode-dependent -- see Conditional Behavior section.

3. **Waveshaping** (lines 912-931): If oversampling is enabled, the block is upsampled 4x via per-voice `ShapeFX::Oversampler`, shaped, then downsampled. Otherwise shaped directly via `shapers[mode]->processBlock()`.

4. **Post-shaping attenuation** (lines 933-939): Per-sample division by `(0.03162 * scratch[i] + 1.0)`. The constant 0.03162 is approximately -30 dB. This provides a gentle drive-dependent output compensation -- higher drive values get more attenuation.

5. **DC removal** (lines 941-945): A per-voice `SimpleOnePole` high-pass filter at 20 Hz is applied only when `bias != 0` or `mode == AsymetricalCurve`. This removes DC offset introduced by the bias or asymmetrical shaping.

There is no dry/wet mix and no pre-shaper filtering (unlike ShapeFX).

### shared-code-with-shapefx: Does PolyshapeFX share the same shaping engine as ShapeFX?

Yes, partially. PolyshapeFX shares:
- The `ShapeFX::ShaperBase` abstract interface for all shapers
- The `ShapeFX::ShapeFunctions` struct containing the mathematical shaping functions (Linear, Atan, Sin, etc.)
- The `ShapeFX::FuncShaper<T>` template for wrapping math functions as ShaperBase instances
- The `ShapeFX::Oversampler` typedef (JUCE `dsp::Oversampling<float>`)
- The `ShapeFX::ShapeMode` enum for mode indices

PolyshapeFX does NOT share:
- The table-based shapers: it has its own `PolytableShaper` and `PolytableAsymetricalShaper` classes (WaveShapers.cpp:84-172) instead of ShapeFX's `TableShaper`/`AsymetricalTableShaper`
- The signal path structure: no pre-shaper filters, no Mix, no Autogain, no soft limiter, no bitcrusher
- The set of registered modes: different subset (see mode-values gap)

The two classes are defined in the same files (ShapeFX.h/cpp) but are independent implementations sharing a common shaper toolkit.

### drive-application: How is Drive (0-60 dB) applied?

Drive is converted to linear gain in `setInternalAttribute()` (line 778): `drive = Decibels::decibelsToGain(newValue)`. The member `drive` is stored as a linear multiplier (1.0 at 0 dB, ~1000 at 60 dB).

The modulation chain interaction (lines 859-883):
1. Read modulation values into scratch buffer (audio-rate or constant, range 0-1 in ScaleOnly mode)
2. Multiply scratch by smoothed `(drive - 1)` via `LinearSmoothedValue::applyGain()`
3. Add 1.0: `scratch = 1 + modValue * (drive - 1)`

At 0 dB drive (linear 1.0), scratch = 1 regardless of modulation (since drive-1 = 0). At 60 dB, scratch ranges from 1 (mod=0) to ~1000 (mod=1).

The scratch buffer is then applied to the audio differently depending on mode (see Conditional Behavior). The smoother has a 50ms smoothing time (set in `prepareToPlay`, line 838). Voice start initializes the smoother to `drive - 1` without smoothing (line 986).

### bias-application: How is Bias (0-1) applied?

Bias is stored directly as a float (line 781). It is added as a DC offset to both channels before shaping. The exact application order depends on mode:

- **Sin/TanCos modes** (lines 888-901): Drive is applied first (`signal *= scratch`), then bias is added (`signal += bias`). Bias is only added when non-zero (optimization branch).
- **All other modes** (lines 904-909): Bias is added first, then drive: `(signal + bias) * (1 + scratch)`.

DC removal after shaping is conditional (line 941): the per-voice 20 Hz HP filter runs only when `bias != 0.0f` or `mode == AsymetricalCurve`. When bias is zero and mode is not AsymetricalCurve, no DC removal occurs. This means bias-induced DC offset IS removed from the output when bias is active.

### mode-values: What shaping functions are available for each Mode value?

PolyshapeFX registers these modes in `initShapers()` (WaveShapers.cpp:422-440):

| Value | Enum Name | Function | Registered |
|-------|-----------|----------|------------|
| 1 | Linear | Passthrough | Yes |
| 2 | Atan | Arctangent soft clipping | Yes |
| 3 | Tanh | - | No (falls back to Linear) |
| 4 | Sin | Sine wavefold | Yes |
| 5 | Asinh | Inverse hyperbolic sine | Yes |
| 6 | Saturate | - | No (falls back to Linear) |
| 7 | Square | - | No (falls back to Linear) |
| 8 | SquareRoot | - | No (falls back to Linear) |
| 9 | TanCos | Tangent-cosine combination | Yes |
| 10 | Chebichev1 | Chebyshev 1st order harmonic | Yes |
| 11 | Chebichev2 | Chebyshev 2nd order harmonic | Yes |
| 12 | Chebichev3 | Chebyshev 3rd order harmonic | Yes |
| 13-31 | (gap) | - | No (falls back to Linear) |
| 32 | Curve | User table lookup (symmetric) | Yes (PolytableShaper) |
| 33 | AsymetricalCurve | User table lookup (asymmetric) | Yes (PolytableAsymetricalShaper) |

The array is pre-filled with Linear shapers for all 34 slots. Only 10 modes are overwritten with actual shapers. Selecting an unregistered mode (Tanh, Saturate, Square, SquareRoot, or any gap value) silently gives Linear passthrough.

**Difference vs ShapeFX:** ShapeFX has Tanh, Saturate, Square, SquareRoot but lacks TanCos, Chebichev1-3, and AsymetricalCurve. The two modules have overlapping but different mode subsets.

The Curve mode (PolytableShaper) uses Table 0. It maps absolute input value to table position (symmetric around zero, sign-preserving). The AsymetricalCurve mode (PolytableAsymetricalShaper) uses Table 1. It maps the full -1..+1 input range to table position, allowing different shaping for positive and negative signal halves.

### table-lookup-position: Where does the TableProcessor lookup occur in the signal path?

The table lookup IS the shaping function for modes 32 (Curve) and 33 (AsymetricalCurve). When these modes are selected, `shapers[mode]->processBlock()` calls the PolytableShaper or PolytableAsymetricalShaper, which reads from the SampleLookupTable with linear interpolation.

The lookup happens at the same position in the signal chain as all other shaping modes -- after drive/bias application, inside oversampling if active, before post-shaping attenuation and DC removal.

PolyshapeFX uses `ProcessorWithStaticExternalData` (not `LookupTableProcessor` like ShapeFX) with 2 tables and 1 display buffer. Table 0 is used by Curve, Table 1 by AsymetricalCurve.

### oversampling-implementation: Is oversampling a simple on/off 4x toggle?

Yes. The constructor creates one `ShapeFX::Oversampler` per voice with `factor = 2` (meaning 2^2 = 4x oversampling) when `HI_ENABLE_SHAPE_FX_OVERSAMPLER` is 1 (the default). When the define is 0, factor is 0 (2^0 = 1x, effectively disabled at compile time).

The oversampler uses `filterHalfBandPolyphaseIIR` type (no linear phase). Each voice has its own oversampler instance, so oversampling cost scales linearly with active voice count.

**Latency:** The JUCE oversampler introduces latency from its half-band filters. However, since PolyshapeFX has no dry/wet mix (no parallel dry path), there is no need for latency compensation within the effect. The latency does propagate downstream but PolyshapeFX does not report it via `getLatencySamples()`.

**CPU cost:** At 4x oversampling, the shaping function processes 4x as many samples per voice. Combined with per-voice allocation of oversamplers, enabling oversampling with high polyphony is expensive.

### dry-wet-mix: Is there a dry/wet mix in PolyshapeFX?

No. There is no Mix parameter, no dry buffer copy, and no hardcoded mix ratio. The output is always 100% wet (fully shaped). The only output compensation is the post-shaping attenuation divider `(0.03162 * scratch + 1)`, which is drive-dependent gain reduction, not a dry/wet blend.

## Processing Chain Detail

1. **Drive buffer construction** - Read mod chain, apply smoothed drive, add 1.0. Per-sample (via smoother). CPU: negligible.
2. **Drive + bias application** - Multiply signal by drive factor and add bias offset. Per-sample. Mode-dependent order. CPU: negligible.
3. **Oversampling up** - 4x upsample via half-band polyphase IIR. Per-voice oversampler. Gated by Oversampling toggle. CPU: medium (per voice).
4. **Waveshaping** - Per-sample nonlinear function selected by Mode. CPU: low (math modes) to low (table lookup with interpolation).
5. **Oversampling down** - 4x downsample. Per-voice. Gated by Oversampling toggle. CPU: medium (per voice).
6. **Post-shaping attenuation** - Per-sample division by drive-dependent factor. CPU: negligible.
7. **DC removal** - 20 Hz SimpleOnePole HP filter, per-voice. Gated by `bias != 0 || mode == AsymetricalCurve`. CPU: negligible.

## Modulation Points

The **Drive Modulation** chain (InternalChains::DriveModulation, index 0) is the only modulation input. It is configured for audio-rate expansion (`setExpandToAudioRate(true)`, line 725) and ScaleOnly mode.

The modulation output scales the drive amount: `effective_drive = 1 + modValue * (drive - 1)`. At modulation = 1.0 (default/no modulator), full drive is applied. At modulation = 0.0, drive collapses to 1.0 (unity, no drive).

The modulation is applied in the drive buffer construction stage, before the audio signal is processed. It affects both the pre-shaping drive gain and the post-shaping attenuation (which uses the same scratch buffer).

## Conditional Behavior

- **Mode (Sin/TanCos vs others)**: The drive/bias application order differs:
  - **Sin (4) and TanCos (9)**: Drive applied first as multiplication (`signal *= scratch`), then bias added. Uses vectorized FloatVectorOperations. When bias is 0, the bias add is skipped entirely.
  - **All other modes**: Per-sample loop applies `(signal + bias) * (1 + scratch)`. Bias is added before the drive multiplication. The extra `(1 + ...)` wrapper means the effective gain at minimum drive (0 dB, scratch=1) is 2x for these modes vs 1x for Sin/TanCos.

- **Oversampling**: When On, wraps the shaping in 4x up/downsampling. When Off, shaping operates at native sample rate.

- **DC Removal**: Applied only when `bias != 0.0f` or `mode == AsymetricalCurve` (line 941). For all other configurations, no DC removal occurs.

- **Mode selection**: Unregistered modes (3, 6, 7, 8, 13-31) silently fall back to Linear passthrough since the shapers array is pre-filled with Linear instances.

## Interface Usage

**ProcessorWithStaticExternalData (Table)**: Provides 2 SampleLookupTables and 1 DisplayBuffer. Table 0 is used by PolytableShaper (Curve, mode 32) -- maps |input| * 512 to table index, preserves sign, symmetric shaping. Table 1 is used by PolytableAsymetricalShaper (AsymetricalCurve, mode 33) -- maps (input+1) * 256 to table index, full -1..+1 range, asymmetric shaping. The DisplayBuffer shows the transfer function curve via WaveformComponent::Broadcaster, updated every 50ms by the PolyUpdater timer.

## Vestigial / Notable

No vestigial parameters found -- all 4 parameters (Drive, Mode, Oversampling, Bias) are functional.

Modes 3 (Tanh), 6 (Saturate), 7 (Square), 8 (SquareRoot) are valid ShapeFX::ShapeMode enum values within the Mode parameter range (0-33) but are not registered in PolyshapeFX's `initShapers()`. Selecting these modes silently gives Linear passthrough.

## CPU Assessment

- **Baseline (no oversampling):** low - per-sample drive multiplication, one shaping function call, and conditional DC removal per voice.
- **Oversampling scaling:** Enabling 4x oversampling roughly quadruples the shaping cost per voice. With high polyphony (e.g. 16 voices), this becomes medium-high aggregate CPU.
- **Mode impact:** All mathematical modes are similar per-sample cost. Table lookup modes involve an array access with linear interpolation, comparable to math modes.
- **Voice scaling:** All processing is per-voice with no shared state. CPU scales linearly with active voices.
- **Overall per-voice:** low without oversampling, medium with oversampling.

## UI Components

The editor class is `PolyShapeFXEditor` (created in ShapeFX.cpp:813-826, defined in PolyShapeFXEditor.h/cpp). No FloatingTile content types discovered -- standard backend editor panel with WaveformComponent display, two TableEditors (Curve and AsymetricalCurve), and parameter sliders.

## Notes

- The `ProcessorWithStaticExternalData` base is constructed with `(mc, 2, 0, 0, 1)` meaning 2 tables, 0 slider packs, 0 audio files, 1 display buffer.
- The post-shaping attenuation constant `0.03162` (~-30 dB) provides mild output level compensation that increases with drive. This is not the same as ShapeFX's autogain which computes a static inverse-average from the transfer function.
- The drive smoother uses `LinearSmoothedValue` with 50ms smoothing time, while ShapeFX's gain uses a `LowpassSmoothedValue` with 40ms. Voice start bypasses smoothing via `setValueWithoutSmoothing`.
- The PolyUpdater timer fires every 50ms to refresh the waveform display, reading the current drive modulation output value for the transfer function visualization.
