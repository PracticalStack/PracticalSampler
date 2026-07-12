---
title: Waveform Generator
moduleId: WaveSynth
type: SoundGenerator
subtype: SoundGenerator
tags: [oscillator]
builderPath: b.SoundGenerators.WaveSynth
screenshot: /images/v2/reference/audio-modules/wavesynth.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: [voice count, second oscillator]
seeAlso: []
commonMistakes:
  - title: "No start phase control"
    wrong: "Looking for a StartPhase parameter to randomise oscillator phase on note-on"
    right: "Use a scriptnode patch with oscillator nodes for random or free-running phase"
    explanation: "Both oscillators always reset to phase 0 on every note-on. There is no built-in phase control. Scriptnode was designed to handle this kind of oscillator customisation."
  - title: "Mix crossfade is linear, not equal-power"
    wrong: "Expecting equal loudness at Mix = 50%"
    right: "Compensate for the ~3 dB dip at centre by adjusting levels or using a scriptnode alternative"
    explanation: "The Mix parameter uses a linear crossfade (osc1 * (1-mix), osc2 * mix), which produces a perceived level dip at the centre position."
forumReferences:
  - id: 1
    title: "No start phase control — oscillators always reset to phase 0"
    summary: "The Waveform Generator provides no StartPhase parameter; both oscillators always start at phase 0 on note-on. Use scriptnode for random or free-running phase."
    topic: 13720
  - id: 2
    title: "PolyBLEP silently falls back to sine above sampleRate/4"
    summary: "The PolyBLEP algorithm reverts to a pure sine wave when oscillator frequency exceeds sampleRate/4, so high-pitched notes on non-sine waveforms may sound unexpectedly pure."
    topic: 9514
  - id: 3
    title: "Osc2PitchChain enables audio-rate FM; hard sync resets osc2 per osc1 cycle"
    summary: "The Osc2 Pitch Modulation chain supports audio-rate FM synthesis, and hard sync is implemented at the PolyBLEP level by resetting osc2 phase on each osc1 cycle completion."
    topic: 953
customEquivalent:
  approach: scriptnode
  moduleType: SoundGenerator
  complexity: medium
  description: "A scriptnode patch with two oscillator nodes, split chain for mixing, and converter nodes for detune. Supports features unavailable in the built-in module such as random start phase and pulse width modulation."
llmRef: |
  Waveform Generator (SoundGenerator)

  Dual-oscillator polyphonic synthesiser using band-limited PolyBLEP waveforms. 9 waveform types per oscillator: Sine, Triangle, Saw, Square, Noise, Triangle 2, Square 2, Trapezoid 1, Trapezoid 2. Supports hard sync, independent pitch for FM synthesis, and modulatable mix/pan.

  Signal flow:
    MIDI note -> pitch calculation -> osc1 PolyBLEP -> gain mod
                                   -> osc2 PolyBLEP (if enabled) -> gain mod
    -> linear mix crossfade -> equal-power pan -> effect chain -> stereo out

  CPU: low per voice, polyphonic. Disabling the second oscillator saves CPU.

  Parameters:
    OctaveTranspose1 (-5 to 5, default 0) - coarse pitch offset for osc1 in octaves
    WaveForm1 (1-9, default Saw) - waveform type for osc1
    Detune1 (-100 to 100 cents, default 0) - fine pitch detune for osc1
    Pan1 (-1 to 1, default 0) - stereo panning for osc1
    SemiTones1 (-12 to 12, default 0) - semitone transpose for osc1
    PulseWidth1 (0-100%, default 50%) - pulse width for osc1 square waveforms
    OctaveTranspose2 (-5 to 5, default 0) - coarse pitch offset for osc2
    WaveForm2 (1-9, default Saw) - waveform type for osc2
    Detune2 (-100 to 100 cents, default 0) - fine pitch detune for osc2
    Pan2 (-1 to 1, default 0) - stereo panning for osc2
    SemiTones2 (-12 to 12, default 0) - semitone transpose for osc2
    PulseWidth2 (0-100%, default 50%) - pulse width for osc2 square waveforms
    Mix (0-100%, default 50%) - linear crossfade between osc1 and osc2
    EnableSecondOscillator (Off/On, default On) - enables/disables osc2
    HardSync (Off/On, default Off) - resets osc2 phase on each osc1 cycle
    Gain (0-100%, default 25%) - output volume
    Balance (-1 to 1, default 0) - stereo balance
    VoiceLimit (1-256, default 256) - maximum polyphony
    KillFadeTime (0-20000 ms, default 20 ms) - voice kill fade-out

  Modulation chains:
    Gain Modulation - scales the output volume
    Pitch Modulation - scales the pitch of both oscillators
    Mix Modulation - modulates the osc1/osc2 balance (audio rate, clamped 0-1)
    Osc2 Pitch Modulation - independent pitch for osc2 (audio rate, enables FM synthesis)

  Limitations:
    No start phase control - oscillators always reset to phase 0 on note-on.
    No pulse width modulation - PulseWidth is a static parameter, not modulatable.
    PolyBLEP falls back to sine above sampleRate/4 to prevent aliasing.
    Linear mix crossfade produces a level dip at 50%.

  Common mistakes:
    Expecting random/free-running phase - use scriptnode instead.
    Expecting equal-power mix crossfade - Mix is linear.

  See also: none
