# PolyphonicFilter - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/Filters.h`, `hi_core/hi_modules/effects/fx/Filters.cpp`
**Base class:** `VoiceEffectProcessor`, `FilterEffect`
**Also inherits:** `ModulatorChain::Handler::Listener`, `scriptnode::data::filter_base`, `ProcessorWithCustomFilterStatistics`

## Signal Path

The module operates in two distinct modes depending on whether any modulation chains contain polyphonic modulators:

**Polyphonic path** (when `hasPolyMods()` is true):
- `applyEffect()` runs per-voice
- Build `RenderData` with all four mod chain values
- `voiceFilters.renderPoly(r)` applies filter to the voice buffer in-place

**Monophonic path** (when no poly mods, and voices are active):
- `renderNextBlock()` processes the summed buffer in sub-blocks of 64 samples via `BlockDivider<64>`
- Each sub-block: read mod chain values, build `RenderData`, `monoFilters.renderMono(r)`
- Coefficients are recalculated per sub-block (every 64 samples)

**Idle path** (no poly mods, no active voices):
- `renderNextBlock()` reads mod values for display only, does not process audio (early return)
- `polyWatchdog` countdown (32 blocks) determines when to transition from active to idle

Within each filter render call:
1. `MultiChannelFilter::update()` applies modulation to frequency/gain/Q via `RenderData`
2. If any coefficient changed (`dirty` flag), `updateCoefficients()` recalculates the filter
3. `internalFilter.processSamples()` applies the filter in-place to the audio buffer

## Gap Answers

### signal-path-order

In `applyEffect()` (polyphonic path, line 460):
1. If `!hasPolyMods()`, set watchdog and return (no processing - mono path handles it)
2. Build `FilterHelpers::RenderData` with voice buffer reference
3. Read `FrequencyChain` mod value (one value for the block)
4. If `bipolarIntensity != 0`, read `BipolarFrequencyChain` mod value and compute `bipolarDelta = bp * bipolarFMod`
5. Read `GainChain` mod value; if != 1.0, compute `gainModValue = dB2gain(gain * (gainMod - 1.0))`
6. Read `ResonanceChain` mod value
7. `voiceFilters.renderPoly(r)` calls `MultiChannelFilter::render()` which:
   - Calls `update(r)`: applies `applyModValue(frequency)` for frequency modulation, multiplies Q by `qModValue`, multiplies gain by `gainModValue`, recalculates coefficients if dirty
   - Calls `internalFilter.processSamples()` to filter the buffer in-place

In `renderNextBlock()` (monophonic path, line 326):
- Same mod reading and coefficient update logic, but processes in sub-blocks via `BlockDivider<64>` using `monoFilters.renderMono(r)`

There is no pre/post processing, saturation, or DC offset removal. The filter is applied directly in-place.

### filter-type-mapping

The `FilterBank::FilterMode` enum (FilterHelpers.h:49-69) defines 18 filter types (indices 0-17):

| Index | Enum name | Topology | Gain-sensitive |
|-------|-----------|----------|---------------|
| 0 | LowPass | StaticBiquad (IIR) | No |
| 1 | HighPass | StaticBiquad (IIR) | No |
| 2 | LowShelf | StaticBiquad (IIR) | Yes |
| 3 | HighShelf | StaticBiquad (IIR) | Yes |
| 4 | Peak | StaticBiquad (IIR) | Yes |
| 5 | ResoLow | StaticBiquad (IIR) | No |
| 6 | StateVariableLP | StateVariableFilter (SVF) | No |
| 7 | StateVariableHP | StateVariableFilter (SVF) | No |
| 8 | MoogLP | MoogFilter (4-pole ladder) | No |
| 9 | OnePoleLowPass | SimpleOnePole (1-pole IIR) | No |
| 10 | OnePoleHighPass | SimpleOnePole (1-pole IIR) | No |
| 11 | StateVariablePeak | *Not wired in setMode()* | Yes |
| 12 | StateVariableNotch | StateVariableFilter (SVF, NOTCH) | No |
| 13 | StateVariableBandPass | StateVariableFilter (SVF, BP) | No |
| 14 | Allpass | PhaseAllpass | No |
| 15 | LadderFourPoleLP | LadderSubType (4-pole) | No |
| 16 | LadderFourPoleHP | *Not wired in setMode()* | No |
| 17 | RingMod | RingmodFilter | No |

