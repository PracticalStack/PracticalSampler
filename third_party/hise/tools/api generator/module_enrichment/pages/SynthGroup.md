---
title: Synthesiser Group
moduleId: SynthGroup
type: SoundGenerator
subtype: SoundGenerator
tags: [container, oscillator]
builderPath: b.SoundGenerators.SynthGroup
screenshot: /images/v2/reference/audio-modules/synthgroup.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: UnisonoVoiceAmount, impact: high, note: "Direct CPU multiplier - 8 unison voices = 8x rendering cost per note. Maximum polyphony is reduced proportionally (256 / unison count)." }
    - { parameter: EnableFM, impact: medium, note: "Adds one extra voice render for the FM modulator plus per-sample pitch multiplication on the carrier." }
seeAlso:
  - { id: SynthChain, type: alternative, reason: "Simple container that sums children independently. Use when children do not need shared modulation, FM, or unison." }
forumReferences:
  - id: 1
    title: "Unison causes clicks without attack envelope"
    summary: "When UnisonoVoiceAmount > 1, each voice gets a random start offset (~10ms) to prevent phase cancellation; without an amplitude envelope this causes audible clicks on note-on."
    topic: 2871
  - id: 2
    title: "Child pitch modulation chains bypassed by group"
    summary: "When a synth is added as a child of a SynthGroup, its own pitch modulation chain is bypassed and hidden; all pitch modulation is driven exclusively by the group's shared chain."
    topic: 669
  - id: 3
    title: "core.pitch_mod misses unison detune in scriptnode"
    summary: "Inside a scriptnode network child of a SynthGroup, core.pitch_mod reads pitch values before the group applies per-voice unison detune offsets, so detune is invisible to scriptnode nodes."
    topic: 13030
commonMistakes:
  - title: "Unison multiplies CPU cost per note"
    wrong: "Setting UnisonoVoiceAmount to 16 with high polyphony and wondering why CPU spikes"
    right: "Keep unison count low (2-4) for polyphonic patches, or reduce VoiceLimit proportionally"
    explanation: "Unison multiplies the actual voice count. 16 unison voices with 16-note polyphony means 256 voice renders per block. The maximum polyphony is automatically reduced to 256 / unison count."
  - title: "Children can't host master effects"
    wrong: "Adding reverb or delay to a child synth inside the group"
    right: "Add master effects to the group's own FX chain, not to children"
    explanation: "Children inside a Synthesiser Group can only use polyphonic (voice-level) effects. Master effects are automatically removed when a child is added to the group because children render at the voice level."
  - title: "FM requires both indices set"
    wrong: "Enabling FM but leaving CarrierIndex or ModulatorIndex at -1"
    right: "Set both CarrierIndex and ModulatorIndex to valid child indices before enabling FM"
    explanation: "FM synthesis requires both a carrier and modulator to be specified. With either index at -1, the FM setup is invalid and no sound is produced."
  - title: "Non-carrier/modulator children silent in FM"
    wrong: "Expecting all children to produce sound when FM is enabled"
    right: "Only the carrier and modulator children produce sound in FM mode"
    explanation: "When FM is enabled with valid indices, all children except the designated carrier and modulator are silenced. Add other sound layers in a separate Container outside the group."
  - title: "Can't nest containers in group"
    wrong: "Nesting a Container or another Synthesiser Group as a child"
    right: "Only add simple sound generators (SineSynth, StreamingSampler, etc.) as children"
    explanation: "Containers, other groups, Global Modulator Containers, and Macro Modulation Sources cannot be children of a Synthesiser Group because they have their own voice management that conflicts with the group's shared rendering model."
  - title: "Bypassing children still allocates voices"
    wrong: "Using `Synth.setBypassed()` or setting gain to -100 dB to silence a child synth"
    right: "Use a MidiMuter on the child to prevent voice allocation entirely"
    explanation: "A bypassed or silent child still allocates voices on note-on, consuming the voice pool. A MidiMuter blocks MIDI events before voice allocation, keeping voices available for other children."
customEquivalent:
  approach: scriptnode
  moduleType: SoundGenerator
  complexity: complex
  description: "A scriptnode network with parallel oscillators, shared modulation routing, and optional FM feedback path"
