---
title: Sampler
moduleId: StreamingSampler
type: SoundGenerator
subtype: SoundGenerator
tags: [sample_playback]
builderPath: b.SoundGenerators.StreamingSampler
screenshot: /images/v2/reference/audio-modules/streamingsampler.png
cpuProfile:
  baseline: medium
  polyphonic: true
  scalingFactors: [voice count, streaming buffer size, sample count]
seeAlso:
  - $MODULES.AudioLooper$
commonMistakes:
  - title: "Sample Start modulation requires sufficient SampleStartMod range"
    wrong: "Adding a Sample Start modulator but hearing no variation"
    right: "Set each sample's `SampleStartMod` property in the sample map to define the modulation range. Increase `PreloadSize` if you need a larger range."
    explanation: "The Sample Start modulation chain offsets playback within each sample's SampleStartMod range. If SampleStartMod is 0 (default), the modulator has no effect. The preload buffer is automatically extended to cover the SampleStartMod range."
  - title: "VoiceAmount controls streaming buffer allocation"
    wrong: "Leaving VoiceAmount at 256 when the patch only needs 16 voices"
    right: "Set VoiceAmount to the actual polyphony you need"
    explanation: "Each voice allocates two streaming buffers. Unlike other sound generators where voice overhead is negligible, the Sampler's per-voice memory cost is significant. Unused voices waste RAM."
  - title: "Monolith format supports only four sample rates"
    wrong: "Building a monolith from samples at 22050 Hz, 32000 Hz, or other non-standard rates"
    right: "Ensure all samples use 44100, 48000, 88200, or 96000 Hz before building the monolith"
    explanation: "The monolith format stores sample rate as a 2-bit index. Non-standard rates silently default to 44100 Hz during encoding, causing pitch and timing errors on playback."
  - title: "Default AHDSR has non-zero attack"
    wrong: "Wondering why samples sound softer or duller after adding a Sampler"
    right: "Check the auto-created AHDSR envelope in the Gain chain and set its attack to 0 if needed"
    explanation: "HISE automatically adds an AHDSR envelope to every new Sampler's Gain chain. Its attack time defaults to a non-zero value, which softens the onset of every sample."
  - title: "Loading sample maps during playback is unsafe"
    wrong: "Calling `Sampler.loadSampleMap()` while notes are playing"
    right: "Schedule sample map changes during silence, or use a crossfade between two Sampler instances"
    explanation: "Swapping sample maps is not a real-time operation. Doing so during playback can cause glitches or crashes."
forumReferences:
  - id: 1
    title: "Global Modulator Container must precede the Sampler in the module tree"
    summary: "Unlike other synths, the Sampler requires the Global Modulator Container to appear before it in the module tree for Global Time Variant Modulators to work."
    topic: 7177
  - id: 2
    title: "Minimum 2048-sample preload enforced internally"
    summary: "The Sampler enforces a minimum preload buffer of 2048 samples; earlier versions would crash on very short files (e.g., single-cycle waveforms) before this guard was added."
    topic: 6623
  - id: 3
    title: "Sampler.ID constant is broken — use integer 0 instead"
    summary: "The Sampler.ID constant throws a runtime error; use the integer 0 as a substitute (e.g., Sample.get(0) instead of Sample.get(Sampler.ID))."
    topic: 3598
customEquivalent:
  approach: scriptnode
  moduleType: SoundGenerator
  complexity: high
  description: "A scriptnode-based sample player for scenarios requiring many independent channels (e.g., drum machines) where per-instance streaming buffer overhead is prohibitive."