The default is index 6 (StateVariableLP).

**Important:** Modes 11 (StateVariablePeak) and 16 (LadderFourPoleHP) are defined in the enum and have display coefficient entries, but are missing from `FilterBank::setMode()`. They fall through to `default: break;` which means selecting them does not change the underlying filter type - the previously selected topology remains active. These are effectively broken/vestigial modes.

### bipolar-frequency-interaction

The two frequency modulation paths combine in `RenderData::applyModValue()` (MultiChannelFilters.cpp:1612):

1. Normalize frequency to 0-1 range: `f = (f - 20) / 19980`
2. Optionally apply log skew (if `HISE_LOG_FILTER_FREQMOD` is enabled, default: off)
3. Add bipolar delta: `f += bipolarDelta`
4. Multiply by frequency mod: `f *= freqModValue`
5. Optionally reverse log skew
6. Denormalize back to Hz: `f = f * 19980 + 20`

The bipolar delta is computed as `bipolarIntensity * bipolarModChainValue`. Since the bipolar chain uses `Modulation::OffsetMode`, its output is centered around 0 (range -1 to +1). The intensity parameter (-1 to +1) scales this offset.

The interaction is: bipolar offset is **added** to the normalized frequency, then the standard frequency mod **multiplies** the result. This means:
- Standard frequency mod (chain 0, gain mode): scales the frequency proportionally (0 = silence, 1 = full base frequency)
- Bipolar frequency mod (chain 2, offset mode): shifts the normalized frequency up or down before scaling
- The bipolar delta operates in **normalized frequency space** (0-1 mapping to 20-20000 Hz), not in Hz or semitones directly

### quality-parameter-behavior

The Quality parameter maps to `FilterEffect::quality` via `setRenderQuality()` (FilterHelpers.cpp:336):
- Only accepts power-of-two values: `if (powerOfTwo != 0 && isPowerOfTwo(powerOfTwo))` - otherwise the set is silently ignored
- The default is `HISE_MAX_PROCESSING_BLOCKSIZE` (typically 512)
- Quality=0 is rejected (the `powerOfTwo != 0` guard) - the previous value is retained

However, the Quality parameter does NOT directly control the coefficient update rate in the rendering path. The mono path uses `BlockDivider<64>` which is hardcoded to 64-sample sub-blocks. The poly path processes the entire voice buffer as a single block with one coefficient update.

The `quality` field is stored and retrievable via `getSampleAmountForRenderQuality()` but is not referenced in `renderNextBlock()` or `applyEffect()`. The `BlockDivider<64>` template parameter is a compile-time constant. This makes the Quality parameter's described behavior ("power-of-two buffer size for modulation processing") not match the actual implementation - the coefficient update rate is fixed at 64 samples in mono mode and once-per-block in poly mode regardless of the Quality setting.

### gain-parameter-conditional

The Gain parameter's effect depends on the filter mode. In `FilterBank::setMode()`, only three modes set `calculateGainModValue = true`:
- LowShelf (index 2)
- HighShelf (index 3)
- Peak (index 4)

For other modes, `calculateGainModValue` remains false (or is not set to true).

