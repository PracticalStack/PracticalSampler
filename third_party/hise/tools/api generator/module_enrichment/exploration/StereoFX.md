# StereoFX - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/StereoFX.h`, `hi_core/hi_modules/effects/fx/StereoFX.cpp`
**Base class:** `VoiceEffectProcessor`

## Signal Path

Per-voice audio -> pan (per-voice, modulation-driven) -> combined buffer -> width (mid/side, shared) -> output.

StereoFX is a polyphonic stereo processor with two distinct processing stages:

1. **`applyEffect()` (per-voice):** Applies panning to each voice individually. The pan position is driven entirely by the Pan Modulation chain - without modulators in the chain, this stage is skipped. The Pan parameter defines the modulation range, not a static position.

2. **`renderNextBlock()` (shared buffer):** Applies stereo width via mid/side encoding to the combined output. This runs once on the summed buffer after all voices are rendered.

## Gap Answers

### signal-path-order: What is the processing order in applyEffect()? Is Width applied before or after Pan? Is there mid/side encoding involved?

Width and Pan are in separate methods with different scopes:

1. **Pan** is applied per-voice in `applyEffect()`. For each active voice, the modulation chain output is scaled by the Pan parameter and converted to L/R gain factors via `BalanceCalculator::getGainFactorForBalance()`.

2. **Width** is applied to the combined output buffer in `renderNextBlock()` using mid/side encoding via `MidSideDecoder::calculateStereoValues()`.

The VoiceEffectProcessor pipeline runs `applyEffect()` during voice rendering, then `renderNextBlock()` on the summed output. So the order is: **Pan first (per-voice), then Width (shared).**

### pan-parameter-semantics: Does 'Maximum' mean this defines the range/extent of the pan modulation chain rather than a static pan position?

Yes, confirmed. The Pan parameter is purely a modulation range scaler, not a static pan position. Evidence:

- In `applyEffect()`, the very first check is `if (!balanceChain.getChain()->shouldBeProcessedAtAll()) return;`. If the Pan Modulation chain has no modulators, the entire method exits immediately - **the Pan parameter has zero effect without modulators**.

- The internal storage maps the -100..+100 external range to 0.0..1.0: `pan = (newValue + 100.0f) / 200.0f`.

- The pan value is used as a scaler: `normalizedPan = (pan - 0.5f) * 200.0f`, then `scaledPanValue = panValues * normalizedPan`. With default Pan=100 (internal=1.0), normalizedPan=100.0, giving the modulation chain full -100..+100 range. With Pan=50 (internal=0.75), normalizedPan=50.0, limiting the sweep to -50..+50.

- The constructor's table value converter confirms this: `auto v = tmp->getAttribute(StereoEffect::Pan) * input` - it multiplies the Pan parameter by the modulation value.

### pan-law: What panning law is used?

**Equal-power panning** with sqrt(2) compensation. From `BalanceCalculator::getGainFactorForBalance()`:

1. Normalize input: `balance = clamp(balanceValue / 100.0, -1.0, 1.0)`
2. Convert to angle: `panValue = PI * (balance + 1.0) * 0.25` (maps -1..+1 to 0..PI/2)
3. Apply: Left = `sqrt(2) * cos(angle)`, Right = `sqrt(2) * sin(angle)`

Key positions:
- Centre (0): L = sqrt(2) * cos(PI/4) = 1.0, R = sqrt(2) * sin(PI/4) = 1.0 (unity)
- Full left (-100): L = sqrt(2) * cos(0) = sqrt(2) (~+3dB), R = 0
- Full right (+100): L = 0, R = sqrt(2) (~+3dB)

This is a standard constant-power pan law. The sqrt(2) factor ensures unity gain at centre while preserving perceived loudness across the stereo field.

### width-implementation: How is Width implemented? What is the exact formula?

Mid/side encoding via `MidSideDecoder::calculateStereoValues()`:

```
mid  = (L + R) * 0.5
side = (R - L) * width * 0.5
L_out = mid - side
R_out = mid + side
```

Where `width = Width_parameter / 100.0` (stored internally as 0.0..2.0):

- **Width=0 (0.0):** side=0, L=R=(L+R)/2 - full mono collapse
- **Width=100 (1.0):** passthrough (side reconstruction perfectly recovers original L/R)
- **Width=200 (2.0):** side signal doubled, exaggerating stereo differences. L_out = (3L-R)/2, R_out = (3R-L)/2 - can produce out-of-phase content

Width processing is skipped entirely when width equals 1.0.

### voice-pan-interaction: How does StereoFX interact with the voice's existing stereo signal?

