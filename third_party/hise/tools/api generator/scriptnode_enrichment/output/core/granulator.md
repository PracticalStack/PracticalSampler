---
title: Granulator
description: "A granular synthesiser that generates overlapping grains from an audio file with controls for position, pitch, density, spread, and detune."
factoryPath: core.granulator
factory: core
polyphonic: false
tags: [core, granulator, granular, synthesis, audio-file]
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors:
    - { parameter: "Density", impact: high, note: "Higher density increases the number of simultaneous active grains" }
    - { parameter: "GrainSize", impact: medium, note: "Longer grains increase overlap potential" }
forumReferences:
  - { tid: 5221, reason: "SampleMap wildcard loading and External slot for UI display" }
  - { tid: 7388, reason: "Granulator is monophonic -- polyphonic context distributes grains" }
seeAlso:
  - { id: "core.file_player", type: alternative, reason: "Conventional file playback without granular processing" }
  - { id: "core.stretch_player", type: alternative, reason: "Phase-vocoder time stretching as an alternative to granular" }
  - { id: "NoiseGrainPlayer", type: module, reason: "Module-tree granular effect using FFT-based noise decomposition" }
commonMistakes:
  - title: "Requires MIDI notes to produce sound"
    wrong: "Adding core.granulator to a chain and expecting continuous grain output"
    right: "Place the granulator inside a container.midichain and send MIDI note-on events to trigger grain generation."
    explanation: "The granulator uses an internal voice manager that starts grain generation only when MIDI notes are active. Without note-on events, no grains are produced."
  - title: "Position does not auto-advance"
    wrong: "Setting Position to 0.5 and expecting the grains to scan through the file"
    right: "Modulate Position externally (e.g. with a ramp or LFO) to scan through the file over time."
    explanation: "The Position parameter is static. It sets the centre point for grain generation but does not move on its own."
  - title: "Polyphonic context distributes grains across voices"
    wrong: "Placing core.granulator inside a Scriptnode Synthesiser and expecting independent per-voice granulation"
    right: "Use the granulator inside a Script FX (monophonic). In a polyphonic context, grains are distributed across active notes rather than duplicated per voice."
    explanation: "The granulator has no per-voice state. When multiple MIDI notes are held in a polyphonic context, grains cycle through the active pitches rather than providing separate granulation per voice."
llmRef: |
  core.granulator

  A granular synthesiser that reads from an audio file slot and generates overlapping grains. Requires MIDI note-on events via a midichain container. Output is additive (stereo, fixed 2 channels).

  Signal flow:
    MIDI note-on -> voice manager -> grain scheduler -> grain pool (up to 128)
    audio file + Position -> grain read with interpolation -> envelope -> stereo spread -> += audio out

  CPU: medium, monophonic (internal voice management, not scriptnode polyphony)

  Parameters:
    Position (0.0 - 1.0, default 0.0): normalised read position in file, static (must be modulated externally)
    Pitch (0.5 - 2.0, default 1.0): playback speed ratio per grain
    GrainSize (20 - 800 ms, default 80): duration of each grain
    Density (0.0 - 1.0, default 0.0): grain overlap amount, higher = more simultaneous grains
    Spread (0.0 - 1.0, default 0.0): stereo pan and position randomisation
    Detune (0.0 - 1.0, default 0.0): random pitch variation per grain (1.0 = +/- 1 octave)

  When to use:
    Rarely used (rank 92, 2 instances). Use for granular textures, ambient soundscapes, and creative audio manipulation. For conventional playback, use core.file_player. For tempo-synced stretching, use core.stretch_player.

  Common mistakes:
    Requires MIDI notes to produce sound -- place in a midichain.
    Position does not auto-advance -- modulate it externally.
    No per-voice state -- use in Script FX (monophonic) for predictable behaviour.

  Forum references: tid:5221 (SampleMap wildcard and External slot), tid:7388 (monophonic design)

  See also:
    [alternative] core.file_player -- conventional file playback
    [alternative] core.stretch_player -- phase-vocoder time stretching
    [module] NoiseGrainPlayer -- module-tree granular effect using FFT-based noise decomposition
---

This node is a granular synthesiser that generates overlapping grains from an audio file. Up to 128 grains can be active simultaneously, each reading from a position in the file with its own pitch, envelope, and stereo placement. The output is additive -- grain audio is summed onto the existing signal on both channels. The node is fixed to stereo output.