llmRef: |
  Sampler (SoundGenerator)

  Disk-streaming polyphonic sampler with sample maps, round-robin groups, crossfade groups, timestretching, multi-mic support, and release start. The core sample playback module in HISE.

  Signal flow:
    MIDI note -> sample map lookup (key/velocity/RR group) -> sample start offset -> disk streaming -> pitch tracking -> timestretching (if enabled) -> gain modulation -> mono/stereo -> effect chain -> audio out

  CPU: medium per voice, polyphonic. Dominated by disk I/O and streaming buffer management. Timestretching adds significant CPU cost.

  Parameters:
    PreloadSize (-1 to 65536 samples, default 8192) - RAM preload buffer per sample. -1 loads entire sample.
    BufferSize (512-65536 samples, default 4096) - streaming buffer size per voice (two buffers swapped)
    VoiceAmount (1-256, default 256) - number of allocated streaming voice slots. Each allocates two buffers.
    RRGroupAmount (1-128, default 1) - number of round-robin groups (also used for velocity layers/articulations)
    SamplerRepeatMode (Kill Note / Note off / Do nothing / Kill Duplicate / Kill Third) - retrigger behaviour
    PitchTracking (Off/On, default On) - transpose playback relative to root note
    OneShot (Off/On, default Off) - ignore note-off, play full sample
    CrossfadeGroups (Off/On, default Off) - play all RR groups simultaneously with crossfade
    Purged (Disabled/Enabled/Lazy Load, default Disabled) - memory management mode
    Reversed (Off/On, default Off) - reverse playback (loads full sample into RAM)
    UseStaticMatrix (Off/On, default Off) - prevents routing matrix resize on sample map change
    LowPassEnvelopeOrder (0-48, step 6, default 0) - envelope follower filter order
    Gain (0-100%, default 100%) - output volume
    Balance (-1 to 1, default 0) - stereo balance
    VoiceLimit (1-256, default 256) - maximum polyphony
    KillFadeTime (0-20000 ms, default 20 ms) - voice kill fade-out

  Modulation chains:
    Gain Modulation - scales the output volume
    Pitch Modulation - scales the pitch of all voices
    Sample Start - offsets playback start within each sample's SampleStartMod range (voice-start only)
    Group Fade - crossfades between round-robin groups when CrossfadeGroups is enabled

  Interfaces: Sampler, RoutingMatrix, TableProcessor

  Sample map data model:
    Samples organised in 3D: note number (x) x velocity (y) x RR group (z).
    Per-sample properties: Root, LoKey, HiKey, LoVel, HiVel, RRGroup, Volume, Pan, Pitch, SampleStart, SampleEnd, SampleStartMod, LoopStart, LoopEnd, LoopXFade, LoopEnabled, ReleaseStart, LowerVelocityXFade, UpperVelocityXFade, SampleState (Normal/Disabled/Purged), Reversed, GainTable, PitchTable.

  Multi-mic support:
    Each sample can have multiple mic positions (separate audio files). All mic positions play simultaneously per voice. Each mic maps to a routing matrix channel. UseStaticMatrix prevents resize on sample map change. Mic positions defined as semicolon-separated string in the sample map.

  Release Start (HISE 4.1.0+):
    Reuses the sustain sample's natural decay as a release tail. Per-sample ReleaseStart property defines where playback jumps on note-off. Options: ReleaseFadeTime (samples), FadeGamma (curve shape), UseAscendingZeroCrossing (phase alignment), GainMatchingMode (None/Volume/Offset), PeakSmoothing.

  Timestretching (HISE 3.5.1+):
    4 modes: Disabled, VoiceStart (static ratio per voice), TimeVariant (dynamic ratio for all voices), TempoSynced (auto BPM sync).
    Options: Mode, SkipLatency (true=zero latency but CPU spikes, false=~270 sample latency), NumQuarters (for tempo sync), Tonality (timbre retention 0-1).
    Uses Signalsmith algorithm.

  SFZ import:
    Supported opcodes: sample, lokey, hikey, lovel, hivel, offset, end, loop_mode (loop_continuous only), loop_start, loop_end, tune, pitch_keycenter, volume, group_volume, pan, key, default_path, lorand/hirand, seq_length/seq_position. Release trigger opcodes not supported.

  Complex Group Manager:
    Alternative to standard RR groups. Uses layers with different logic types: Custom, RoundRobin, Keyswitch, TableFade, XFade, LegatoInterval, ReleaseTrigger, Choke. Multiple layers can be combined for multidimensional sample organisation.

  Key API:
    Synth.getSampler(processorId) - returns Sampler reference for scripted access
    Sampler.loadSampleMap(mapName) - loads a sample map by name
    Sampler.createSelection(regex) - selects samples matching regex pattern
    Sampler.setActiveGroup(groupIndex) - activates a single RR group
    Sampler.enableRoundRobin(enable) - enables/disables automatic round-robin cycling

  Limitations:
    Monolith format supports only 44100, 48000, 88200, and 96000 Hz.
    Sample map loading is not real-time safe - schedule during silence.
    setActiveGroup() activates one group at a time, not multiple.
    VoiceAmount directly controls streaming buffer allocation - high values waste RAM.
    Kill Third repeat mode is not implemented (falls to assertion).

  Common mistakes:
    Default AHDSR has non-zero attack - set to 0 for unaltered sample onset.
    VoiceAmount left at 256 when only 16 voices needed - wastes RAM.
    Monolith built with non-standard sample rates - pitch/timing errors.
    Sample Start modulator has no effect without SampleStartMod property set per sample.
    Loading sample maps during playback causes glitches.

  See also: none
