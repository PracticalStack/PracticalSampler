---
title: Audio Loop Player
moduleId: AudioLooper
type: SoundGenerator
subtype: SoundGenerator
tags: [sample_playback, sequencing]
builderPath: b.SoundGenerators.AudioLooper
screenshot: /images/v2/reference/audio-modules/audiolooper.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: [voice count]
seeAlso:
  - modules:StreamingSampler
  - { id: "core.file_player", type: scriptnode, reason: "Scriptnode file playback with polyphonic voice handling" }
  - { id: "core.stretch_player", type: scriptnode, reason: "Scriptnode file playback with time stretching and pitch shifting" }
commonMistakes:
  - title: "Automatic pitch detection can be inaccurate"
    wrong: "Trusting the auto-detected RootNote value after loading a file"
    right: "Always verify and manually correct the RootNote if pitch tracking sounds wrong"
    explanation: "The pitch detection algorithm that runs on file load can produce incorrect results. Set the RootNote manually if the playback pitch is off."
  - title: "AudioLooper is a sound generator, not an effect"
    wrong: "Using an AudioLooper in an FX plugin project and expecting audio output"
    right: "Enable **Sound Generators FX** in Project Preferences, or use an instrument plugin type instead"
    explanation: "The AudioLooper generates audio rather than processing incoming audio. FX plugin builds require the Sound Generators FX setting to be enabled."
forumReferences:
  - id: 1
    title: "Pitch detection frequently off by ~10 semitones"
    summary: "The automatic root-note pitch detection that runs on file load frequently produces incorrect results, with a consistent 10-semitone offset reported across multiple users."
    topic: 5587
  - id: 2
    title: "RootNote stops auto-updating after first manual change"
    summary: "Once RootNote has been set to any value other than the default (64), the auto-detection on file load stops updating it — intentional protection against overwriting preset values, but surprising when you expect detection to always run."
    topic: 8741
customEquivalent:
  approach: scriptnode
  moduleType: SoundGenerator
  complexity: medium
  description: "A scriptnode file player with custom loop logic and tempo sync for scenarios requiring more control than the built-in module."
llmRef: |
  Audio Loop Player (SoundGenerator)

  Single-file polyphonic audio player with looping, tempo sync, pitch tracking, reverse playback, and random sample start. Plays one audio file per instance, loaded via the AudioSampleProcessor interface.

  Signal flow:
    MIDI note -> file playback (from SampleStartMod offset) -> pitch tracking -> tempo sync -> gain modulation -> stereo -> effect chain -> audio out

  CPU: low per voice, polyphonic.

  Parameters:
    SyncMode (Free running / 1 Beat / 2 Beats / 1 Bar / 2 Bars / 4 Bars / 8 Bars / 12 Bars / 16 Bars) - tempo sync division
    LoopEnabled (Off/On, default Off) - continuous looping
    PitchTracking (Off/On, default Off) - transpose relative to RootNote
    RootNote (0-127, default 64) - original pitch reference note
    SampleStartMod (0-20000 samples, default 0) - random start offset range per note
    Reversed (Off/On, default Off) - reverse playback
    Gain (0-100%, default 25%) - output volume
    Balance (-1 to 1, default 0) - stereo balance
    VoiceLimit (1-256, default 256) - maximum polyphony
    KillFadeTime (0-20000 ms, default 20 ms) - voice kill fade-out

  Modulation chains:
    Gain Modulation - scales the output volume
    Pitch Modulation - scales the pitch of all voices

  Interfaces: AudioSampleProcessor, RoutingMatrix

  Key API:
    AudioSampleProcessor.setFile(path) - loads an audio file
    AudioSampleProcessor.getSampleLength() - returns sample length (0 if no file loaded)
    AudioFile.setContentCallback(fn) - callback when file content changes

  When to use (vs Sampler):
    AudioLooper: single audio file, loop playback, tempo-synced loops, simple pitch-shifted playback, background textures.
    Sampler: multiple samples mapped across keys/velocities, round-robin, crossfade groups, multi-mic, disk streaming, complex sample organisation.

  See also:
    StreamingSampler
    [scriptnode] core.file_player -- scriptnode file playback with polyphonic voice handling
    [scriptnode] core.stretch_player -- scriptnode file playback with time stretching and pitch shifting
