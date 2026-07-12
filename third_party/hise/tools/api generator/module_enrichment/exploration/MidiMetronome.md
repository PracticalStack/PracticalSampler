# MidiMetronome - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/GainEffect.h` (lines 90-293), `GainEffect.cpp` (lines 392-528)
**Base class:** `MasterEffectProcessor`

## Signal Path

MidiMetronome is an audio generator disguised as a MasterEffect. It adds synthesised click sounds on top of the input audio, synchronised to a connected MidiPlayer's playback position. When disabled or when no player is connected, audio passes through unchanged.

audio in + [click generation on beat] -> audio out

## Gap Answers

### click-generation

**Question:** How does MidiMetronome generate the click sound?

**Answer:** The click is a per-sample blend of a sine wave and white noise, shaped by an exponential decay envelope. In `applyEffect()` (GainEffect.h:218-237):

1. A decay ramp (`rampValue`) starts at 1.0 on each new beat and decays per-sample by multiplying with ~0.9988
2. Noise component: `Random::getSystemRandom().nextFloat() * 0.5 - 0.5` scaled by `rampValue`
3. Sine component: `sin(uptime)` scaled by `rampValue`, where `uptime` increments by `uptimeDelta` (0.1 for normal beats, 0.2 for downbeats)
4. Final mix: `gain * (noiseAmount * noise + (1 - noiseAmount) * sine)`
5. The result is added to both L and R channels (mono click)

The `uptimeDelta` of 0.1 (or 0.2 for downbeats) determines the sine frequency. At 44100 Hz sample rate, `uptimeDelta = 0.1` gives approximately 700 Hz and `uptimeDelta = 0.2` gives approximately 1400 Hz.

### beat-detection

**Question:** How does the metronome detect beat positions?

**Answer:** `getBeatFromPosition()` (GainEffect.h:242-257) reads the MidiPlayer's current playback position and converts it to a beat index:

1. Returns -1 if the player is stopped or has no sequence
2. Computes a beat factor: `(float)denom / 4.0f` (adjusts for time signatures where the denominator is not 4)
3. Multiplies: `player->getPlaybackPosition() * sequence->getLengthInQuarters() * beatFactor`
4. Returns `floor(result)` as the current beat index

When `thisQuarter != lastQuarter` in `applyEffect()`, a new beat has been detected, triggering a click.

### downbeat-accent

**Question:** Does the metronome accent downbeats?

**Answer:** Yes. When a new beat is detected (GainEffect.h:205-215), the code checks `if (thisQuarter % nom == 0)` where `nom` is the numerator of the time signature (default 4). On downbeats, `uptimeDelta` is doubled from 0.1 to 0.2, producing a higher-pitched sine tone (approximately one octave up). Both `nom` and `denom` are read from the current sequence's time signature pointer on every processing block.

### audio-passthrough

**Question:** Does MidiMetronome pass input audio through and add clicks on top?

**Answer:** Yes. The click is additive. In the per-sample loop (GainEffect.h:234-236), the mixed value `mValue` is added to the existing buffer contents using `*l++ += (float)mValue` and `*r++ += (float)mValue`. The original audio is preserved. When `enabled` is false, `player` is null, or `rampValue` is 0, no processing occurs and the audio passes through unchanged.

### player-connection

**Question:** How is the MidiPlayer connection established?

**Answer:** The connection is not a standard parameter. `connectToPlayer()` (GainEffect.h:259-268) accepts either a MidiPlayer pointer or a string ID. When called with a string, it uses `ProcessorHelpers::getFirstProcessorWithName()` to find the MidiPlayer in the module tree. The connection is serialised as a `"PlayerID"` property in the ValueTree (GainEffect.h:143-144, 154). The editor (GainEffect.cpp:448-476) provides a ComboBox listing all MidiPlayer instances in the module tree, allowing the user to select one.

### click-envelope

**Question:** What is the decay envelope?

**Answer:** The decay is an exponential ramp. `rampValue` starts at 1.0 on each new beat and is multiplied by ~0.9988 per sample (GainEffect.h:225). At 44100 Hz:
- After ~576 samples (~13 ms): rampValue reaches ~0.5
- After ~5760 samples (~130 ms): rampValue reaches ~0.001 (effectively silent)

The click is quite short - approximately 130 ms from onset to silence. The processing loop exits early when `rampValue` is exactly 0.0 (via the `if (rampValue != 0.0f)` check at line 218), though in practice the float multiplication never reaches exactly zero, so the loop runs for the entire block until the next beat or until the ramp becomes inaudible.

## Processing Chain Detail

1. **Enable/player check** (negligible): Skip if disabled or no player connected
2. **Time signature read** (negligible): Read nom/denom from current sequence
3. **Beat detection** (negligible): Compare current beat index with last beat index
4. **Click synthesis** (low, per-sample): Sine + noise mix with exponential decay, only active for ~130 ms after each beat
5. **Additive mix** (negligible): Add click to both channels

## Conditional Behavior

- **Enabled = Off**: No processing, pure passthrough
- **No player connected**: No processing, pure passthrough
- **Player stopped**: `getBeatFromPosition()` returns -1, lastQuarter reset, no click
- **Downbeat** (thisQuarter % nominator == 0): Doubled uptimeDelta gives higher-pitched click
- **rampValue = 0**: Skip the per-sample synthesis loop (saves CPU between clicks)

## CPU Assessment

- **Overall baseline**: negligible (only active for brief bursts after each beat)
- **During click**: low (per-sample sine + noise + multiply, but only for ~130 ms)
- **Between clicks**: negligible (only beat detection comparison per block)
- **No scaling factors**: CPU is constant regardless of parameter values

## UI Components

- Backend editor: `MetronomeEditorBody` (GainEffect.cpp:434-510) - provides enable toggle, MidiPlayer selector ComboBox, volume slider, and noise amount slider

## Notes

The ramp decay coefficient (0.9988) and sine uptimeDelta (0.1) are defined using `JUCE_LIVE_CONSTANT_OFF`, suggesting they were originally tuneable via the JUCE live constant editor but are now fixed. The default volume is -12 dB (gain ~0.25). The default NoiseAmount of 0.5 gives a 50/50 blend of sine and noise. The gain member is initialised to 0.25 in the class declaration (matching -12 dB), which is consistent with the metadata default.
