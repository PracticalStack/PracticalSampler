---
title: MidiMetronome
moduleId: MidiMetronome
type: Effect
subtype: MasterEffect
tags: [utility, sequencing]
builderPath: b.Effects.MidiMetronome
screenshot: /images/v2/reference/audio-modules/midimetronome.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Requires MIDI Player connection"
    wrong: "Adding a MidiMetronome without connecting it to a MIDI Player"
    right: "Select a MIDI Player from the dropdown in the module editor before enabling the metronome"
    explanation: "The metronome requires a connected MIDI Player to read playback position and time signature. Without one, no clicks are produced even when enabled."
llmRef: |
  MidiMetronome (MasterEffect)

  Generates metronome click sounds synchronised to a connected MIDI Player's playback position. The click is a blend of sine wave and white noise with an exponential decay envelope, added on top of the input audio. Downbeats are accented with a higher-pitched tone.

  Signal flow:
    audio in + [click on each beat from MIDI Player] -> audio out

  CPU: negligible (only brief bursts of per-sample synthesis on each beat), monophonic.

  Parameters:
    Enabled (Off/On, default Off) - enables the metronome click
    Volume (-100 to 0 dB, default -12 dB) - click volume
    NoiseAmount (0-100%, default 50%) - blend between sine (0%) and noise (100%)

  Requires: a connected MIDI Player (selected via the editor dropdown, not a parameter).

  When to use:
    Monitoring beat timing during composition or rehearsal with MIDI Player sequences. The metronome is additive - it does not affect the original audio when disabled.

  Common mistakes:
    Must connect a MIDI Player first - no clicks without one.

  See also: (none)
---

::category-tags
---
tags:
  - { name: utility, desc: "Modules for analysis, placeholders, or structural purposes without audio processing" }
  - { name: sequencing, desc: "MIDI processors that generate or play back note sequences" }
---
::

![MidiMetronome screenshot](/images/v2/reference/audio-modules/midimetronome.png)

The MidiMetronome generates click sounds synchronised to a connected MIDI Player's playback position. Each click is a short burst of sine wave and white noise shaped by an exponential decay envelope, mixed additively on top of the input audio. When disabled or when no MIDI Player is connected, audio passes through unchanged.

Downbeats (the first beat of each bar, determined by the sequence's time signature) are accented with a higher-pitched tone - approximately one octave above normal beats. The time signature is read from the connected MIDI Player's current sequence.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Enabled:
      desc: "Enables the metronome click output"
      range: "Off / On"
      default: "Off"
    Volume:
      desc: "Click volume in decibels"
      range: "-100 - 0 dB"
      default: "-12 dB"
    NoiseAmount:
      desc: "Blend between sine wave and noise for the click sound"
      range: "0 - 100%"
      default: "50%"
  functions:
    detectBeat:
      desc: "Reads the MIDI Player's playback position and detects when a new beat occurs"
    synthesiseClick:
      desc: "Generates a click from a sine/noise mix with exponential decay (~130 ms)"
---

```
// MidiMetronome - monophonic, additive click generator
// stereo in + click -> stereo out

if not Enabled or no MIDI Player connected:
    output = input    // passthrough

beat = detectBeat(MidiPlayer.position, timeSignature)

if new beat:
    if beat is downbeat:
        pitch = high    // ~1400 Hz
    else:
        pitch = normal  // ~700 Hz

    // Per-sample click synthesis (~130 ms decay)
    click = synthesiseClick(pitch, Volume, NoiseAmount)
    output = input + click
```

::

## Parameters

::parameter-table
---
groups:
  - label: Click Control
    params:
      - { name: Enabled, desc: "Enables the metronome click output. When off, audio passes through unchanged.", range: "Off / On", default: "Off" }
      - { name: Volume, desc: "Volume of the metronome click in decibels.", range: "-100 - 0 dB", default: "-12 dB" }
      - { name: NoiseAmount, desc: "Controls the tonal character of the click. At 0% the click is a pure sine tone; at 100% it is pure noise. The default of 50% gives a blend of both.", range: "0 - 100%", default: "50%" }
---
::

### MIDI Player Connection

The MidiMetronome requires a connected MIDI Player to function. The connection is established through a dropdown in the module editor, not through a standard parameter. The selected MIDI Player's ID is saved with the preset.

### Click Characteristics

The click envelope decays exponentially over approximately 130 ms. Each click is a mono signal added equally to both channels.

### Playback Requirement

The metronome only produces clicks while the connected MIDI Player is playing. When the player is stopped, no clicks are generated and the audio passes through unchanged.