---

::category-tags
---
tags:
  - { name: sample_playback, desc: "Modules that play back audio samples from disk or memory" }
  - { name: sequencing, desc: "Modules related to sequencing, looping, or tempo-synced playback" }
---
::

![Audio Loop Player screenshot](/images/v2/reference/audio-modules/audiolooper.png)

The Audio Loop Player plays a single audio file with optional looping, tempo synchronisation, pitch tracking, and reverse playback. It loads the entire file into memory (no disk streaming), making it suitable for loops, one-shots, background textures, and any scenario where a single audio file needs to be triggered polyphonically from MIDI.

Unlike the [Sampler]($MODULES.StreamingSampler$), which organises many samples in a sample map with key/velocity mapping and disk streaming, the Audio Loop Player is designed for simplicity: one file, loaded into RAM, played back with minimal configuration.

### When to Use the Audio Loop Player vs the Sampler

| | Audio Loop Player | Sampler |
|---|---|---|
| **Use case** | Single loops, one-shots, background textures | Multi-sample instruments, mapped libraries |
| **File handling** | One file per instance, fully loaded into RAM | Many files per sample map, disk-streamed |
| **Mapping** | No key/velocity mapping | Full key, velocity, and group mapping |
| **Tempo sync** | Built-in SyncMode with beat/bar divisions | Requires timestretching configuration |
| **Looping** | Simple on/off with adjustable loop range | Per-sample loop points in the sample map |
| **Pitch tracking** | Single RootNote reference | Per-sample root notes in the map |
| **Sample start** | Random offset per note (SampleStartMod) | Modulatable offset with dedicated modulation chain |
| **Multi-mic** | Not supported | Full multi-mic with routing matrix |
| **Round-robin** | Not supported | Round-robin groups, Complex Group Manager |
| **Memory** | Entire file in RAM | Preload buffer + disk streaming |

If you need fine-grained sample start control, multiple samples mapped across keys, or disk streaming for large libraries, use the Sampler instead.

### FX Plugin Builds

The Audio Loop Player is a sound generator, not an audio effect. It produces its own audio rather than processing incoming signal. To use it in an FX plugin, enable **Sound Generators FX** in Project Preferences before compiling.

## Signal Path

::signal-path
---
glossary:
  parameters:
    SyncMode:
      desc: "Tempo sync division for playback speed"
      range: "Free running / 1 Beat - 16 Bars"
      default: "Free running"
    LoopEnabled:
      desc: "Enables continuous looping"
      range: "Off / On"
      default: "Off"
    PitchTracking:
      desc: "Transpose playback relative to RootNote"
      range: "Off / On"
      default: "Off"
    RootNote:
      desc: "MIDI note at which the file plays at original pitch"
      range: "0 - 127"
      default: "64"
    SampleStartMod:
      desc: "Random start offset range per note"
      range: "0 - 20000 samples"
      default: "0"
    Reversed:
      desc: "Reverse playback direction"
      range: "Off / On"
      default: "Off"
  functions:
    filePlayback:
      desc: "Reads from the in-memory audio buffer at the calculated playback rate"
  modulations:
    GainModulation:
      desc: "Scales the output volume per voice"
      scope: "per-voice"
    PitchModulation:
      desc: "Multiplies the playback speed"
      scope: "per-voice"
---

```
// Audio Loop Player - per-voice processing
// polyphonic, fully loaded into RAM

// On note-on
startOffset = random(0, SampleStartMod)

// Per-block generation
if PitchTracking:
    playbackRate = 2^((noteNumber - RootNote) / 12) * PitchModulation
else:
    playbackRate = 1.0 * PitchModulation

if SyncMode != FreeRunning:
    playbackRate *= syncFactor    // calculated from file length and tempo

output = filePlayback(startOffset, playbackRate, Reversed)

// Loop back to start when reaching the end (if LoopEnabled)
if LoopEnabled and position >= loopEnd:
    position = loopStart

output *= Gain * GainModulation
```

