# Saturator - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/Saturator.h`, `hi_core/hi_modules/effects/fx/Saturator.cpp`
**Base class:** `MasterEffectProcessor`

## Signal Path

Audio input -> PreGain (linear) -> waveshaping -> PostGain (linear) -> wet/dry mix -> output.

The Saturator applies a soft-clip waveshaping function with pre/post gain staging and a linear dry/wet crossfade. The Saturation Modulation chain scales the saturation amount, updated every 8 samples when audio-rate modulation is active.

## Gap Answers

### signal-path-order

In `applyEffect()`, the per-sample processing is:
```
output = dry * input + wet * (postGain * saturator.getSaturatedSample(preGain * input))
```

Order:
1. Apply PreGain (multiply by linear gain)
2. Waveshape the pre-gained sample
3. Apply PostGain (multiply by linear gain)
4. Mix: `dry * original + wet * processed`

### waveshaping-algorithm

Uses a soft-clip transfer function: `(1 + k) * x / (1 + k * |x|)` where `k = 2 * saturation / (1 - saturation)`.

At Saturation=0: k=0, output = x (linear passthrough).
At Saturation=0.5: k=2, moderate soft clipping.
As Saturation approaches 1.0: k approaches infinity, hard clipping toward a square wave.
The saturation value is clamped to 0.999 maximum to prevent division by zero.

This is a classic soft-clip curve that progressively limits the signal amplitude while introducing odd harmonics. It is smooth and continuous - no hard edges in the transfer function.

### modulation-application

The Saturation Modulation chain multiplies the saturation parameter value: `saturator.setSaturationAmount(modValue * saturation)`.

When audio-rate modulation is active (`getReadPointerForVoiceValues` returns non-null), the saturation amount is updated every 8 samples: `if (i & 7)` means the update fires when the sample index has any of bits 0-2 set, which is 7 out of every 8 samples. On every 8th sample (index 0, 8, 16...) the update is skipped. This is a cost-saving measure to avoid recalculating the k coefficient every sample.

When no audio-rate modulation is active, a single constant modulation value is used for the entire block.

### pregain-range

PreGain is stored as linear gain, converted from dB: `preGain = Decibels::decibelsToGain(newValue)`. At 0 dB, preGain = 1.0 (unity). The range 0-24 dB maps to linear 1.0 to ~15.85x.

PostGain is similarly stored as linear: `postGain = Decibels::decibelsToGain(newValue)`. At 0 dB, postGain = 1.0. The range -24 to 0 dB maps to linear ~0.063 to 1.0.

The asymmetric ranges (PreGain boost-only, PostGain cut-only) are confirmed in the code and metadata. This enforces a gain-staging workflow: boost into the saturator, then compensate the output level.

### wet-mix-implementation

Linear crossfade: `dry * input + wet * processed` where `dry = 1.0 - wetAmount` and `wet = wetAmount`. At WetAmount=1.0 (default), the output is fully saturated. At WetAmount=0.0, the original signal passes through unaffected.

### performance

The waveshaping is per-sample: `getSaturatedSample()` is called once per sample per channel. The function itself is cheap (1 multiply, 1 division, 1 absolute value). No oversampling is used. The `setSaturationAmount()` involves a division (`k = 2 * sat / (1 - sat)`) but is only called once per 8 samples in the modulated path, or once per block otherwise.

The Saturation parameter does not change the computational cost - the waveshaping function runs regardless of the saturation amount. At Saturation=0 it's a multiplication by 1 (passthrough), which is still evaluated.

## Processing Chain Detail

1. **Modulation read** (per-block or per-8-samples): saturation modulation chain value read
2. **Saturation coefficient update** (per-block or per-8-samples): k coefficient recalculated
3. **Per-sample processing** (per-sample): preGain -> waveshape -> postGain -> wet/dry mix

## CPU Assessment

- **Baseline:** low
- Simple per-sample waveshaping (multiply, divide, abs)
- No oversampling, no filters
- No scaling factors

## UI Components

Uses `SaturationEditor` - standard parameter editor with no FloatingTile content type.

## Notes

- The saturation modulation chain is expanded to audio rate and allows voice value modification, enabling sample-accurate modulation when audio-rate modulators are used
- The modulation chain factory is restricted to `TimeVariantModulatorFactoryType` in GainMode, meaning only time-variant (monophonic) modulators can be added
- At Saturation=0, the waveshaping function degenerates to `x * 1 / (1 + 0) = x` (linear passthrough), so the module behaves as a pure gain stage with just PreGain and PostGain
