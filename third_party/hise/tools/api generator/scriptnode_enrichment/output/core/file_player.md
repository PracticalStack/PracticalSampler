---
title: File Player
description: "A polyphonic file player with three playback modes: static, signal-driven, and MIDI pitch-tracked."
factoryPath: core.file_player
factory: core
polyphonic: true
tags: [core, file-player, sample, playback, polyphonic]
screenshot: /images/v2/reference/scriptnodes/core/file_player.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: "PlaybackMode", impact: low, note: "SignalInput mode uses a different interpolation path" }
forumReferences:
  - { tid: 13579, reason: "Audio file embedding for export using {PROJECT_FOLDER} wildcard" }
  - { tid: 13406, reason: "Accessing multiple audio file slots via ScriptFX.getAudioFile(index)" }
seeAlso:
  - { id: "core.stretch_player", type: alternative, reason: "File player with independent time stretching and pitch shifting" }
  - { id: "core.granulator", type: alternative, reason: "Granular playback from an audio file" }
  - { id: "core.recorder", type: companion, reason: "Records audio into the same audio file slot" }
  - { id: "AudioLooper", type: module, reason: "Module-tree audio file player with looping and tempo sync" }
commonMistakes:
  - title: "Output mode differs between playback modes"
    wrong: "Expecting consistent additive or replacing behaviour across all three modes"
    right: "Static and MIDI modes add to the existing signal. Signal Input mode replaces it."
    explanation: "In Static and MIDI Frequency modes, the file output is added to the existing audio buffer. In Signal Input mode, the output replaces whatever is already in the buffer."
  - title: "Position must be driven externally in Signal Input mode"
    wrong: "Selecting Signal Input mode and expecting automatic playback"
    right: "Feed a ramp signal (0 to 1 over the sample duration) into the input to get standard playback in Signal Input mode."
    explanation: "In Signal Input mode, channel 0 of the input signal is used as a normalised read position into the file. Without an external signal driving it, no meaningful playback occurs."
  - title: "Audio files loaded from filesystem paths are not embedded in compiled plugins"
    wrong: "Loading audio files using a raw filesystem path and expecting them to be available in the exported plugin"
    right: "Call Engine.loadAudioFilesIntoPool() and reference files with the {PROJECT_FOLDER} wildcard to ensure they are embedded in the plugin binary."
    explanation: "Files loaded via an absolute filesystem path work during development but are not bundled into the exported plugin. Only files in the audio pool referenced with {PROJECT_FOLDER} are embedded."
llmRef: |
  core.file_player

  A polyphonic file player with three playback modes: Static (fixed pitch, gate-triggered), Signal Input (input signal drives read position), and MIDI Frequency (pitch-tracked from MIDI notes).

  Signal flow:
    Static/MIDI: audio file -> interpolated read -> += audio out (additive)
    Signal Input: input signal -> position lookup -> = audio out (replacing)

  CPU: low, polyphonic

  Parameters:
    PlaybackMode (Static / Signal Input / MIDI, default Static): selects playback behaviour
    Gate (Off / On, default On): starts/stops playback, resets position on transition to On
    RootFrequency (20 - 2000 Hz, default 440): reference pitch for MIDI frequency mode
    FreqRatio (0.0 - 2.0, default 1.0): playback speed multiplier

  When to use:
    Occasionally used (rank 91, 2 instances). Use for sample playback, wavetable-style position scrubbing, or pitch-tracked instrument building. For time-stretched playback, use core.stretch_player. For granular textures, use core.granulator.

  Common mistakes:
    Output is additive in Static/MIDI modes but replacing in Signal Input mode.
    Signal Input mode requires an external signal to drive the read position.
    Files loaded from filesystem paths are not embedded in compiled plugins -- use {PROJECT_FOLDER} wildcard.

  Forum references: tid:13579 (audio file embedding for export), tid:13406 (multiple audio file slots via getAudioFile)

  See also:
    [alternative] core.stretch_player -- time-stretched file playback
    [alternative] core.granulator -- granular playback from audio file
    [companion] core.recorder -- records into the same audio file slot
    [module] AudioLooper -- module-tree audio file player with looping and tempo sync
---

![Node screenshot](/images/custom/scriptnode/file_player.png)

This node plays audio from an audio file slot with three distinct playback modes. It supports polyphonic operation, with each voice maintaining its own playback position and pitch state.