---

::category-tags
---
tags:
  - { name: oscillator, desc: "Modules that generate audio or modulation signals from oscillators or synthesis algorithms" }
---
::

![Waveform Generator screenshot](/images/v2/reference/audio-modules/wavesynth.png)

The Waveform Generator is a dual-oscillator polyphonic synthesiser that uses band-limited PolyBLEP waveforms to produce classic analogue-style tones without aliasing. Nine waveform types are available per oscillator: Sine, Triangle, Saw, Square, Noise, Triangle 2, Square 2, Trapezoid 1, and Trapezoid 2. The two oscillators can be mixed, panned independently, and hard-synced for aggressive timbral effects.

The Osc2 Pitch Modulation chain operates at audio rate, enabling FM-style synthesis where osc1 modulates the pitch of osc2. Combined with hard sync, this provides a wide range of timbres from a lightweight module.

### Oscillator Phase

Both oscillators always reset to phase 0 on every note-on event. There is no start phase parameter. [1]($FORUM_REF.13720$) For random or free-running phase behaviour (useful for avoiding phase cancellation in unison patches), rebuild the oscillator setup in scriptnode, which was specifically designed for this kind of customisation.

### PolyBLEP Sine Fallback

The PolyBLEP algorithm silently reverts to a pure sine wave when the oscillator frequency exceeds sampleRate/4. This prevents aliasing at very high frequencies but means that non-sine waveforms played in the upper octaves may sound unexpectedly pure. [2]($FORUM_REF.9514$)

## Signal Path

