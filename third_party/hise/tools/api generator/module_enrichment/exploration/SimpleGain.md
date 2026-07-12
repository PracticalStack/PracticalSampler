# SimpleGain - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/GainEffect.h`, `hi_core/hi_modules/effects/fx/GainEffect.cpp`
**Base class:** `MasterEffectProcessor`

## Signal Path

Audio input -> polarity inversion (if enabled) -> delay (if > 0ms) -> gain (smoothed) -> stereo width (mid/side) -> balance (pan) -> output.

SimpleGain is a utility processor that combines gain, delay, stereo width, balance, and polarity inversion into a single module. Each major parameter (Gain, Delay, Width, Balance) has its own modulation chain. The processing order is fixed: polarity first, then delay, gain, width, and finally balance.

## Gap Answers

### signal-path-order

In `applyEffect()`, the processing order is:

1. **InvertPolarity** (per-block): `buffer.applyGain(-1.0f)` applied to the entire buffer if enabled
2. **Delay modulation read** (per-block): single modulation value scales the delay time
3. **Delay** (per-block, if delay > 0): `leftDelay.processBlock()` and `rightDelay.processBlock()`
4. **Gain** (per-block): smoothed gain with 50ms ramp applied via `smoothedGainL.applyGain()`
5. **Width** (per-4-samples, if width != 1.0): `msDecoder.calculateStereoValues()` called 4 times per iteration
6. **Balance** (per-block, if not centred): L/R gain factors from `BalanceCalculator`

### delay-implementation

Uses `DelayLine<16384>` per channel (max 16384 samples = ~341ms at 48kHz, ~371ms at 44.1kHz). In `prepareToPlay()`, the fade time is set to `samplesPerBlock` for click-free delay time changes. The delay processes per-block via `processBlock()`.

When delay is 0ms, the delay processing is skipped entirely (the gain is still applied). Delay modulation: `thisDelayTime = delay * delayModValue` - the modulation value scales the delay time parameter.

### width-implementation

Uses `MidSideDecoder` class. `setWidth(newValue / 100.0f)` normalises the 0-200% range to 0.0-2.0:
- Width=0 (0.0): full mono (mid only)
- Width=100 (1.0): no change (original stereo)
- Width=200 (2.0): exaggerated sides

The `calculateStereoValues()` method encodes L/R to mid/side, scales the side signal by the width factor, then decodes back to L/R. Processing is done in groups of 4 samples (loop unrolled).

Width modulation formula: `thisWidth = (width - 1.0) * widthModValue + 1.0`. This interpolates between 1.0 (no change) and the set width value. When modulation is 1.0, the full width value is used. When modulation is 0.0, width is 1.0 (passthrough).

Width processing is skipped entirely when width equals 1.0 (100%).

### balance-implementation

Balance uses a range of 0-1 with Pan mode, but the internal storage uses a different convention. Looking at `setInternalAttribute`: `balance = newValue` stores the raw value. The `BalanceCalculator::getGainFactorForBalance()` function takes this value and returns separate L/R gain factors.

The default of 0 with Pan mode represents the centre position. Values < 0 pan left, values > 0 pan right (typical -1 to +1 mapped onto 0 to 1 via the Pan slider mode).

Balance is smoothed with a `Smoother` at 1000ms smoothing time (very slow, for smooth panning transitions).

Balance modulation: `smoothedBalance *= balanceModValue`. The modulation chain uses Pan mode, so the modulation value is a pan position multiplier.

### polarity-inversion-point

InvertPolarity is applied first, before all other processing: `if (invertPolarity) buffer.applyGain(-1.0f)`. This multiplies the entire buffer by -1, flipping the phase of both channels. It runs before delay, gain, width, and balance.

### modulation-resolution

All 4 modulation chains use `getOneModulationValue(startSample)` - a single value per block (not per-sample). This is the lightest modulation resolution.

- **Gain modulation**: multiplied with the gain parameter to set the smoothed gain target
- **Delay modulation**: scales the delay time parameter
- **Width modulation**: interpolates between 1.0 and the width parameter
- **Balance modulation**: multiplies the smoothed balance value (only if the balance chain has processors)

### performance

With default settings (gain=0dB, delay=0ms, width=100%, balance=centre):
- InvertPolarity check: negligible
- Delay: skipped (delay == 0)
- Gain: `smoothedGainL.applyGain()` is a simple per-sample multiply with smoothing
- Width: skipped (width == 1.0)
- Balance: skipped if L/R gains are equal (centre)

In practice, with defaults, this module is essentially just a smoothed gain multiply - very low CPU. Adding delay or width processing increases the cost moderately.

## Processing Chain Detail

1. **Polarity** (per-block): multiply by -1 if enabled
2. **Gain modulation** (per-block): single modulation value read
3. **Delay modulation** (per-block): single modulation value, scales delay time
4. **Delay** (per-block): DelayLine processBlock per channel (skipped if delay=0)
5. **Gain** (per-block): smoothed gain multiply (50ms ramp)
6. **Width modulation** (per-block): single modulation value, scales width
7. **Width** (per-4-samples): mid/side encode/decode (skipped if width=100%)
8. **Balance modulation** (per-block): single modulation value, scales balance
9. **Balance** (per-block): L/R gain factors from BalanceCalculator (skipped if centred)

## CPU Assessment

- **Baseline:** low
- With defaults (no delay, no width change, centred balance): just a smoothed gain multiply
- **Scaling factors:**
  - Delay > 0: adds delay line read/write per channel
  - Width != 100%: adds mid/side encode/decode per sample (moderate)

## UI Components

Uses `GainEditor` - standard parameter editor with no FloatingTile content type.

## Notes

- The delay line has a maximum of 16384 samples (~341ms at 48kHz), which is less than the parameter range maximum of 500ms. At sample rates below ~32.8kHz, the full 500ms range would exceed the buffer. This is unlikely to be an issue in practice since standard sample rates are 44.1kHz+.
- Gain smoothing uses a 50ms ramp (`smoothedGainL.reset(sampleRate, 0.05)`)
- Balance smoothing is very slow at 1000ms, designed for smooth panning transitions
- The width processing loop assumes the buffer size is a multiple of 4 (loop processes 4 samples per iteration with `numSamples -= 4`). Non-multiple-of-4 buffer sizes could skip or over-read samples.