llmRef: |
  Synthesiser Group (SoundGenerator)

  Advanced polyphonic container that shares modulation across child sound generators, with optional FM synthesis and unison voice stacking. Children render at the voice level under the group's modulation control.

  Signal flow:
    MIDI in -> MIDI processors -> per voice:
      [FM mode] render modulator -> audio * gain + 1.0 -> multiply into carrier pitch -> render carrier with detune
      [Normal mode] for each unisono voice, for each child: apply group pitch * detune -> render child -> mix with equal-power gain
      -> apply group Gain Modulation -> voice effects
    -> sum all voices -> master effects -> Gain * Balance -> audio out

  CPU: low baseline, polyphonic
    UnisonoVoiceAmount: direct CPU multiplier (high impact)
    EnableFM: extra voice render + per-sample multiply (medium impact)

  Parameters:
    Gain (0-100%, default 100%) - output volume, modulatable via Gain Modulation
    Balance (-1 to 1, default 0) - stereo balance
    VoiceLimit (1-256, default 256) - max voices (auto-reduced by unison count)
    KillFadeTime (0-20000 ms, default 20 ms) - kill fade time
    EnableFM (Off/On, default Off) - enables FM synthesis
    CarrierIndex (-1 to 16, default -1) - FM carrier child index (-1 = not set)
    ModulatorIndex (-1 to 16, default -1) - FM modulator child index (-1 = not set)
    UnisonoVoiceAmount (1-16, default 1) - unison voices per note. WARNING: causes clicks without ~5ms attack envelope. Changes do not apply to held notes.
    UnisonoDetune (0-24 st, default 0) - detune spread in semitones. Read at note-on only; use Detune Modulation chain for real-time control.
    UnisonoSpread (0-100%, default 100%) - stereo spread. Read at note-on only; use Spread Modulation chain for real-time control.
    ForceMono (Off/On, default Off) - collapses child stereo to mono before unison spread
    KillSecondVoices (Off/On, default On) - kills older voice when same note retriggered

  Modulation chains:
    Gain Modulation - per-voice, scales combined output (all modulator types)
    Pitch Modulation - per-voice, multiplied onto each child's pitch (shared). Children's own pitch chains are bypassed and hidden.
    Detune Modulation - scales UnisonoDetune (auto-bypassed when unison = 1)
    Spread Modulation - scales UnisonoSpread (auto-bypassed when unison = 1)

  When to use:
    Multiple oscillators sharing a common envelope/pitch. FM synthesis between two child synths. Unison detune/spread for thicker sound. Use Container instead if children need independent modulation.

  Common mistakes:
    High unison + high polyphony = CPU spike (unison multiplies voice count).
    Master effects on children are removed (only voice-level FX allowed).
    FM with index -1 produces no sound.
    All non-carrier/modulator children silenced when FM is on.
    Bypassing a child still allocates voices - use MidiMuter to silence children instead.
    Changing child Gain directly causes zipper noise - use a gain modulator instead.

  Scriptnode interaction:
    core.pitch_mod does not receive unison detune offsets (modulation chains run before detune is applied). Use a clone container for scriptnode-based unison with per-voice detune.

  Monophonic mode:
    No built-in mono mode. Add the "Legato with Retrigger" MIDI script for monophonic unison.

  Custom equivalent:
    scriptnode SoundGenerator (complex) with parallel oscillators and shared modulation routing.

  See also:
    alternative SynthChain - simple container without shared modulation
---

::category-tags
---
tags:
  - { name: container, desc: "Modules that host and organise child processors" }
  - { name: oscillator, desc: "Modules that generate audio or modulation signals from oscillators or synthesis algorithms" }
---
::

![Synthesiser Group screenshot](/images/v2/reference/audio-modules/synthgroup.png)

The Synthesiser Group is an advanced container for sound generators that share common modulation. Unlike a Container, which sums children independently, the group applies its own gain and pitch modulation chains to all children collectively. This means a single envelope on the group controls the volume shape for the combined output, and a single LFO on the group modulates the pitch of all children simultaneously. When a synth is added as a child, its own pitch modulation chain is bypassed and hidden in the UI - all pitch modulation is driven exclusively by the group's shared Pitch Modulation chain. [2]($FORUM_REF.669$)

The group also provides FM synthesis between two designated child synths and unison voice stacking with configurable detune and stereo spread. Children inside a group can only use polyphonic (voice-level) effects - master effects such as reverb or delay must be placed on the group's own FX chain.

### Child Restrictions