::signal-path
---
glossary:
  parameters:
    OctaveTranspose1:
      desc: "Coarse pitch offset for osc1 in octaves"
      range: "-5 - 5"
      default: "0"
    WaveForm1:
      desc: "Waveform type for osc1"
      range: "Sine / Triangle / Saw / Square / Noise / Triangle 2 / Square 2 / Trapezoid 1 / Trapezoid 2"
      default: "Saw"
    Detune1:
      desc: "Fine pitch detune for osc1 in cents"
      range: "-100 - 100"
      default: "0"
    Pan1:
      desc: "Stereo panning for osc1"
      range: "-1 - 1"
      default: "0"
    SemiTones1:
      desc: "Semitone transpose for osc1"
      range: "-12 - 12"
      default: "0"
    PulseWidth1:
      desc: "Pulse width for osc1 (affects square-type waveforms)"
      range: "0 - 100%"
      default: "50%"
    OctaveTranspose2:
      desc: "Coarse pitch offset for osc2 in octaves"
      range: "-5 - 5"
      default: "0"
    WaveForm2:
      desc: "Waveform type for osc2"
      range: "Sine / Triangle / Saw / Square / Noise / Triangle 2 / Square 2 / Trapezoid 1 / Trapezoid 2"
      default: "Saw"
    Detune2:
      desc: "Fine pitch detune for osc2 in cents"
      range: "-100 - 100"
      default: "0"
    Pan2:
      desc: "Stereo panning for osc2"
      range: "-1 - 1"
      default: "0"
    SemiTones2:
      desc: "Semitone transpose for osc2"
      range: "-12 - 12"
      default: "0"
    PulseWidth2:
      desc: "Pulse width for osc2 (affects square-type waveforms)"
      range: "0 - 100%"
      default: "50%"
    Mix:
      desc: "Linear crossfade between osc1 and osc2"
      range: "0 - 100%"
      default: "50%"
    EnableSecondOscillator:
      desc: "Enables or disables the second oscillator"
      range: "Off / On"
      default: "On"
    HardSync:
      desc: "Resets osc2 phase on each osc1 cycle completion"
      range: "Off / On"
      default: "Off"
  functions:
    polyBLEP:
      desc: "Band-limited waveform generation using polynomial approximation. Falls back to sine above sampleRate/4."
    mixCrossfade:
      desc: "Linear crossfade: osc1 * (1 - mix), osc2 * mix"
    panBalance:
      desc: "Equal-power panning using sqrt(2) * cos/sin law per oscillator"
  modulations:
    GainModulation:
      desc: "Scales the output volume per voice"
      scope: "per-voice"
    PitchModulation:
      desc: "Multiplies into the phase increment of both oscillators"
      scope: "per-voice"
    MixModulation:
      desc: "Modulates the osc1/osc2 balance (clamped 0-1, audio rate)"
      scope: "per-voice"
    Osc2PitchModulation:
      desc: "Independent pitch modulation for osc2 (audio rate, enables FM)"
      scope: "per-voice"
---

```
// Waveform Generator - per-voice processing
// polyphonic, one voice per note

// Pitch calculation (on note start)
freq = midiToHz(noteNumber)
factor1 = 2^(OctaveTranspose1 + SemiTones1/12 + Detune1/1200)
factor2 = 2^(OctaveTranspose2 + SemiTones2/12 + Detune2/1200)

// Per-sample generation
osc1 = polyBLEP(freq * factor1 * PitchModulation, WaveForm1, PulseWidth1)
osc2 = polyBLEP(freq * factor2 * PitchModulation * Osc2PitchModulation, WaveForm2, PulseWidth2)

// Hard sync (if enabled)
if HardSync and osc1CycleComplete:
    resetPhase(osc2)

// Mix (linear crossfade)
mix = clamp(Mix * MixModulation, 0, 1)
osc1 *= (1 - mix)
osc2 *= mix

// Pan (equal-power per oscillator, then sum to stereo)
left  = panBalance(osc1, Pan1).L + panBalance(osc2, Pan2).L
right = panBalance(osc1, Pan1).R + panBalance(osc2, Pan2).R

// Apply gain
left  *= Gain * GainModulation
right *= Gain * GainModulation
```

::

## Parameters

