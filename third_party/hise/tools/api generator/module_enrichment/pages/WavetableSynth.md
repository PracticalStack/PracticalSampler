---
title: Wavetable Synthesiser
moduleId: WavetableSynth
type: SoundGenerator
subtype: SoundGenerator
tags: [oscillator]
builderPath: b.SoundGenerators.WavetableSynth
screenshot: /images/v2/reference/audio-modules/wavetablesynth.png
cpuProfile:
  baseline: medium
  polyphonic: true
  scalingFactors: [voice count, HqMode]
seeAlso: []
commonMistakes:
  - title: "Wavetable cycle length must be exactly 2048 samples"
    wrong: "Loading an audio file with arbitrary cycle lengths and expecting correct wavetable conversion"
    right: "Ensure each cycle in the source audio is exactly 2048 samples. The total file length must be a multiple of 2048."
    explanation: "The converter derives the number of wavetables by dividing the file length by the cycle length. Non-multiples produce incorrect or corrupted wavetable data."
  - title: "HWT files are not auto-deployed"
    wrong: "Expecting .hwt files to be embedded in the compiled plugin automatically"
    right: "Include .hwt files in your installer and copy them to the user's AppData/AudioFiles folder"
    explanation: "HWT wavetable files are not embedded in the plugin binary. They must be distributed separately via an installer script or post-install copy step."
  - title: "Loading a new wavetable kills all voices"
    wrong: "Calling `setFile()` during playback and expecting a seamless transition"
    right: "Preload wavetables, or concatenate multiple wavetables into one file and use Table Index offset to switch between sections"
    explanation: "Calling `setFile()` triggers a full resynthesis that takes 1-2 seconds and kills all active voices. There is no seamless swap mechanism."
  - title: "LoadedBankIndex is 1-based"
    wrong: "Setting `LoadedBankIndex` to 0 to select the first wavetable bank"
    right: "Use index 1 for the first bank, 2 for the second, etc. Banks are sorted alphabetically."
    explanation: "The attribute uses 1-based indexing into an alphabetically sorted list of wavetable files. Use `Engine.getWavetableList()` to retrieve the available bank names."
forumReferences:
  - id: 1
    title: "setFile() kills all voices and takes 1-2 seconds"
    summary: "Calling setFile() on the AudioSampleProcessor reference triggers a full reload and resynthesis, causing an audible gap with no way to swap data without killing active voices."
    topic: 14027
  - id: 2
    title: "Wavetable format changed in v3.5 — older HWT files require reconversion"
    summary: "From v3.5, the wavetable synthesiser requires power-of-two cycle lengths; older .hwt files must be reconverted or HISE recompiled with USE_MOD2_WAVETABLESIZE=0."
    topic: 7923
  - id: 3
    title: "Unison + detune produces flanger artifacts due to fixed phase start"
    summary: "All unison voices start from the same wavetable phase position, so detuned unison produces comb-filtering instead of a proper spread; per-voice phase randomisation is not supported."
    topic: 5747
customEquivalent:
  approach: scriptnode
  moduleType: SoundGenerator
  complexity: high
  description: "A scriptnode wavetable oscillator with custom morphing and phase control. Requires manual wavetable loading and interpolation setup."
