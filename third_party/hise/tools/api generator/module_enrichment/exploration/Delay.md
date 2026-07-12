# Delay - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/Delay.h`, `hi_core/hi_modules/effects/fx/Delay.cpp`
**Base class:** `MasterEffectProcessor`, `TempoListener`

## Signal Path

Audio input -> per-sample feedback loop (input + lastOutput * feedback -> delay line -> read) -> overlap fader dry/wet mix -> output.

The delay uses independent `DelayLine<>` instances for left and right channels. Each channel has independent delay time and feedback. The dry/wet mix uses an overlap fader (not a simple linear crossfade). Tempo sync is supported via the TempoSyncer system.

## Gap Answers

### signal-path-order

In `applyEffect()`, the processing is per-sample using `snex::block` iteration:

```
s = dryMix * s + wetMix * leftDelay.getDelayedValue(s + leftDelay.getLastValue() * feedbackLeft)
```

So per sample:
1. Read last delayed output: `leftDelay.getLastValue()`
2. Calculate feedback input: `currentSample + lastOutput * feedback`
3. Write to delay line and read delayed value: `getDelayedValue(feedbackInput)`
4. Mix: `dryMix * currentSample + wetMix * delayedOutput`

The first buffer after initialization is skipped (`skipFirstBuffer` flag).

### filter-placement

**LowPassFreq and HiPassFreq are vestigial.** The parameters are defined in the enum (indices 4 and 5), stored as member variables, serialized, and exposed in metadata with descriptions claiming they are "applied to the delay feedback". However, no filter objects exist in the class and `applyEffect()` does not reference these values. The parameters have zero DSP effect.

### crossfade-on-time-change

The `DelayLine<>` template supports crossfade on delay time changes. In `GainEffect::prepareToPlay()`, `setFadeTimeSamples(samplesPerBlock)` is called to set the crossfade length. However, `DelayEffect` does not call `setFadeTimeSamples()` - it only calls `prepareToPlay(sampleRate)` on each delay line. The default fade behaviour of the DelayLine depends on the `AllowFade` template parameter (defaulting to true). The DelayLine internally uses a fade counter and crossfades between old and new read positions when delay time changes.

### tempo-sync-values

Uses the full `TempoSyncer::Tempo` enum. Available note values (without extended tempo):
Whole (1/1), HalfDuet (1/2D), Half (1/2), HalfTriplet (1/2T), QuarterDuet (1/4D), Quarter (1/4), QuarterTriplet (1/4T), EighthDuet (1/8D), Eighth (1/8), EighthTriplet (1/8T), SixteenthDuet (1/16D), Sixteenth (1/16), SixteenthTriplet (1/16T), ThirtyTwoDuet (1/32D), ThirtyTwo (1/32), ThirtyTwoTriplet (1/32T), SixtyForthDuet (1/64D), SixtyForth (1/64), SixtyForthTriplet (1/64T).

Default sync values: Left = QuarterTriplet, Right = Quarter.

When TempoSync is on, the delay time parameter value is cast to `TempoSyncer::Tempo` enum index. `TempoSyncer::getTempoInMilliSeconds()` converts to actual milliseconds at the current host BPM.

### mix-implementation

Uses `scriptnode::faders::overlap()` for dry/wet mixing. This is not a simple linear crossfade - it provides overlapping gain curves so both dry and wet signals are louder near the midpoint than they would be with linear crossfading. The dry value is `getFadeValue<0>(2, mix)` and wet is `getFadeValue<1>(2, mix)`.

### max-delay-buffer

`DelayLine<>` uses the default template parameter `HISE_MAX_DELAY_TIME_SAMPLES`. This is typically 131072 samples (~2.7 seconds at 48kHz). The parameter range of 3000ms is within this limit.

### performance

Processing is per-sample via `snex::block` iteration. The `getDelayedValue()` method writes the input to the delay buffer, then reads from the delayed position. The DelayLine uses integer delay times (set via `setDelayTimeSamples(int)`), so no interpolation is needed for the standard read. When crossfading between old and new delay times, two read positions are blended.

## Processing Chain Detail

1. **Skip check** (per-block): first buffer after init is skipped
2. **Overlap fader calculation** (per-block): dry/wet gain values from overlap fader
3. **Left channel processing** (per-sample): feedback -> delay write/read -> mix
4. **Right channel processing** (per-sample): feedback -> delay write/read -> mix

## Conditional Behaviour

- **TempoSync on/off**: When on, delay times come from `TempoSyncer::getTempoInMilliSeconds()` using the host BPM. When off, delay times are in raw milliseconds.
- **First buffer skip**: The first buffer after initialization is skipped entirely to avoid garbage in the delay line.

## Vestigial / Notable

- **LowPassFreq and HiPassFreq are vestigial.** They are stored, serialized, and exposed in metadata but have no effect on audio processing. No filter objects exist in the class.

## CPU Assessment

- **Baseline:** medium
- Per-sample delay buffer write and read per channel
- Overlap fader calculation per block (negligible)
- No significant scaling factors

## UI Components

Uses `DelayEditor` - standard parameter editor with no FloatingTile content type.

## Notes

- The module implements `TempoListener` to receive tempo change callbacks from the host
- There is a bug in `tempoChanged()`: both channels use `syncTimeLeft` for the tempo calculation (line 76). However, `calcDelayTimes()` is called immediately after and correctly uses `syncTimeRight` for the right channel, so the bug's impact is limited to the cached `delayTimeRight` member value
- `hasTail()` returns true, so the effect continues processing after input stops (for delay repeats)