The following module types cannot be added as children: Containers, other Synthesiser Groups, Global Modulator Containers, and Macro Modulation Sources. These have their own voice management that conflicts with the group's shared rendering model.

### FM Synthesis

The FM modulator always renders as a single voice without unison detune. Only the carrier receives unison detune and spread. The modulator's audio output is centred around 1.0 (silence = no pitch change), so the modulator's amplitude directly controls the FM depth while its frequency determines the modulation rate.

When FM is disabled but CarrierIndex is set to a valid child index, that child is soloed - only it produces sound. This can be useful for quickly auditioning individual children within the group.

### Unison Behaviour

Unison voices receive randomised start offsets (up to ~10 ms) to prevent phase cancellation when multiple copies of the same waveform play simultaneously. The gain of each unison voice is compensated using equal-power scaling (divided by the square root of the voice count) to maintain consistent overall volume. When no amplitude envelope is present, these offsets cause audible clicks on note-on; adding ~5 ms of attack time to the group's gain envelope eliminates this. [1]($FORUM_REF.2871$)

### Controlling Children

To control the volume of individual children dynamically, use a gain modulator rather than setting the child's Gain parameter directly. The Gain parameter does not apply smoothing, so rapid changes cause audible zipper noise. Gain modulators operate in the 0-1 range and are interpolated smoothly.

To achieve monophonic behaviour with unison voices, add the built-in "Legato with Retrigger" MIDI script to the Synthesiser Group. There is no built-in mono mode for the group.

### Scriptnode Interaction

When using a scriptnode network as a child of a Synthesiser Group, the `core.pitch_mod` node reads pitch modulation values before the group applies its per-voice unison detune offsets. This means unison detune is not visible inside scriptnode. For fully scriptnode-based unison with per-voice detune, use a scriptnode clone container instead. [3]($FORUM_REF.13030$)

## Signal Path

::signal-path
---
glossary:
  parameters:
    EnableFM:
      desc: "Enables FM synthesis between carrier and modulator children"
      range: "Off / On"
      default: "Off"
    CarrierIndex:
      desc: "Child synth index used as FM carrier (-1 = not set)"
      range: "-1 - 16"
      default: "-1"
    ModulatorIndex:
      desc: "Child synth index used as FM modulator (-1 = not set)"
      range: "-1 - 16"
      default: "-1"
    UnisonoVoiceAmount:
      desc: "Number of unison voices per note. Direct CPU multiplier."
      range: "1 - 16"
      default: "1"
    UnisonoDetune:
      desc: "Detune spread in semitones across unison voices"
      range: "0 - 24 st"
      default: "0"
    UnisonoSpread:
      desc: "Stereo spread of unison voices"
      range: "0 - 100%"
      default: "100%"
    ForceMono:
      desc: "Collapses child stereo to mono before unison spread"
      range: "Off / On"
      default: "Off"
    Gain:
      desc: "Output volume as normalised linear gain"
      range: "0 - 100%"
      default: "100%"
  functions:
    renderChild:
      desc: "Renders a child synth voice block with the group's shared pitch applied"
    calculateDetune:
      desc: "Distributes pitch offsets and stereo positions across unison voices"
  modulations:
    GainModulation:
      desc: "Scales the combined voice output after all children render"
      scope: "per-voice"
    PitchModulation:
      desc: "Multiplied onto each child's own pitch values"
      scope: "per-voice"
    DetuneModulation:
      desc: "Scales the UnisonoDetune parameter"
      scope: "per-voice"
    SpreadModulation:
      desc: "Scales the UnisonoSpread parameter"
      scope: "per-voice"
---

```
// Synthesiser Group - per-voice processing
// MIDI in -> shared modulation -> children -> audio out

// Determine active children
if EnableFM and CarrierIndex >= 0 and ModulatorIndex >= 0:
    activeChildren = [carrier, modulator]
else if CarrierIndex >= 0:
    activeChildren = [carrier]    // solo mode
else:
    activeChildren = allChildren

// FM path: render modulator first
if EnableFM:
    fmModBuffer = renderChild(modulator) * modulatorGain + 1.0

// For each unison voice
for i in 0..UnisonoVoiceAmount:
    pitchOffset, panPosition = calculateDetune(i, UnisonoDetune * DetuneModulation, UnisonoSpread * SpreadModulation)

    for child in activeChildren:
        childPitch = child.ownPitch * PitchModulation * pitchOffset
        if EnableFM:
            childPitch *= fmModBuffer

        audio = renderChild(child, childPitch)
        if ForceMono:
            audio = (audio.left + audio.right) * 0.5

        voiceBuffer += audio * childGain * panPosition / sqrt(UnisonoVoiceAmount)

// Apply group gain modulation
voiceBuffer *= GainModulation
```

