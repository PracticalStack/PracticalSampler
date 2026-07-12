# core.granulator - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:2489`
**Base class:** `data::base` (via SNEX_NODE macro)
**Classification:** audio_processor

## Signal Path

A granular synthesiser that generates overlapping grains from an AudioFile source. Output is additive -- grain output is added to the existing signal on channels 0 and 1 (stereo only).

**Grain generation:** `startNextGrain()` (line 2645) is called once per sample when the time since the last grain start exceeds `timeBetweenGrains`. A new grain is started from the pool of 128 `Grain` objects by finding one in IDLE state.

**Grain processing:** Each active grain reads from its assigned region of the audio file using linear interpolation (`index::lerp<index::unscaled<double, index::clamped<0>>>`). Grains have a trapezoidal (attack-sustain-release) amplitude envelope with a quadratic fade curve (`fadeValue * fadeValue`).

**Processing path:** `process()` requires 2 channels and uses `DataTryReadLock` for thread safety. It dispatches to `processFix(ProcessData<2>&)` which iterates per-frame. Each frame calls `processFrame()` which calls `startNextGrain(1)` then sums all grain ticks.

The node handles MIDI events for note management -- it tracks up to 8 active notes and selects XYZ samples based on active notes when using multisample data.

## Gap Answers

### grain-generation: How are grains generated?

Up to 128 simultaneous grains (`span<Grain, NumGrains>`). New grains are triggered based on timing: when `uptime - timeOfLastGrainStart > timeBetweenGrains`. Each grain:
- Selects a position in the audio file based on `currentPosition` plus random offset scaled by `spread`
- Has a trapezoidal envelope (attack = grainSize/4, sustain, release = grainSize/4)
- Fade curve is quadratic: `gainValue * (fadeValue * fadeValue)`
- Output is stereo with random L/R balance based on `spread`

Additive output: `data[0] += totalGrainGain * sum[0]; data[1] += totalGrainGain * sum[1]`.

### position-mapping: How does Position work?

Position is a normalized 0..1 value. In `setParameter<0>()` (line 2828): `currentPosition = range(v, 0.0, 1.0)`. In `startNextGrain()` (line 2680): `idx = (int)(currentPosition * (fileSize - 2*grainLength))`. Position is static -- it does NOT auto-advance. The user must modulate Position externally for scanning playback.

### density-semantics: What does Density control?

Density (0-0.99) controls the overlap between grains. In `updateGrainLength()` (line 2792): `timeBetweenGrains = (grainLength / pitchRatio * (1.0 - density)) / 2`, clamped to minimum 400 samples. Higher density = shorter time between grains = more overlap. At density=0, grains are spaced far apart. At density=0.99, grains overlap maximally.

### spread-semantics: What does Spread control?

Spread (0-1) has two effects in `Grain::setSpread()` (line 2530):
1. **Stereo panning randomisation:** Random L/R balance scaled by spread. `lGain = 1 + spread*randomBalance`, `rGain = 1 - spread*randomBalance`.
2. **Position randomisation:** In `startNextGrain()` (line 2682): `idx += spread * random * grainLengthSamples` -- adds random offset to the grain start position.

### detune-semantics: What pitch range?

Detune (0-1) adds random pitch variation per grain. In `Grain::setSpread()` (line 2540): `pf = (2*random - 1) * detune` then `uptimeDelta *= pow(2, pf)`. So detune=1.0 gives up to +/- 1 octave of random pitch variation. Detune=0.5 gives up to +/- half octave.

### pitch-as-ratio: Is Pitch a speed ratio?

Yes. Pitch (0.5-2.0) is a playback speed ratio. `Grain::setPitchRatio(delta)` (line 2609) sets `uptimeDelta = delta`. In `startNextGrain()`, `thisPitch = pitchRatio * sourceSampleRate / sampleRate`. Pitch=1.0 means original speed, 0.5 means half speed (octave down), 2.0 means double speed (octave up).

### no-polyphony-no-midi: MIDI handling?

Despite not being flagged as IsProcessingHiseEvent in the preliminary JSON, the granulator DOES handle MIDI events. `handleHiseEvent()` (line 2734) processes:
- Note-on: adds to voice stack (up to 8 simultaneous notes)
- Note-off: removes from voice stack (with sustain pedal support)
- CC#64 (sustain pedal): delays note-offs
- All-notes-off: resets everything

When voices are active (`voiceCounter > 0`), grain generation uses the active notes for XYZ sample selection. When no voices are active, grain generation stops. The granulator is NOT a continuous texture generator -- it requires MIDI note-on events to produce sound (when using its built-in voice management).

## Parameters

- **Position** (0-1): Normalised read position in audio file. Static, must be modulated externally.
- **Pitch** (0.5-2.0, default 1.0): Playback speed ratio.
- **GrainSize** (20-800 ms, default 80): Duration of each grain.
- **Density** (0-1): Grain overlap amount.
- **Spread** (0-1): Stereo pan randomisation and position randomisation.
- **Detune** (0-1): Random pitch variation per grain (0 = none, 1 = +/- 1 octave).

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: [
  {"parameter": "Density", "impact": "high", "note": "Higher density = more simultaneous active grains"},
  {"parameter": "GrainSize", "impact": "medium", "note": "Longer grains increase overlap potential"}
]

## Notes

The granulator uses SNEX_NODE macro which provides `hmath Math` member. It manages its own internal voice stack (up to 8 notes) via `span<HiseEvent, 8> voices`. The sustain pedal (CC#64) support with delayed note-offs is a sophisticated feature. The `totalGrainGain` auto-compensation adjusts output level based on the grain density to prevent volume spikes.
