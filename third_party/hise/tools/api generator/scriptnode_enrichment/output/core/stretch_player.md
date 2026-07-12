---
title: Stretch Player
description: "A stereo file player with independent time stretching and pitch shifting, plus optional DAW tempo sync."
factoryPath: core.stretch_player
factory: core
polyphonic: true
tags: [core, stretch, timestretch, pitch-shift, file-player, tempo-sync]
cpuProfile:
  baseline: high
  polyphonic: true
  scalingFactors:
    - { parameter: "Enable", impact: high, note: "Timestretching is significantly more expensive than direct playback" }
seeAlso:
  - { id: "core.file_player", type: alternative, reason: "Simpler file player without timestretching" }
  - { id: "core.granulator", type: alternative, reason: "Granular approach to time manipulation" }
  - { id: "AudioLooper", type: module, reason: "Module-tree audio file player with looping and tempo sync" }
commonMistakes:
  - title: "High CPU with multiple polyphonic voices"
    wrong: "Using core.stretch_player in a polyphonic context with many voices"
    right: "Limit the voice count or disable timestretching (Enable = Off) for voices that do not need it."
    explanation: "Each polyphonic voice runs its own independent time stretcher instance. The CPU cost scales linearly with the number of active voices."
  - title: "ClockSync overrides TimeRatio"
    wrong: "Adjusting TimeRatio while ClockSync is enabled and expecting it to take effect"
    right: "Disable ClockSync to use manual TimeRatio control."
    explanation: "When ClockSync is active, the playback speed is derived from the DAW tempo relative to the file's detected tempo. The TimeRatio parameter is ignored."
llmRef: |
  core.stretch_player

  A stereo file player with independent time stretching and pitch shifting. Can sync playback speed and position to DAW tempo. Fixed to 2 channels.

  Signal flow:
    audio file -> read samples -> time stretcher (when enabled) -> audio out (stereo)
    audio file -> read samples -> interpolated playback (when disabled) -> audio out (stereo)

  CPU: high (stretching enabled), low (stretching disabled), polyphonic

  Parameters:
    Gate (Off / On, default On): starts/stops playback, seeks to start on transition
    TimeRatio (0.5 - 2.0, default 1.0): playback speed multiplier (overridden by ClockSync)
    Pitch (-12 - 12 semitones, default 0): pitch shift independent of time
    Enable (Off / On, default On): toggles timestretching on/off
    ClockSync (Off / On, default Off): syncs speed and position to DAW transport

  When to use:
    Unused in surveyed networks. Use when you need to play audio files at different speeds without changing pitch, or shift pitch without changing speed. Ideal for tempo-synced loops and atmospheric textures.

  Common mistakes:
    High CPU with multiple polyphonic voices -- each voice has its own stretcher.
    ClockSync overrides TimeRatio -- disable it for manual speed control.

  See also:
    [alternative] core.file_player -- simpler file player without timestretching
    [alternative] core.granulator -- granular time manipulation
    [module] AudioLooper -- module-tree audio file player with looping and tempo sync
---

This node plays back stereo audio files with the ability to change playback speed and pitch independently. Time stretching and pitch shifting are handled by a phase-vocoder algorithm that preserves audio quality across a wide range of settings. The node is fixed to stereo (2 channels).

When timestretching is enabled (the default), the time stretcher processes audio in the frequency domain to separate timing from pitch. The TimeRatio parameter controls playback speed (1.0 = original, 0.5 = half speed, 2.0 = double speed) while the Pitch parameter shifts pitch in semitones without affecting duration.

When timestretching is disabled via the Enable parameter, the node falls back to simple interpolated playback, similar to [core.file_player]($SN.core.file_player$) in Static mode. This dramatically reduces CPU usage.

The ClockSync feature automatically derives the playback speed from the DAW tempo, matching the file's detected tempo to the current project tempo. It also synchronises the playback position to the DAW transport, so the file stays aligned with the timeline. When ClockSync is active, the TimeRatio parameter is overridden.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gate:
      desc: "Starts or stops playback"
      range: "Off / On"
      default: "On"
    TimeRatio:
      desc: "Playback speed multiplier"
      range: "0.5 - 2.0"
      default: "1.0"
    Pitch:
      desc: "Pitch shift in semitones, independent of time"
      range: "-12 - 12 st"
      default: "0"
    Enable:
      desc: "Toggles timestretching on or off"
      range: "Off / On"
      default: "On"
    ClockSync:
      desc: "Syncs speed and position to DAW tempo"
      range: "Off / On"
      default: "Off"
  functions:
    readSamples:
      desc: "Reads source samples from the audio file at the current position"
    timeStretch:
      desc: "Phase-vocoder time stretching with independent pitch control"
    interpolate:
      desc: "Hermite interpolation for direct playback without stretching"
---

```
// core.stretch_player - time-stretched file playback
// audio file -> audio out (stereo)

process() {
    samples = readSamples(position, TimeRatio)

    if (Enable == On) {
        output = timeStretch(samples, TimeRatio, Pitch)
    } else {
        output = interpolate(samples)
    }

    // if ClockSync is On, TimeRatio is derived from DAW tempo
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Playback
    params:
      - { name: Gate, desc: "Starts and stops playback. Transition to On seeks to the beginning of the file.", range: "Off / On", default: "On" }
      - { name: Enable, desc: "Toggles timestretching. When Off, plays back with simple interpolation at much lower CPU cost.", range: "Off / On", default: "On" }
  - label: Time and Pitch
    params:
      - { name: TimeRatio, desc: "Playback speed multiplier. 1.0 is original speed, 0.5 is half speed, 2.0 is double speed. Overridden when ClockSync is active. The skewed range centres around 1.0.", range: "0.5 - 2.0", default: "1.0" }
      - { name: Pitch, desc: "Pitch shift in semitones, independent of playback speed.", range: "-12 - 12 st", default: "0" }
  - label: Sync
    params:
      - { name: ClockSync, desc: "Syncs playback speed and position to the DAW transport. Overrides TimeRatio with a tempo-derived value.", range: "Off / On", default: "Off" }
---
::

### Limitations

The node is fixed to stereo output. If placed in a mono context, it produces no output.

### Playback control

MIDI note events do not directly trigger playback. Use the Gate parameter (driven by a control node or automation) to start and stop playback. In a polyphonic context, each voice maintains its own playback position and stretcher state.

### Tempo detection

When ClockSync is enabled, the node auto-detects the source file's tempo by estimating the quarter-note count from the file duration. This works best with loop-based material that has a clear rhythmic structure.

**See also:** $SN.core.file_player$ -- simpler file player without timestretching, $SN.core.granulator$ -- granular approach to time manipulation, $MODULES.AudioLooper$ -- module-tree audio file player with looping and tempo sync