---

::category-tags
---
tags:
  - { name: sample_playback, desc: "Modules that play back audio samples from disk or memory" }
---
::

![Sampler screenshot](/images/v2/reference/audio-modules/streamingsampler.png)

The Sampler is the primary sample playback module in HISE. It streams audio from disk using a double-buffered architecture where each voice maintains two streaming buffers that are swapped between disk reading and audio output. Samples are organised in sample maps that define key ranges, velocity layers, round-robin groups, and multi-mic positions.

The module supports three memory modes: normal (preload buffer in RAM, stream the rest from disk), fully loaded (entire sample in RAM with `PreloadSize = -1`), and purged (no samples in RAM until triggered).

### Streaming Architecture

Each voice allocates two streaming buffers of `BufferSize` samples. The `VoiceAmount` parameter controls how many voice slots are pre-allocated. Unlike other sound generators where voice overhead is negligible, the Sampler's per-voice memory cost is significant - set VoiceAmount to match your actual polyphony needs rather than leaving it at the maximum.

The `PreloadSize` parameter determines how many samples are loaded into RAM at the start of each sample. The remainder is streamed from disk. Setting PreloadSize to -1 loads the entire sample into memory, eliminating disk streaming but increasing RAM usage. A minimum of 2048 samples is enforced internally — earlier versions would crash on very short files such as single-cycle waveforms before this guard was added. [2]($FORUM_REF.6623$)

## Signal Path

::signal-path
---
glossary:
  parameters:
    PreloadSize:
      desc: "Preload buffer size in samples. -1 loads entire sample."
      range: "-1 - 65536"
      default: "8192"
    BufferSize:
      desc: "Streaming buffer size per voice (two buffers swapped)"
      range: "512 - 65536"
      default: "4096"
    VoiceAmount:
      desc: "Number of allocated streaming voice slots"
      range: "1 - 256"
      default: "256"
    RRGroupAmount:
      desc: "Number of round-robin groups"
      range: "1 - 128"
      default: "1"
    SamplerRepeatMode:
      desc: "Retrigger handling on the same key"
      range: "Kill Note / Note off / Do nothing / Kill Duplicate"
      default: "Do nothing"
    PitchTracking:
      desc: "Transpose playback relative to root note"
      range: "Off / On"
      default: "On"
    OneShot:
      desc: "Play full sample ignoring note-off"
      range: "Off / On"
      default: "Off"
    CrossfadeGroups:
      desc: "Play all RR groups simultaneously with crossfade"
      range: "Off / On"
      default: "Off"
    Purged:
      desc: "Memory management mode"
      range: "Disabled / Enabled / Lazy Load"
      default: "Disabled"
    Reversed:
      desc: "Reverse playback direction"
      range: "Off / On"
      default: "Off"
  functions:
    sampleMapLookup:
      desc: "Finds the matching sample by key, velocity, and active RR group"
    diskStream:
      desc: "Double-buffered disk streaming with preload buffer"
  modulations:
    GainModulation:
      desc: "Scales the output volume per voice"
      scope: "per-voice"
    PitchModulation:
      desc: "Multiplies the playback speed for pitch transposition"
      scope: "per-voice"
    SampleStart:
      desc: "Offsets the playback start position within the SampleStartMod range"
      scope: "per-voice (voice-start only)"
    GroupFade:
      desc: "Crossfades between RR groups when CrossfadeGroups is enabled"
      scope: "per-voice"
