# PhaseFX - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/Phaser.h`, `hi_core/hi_modules/effects/fx/Phaser.cpp`
**Base class:** `MasterEffectProcessor`

## Signal Path

Audio input -> dry/wet split -> allpass cascade (6 stages with feedback around all) -> linear mix with dry -> output.

The phaser uses 6 first-order allpass filters in cascade per channel (left and right are independent PhaseModulator instances). Feedback is applied around the entire cascade: the output of the previous sample is fed back into the input of the first allpass stage. The modulation chain controls the sweep position between Frequency1 and Frequency2. There is no internal LFO - sweep movement relies entirely on the Phase Modulation chain.

The output of the allpass cascade is summed with the original input: `return input + output`, producing the characteristic notch/peak comb pattern. The Mix parameter then crossfades between the original input and this processed signal.

## Gap Answers

### signal-path-order

6 allpass filters in cascade per channel (PhaseModulator contains `AllpassDelay allpassFilters[6]`). In `getNextSample()`, the signal flows: `input + currentValue * feedback` -> allpass[5] -> allpass[4] -> allpass[3] -> allpass[2] -> allpass[1] -> allpass[0] -> output. The `currentValue` (previous output) is fed back at the cascade input. The return value is `input + output` (sum of dry and allpass-filtered signal).

### sweep-modulation

The Phase Modulation chain value linearly interpolates between Frequency1 and Frequency2. In `setConstDelay()`: `delayThisSample = minDelay + (maxDelay - minDelay) * modValue`. The frequencies are converted to normalised delay values: `minDelay = freq1 / (sampleRate / 2)`, `maxDelay = freq2 / (sampleRate / 2)`. So modValue=0 sets allpass delays to Frequency1, modValue=1 sets them to Frequency2.

The modulation chain is expanded to audio rate (`setExpandToAudioRate(true)`) and clamped to 0-1 (`setClampTo0To1(true)`). All 6 allpass filters receive the same delay coefficient per sample.

### feedback-placement

Feedback is applied around the entire 6-stage cascade. In `getNextSample()`: the input to the first allpass stage is `input + currentValue * feedback`, where `currentValue` is the full cascade output from the previous sample. The feedback coefficient is `0.99 * feedbackParam`, providing a small safety margin below unity.

### mix-implementation

Linear crossfade in `applyEffect()`: `output = input * (1 - Mix) + Mix * phaserOutput`. Where `phaserOutput` is the result of `getNextSample()` which returns `input + allpassOutput`.

At Mix=1.0 (default), the output is entirely the phaser signal (input + allpass output). At Mix=0.0, the output is the unprocessed input.

### internal-lfo

PhaseFX has no internal LFO. The sweep position is controlled entirely by the Phase Modulation chain. Without any modulators added to the chain, a constant modulation value is used (`getConstantModulationValue()`), resulting in a static phaser (no sweep). Users must add an LFO or other modulator to the Phase Modulation chain to create sweep movement.

### performance

All processing is per-sample. The inner loop in `applyEffect()` processes one sample at a time through `getNextSample()`, which runs all 6 allpass filters in series. Each allpass filter performs 2 multiplies and 2 additions. Total per sample per channel: 12 multiplies + 12 additions + feedback + mix. This is moderate CPU cost.

## Processing Chain Detail

1. **Frequency smoothing** (per-block): freq1Smoothed and freq2Smoothed updated, ranges set on both PhaseModulators
2. **Modulation read** (per-sample or constant): Phase Modulation chain value read
3. **Delay coefficient calculation** (per-sample): modValue interpolates between minDelay and maxDelay, converted to allpass coefficient
4. **Allpass cascade** (per-sample, 6 stages): input + feedback -> 6 allpass filters in series
5. **Feedback store** (per-sample): cascade output stored as currentValue
6. **Output sum** (per-sample): input + allpass cascade output
7. **Dry/wet mix** (per-sample): linear crossfade between input and phaser output

## CPU Assessment

- **Baseline:** medium
- 6 allpass filters per-sample per-channel = 12 allpass operations per sample
- No significant scaling factors - all parameters are cheap to apply
- Modulation chain expanded to audio rate adds slight overhead when modulators are present

## UI Components

Uses `PhaserEditor` - a standard parameter editor with no FloatingTile content type.

## Notes

- The Phase Modulation chain uses CombinedMode and is expanded to audio rate, allowing sample-accurate sweep control
- Frequency1 and Frequency2 are smoothed with a 50ms ramp time to prevent clicks when changed
- The allpass cascade processes from index 5 to 0 (innermost to outermost in the nested call), but since all stages use the same coefficient, the order does not affect the output