::parameter-table
---
groups:
  - label: Oscillator 1
    params:
      - { name: WaveForm1, desc: "Waveform type for oscillator 1. All waveforms are band-limited using PolyBLEP to prevent aliasing.", range: "Sine / Triangle / Saw / Square / Noise / Triangle 2 / Square 2 / Trapezoid 1 / Trapezoid 2", default: "Saw" }
      - { name: OctaveTranspose1, desc: "Coarse pitch offset for oscillator 1 in whole octaves.", range: "-5 - 5", default: "0" }
      - { name: SemiTones1, desc: "Semitone transpose for oscillator 1. Combined with OctaveTranspose1 and Detune1.", range: "-12 - 12", default: "0" }
      - { name: Detune1, desc: "Fine pitch detune for oscillator 1 in cents.", range: "-100 - 100 cents", default: "0" }
      - name: PulseWidth1
        desc: "Pulse width for oscillator 1. Only affects Square and Square 2 waveforms. 50% is a symmetric square wave; lower or higher values narrow one half-cycle."
        range: "0 - 100%"
        default: "50%"
        hints:
          - type: warning
            text: "This is a static parameter, not modulatable. For pulse width modulation, use a wavetable with pre-rendered PWM cycles or a scriptnode patch."
      - { name: Pan1, desc: "Stereo panning for oscillator 1. Uses equal-power panning.", range: "-1 - 1", default: "0" }
  - label: Oscillator 2
    params:
      - { name: EnableSecondOscillator, desc: "Enables or disables oscillator 2. When Off, osc1 output is copied to both internal channels and the Mix/Pan stages are skipped, saving CPU.", range: "Off / On", default: "On" }
      - { name: WaveForm2, desc: "Waveform type for oscillator 2.", range: "Sine / Triangle / Saw / Square / Noise / Triangle 2 / Square 2 / Trapezoid 1 / Trapezoid 2", default: "Saw" }
      - { name: OctaveTranspose2, desc: "Coarse pitch offset for oscillator 2 in whole octaves.", range: "-5 - 5", default: "0" }
      - { name: SemiTones2, desc: "Semitone transpose for oscillator 2. Combined with OctaveTranspose2 and Detune2.", range: "-12 - 12", default: "0" }
      - { name: Detune2, desc: "Fine pitch detune for oscillator 2 in cents.", range: "-100 - 100 cents", default: "0" }
      - { name: PulseWidth2, desc: "Pulse width for oscillator 2. Only affects Square and Square 2 waveforms.", range: "0 - 100%", default: "50%" }
      - { name: Pan2, desc: "Stereo panning for oscillator 2. Uses equal-power panning.", range: "-1 - 1", default: "0" }
  - label: Mixing
    params:
      - name: Mix
        desc: "Linear crossfade between oscillator 1 and oscillator 2. At 0% only osc1 is heard, at 100% only osc2. Modulatable via the Mix Modulation chain."
        range: "0 - 100%"
        default: "50%"
        hints:
          - type: warning
            text: "Uses a linear crossfade, which produces a perceived level dip at 50%. This is not equal-power."
      - { name: HardSync, desc: "When enabled, osc2's phase is reset every time osc1 completes a cycle. Produces the characteristic hard sync timbre. Most effective when osc2 is tuned higher than osc1.", range: "Off / On", default: "Off" }
  - label: Output
    params:
      - { name: Gain, desc: "Output volume as normalised linear gain (not decibels). Modulatable via the Gain Modulation chain.", range: "0 - 100%", default: "25%" }
      - { name: Balance, desc: "Stereo balance. Applied by the base class after per-voice processing.", range: "-1 - 1", default: "0" }
  - label: Voice Management
    params:
      - { name: VoiceLimit, desc: "Maximum number of simultaneous voices.", range: "1 - 256", default: "256" }
      - { name: KillFadeTime, desc: "Fade-out time when voices are killed by exceeding the voice limit or by a voice killer.", range: "0 - 20000 ms", default: "20 ms" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Gain Modulation", desc: "Scales the output volume. Applied as a per-voice multiply after oscillator generation and mixing.", scope: "per-voice", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Modulates the pitch of both oscillators. Applied per-sample as a multiplier on the phase increment.", scope: "per-voice", constrainer: "Any" }
  - { name: "Mix Modulation", desc: "Modulates the crossfade balance between osc1 and osc2. Operates at audio rate. Values are clamped to 0-1.", scope: "per-voice", constrainer: "Any" }
  - { name: "Osc2 Pitch Modulation", desc: "Independent pitch modulation for oscillator 2 only. Operates at audio rate, enabling FM-style synthesis when modulated by another oscillator or modulator.", scope: "per-voice", constrainer: "Any" }
---
::

### FM Synthesis

The Osc2 Pitch Modulation chain accepts audio-rate modulators, making it suitable for FM-style synthesis. Route a modulation signal into this chain to modulate osc2's frequency independently of osc1. Combined with hard sync, this produces a wide range of timbres from metallic FM tones to aggressive sync sweeps. Hard sync is implemented at the PolyBLEP level: when osc1 completes a cycle, osc2's phase is reset — the canonical hard sync definition. [3]($FORUM_REF.953$)

### Limitations

The Waveform Generator does not support pulse width modulation as a modulatable parameter. PulseWidth1 and PulseWidth2 are static values that only take effect on the next note-on. For PWM, use a wavetable with pre-rendered PWM sweep cycles or build a custom scriptnode oscillator.