---

```
// Sampler - per-voice processing
// polyphonic, disk-streaming

// On note-on
sample = sampleMapLookup(noteNumber, velocity, activeRRGroup)
startOffset = SampleStart * sample.SampleStartMod

// Preload buffer provides initial samples
playbackPosition = startOffset

// Per-block generation
buffer = diskStream(sample, playbackPosition, BufferSize)

if PitchTracking:
    playbackRate = 2^((noteNumber - sample.rootNote) / 12)
    playbackRate *= PitchModulation
else:
    playbackRate = 1.0

// If CrossfadeGroups enabled, all groups play and mix via GroupFade
output = buffer * Gain * GainModulation
```

::

## Parameters

::parameter-table
---
groups:
  - label: Streaming
    params:
      - name: PreloadSize
        desc: "Number of samples loaded into RAM at the start of each sample file. The remainder is streamed from disk. Set to -1 to load entire samples into memory (no disk streaming)."
        range: "-1 - 65536 samples"
        default: "8192"
        hints:
          - type: warning
            text: "Must be large enough to cover any `SampleStartMod` range set on individual samples. The preload buffer is automatically extended to accommodate Sample Start modulation."
      - { name: BufferSize, desc: "Size of each streaming buffer in samples. Two buffers per voice are swapped between disk reading and audio output. Larger buffers reduce disk I/O frequency but increase latency and RAM usage.", range: "512 - 65536 samples", default: "4096" }
      - name: VoiceAmount
        desc: "Number of streaming voice slots to pre-allocate. Each slot allocates two streaming buffers."
        range: "1 - 256"
        default: "256"
        hints:
          - type: warning
            text: "Each voice allocates two streaming buffers of `BufferSize` samples. Set this to your actual polyphony needs - unused voices waste RAM."
      - { name: Purged, desc: "Memory management mode. Disabled keeps all preload buffers loaded. Enabled unloads all buffers. Lazy Load delays preloading until each sample is first triggered.", range: "Disabled / Enabled / Lazy Load", default: "Disabled" }
  - label: Sample Groups
    params:
      - { name: RRGroupAmount, desc: "Number of round-robin groups. Also used as a general mapping dimension for velocity layers, articulations, or any custom sample categorisation.", range: "1 - 128", default: "1" }
      - { name: CrossfadeGroups, desc: "When enabled, all round-robin groups play simultaneously. Use the Group Fade modulation chain to crossfade between them.", range: "Off / On", default: "Off" }
  - label: Playback
    params:
      - name: SamplerRepeatMode
        desc: "How the sampler handles retriggered notes on the same key."
        range: "Kill Note / Note off / Do nothing / Kill Duplicate"
        default: "Do nothing"
        hints:
          - type: tip
            text: "**Kill Duplicate** kills any older voice on the same key when a new note arrives - useful for hi-hats and other choke-group patterns."
      - { name: PitchTracking, desc: "Transposes playback speed based on the MIDI note relative to the sample's root note. Disable for drum samples mapped one-to-one.", range: "Off / On", default: "On" }
      - { name: OneShot, desc: "Plays the entire sample, ignoring note-off events. Useful for percussion hits and one-shot effects.", range: "Off / On", default: "Off" }
      - { name: Reversed, desc: "Reverses playback direction. Loads the full sample into RAM (no disk streaming) since reversed playback cannot stream forward.", range: "Off / On", default: "Off" }
  - label: Advanced
    params:
      - { name: UseStaticMatrix, desc: "Prevents the routing matrix from resizing when loading a sample map with a different mic position count. Enable this when switching between sample maps with different channel configurations.", range: "Off / On", default: "Off" }
      - { name: LowPassEnvelopeOrder, desc: "Filter order for the internal envelope follower low-pass filter, in multiples of 6 dB/octave. 0 disables the filter.", range: "0 - 48 (step 6)", default: "0" }
  - label: Output
    params:
      - { name: Gain, desc: "Output volume as normalised linear gain (not decibels). Modulatable via the Gain Modulation chain.", range: "0 - 100%", default: "100%" }
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
  - { name: "Gain Modulation", desc: "Scales the output volume. Applied as a per-voice multiply after sample playback. An AHDSR envelope is automatically added to this chain when the Sampler is created.", scope: "per-voice", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Modulates the pitch (playback speed) of all voices. Applied per-sample as a multiplier on the playback rate.", scope: "per-voice", constrainer: "Any" }
  - name: "Sample Start"
    desc: "Offsets the playback start position within each sample's SampleStartMod range. Voice-start only - evaluated once per note-on, not continuously."
    scope: "per-voice (voice-start only)"
    constrainer: "Any"
    hints:
      - type: warning
        text: "Each sample's `SampleStartMod` property must be set to a non-zero value in the sample map editor for this chain to have any effect."
  - { name: "Group Fade", desc: "Crossfades between round-robin groups when CrossfadeGroups is enabled. Each group has its own crossfade table that maps the modulation value to a volume curve.", scope: "per-voice", constrainer: "Any" }
---
::

> **Global Modulator Container ordering:** Unlike other sound generators, the Sampler requires the Global Modulator Container to appear **before** it in the module tree for Global Time Variant Modulators to apply correctly. Moving the container below the Sampler silently breaks the modulation. [1]($FORUM_REF.7177$)

## Group Management

### Round-Robin Groups

The `RRGroupAmount` parameter defines the number of round-robin groups, but these groups serve a dual purpose: they can represent round-robin variations, velocity layers, articulations, or any other sample categorisation. Use `Sampler.setActiveGroup()` to select a specific group from script, or enable automatic round-robin cycling with `Sampler.enableRoundRobin(true)`.

`setActiveGroup()` activates only one group at a time. To play samples from multiple groups simultaneously (for example, an always-on group plus a round-robin group), either map the always-on samples into every group, or use separate Sampler instances.

### Crossfade Groups

When `CrossfadeGroups` is enabled, all round-robin groups play simultaneously. The Group Fade modulation chain controls the volume balance between groups using crossfade tables. This is the standard approach for dynamic velocity crossfading - map velocity layers to RR groups and use a velocity modulator in the Group Fade chain.

### Complex Group Manager

The Complex Group Manager replaces the standard round-robin system with a multidimensional sample organisation scheme. Instead of a single group dimension, it supports multiple independent layers, each with its own behaviour type. This enables complex sample libraries that combine round-robin, keyswitching, crossfading, and release triggers in a single Sampler instance.

Each layer has a **logic type** that determines its behaviour:

| Logic Type | Description |
|---|---|
| Custom | Free-form grouping with manual script control |
| RoundRobin | Automatic cycling through groups on each note-on |
| Keyswitch | MIDI notes outside the sample range select between groups |
| TableFade | Crossfade between groups using a lookup table (for morphing) |
| XFade | Smooth velocity or modulator-driven crossfade between groups |
| LegatoInterval | Selects samples based on the interval between consecutive legato notes |
| ReleaseTrigger | Separate samples for the release phase |
| Choke | Samples that cancel other samples (e.g., open/closed hi-hat) |

Multiple layers can be combined. For example, a layer of type RoundRobin for natural variation, a Keyswitch layer for articulation switching, and a ReleaseTrigger layer for release samples - all in the same Sampler.

When setting up, create the layers and groups first, then assign samples. The right-click option "Assign from file token" can create groups and assign samples in one step if filenames contain a matching token.

## Data Model

### Sample Map

Samples are organised in a three-dimensional structure: note number (x-axis), velocity (y-axis), and round-robin group (z-axis). A sample map defines this mapping as an XML file that stores all per-sample properties. Sample maps are created and edited using the Map Editor and saved as `.xml` files in the project's SampleMaps folder.

Each sample in the map has the following properties, accessible from both the sample editor UI and from script via `Sampler.createSelection()`:

| Property | Script Constant | Description |
|---|---|---|
| Root | `Sampler.Root` | Root note (MIDI note number for pitch calculation) |
| LoKey / HiKey | `Sampler.LoKey` / `Sampler.HiKey` | Key range (lowest and highest mapped MIDI note) |
| LoVel / HiVel | `Sampler.LoVel` / `Sampler.HiVel` | Velocity range (0-127) |
| RRGroup | `Sampler.RRGroup` | Round-robin group index |
| Volume | `Sampler.Volume` | Gain in decibels |
| Pan | `Sampler.Pan` | Stereo balance |
| Pitch | `Sampler.Pitch` | Fine tuning in cents (+/-100) |
| SampleStart / SampleEnd | `Sampler.SampleStart` / `Sampler.SampleEnd` | Playback region boundaries in samples |
| SampleStartMod | `Sampler.SampleStartMod` | Maximum offset for Sample Start modulation chain |
| LoopStart / LoopEnd | `Sampler.LoopStart` / `Sampler.LoopEnd` | Loop boundaries in samples |
| LoopXFade | `Sampler.LoopXFade` | Crossfade length at the loop point |
| LoopEnabled | `Sampler.LoopEnabled` | Enables looping |
| ReleaseStart | `Sampler.ReleaseStart` | Jump-to position on note-off (for release tails) |
| LowerVelocityXFade | `Sampler.LowerVelocityXFade` | Velocity crossfade range at the low end |
| UpperVelocityXFade | `Sampler.UpperVelocityXFade` | Velocity crossfade range at the high end |
| SampleState | `Sampler.SampleState` | Normal (0), Disabled (1), or Purged (2) |
| Reversed | `Sampler.Reversed` | Reverse playback |

Additional per-sample lookup tables (GainTable, PitchTable) provide sample-level modulation envelopes that shape gain or pitch across the sample duration.

### SFZ Import

The Sampler can import SFZ files to create sample maps. The following SFZ opcodes are supported:

| SFZ Opcode | Maps To | Notes |
|---|---|---|
| `sample` | FileName | Audio file path |
| `lokey` / `hikey` | LoKey / HiKey | Key range |
| `key` | LoKey, HiKey, Root | Sets all three to the same value |
| `lovel` / `hivel` | LoVel / HiVel | Velocity range |
| `pitch_keycenter` | Root | Root note |
| `offset` / `end` | SampleStart / SampleEnd | Playback region |
| `loop_mode` | LoopEnabled | Only `loop_continuous` is supported |
| `loop_start` / `loop_end` | LoopStart / LoopEnd | Loop boundaries |
| `tune` | Pitch | Fine tuning in cents |
| `volume` / `group_volume` | Volume | Gain (both are combined) |
| `pan` | Pan | Stereo balance |
| `default_path` | - | Default path prefix for sample files |
| `lorand` / `hirand` | - | Random selection ranges |
| `seq_length` / `seq_position` | - | Round-robin sequence cycling |

Unsupported opcodes (including release trigger opcodes, filter opcodes, and amplitude envelope opcodes) are silently ignored during import. After importing, review the sample map in the Map Editor to verify the mapping and adjust any properties that could not be imported.

### Multi-Mic Support

The Sampler supports multiple microphone positions per sample. Each mic position is stored as a separate audio file, and all mic positions play simultaneously per voice. Mic positions are defined as a semicolon-separated string in the sample map (e.g., `Close;OH;Room`).

Each mic position maps to a separate channel in the Sampler's routing matrix, allowing independent volume and routing control per mic. This is the standard approach for orchestral libraries and multi-mic instrument recordings.

When switching between sample maps with different mic position counts, the routing matrix resizes to match. Enable `UseStaticMatrix` to prevent this resize - useful when all your sample maps share the same mic configuration and you want the routing to remain stable.

### Monolith Format (HLAC)

The HLAC-compressed monolith format bundles all samples from a sample map into a single `.ch1` file (or multiple `.ch1`, `.ch2`, `.ch3` files for multi-mic setups). The uncompressed monolith format is deprecated - always use HLAC compression, which has negligible performance overhead.

Three compression modes are available when exporting:

- **No Normalisation** - Direct compression without any level adjustment
- **Normalise Every Sample** - Each sample is individually normalised before compression
- **Full Dynamics** - Uses a 16-bit signal path with internal 1024-sample normalisation chunks. This preserves the dynamic range in decay portions of samples while maintaining compression efficiency. Recommended for most use cases.

The monolith format encodes sample rate as a 2-bit index, supporting only four rates: **44100, 48000, 88200, and 96000 Hz**. Samples at other rates are silently encoded as 44100 Hz, causing pitch and timing errors on playback. Ensure all samples match one of these four rates before building a monolith.

Monolith files are created per mic position, with each file containing the compressed samples for that mic channel.

### HISE Resource Archive

For distribution, monolith files can be further compressed into a HISE Resource Archive (`.hr1`). The archive uses FLAC codec compression (10-20% better than HLAC alone) and can be split into configurable chunks (500 MB - 1 GB) for download-friendly distribution. On first launch, the end user locates the `.hr1` files and HISE extracts the monoliths to a user-selected location.

## Playback Features

### Release Start

Release Start (HISE 4.1.0+) reuses the natural decay of sustain samples as release tails, eliminating the need for separate release sample recordings. When a note-off event occurs, playback jumps to the position defined by each sample's `ReleaseStart` property and crossfades into the release portion.

This is configured via JSON options on the Sampler or through the popup editor in the sample editor:

| Option | Type | Default | Description |
|---|---|---|---|
| ReleaseFadeTime | int | 4096 | Fade-in duration in samples when transitioning to the release portion |
| FadeGamma | float | 1.0 | Curve shape for the fade (range 0.125 - 4.0). Values below 1.0 produce a faster initial fade, values above 1.0 produce a slower onset. |
| UseAscendingZeroCrossing | bool | false | Aligns the jump point to a zero crossing to avoid clicks |
| GainMatchingMode | string | "None" | How to match volume between the sustain and release portions: "None" (no matching), "Volume" (match peak levels), or "Offset" (blend with an offset) |
| PeakSmoothing | float | 0.96 | Smoothing factor for the peak level detection used by gain matching |

The `ReleaseStart` position should be set to a point in the sample where the natural decay begins - typically after the sustained portion. The Sampler handles the crossfade between the current playback position and the release position automatically.

### Timestretching

Timestretching (HISE 3.5.1+) provides independent control over pitch and time, allowing samples to be played at different tempos without affecting pitch. It uses a modified Signalsmith algorithm.

Four modes are available:

| Mode | Description |
|---|---|
| Disabled | No timestretching. Default behaviour with minimal CPU overhead. |
| VoiceStart | A static stretch ratio is applied when each voice starts. Already-playing voices keep their original ratio. |
| TimeVariant | A dynamic stretch ratio is applied to all active voices simultaneously. Changes affect all playing voices immediately. |
| TempoSynced | The stretch ratio is calculated automatically from the sample length and the current DAW/host tempo. |

#### Timestretching Options

| Option | Type | Default | Description |
|---|---|---|---|
| Mode | string | "Disabled" | One of the four modes above |
| SkipLatency | bool | false | When false, introduces minimal latency (~270 samples at 512 buffer) by processing the initial segment on a background thread. When true, starts immediately but causes CPU spikes during voice onset. |
| NumQuarters | double | 0.0 | For TempoSynced mode: the length of the sample in quarter notes. Used to calculate the correct stretch ratio from the current tempo. |
| Tonality | double | 0.0 | Controls timbre retention during pitch shifting (0.0 - 1.0). Higher values preserve the formant characteristics of the original sample. |

> SkipLatency behaviour was refined in HISE 4.1.0. The default (false) is recommended for most use cases.

## Scripting Access

Use `Synth.getSampler()` (not `Synth.getModulator()`) to get a Sampler reference with access to the full Sampler API. Note that the `Sampler.ID` constant is broken and throws a runtime error — use the integer `0` directly instead (e.g., `sample.get(0)` rather than `sample.get(Sampler.ID)`). [3]($FORUM_REF.3598$)

```javascript
const var sampler = Synth.getSampler("Sampler1");

// Load a sample map
sampler.loadSampleMap("MyInstrument");

// Select and modify samples at runtime
const var selection = sampler.createSelection(".*");
for (s in selection)
    s.set(Sampler.LoopEnabled, 1);

// Switch round-robin group from script
sampler.setActiveGroup(2);
```