However, the gain value is always passed to the underlying filter's `updateCoefficients()` method. Whether it actually affects the filter response depends on the filter topology:
- **StaticBiquadSubType** (LowShelf, HighShelf, Peak, LowPass, HighPass, ResoLow): The biquad coefficient formulas for shelf and peak types use the gain parameter; LP/HP/ResoLow formulas ignore it
- **StateVariableFilterSubType**: The `updateCoefficients()` takes a gain parameter but SVF LP/HP/BP/Notch modes typically don't use it in their coefficient calculation
- **MoogFilterSubType**, **LadderSubType**, **SimpleOnePoleSubType**, **PhaseAllpassSubType**, **RingmodFilterSubType**: These accept gain but their `updateCoefficients` signature takes `double /*gain*/` (unnamed parameter), indicating gain is unused

The `calculateGainModValue` flag on the FilterBank is used to control whether gain modulation is computed for display purposes, not to gate the gain parameter itself. The `StateVariablePeak` mode (index 11) would also use gain, but it's not wired in `setMode()`.

### frequency-default-20k

The Frequency default of 20000 Hz is confirmed intentional in the metadata (Filters.cpp:54): `.withDefault(20000.0f)`. At 20 kHz with the default StateVariableLP mode, the filter is effectively passing all audible frequencies - functioning as a "transparent" default. There is no separate bypass mechanism specific to the filter; the standard EffectProcessor bypass (`isBypassed()`) inherited from the base class handles that.

### coefficient-calculation

Filter topology varies by Mode:
- **StaticBiquadSubType** (modes 0-5): Standard IIR biquad (2nd order). Per-block coefficient calculation via `updateCoefficients()`.
- **StateVariableFilterSubType** (modes 6-7, 12-13): State Variable Filter (2nd order, topology-preserving). Per-sample processing with coefficient update per render call.
- **MoogFilterSubType** (mode 8): 4-pole ladder filter (Moog-style). Per-sample with nonlinear feedback.
- **LadderSubType** (mode 15): 4-pole ladder. Per-sample processing.
- **SimpleOnePoleSubType** (modes 9-10): 1-pole IIR. Very cheap.
- **PhaseAllpassSubType** (mode 14): Allpass filter.
- **RingmodFilterSubType** (mode 17): Ring modulation effect.

Coefficients are recalculated once per render call in `MultiChannelFilter::update()`. In the mono path, this means once per 64-sample sub-block. In the poly path, once per voice buffer. The `dirty` flag prevents redundant recalculation when parameters haven't changed.

Per-voice CPU cost: each active voice runs its own filter instance. For 16 voices with SVF, that's 16 independent per-sample filter evaluations. MoogLP and Ladder modes are more expensive due to 4-pole cascades.

## Processing Chain Detail

1. **Poly/Mono mode detection** (per-callback): `hasPolyMods()` checks if any mod chain has active polyphonic modulators
2. **Mod chain value read** (per-block or per-64-sample sub-block): frequency, gain, bipolar, Q modulation values read
3. **Bipolar frequency delta** (conditional, per-block): computed only when `bipolarIntensity != 0`
4. **Gain modulation calculation** (per-block): `gainModValue = dB2gain(gain * (gainMod - 1.0))` - asymmetric formula means at gainMod=1.0, gainModValue=1.0 (no change)
5. **Coefficient update** (per-block, if dirty): `updateCoefficients(sampleRate, freq, q, gain)` on the filter sub-type
6. **Filter processing** (per-sample): `processSamples()` on the specific filter topology, in-place on the audio buffer

## Modulation Points

- **FrequencyChain** (index 0, gain mode): Scales normalized frequency via `freqModValue` multiplication in `applyModValue()`. Range 0-1 maps to 20 Hz to the base Frequency value.
- **GainChain** (index 1, gain mode): Multiplied as `dB2gain(gain * (gainMod - 1.0))` - at mod=1.0 no change; at mod=0.0 applies full negative gain in dB.
- **BipolarFrequencyChain** (index 2, offset mode): Added as signed offset in normalized frequency space, scaled by `bipolarIntensity`. Applied before standard frequency modulation.
- **ResonanceChain** (index 3, gain mode): Multiplies the Q value directly: `q.getNextValue() * renderData.qModValue`.

## Conditional Behavior