The granulator manages its own internal voice system (up to 8 simultaneous MIDI notes) rather than using scriptnode's polyphonic voice infrastructure. When multiple notes are held, grains cycle through the active note pitches, distributing them evenly. With a single note held, all grains use that pitch. With two notes, alternating grains use each pitch. This creates a natural blend rather than separate layers, and the grain density remains consistent regardless of the number of held keys.

Each grain has a trapezoidal amplitude envelope (attack, sustain, release) with a quadratic fade curve. The grain output level is automatically compensated based on the current density to prevent volume spikes when many grains overlap. The Spread parameter adds both stereo panning randomisation and position randomisation, while Detune adds random pitch variation per grain.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Position:
      desc: "Normalised read position in the audio file (0-1)"
      range: "0.0 - 1.0"
      default: "0.0"
    Pitch:
      desc: "Playback speed ratio for each grain"
      range: "0.5 - 2.0"
      default: "1.0"
    GrainSize:
      desc: "Duration of each grain in milliseconds"
      range: "20 - 800 ms"
      default: "80"
    Density:
      desc: "Controls grain overlap (higher = more simultaneous grains)"
      range: "0.0 - 1.0"
      default: "0.0"
    Spread:
      desc: "Stereo pan and position randomisation"
      range: "0.0 - 1.0"
      default: "0.0"
    Detune:
      desc: "Random pitch variation per grain"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    scheduleGrain:
      desc: "Triggers a new grain when the time since the last grain exceeds the density-derived interval"
    grainRead:
      desc: "Reads from the audio file with interpolation, applying envelope and stereo spread"
---

```
// core.granulator - granular synthesis from audio file
// MIDI + audio file -> audio out (stereo, additive)

process(input) {
    // grain scheduling (per sample)
    if (timeSinceLastGrain > interval(GrainSize, Density))
        scheduleGrain(Position, Pitch, Spread, Detune)

    // sum all active grains
    for each activeGrain {
        sample = grainRead(activeGrain, GrainSize)
        output += sample
    }
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Position and Pitch
    params:
      - { name: Position, desc: "Normalised read position in the audio file. This value is static and must be modulated externally to scan through the file.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Pitch, desc: "Playback speed ratio for each grain. 1.0 is original speed, 0.5 is an octave down, 2.0 is an octave up. The skewed range centres around 1.0.", range: "0.5 - 2.0", default: "1.0" }
  - label: Grain Control
    params:
      - { name: GrainSize, desc: "Duration of each grain. Shorter grains produce a more granular texture, longer grains sound smoother.", range: "20 - 800 ms", default: "80" }
      - { name: Density, desc: "Controls grain overlap. At 0, grains are spaced far apart. Higher values increase overlap, producing a denser texture with more simultaneous grains.", range: "0.0 - 1.0", default: "0.0" }
  - label: Randomisation
    params:
      - { name: Spread, desc: "Adds stereo panning randomisation and random offset to the grain start position within the file.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Detune, desc: "Adds random pitch variation to each grain. At 1.0, grains can vary by up to one octave in either direction.", range: "0.0 - 1.0", default: "0.0" }
---
::

### Setup

The granulator must be placed inside a [container.midichain]($SN.container.midichain$) to receive MIDI events. Without MIDI note-on messages, no grains are generated. Sustain pedal (CC#64) is supported -- held notes are released only after the pedal is lifted.

To connect the granulator's audio content to an AudioWaveform UI component (for drag-and-drop file loading and waveform display), set the node's audio file slot to **External** mode. Without this setting, the audio file is not accessible from outside the scriptnode network.

To load a sample map into the granulator from HiseScript, use the `{XYZ::SampleMap}` wildcard: `audioFile.loadFile("{XYZ::SampleMap}MySampleMap")`. The `{PROJECT_FOLDER}` wildcard works for single audio files but not for sample maps.

### Scanning textures

To create a scanning granular texture, modulate the Position parameter with an LFO or ramp. Without external modulation, all grains read from the same region of the file.

**See also:** $SN.core.file_player$ -- conventional file playback, $SN.core.stretch_player$ -- phase-vocoder time stretching, $MODULES.NoiseGrainPlayer$ -- module-tree granular effect using FFT-based noise decomposition