llmRef: |
  Wavetable Synthesiser (SoundGenerator)

  Two-dimensional wavetable synthesiser that morphs between waveforms using a table index. Supports audio file resynthesis into band-limited mip-mapped wavetables and precomputed .hwt files. Uses octave-based mip maps to eliminate aliasing at all pitches.

  Signal flow:
    MIDI note -> mip map selection (by octave) -> wavetable interpolation (by Table Index) -> gain modulation -> mono-to-stereo -> effect chain -> stereo out

  CPU: medium per voice, polyphonic. HqMode increases quality and CPU cost.

  Parameters:
    HqMode (Off/On, default On) - higher-quality interpolation at higher CPU cost
    LoadedBankIndex (-1 to 128, default 0) - 1-based index of the loaded wavetable bank (alphabetical order)
    TableIndexValue (0-100%, default 0%) - normalised wavetable morph position
    RefreshMipMap (Off/On, default Off) - refreshes the mip map when pitch modulation exceeds the current octave range
    Gain (0-100%, default 25%) - output volume
    Balance (-1 to 1, default 0) - stereo balance
    VoiceLimit (1-256, default 256) - maximum polyphony
    KillFadeTime (0-20000 ms, default 20 ms) - voice kill fade-out

  Modulation chains:
    Gain Modulation - scales the output volume
    Pitch Modulation - scales the pitch of all voices
    Table Index - scales the wavetable position (unipolar, multiplicative)
    Table Index Bipolar - adds a bipolar offset to the wavetable position

  Interfaces: WavetableController, AudioSampleProcessor, RoutingMatrix

  Key API:
    Synth.getWavetableController(processorId) - returns WavetableController for scripted access
    WavetableController.saveAsHwt(file) - precomputes and saves .hwt file
    Engine.getWavetableList() - returns available bank names

  Limitations:
    No hard sync (architectural constraint - destroys band-limiting).
    No per-voice phase randomisation (unison + detune produces flanger artifacts).
    setFile() kills all voices and takes 1-2 seconds.
    HWT files must be deployed separately (not embedded in plugin).
    Cycle length must be exactly 2048 samples.

  Common mistakes:
    Non-2048 cycle length -> corrupted wavetable.
    HWT files not deployed -> no sound in exported plugin.
    setFile() during playback -> voice kill and audible gap.
    LoadedBankIndex is 1-based, not 0-based.

  See also: none
---

::category-tags
---
tags:
  - { name: oscillator, desc: "Modules that generate audio or modulation signals from oscillators or synthesis algorithms" }
---
::

![Wavetable Synthesiser screenshot](/images/v2/reference/audio-modules/wavetablesynth.png)

The Wavetable Synthesiser plays back precomputed wavetable data with real-time morphing between waveforms. It resynthesises loaded audio files into band-limited mip maps (one per octave) to eliminate aliasing at all pitches, then interpolates between adjacent waveforms based on the Table Index position. This makes it suitable for evolving pads, morphing leads, and any sound design that benefits from smooth timbral transitions.

Wavetable data can be loaded from audio files (which are resynthesised on load) or from precomputed `.hwt` files (which load instantly). For production use, always precompute `.hwt` files using `WavetableController.saveAsHwt()` to avoid the 1-2 second resynthesis delay on load.

### Wavetable File Requirements

Each cycle in the source audio file must be exactly **2048 samples** long. The total file length must be an exact multiple of 2048. The converter derives the number of wavetable frames by dividing the file length by the cycle length - files that are not exact multiples produce incorrect results.

### HWT File Deployment

HWT files are **not embedded** in the compiled plugin. They must be copied to the user's AppData/AudioFiles folder via an installer script or post-install step. Without the HWT files present, the synthesiser produces no output in the exported plugin.

### Format Compatibility

The wavetable format changed to require power-of-two cycle lengths for better performance. Older `.hwt` files created before this change will not play correctly. Either reconvert the source audio using Resample mode, or add `USE_MOD2_WAVETABLESIZE=0` to the project's Extra Definitions (with a performance penalty). [2]($FORUM_REF.7923$)

## Signal Path

::signal-path
---
glossary:
  parameters:
    HqMode:
      desc: "Enables higher-quality interpolation"
      range: "Off / On"
      default: "On"
    LoadedBankIndex:
      desc: "1-based index of the loaded wavetable bank"
      range: "-1 - 128"
      default: "0"
    TableIndexValue:
      desc: "Normalised wavetable morph position"
      range: "0 - 100%"
      default: "0%"
    RefreshMipMap:
      desc: "Refreshes the mip map when pitch exceeds the current octave range"
      range: "Off / On"
      default: "Off"
  functions:
    mipMapSelect:
      desc: "Selects the appropriate band-limited mip map based on the note's octave"
    wavetableInterpolate:
      desc: "Linearly interpolates between adjacent wavetable frames based on the Table Index position"
  modulations:
    GainModulation:
      desc: "Scales the output volume per voice"
      scope: "per-voice"
    PitchModulation:
      desc: "Multiplies into the phase increment per sample"
      scope: "per-voice"
    TableIndex:
      desc: "Scales the wavetable position (unipolar, multiplicative)"
      scope: "per-voice"
    TableIndexBipolar:
      desc: "Adds a bipolar offset to the wavetable position"
      scope: "per-voice"