- **Poly vs Mono routing**: If any mod chain has polyphonic modulators (`polyMode`), processing happens per-voice in `applyEffect()`. Otherwise, `renderNextBlock()` processes the summed mono buffer. The `processorChanged()` listener detects when modulators are added/removed and switches modes.
- **Bipolar intensity gate**: The bipolar modulation chain is only evaluated when `bipolarIntensity != 0.0`. When intensity is zero, `bipolarDelta` stays at 0.0 and the chain is not read.
- **Gain modulation optimization**: In `applyEffect()`, gain mod is only calculated if `gainMod != 1.0`.
- **Filter mode**: The Mode parameter selects which filter topology and sub-type are instantiated. Changing mode swaps the internal filter object entirely (new allocation under SpinLock).
- **Block activity tracking**: `blockIsActive` and `polyWatchdog` (countdown from 32) track whether voices are active. When watchdog reaches 0, `blockIsActive` becomes false and `renderNextBlock()` skips processing (display-only path).

## Vestigial / Notable

- **StateVariablePeak (mode 11)** and **LadderFourPoleHP (mode 16)** are defined in the `FilterMode` enum and have display coefficient entries in `getDisplayCoefficients()`, but are missing from `FilterBank::setMode()`. Selecting these modes leaves the previously active filter topology in place.
- **Quality parameter** is stored and serialized but does not affect the actual coefficient update rate. The mono path hardcodes 64-sample sub-blocks via `BlockDivider<64>`, and the poly path updates once per voice buffer.
- The `calculateGainModValue` flag is set for shelf/peak modes but is only used for display purposes, not to gate gain processing.

## CPU Assessment

- **Baseline:** medium (per-voice filter processing)
- **SimpleOnePole modes (9, 10):** low - trivial 1-pole IIR per sample
- **StaticBiquad modes (0-5):** low-medium - standard 2nd order IIR biquad per sample
- **StateVariable modes (6-7, 12-13):** medium - SVF topology with per-sample state updates
- **MoogLP (8):** medium-high - 4-pole nonlinear ladder per sample
- **LadderFourPoleLP (15):** medium-high - 4-pole cascade per sample
- **RingMod (17):** low - simple ring modulation
- **Allpass (14):** low - simple allpass
- **Scaling factor:** Cost scales linearly with active voice count. 16 voices with MoogLP is significantly more expensive than 16 voices with OnePoleLowPass.
- **Coefficient update:** Negligible cost (once per 64 samples mono, once per block poly). The dirty flag skips recalculation when parameters are static.

## UI Components

Uses `FilterEditor` - standard parameter editor with frequency response display. No FloatingTile content type registered. The editor uses `FilterDataObject::CoefficientData` for the frequency response graph via `getApproximateCoefficients()`, which returns coefficients from the last started voice (poly) or the mono filter.

## Notes

- The module auto-switches between polyphonic and monophonic processing based on whether any modulation chain contains polyphonic modulators. This is detected by `ModulatorChain::Handler::Listener::processorChanged()` callback.
- In mono mode, the `monoFilters` FilterBank (1 voice) processes the summed signal. In poly mode, `voiceFilters` FilterBank (N voices) has independent filter state per voice.
- The `forceMono` flag (inherited from `EffectProcessor`) can override the poly detection, forcing mono-path processing even when poly mods exist.
- `startVoice()` resets the per-voice filter state and sets `blockIsActive = true`. The first voice after idle also resets the mono filter.
- The gain modulation formula `dB2gain(gain * (gainMod - 1.0))` is asymmetric: at gainMod=1.0 it produces 0 dB (unity). At gainMod=0.5, it applies half the gain in dB (attenuated shelf). At gainMod=0.0, it applies the full negative gain as dB.
- `HISE_LOG_FILTER_FREQMOD` (default: 0/disabled) would add logarithmic skewing to frequency modulation if enabled, making the modulation more perceptually linear across the frequency range.
