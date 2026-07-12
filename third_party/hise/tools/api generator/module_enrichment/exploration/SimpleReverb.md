# SimpleReverb - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/SimpleReverb.h`, `hi_core/hi_modules/effects/fx/SimpleReverb.cpp`
**Base class:** `MasterEffectProcessor`

## Signal Path

Audio input -> JUCE Reverb (Freeverb algorithm: 8 comb filters + 4 allpass filters per channel) -> 0.5x gain reduction -> output.

SimpleReverb is a thin wrapper around the JUCE `Reverb` class (Freeverb implementation). All parameters are mapped directly to the `Reverb::Parameters` struct. The reverb handles wet/dry mixing, stereo width, and freeze mode internally. A fixed 0.5x gain is applied after processing to compensate for the reverb's tendency to boost overall level.

## Gap Answers

### signal-path-order

Uses `juce::Reverb` directly (Freeverb algorithm). In `applyEffect()`:
1. `reverb.processStereo(left, right, numSamples)` - the JUCE Reverb processes both channels in-place
2. `buffer.applyGain(0.5f)` - halves the output level

The JUCE Reverb internally consists of 8 parallel comb filters feeding into 4 series allpass filters per channel, with slightly different tunings for left and right to create stereo decorrelation. The wet and dry levels, damping, room size, width, and freeze mode are all handled internally by the JUCE Reverb class.

### freeze-mode-behavior

FreezeMode is passed directly to `Reverb::Parameters::freezeMode` as a continuous 0-1 value. Inside the JUCE Reverb, this parameter controls the comb filter feedback and damping. When freezeMode >= 0.5, the Freeverb algorithm sets feedback to 1.0 and damping to 0.0, effectively freezing the reverb tail. Below 0.5, it has no effect. So despite the continuous 0-1 range, it behaves as a threshold toggle at 0.5.

The default value of 0.1 means freeze is off by default.

### wet-dry-interaction

**DryLevel is effectively vestigial.** In `setInternalAttribute()`:
- Setting WetLevel: `parameters.wetLevel = newValue; parameters.dryLevel = 1.0f - newValue;` (also sets DryLevel)
- Setting DryLevel: `break;` (does nothing)

So changing WetLevel automatically sets DryLevel to `1 - WetLevel`, and the DryLevel slider has no effect. The UI shows two separate sliders but only WetLevel actually controls the mix. The levels are passed to the JUCE Reverb which applies them as simple linear gain multipliers to the wet and dry signals respectively before summing.

### width-implementation

Width is handled internally by the JUCE Reverb. The Freeverb algorithm produces decorrelated left and right outputs from its differently-tuned comb/allpass filters. The Width parameter controls the stereo spread: at Width=0, left and right outputs are summed to mono; at Width=1, the full decorrelated stereo output is used. This is implemented as a mix between the sum and difference of the two channels.

### performance

The JUCE Reverb (Freeverb) uses:
- 8 parallel comb filters per channel (per-sample IIR)
- 4 series allpass filters per channel (per-sample IIR)
- Total: 24 IIR filter operations per sample (12 per channel)

All processing is per-sample inside the JUCE Reverb. CPU cost is medium - the 8+4 filter structure is moderately expensive but well-optimised in JUCE.

## Processing Chain Detail

1. **Parameter update** (on change): all parameters forwarded to `reverb.setParameters()`
2. **Reverb processing** (per-sample, inside JUCE Reverb): 8 comb + 4 allpass per channel, with wet/dry mixing, width, damping
3. **Gain compensation** (per-block): `buffer.applyGain(0.5f)` halves the output

## Vestigial / Notable

- DryLevel parameter is vestigial - setting it does nothing. WetLevel controls both wet and dry levels (dry = 1 - wet).
- The 0.5x gain applied after reverb processing is a fixed compensation, not adjustable.

## CPU Assessment

- **Baseline:** medium
- 8 comb + 4 allpass filters per channel, all per-sample
- No scaling factors - RoomSize and Damping change filter coefficients but not complexity
- FreezeMode does not change CPU cost

## UI Components

Uses `ReverbEditor` - standard parameter editor with no FloatingTile content type.

## Notes

- The class comment explicitly states "It does not sound very good compared to commercially available reverb plugins, but for simple stuff, it is still useful"
- `hasTail()` returns true and `isSuspendedOnSilence()` returns true
- `voicesKilled()` resets the reverb state, clearing any lingering tail