---

```
// Wavetable Synthesiser - per-voice processing
// polyphonic, one voice per note

// On note start
mipMap = mipMapSelect(noteNumber)    // select band-limited table for this octave
phase = startOffset / 441.0 * tableSize

// Per-sample generation
tablePos = clamp(TableIndexValue * TableIndex + TableIndexBipolar, 0, 1)
frameIndex = tablePos * (numFrames - 1)
lowerFrame = floor(frameIndex)
upperFrame = min(lowerFrame + 1, numFrames - 1)
alpha = frameIndex - lowerFrame

value = lerp(mipMap[lowerFrame][phase], mipMap[upperFrame][phase], alpha)
phase += phaseIncrement * PitchModulation

// Output
output = value * Gain * GainModulation
left = output
right = output    // mono copied to stereo
```

::

## Parameters

::parameter-table
---
groups:
  - label: Wavetable
    params:
      - name: LoadedBankIndex
        desc: "Index of the currently loaded wavetable bank. Banks are sorted alphabetically from the project's wavetable directory."
        range: "-1 - 128"
        default: "0"
        hints:
          - type: warning
            text: "**1-based indexing.** Use 1 for the first bank, 2 for the second, etc. Use `Engine.getWavetableList()` to retrieve available bank names."
      - name: TableIndexValue
        desc: "Normalised position in the wavetable where 0% is the first frame and 100% is the last. Modulatable via the Table Index and Table Index Bipolar chains."
        range: "0 - 100%"
        default: "0%"
      - { name: HqMode, desc: "Enables higher-quality rendering with better interpolation at the cost of more CPU usage.", range: "Off / On", default: "On" }
      - { name: RefreshMipMap, desc: "When enabled, updates the mip map selection when pitch modulation pushes the frequency outside the current octave range. Reduces aliasing for large pitch sweeps at additional CPU cost.", range: "Off / On", default: "Off" }
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
  - { name: "Gain Modulation", desc: "Scales the output volume. Applied as a per-voice multiply after wavetable generation.", scope: "per-voice", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Modulates the pitch of all voices. Applied per-sample as a multiplier on the phase increment.", scope: "per-voice", constrainer: "Any" }
  - name: "Table Index"
    desc: "Scales the wavetable morph position. Multiplicative (unipolar). To expose the table position as a knob, add a Constant Modulator to this chain and connect its Intensity to a GUI slider."
    scope: "per-voice"
    constrainer: "Any"
    hints:
      - type: tip
        text: "Add a **Constant Modulator** to this chain and connect its Intensity to a slider for a simple wavetable morph knob."
  - { name: "Table Index Bipolar", desc: "Adds a bipolar offset to the wavetable morph position. Use this for modulation-driven morphing (e.g. LFO sweep, velocity-to-table-position).", scope: "per-voice", constrainer: "Any" }
---
::

### Hard Sync

Hard sync is not supported by the Wavetable Synthesiser. The band-limited mip map architecture that eliminates aliasing is fundamentally incompatible with hard sync, which resets the phase mid-cycle and destroys the band-limiting guarantees. This is an architectural constraint, not a missing feature.

### Unison and Detune

When using unison voices with detune, all voices start from the same wavetable phase position. There is no per-voice phase randomisation, which causes the detuned voices to produce a flanger-like comb filtering effect instead of the expected thick unison spread. [3]($FORUM_REF.5747$) For proper unison with randomised phase, use a scriptnode wavetable implementation.

### Precomputing HWT Files

For production workflows, precompute `.hwt` files from your source audio to eliminate the 1-2 second resynthesis delay:

```javascript
// Precompute and save an HWT file
const var wt = Synth.getWavetableController("WavetableSynth1");
wt.saveAsHwt(FileSystem.getFolder(FileSystem.AudioFiles).getChildFile("MyWavetable.hwt"));
```

### Instant Wavetable Switching

Since `setFile()` kills all voices and takes 1-2 seconds, [1]($FORUM_REF.14027$) a workaround for switching between multiple wavetables during playback is to concatenate them into a single file and control which section plays by changing the Table Index offset. Use the Table Index Bipolar modulation source to sweep the desired portion of the combined wavetable.