::

## Parameters

::parameter-table
---
groups:
  - label: Playback
    params:
      - { name: SyncMode, desc: "Syncs the playback length to the host tempo at the selected note division. The playback speed is adjusted so that the file completes in exactly the specified number of beats or bars.", range: "Free running / 1 Beat / 2 Beats / 1 Bar / 2 Bars / 4 Bars / 8 Bars / 12 Bars / 16 Bars", default: "Free running" }
      - { name: LoopEnabled, desc: "Enables continuous looping. The loop range can be adjusted in the audio waveform editor.", range: "Off / On", default: "Off" }
      - { name: Reversed, desc: "Reverses the playback direction of the audio file.", range: "Off / On", default: "Off" }
  - label: Pitch
    params:
      - { name: PitchTracking, desc: "Transposes playback pitch based on the MIDI note relative to the RootNote. When Off, all notes play the file at its original pitch.", range: "Off / On", default: "Off" }
      - name: RootNote
        desc: "The MIDI note number at which the file plays at its original pitch. When a file is loaded, an automatic pitch detection algorithm attempts to set this value. Once you manually change RootNote away from its default (64), auto-detection stops updating it on subsequent file loads — this protects preset values but can be surprising when swapping files. [2]($FORUM_REF.8741$)"
        range: "0 - 127"
        default: "64"
        hints:
          - type: warning
            text: "The automatic pitch detection can be inaccurate — multiple users have reported consistent offsets of around 10 semitones. Always verify the RootNote manually if pitch tracking sounds wrong. [1]($FORUM_REF.5587$)"
  - label: Sample Start
    params:
      - name: SampleStartMod
        desc: "Random sample start offset range in samples. Each new note begins playback at a random position between 0 and this value, adding natural variation to repeated triggers."
        range: "0 - 20000 samples"
        default: "0"
        hints:
          - type: warning
            text: "Keep this value well below the loaded file's sample count. Very short files with a large offset can cause issues."
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
  - { name: "Gain Modulation", desc: "Scales the output volume. Applied as a per-voice multiply after file playback.", scope: "per-voice", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Modulates the pitch (playback speed) of all voices. Applied per-sample as a multiplier on the playback rate.", scope: "per-voice", constrainer: "Any" }
---
::

### Scripting Access

The Audio Loop Player exposes the `AudioSampleProcessor` interface for loading files and querying state:

```javascript
const var looper = Synth.getChildSynth("AudioLooper1");
const var asp = Synth.getAudioSampleProcessor("AudioLooper1");

// Load an audio file
asp.setFile("{PROJECT_FOLDER}Loops/DrumLoop.wav");

// Check if a file is loaded
if (asp.getSampleLength() > 0)
    Console.print("File loaded");

// Trigger playback from script
Synth.playNote(64, 127);
```

To create a play/stop button, place a Script Processor inside the Audio Loop Player's MIDI chain and use `Synth.playNote()` / `Synth.noteOffByEventId()`. Events generated in a child chain only affect that generator, so no note filtering is needed.

### Tempo Sync

When SyncMode is set to a beat or bar division, the playback speed is adjusted so that the file completes in exactly the specified duration at the current host tempo. The sync factor is recalculated automatically when the host tempo changes.

For best results, load files whose natural length matches the selected sync division at a common tempo. Large tempo deviations will stretch the playback significantly, which can affect audio quality since this is simple speed adjustment (not timestretching). For tempo-independent playback, use the [Sampler]($MODULES.StreamingSampler$) with timestretching enabled.

**See also:** $SN.core.file_player$ -- scriptnode file playback with polyphonic voice handling, $SN.core.stretch_player$ -- scriptnode file playback with time stretching and pitch shifting