The three playback modes offer different approaches to file playback:

- **Static**: plays the file at a fixed rate controlled by FreqRatio. The Gate parameter triggers playback and resets the position. The file loops using the audio file's loop range settings.
- **Signal Input**: uses channel 0 of the input audio as a normalised position (0 to 1) into the file. This allows wavetable-style scrubbing where an external signal (such as a [core.ramp]($SN.core.ramp$)) drives the read position. The output replaces the input signal rather than adding to it.
- **MIDI Frequency**: pitch-tracks incoming MIDI note events. The playback speed is calculated from the ratio of the note frequency to RootFrequency, multiplied by FreqRatio. Supports velocity and note-based sample layer selection when using multisample content.

In Static and MIDI Frequency modes, the file output is added to the existing audio buffer. In Signal Input mode, the output replaces it.

## Signal Path

::signal-path
---
glossary:
  parameters:
    PlaybackMode:
      desc: "Selects the playback behaviour"
      range: "Static / Signal Input / MIDI"
      default: "Static"
    Gate:
      desc: "Starts or stops playback"
      range: "Off / On"
      default: "On"
    RootFrequency:
      desc: "Reference frequency for pitch calculation in MIDI mode"
      range: "20 - 2000 Hz"
      default: "440"
    FreqRatio:
      desc: "Playback speed multiplier"
      range: "0.0 - 2.0"
      default: "1.0"
  functions:
    readInterpolated:
      desc: "Reads from the audio file with linear interpolation and loop wrapping"
---

```
// core.file_player - multi-mode file playback
// audio file -> audio out

process(input) {
    if (PlaybackMode == Static) {
        output += readInterpolated(position)
        position += FreqRatio
    }
    else if (PlaybackMode == SignalInput) {
        position = input[0]             // channel 0 as normalised position
        output = readInterpolated(position)
    }
    else if (PlaybackMode == MIDI) {
        speed = noteFrequency / RootFrequency * FreqRatio
        output += readInterpolated(position)
        position += speed
    }
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Playback
    params:
      - { name: PlaybackMode, desc: "Selects the playback mode. Static plays at a fixed rate, Signal Input uses the input as a read position, MIDI tracks incoming note pitch.", range: "Static / Signal Input / MIDI", default: "Static" }
      - { name: Gate, desc: "Starts and stops playback. Transition to On resets the playback position to the start.", range: "Off / On", default: "On" }
  - label: Pitch
    params:
      - { name: RootFrequency, desc: "Reference frequency for MIDI pitch tracking. Only affects MIDI Frequency mode.", range: "20 - 2000 Hz", default: "440" }
      - { name: FreqRatio, desc: "Playback speed multiplier applied on top of any pitch tracking.", range: "0.0 - 2.0", default: "1.0" }
---
::

### Signal Input workflow

In Signal Input mode, feed a [core.ramp]($SN.core.ramp$) generator outputting 0 to 1 over the sample duration for standard linear playback. From there you can apply waveshaping or modulation to the position signal for creative effects.

### Gate and MIDI interaction

The Gate parameter and MIDI note-on events are independent triggers. In MIDI Frequency mode, note-on events override the playback speed with the pitch ratio but Gate must still be On for playback to occur.

### Loading audio files for export

Audio files must be loaded into the pool and referenced with the `{PROJECT_FOLDER}` wildcard to be embedded in the compiled plugin. Call `Engine.loadAudioFilesIntoPool()` during initialisation, then use `audioFile.loadFile("{PROJECT_FOLDER}myfile.wav")`. Files loaded via a raw filesystem path work during development but will not be found in the exported binary.

To access multiple file_player slots from HiseScript, use `ScriptFX.getAudioFile(index)` where the index is zero-based in the order the slots were created. Create all references in `onInit`, not inside control callbacks.

### Maturity

This node was originally implemented as an internal testing tool and may exhibit inconsistent looping behaviour across different DAWs. For production use cases requiring robust file playback, consider the module-tree [AudioLooper]($MODULES.AudioLooper$) or [core.stretch_player]($SN.core.stretch_player$) as alternatives.

**See also:** $SN.core.stretch_player$ -- file playback with independent time stretching and pitch shifting, $SN.core.granulator$ -- granular playback from an audio file, $SN.core.recorder$ -- records audio into the same file slot, $MODULES.AudioLooper$ -- module-tree audio file player with looping and tempo sync