StereoFX processes an already-stereo voice buffer. Both `applyEffect()` and `renderNextBlock()` operate on L/R channels (index 0 and 1) of the buffer.

- **Pan** applies independent gain factors to L and R channels. It does not "place" a mono source - it rebalances an existing stereo signal. A voice already panned left will be further attenuated on the right when pan modulation pushes right.

- **Width** operates on the stereo difference of whatever signal is present. If the voice buffer is effectively mono (L==R), Width has no audible effect because the side signal (R-L) is zero.

### modulation-resolution: Is the Pan modulation applied per-sample or per-block?

**Per-sample when audio-rate modulators are present.** The constructor calls `setExpandToAudioRate(true)` on the BalanceChain, enabling audio-rate modulation expansion.

In `applyEffect()`, two paths exist:

1. **Audio-rate path:** `getReadPointerForVoiceValues(startSample)` returns a per-sample float array. Each sample gets its own `scaledPanValue`, and `getGainFactorForBalance()` is called per-sample. This enables smooth, zipper-free pan sweeps.

2. **Constant path (fallback):** When no audio-rate modulation is needed, `getConstantModulationValue()` returns a single value. L/R gain factors are calculated once, then applied to the entire block via `FloatVectorOperations::multiply()`.

Additionally, `setIncludeMonophonicValuesInVoiceRendering(true)` ensures monophonic modulators (e.g. an LFO in monophonic mode) contribute to per-voice pan values.

### performance-per-voice: What is the per-voice CPU cost?

**Per-voice (applyEffect):**
- Early exit if no modulators in chain - negligible
- Constant modulation path: two `FloatVectorOperations::multiply()` calls - negligible
- Audio-rate path: per-sample `getGainFactorForBalance()` which calls `cosf()`/`sinf()` - low to moderate (trig per sample)

**Shared (renderNextBlock):**
- Skipped if Width=100% - negligible
- Per-sample mid/side encode/decode (additions and multiplies only) - low

**No early-exit for neutral Pan+Width together.** Width is checked independently (width != 1.0). Pan exits only if the chain is empty. There is no combined neutral-settings bypass.

## Processing Chain Detail

1. **Pan modulation chain evaluation** (per-voice): Modulation chain runs per-voice, producing either a per-sample array or a constant value
2. **Pan application** (per-voice, in `applyEffect()`): Modulation output scaled by Pan parameter, converted to L/R gain factors via equal-power formula, applied to voice buffer
3. **Width** (shared, in `renderNextBlock()`): Mid/side encode, scale side by width factor, decode back to L/R. Skipped if width=100%

## Modulation Points

- **Pan Modulation chain (BalanceChain, index 0):** Drives the pan position per-voice. Uses PanMode (output maps to stereo position, not amplitude). The Pan parameter scales the modulation output. Audio-rate capable (`setExpandToAudioRate(true)`). Monophonic modulators included in voice rendering (`setIncludeMonophonicValuesInVoiceRendering(true)`).

- **Width:** No modulation chain. Static per-instance value only.

## Conditional Behavior

- **Pan Modulation chain empty:** `applyEffect()` returns immediately via `shouldBeProcessedAtAll()` check. No panning occurs regardless of Pan parameter value.
- **Width == 100% (1.0):** `renderNextBlock()` skips mid/side processing entirely.
- **Audio-rate vs constant modulation:** `applyEffect()` uses per-sample processing when `getReadPointerForVoiceValues()` returns a valid pointer, otherwise falls back to block-level constant multiplication.

## CPU Assessment

- **Pan (per-voice, audio-rate):** low - per-sample trig calls (cosf/sinf), but modern CPUs handle this well. Scales linearly with voice count.
- **Pan (per-voice, constant):** negligible - two vectorized multiplies per voice
- **Pan (no modulators):** negligible - immediate early exit
- **Width (shared):** low - per-sample additions and multiplies, no trig. Runs once regardless of voice count.
- **Overall baseline:** low

## UI Components

Uses `StereoEditor` - a standard parameter editor. No FloatingTile content types found.

## Notes

- The Pan parameter is a modulation range scaler with a default of 100 (full range). Without modulators in the chain, it has no audible effect. This is by design - StereoFX is intended as a modulation-driven panner, not a static pan control. The description "Maximum stereo pan position" accurately reflects this, though it may confuse users expecting a static pan knob.
- Width is applied to the combined output buffer (shared), not per-voice. This means all voices share the same stereo width setting. If per-voice width control is needed, it would require a different architecture.
- The `msDecoder` is a single instance (not per-voice), confirming Width is shared state.
- The `pan` member variable is also shared, but its effect is per-voice because the modulation chain provides per-voice values that are scaled by it.