::

## Parameters

::parameter-table
---
groups:
  - label: Output
    params:
      - { name: Gain, desc: "Output volume as normalised linear gain (not decibels). Modulatable via the Gain Modulation chain.", range: "0 - 100%", default: "100%" }
      - { name: Balance, desc: "Stereo balance applied at the output stage.", range: "-1 - 1", default: "0" }
  - label: Voice Management
    params:
      - { name: VoiceLimit, desc: "Maximum number of simultaneous voices. Automatically reduced when unison is active (256 / unison count).", range: "1 - 256", default: "256" }
      - { name: KillFadeTime, desc: "Fade-out time when voices are killed.", range: "0 - 20000 ms", default: "20 ms" }
      - { name: KillSecondVoices, desc: "When enabled, retriggering the same MIDI note kills the older voice to prevent voice buildup. Important for unison patches where each note consumes multiple voices.", range: "Off / On", default: "On" }
  - label: FM Synthesis
    params:
      - { name: EnableFM, desc: "Enables FM synthesis between two child synths. When enabled, only the carrier and modulator children produce sound - all others are silenced.", range: "Off / On", default: "Off" }
      - { name: CarrierIndex, desc: "Index of the child synth used as FM carrier. Set to -1 to disable. When FM is off but this is set, the carrier is soloed (only that child plays).", range: "-1 - 16", default: "-1" }
      - { name: ModulatorIndex, desc: "Index of the child synth used as FM modulator. The modulator's audio output scales the carrier's pitch. Set to -1 to disable.", range: "-1 - 16", default: "-1" }
  - label: Unison
    params:
      - name: UnisonoVoiceAmount
        desc: "Number of unison voices per note. Each unison voice is a separate render of all active children with a pitch offset and stereo position. Directly multiplies CPU cost and reduces maximum polyphony."
        range: "1 - 16"
        default: "1"
        hints:
          - type: warning
            text: "With unison > 1, each voice gets a random start offset (~10 ms) that causes audible clicks without an amplitude envelope. Add at least ~5 ms attack time to the group's gain envelope."
          - type: warning
            text: "Changes to the voice count do not apply to held notes. Only new note-on events use the updated count."
      - name: UnisonoDetune
        desc: "Detune spread in semitones distributed linearly across unison voices. The first voice gets maximum negative detune, the last gets maximum positive. Modulatable via the Detune Modulation chain."
        range: "0 - 24 st"
        default: "0"
        hints:
          - type: warning
            text: "Read at note-on time only. Changing the parameter value does not affect held notes. Drive the **Detune Modulation** chain instead for real-time control."
      - name: UnisonoSpread
        desc: "Stereo spread of unison voices. At 0% all voices are centred; at 100% negatively-detuned voices are panned fully left, positively-detuned fully right. Modulatable via the Spread Modulation chain."
        range: "0 - 100%"
        default: "100%"
        hints:
          - type: warning
            text: "Read at note-on time only. Changing the parameter value does not affect held notes. Drive the **Spread Modulation** chain instead for real-time control."
  - label: Output Mode
    params:
      - { name: ForceMono, desc: "Collapses each child's stereo output to mono before the unison stereo spread is applied. The unison spread still creates a stereo image.", range: "Off / On", default: "Off" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Gain Modulation", desc: "Scales the combined output of all children per voice. Applied after all children have rendered into the voice buffer. Supports all modulator types including envelopes.", scope: "per-voice", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Multiplied onto each child's own pitch values. This is the core shared modulation feature - one pitch modulator controls all children simultaneously.", scope: "per-voice", constrainer: "Any" }
  - { name: "Detune Modulation", desc: "Scales the UnisonoDetune parameter value. Automatically bypassed when unison voice count is 1.", scope: "per-voice", constrainer: "Any" }
  - { name: "Spread Modulation", desc: "Scales the UnisonoSpread parameter value. Automatically bypassed when unison voice count is 1.", scope: "per-voice", constrainer: "Any" }
---
::

**See also:** $MODULES.SynthChain$ -- Simple container that sums children independently. Use when children do not need shared modulation, FM, or unison.
