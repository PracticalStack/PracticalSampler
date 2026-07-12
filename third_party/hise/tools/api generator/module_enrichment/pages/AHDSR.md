---
title: AHDSR Envelope
moduleId: AHDSR
type: Modulator
subtype: EnvelopeModulator
tags: [generator]
builderPath: b.Modulators.AHDSR
screenshot: /images/v2/reference/audio-modules/ahdsr.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: FlexAHDSR, type: alternative, reason: "More flexible envelope with independent curve controls for each stage" }
  - { id: SimpleEnvelope, type: alternative, reason: "Lighter envelope with attack and release only - lower CPU when Hold, Decay, and curve shaping are not needed" }
  - { id: TableEnvelope, type: alternative, reason: "Table-driven envelope shapes with separate attack and release curves" }
commonMistakes:
  - title: "DecayCurve controls both decay and release shape"
    wrong: "Expecting separate curve controls for the decay and release phases"
    right: "DecayCurve shapes both phases identically - use FlexAHDSR if you need independent curves"
    explanation: "A single curve parameter drives both the decay and release shapes. There is no separate release curve control."
  - title: "EcoMode has no effect"
    wrong: "Enabling EcoMode expecting reduced CPU usage"
    right: "Leave EcoMode at its default - downsampling is now controlled globally"
    explanation: "The EcoMode parameter is vestigial. It appears in the interface but does not change processing behaviour."
  - title: "Level parameters are in dB but modulation operates in linear gain"
    wrong: "Expecting a modulator on AttackLevel or Sustain to offset the dB value (e.g. subtract 6 dB)"
    right: "Modulators multiply the linear gain value - a modulator outputting 0.5 halves the linear gain (roughly -6 dB)"
    explanation: "AttackLevel and Sustain are displayed in dB, but modulation chains operate on the converted linear gain value as a multiplier."
  - title: "Monophonic mode disables voice killing"
    wrong: "Switching to Monophonic mode and expecting voices to be killed normally when the envelope finishes"
    right: "Add a second polyphonic envelope to handle voice killing when using monophonic mode"
    explanation: "In monophonic mode the envelope never signals the voice to stop, which can cause notes to ring indefinitely."
forumReferences:
  - id: 1
    title: "Monophonic mode disables voice killing"
    summary: "Switching an AHDSR to Monophonic mode causes it to lose its ability to kill voices; add a second polyphonic envelope as a workaround."
    topic: 9608
  - id: 2
    title: "Release curve cannot be shaped in AHDSR"
    summary: "The AHDSR has no release curve control; DecayCurve shapes both decay and release identically. Use TableEnvelope for a shaped release."
    topic: 8306
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedEnvelopeModulator
  complexity: medium
  description: "Rebuild as a scriptnode network using envelope nodes, or use a ScriptEnvelopeModulator with HISEScript callbacks for custom stage logic"
llmRef: |
  AHDSR Envelope (Modulator/EnvelopeModulator)

  Five-stage envelope modulator: Attack, Hold, Decay, Sustain, Release. Runs as a per-voice state machine at control rate. Produces a 0-1 modulation signal that multiplies into the voice audio buffer.

  Signal flow:
    noteOn -> capture mod chains -> compute per-voice coefficients -> ATTACK -> HOLD -> DECAY -> SUSTAIN
    noteOff -> RELEASE -> IDLE (voice killed)

  CPU: low, polyphonic. Slightly heavier than SimpleEnvelope due to additional stages and curve shaping.

  Parameters:
    Attack (0 - 20000 ms, default 20 ms) - time to reach peak level
    AttackLevel (-100 - 0 dB, default 0 dB) - peak level at end of attack
    Hold (0 - 20000 ms, default 10 ms) - time held at peak before decay
    Decay (1 - 20000 ms, default 300 ms) - time to fall from peak to sustain
    Sustain (-100 - 0 dB, default 0 dB) - level held while note is down
    Release (1 - 20000 ms, default 20 ms) - time to fall to zero after note-off
    AttackCurve (0.0 - 1.0, default 0.0) - attack shape: 0.0=concave, 0.5=linear, 1.0=convex
    DecayCurve (0.0 - 1.0, default 0.0) - decay and release shape: 0.0=steep, 1.0=gentle
    Monophonic (Off/On, default dynamic) - shared envelope for all voices
    Retrigger (Off/On, default On) - restart envelope on new note in monophonic mode
    EcoMode (Off/On, default On) - vestigial, no effect

  Modulation chains (all per-voice, VoiceStartModulator, gain mode):
    AttackTimeModulation - multiplies attack time in ms
    AttackLevelModulation - multiplies attack level as linear gain
    DecayTimeModulation - multiplies decay time in ms
    SustainLevelModulation - multiplies sustain level as linear gain
    ReleaseTimeModulation - multiplies release time in ms

  Key behaviours:
    - If AttackLevel is below Sustain, Hold and Decay are skipped entirely.
    - DecayCurve controls both decay and release shape (no separate release curve).
    - Monophonic mode disables voice killing; add a second polyphonic envelope as a workaround.
    - Mod chains are evaluated once at note-on and multiply the parameter value.
    - EcoMode is vestigial; downsampling is controlled globally.

  Common mistakes:
    DecayCurve shapes both decay and release identically.
    Level modulation multiplies linear gain, not dB offset.
    Monophonic mode prevents voice killing.

  Custom equivalent:
    scriptnode via HardcodedEnvelopeModulator (medium complexity), or HISEScript via ScriptEnvelopeModulator.

  See also:
    alternative FlexAHDSR - independent curve controls per stage
    alternative SimpleEnvelope - lighter, attack+release only
    alternative TableEnvelope - table-driven shapes
---

::category-tags
---
tags:
  - { name: generator, desc: "Modulators that create modulation signals internally, such as envelopes and LFOs" }
---
::

![AHDSR Envelope screenshot](/images/v2/reference/audio-modules/ahdsr.png)

The AHDSR is a five-stage envelope modulator that shapes the amplitude of each voice over time. It progresses through Attack, Hold, Decay, Sustain, and Release stages, producing a modulation signal between 0 and 1 that multiplies into the voice audio buffer. Every sound generator in HISE needs at least one envelope to control when voices start and stop - the AHDSR is the standard choice for this role. The exclamation mark icon on an envelope in the module tree indicates that it acts as the voice killer for its parent sound generator.

Each time parameter (Attack, Decay, Release) has a dedicated modulation chain that can vary the timing per voice at note-on. The two level parameters (AttackLevel, Sustain) also have modulation chains that scale the level per voice. The `AttackCurve` parameter shapes the attack from concave (logarithmic) through linear to convex (exponential), while `DecayCurve` shapes both the decay and release phases from steep to gentle. For independent curve control over each stage, use [FlexAHDSR]($MODULES.FlexAHDSR$) instead. For instruments that do not need Hold, Decay, or curve shaping, [SimpleEnvelope]($MODULES.SimpleEnvelope$) is a lighter alternative.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Attack:
      desc: "Time to reach peak level after note-on"
      range: "0 - 20000 ms"
      default: "20 ms"
    AttackLevel:
      desc: "Peak level at the end of the attack phase"
      range: "-100 - 0 dB"
      default: "0 dB"
    Hold:
      desc: "Time held at peak level before decay begins"
      range: "0 - 20000 ms"
      default: "10 ms"
    Decay:
      desc: "Time to fall from peak level to sustain level"
      range: "1 - 20000 ms"
      default: "300 ms"
    Sustain:
      desc: "Level maintained while the note is held"
      range: "-100 - 0 dB"
      default: "0 dB"
    Release:
      desc: "Time to fall from sustain level to zero after note-off"
      range: "1 - 20000 ms"
      default: "20 ms"
    AttackCurve:
      desc: "Shape of the attack phase: 0.0 = concave/logarithmic, 0.5 = linear, 1.0 = convex/exponential"
      range: "0.0 - 1.0"
      default: "0.0"
    DecayCurve:
      desc: "Shape of both decay and release phases: 0.0 = steep, 1.0 = gentle"
      range: "0.0 - 1.0"
      default: "0.0"
    Monophonic:
      desc: "Shares one envelope across all voices instead of per-voice envelopes"
      range: "Off / On"
      default: "(dynamic)"
    Retrigger:
      desc: "Restarts the envelope on new notes in monophonic mode"
      range: "Off / On"
      default: "On"
  functions:
    applyToVoice:
      desc: "Multiplies the envelope output into the voice audio buffer"
  modulations:
    AttackTimeModulation:
      desc: "Scales the attack time per voice at note-on"
      scope: "per-voice"
    AttackLevelModulation:
      desc: "Scales the attack peak level per voice at note-on"
      scope: "per-voice"
    DecayTimeModulation:
      desc: "Scales the decay time per voice at note-on"
      scope: "per-voice"
    SustainLevelModulation:
      desc: "Scales the sustain level per voice at note-on"
      scope: "per-voice"
    ReleaseTimeModulation:
      desc: "Scales the release time per voice at note-on"
      scope: "per-voice"
---

```
// AHDSR Envelope - five-stage per-voice envelope
// noteOn/noteOff in -> modulation out (0-1, per voice)

onNoteOn() {
    // Capture mod chain values (evaluated once at note-on)
    attackMs  = Attack * AttackTimeModulation
    peakLevel = AttackLevel * AttackLevelModulation
    decayMs   = Decay * DecayTimeModulation
    susLevel  = Sustain * SustainLevelModulation
    releaseMs = Release * ReleaseTimeModulation

    if (Monophonic && voicesPlaying > 1) {
        if (Retrigger)
            restart envelope from current value
        else
            continue from current state
    }

    // ATTACK: rise toward peakLevel
    // shape controlled by AttackCurve (0=concave, 0.5=linear, 1=convex)
    value -> peakLevel over attackMs

    // HOLD: stay at peakLevel
    hold at peakLevel for Hold

    // DECAY: fall toward susLevel
    // shape controlled by DecayCurve (0=steep, 1=gentle)
    value -> susLevel over decayMs

    // SUSTAIN: hold at susLevel until note-off
    value = susLevel
}

onNoteOff() {
    // RELEASE: fall toward zero
    // shape also controlled by DecayCurve
    value -> 0.0 over releaseMs

    // IDLE: envelope finished, voice can be killed
    applyToVoice(value)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Envelope
    params:
      - { name: Attack, desc: "Time to reach the peak level after a note-on event", range: "0 - 20000 ms", default: "20 ms" }
      - name: AttackLevel
        desc: "The peak level reached at the end of the attack phase. Displayed in dB; modulation operates on the linear gain value"
        range: "-100 - 0 dB"
        default: "0 dB"
        hints:
          - type: warning
            text: "If AttackLevel is set lower than Sustain, the envelope transitions directly from Attack to Sustain, skipping the Hold and Decay stages entirely."
      - { name: Hold, desc: "Time the envelope stays at the peak level before the decay phase begins. Not modulatable", range: "0 - 20000 ms", default: "10 ms" }
      - { name: Decay, desc: "Time to fall from the peak level to the sustain level", range: "1 - 20000 ms", default: "300 ms" }
      - { name: Sustain, desc: "The level maintained while the note is held. Displayed in dB; modulation operates on the linear gain value. 0 dB = unity, -100 dB = silence", range: "-100 - 0 dB", default: "0 dB" }
      - { name: Release, desc: "Time to fall from the sustain level to zero after a note-off event", range: "1 - 20000 ms", default: "20 ms" }
  - label: Curve Shape
    params:
      - { name: AttackCurve, desc: "Controls the curvature of the attack phase. 0.0 = concave (fast initial rise), 0.5 = nearly linear, 1.0 = convex (slow initial rise)", range: "0.0 - 1.0", default: "0.0" }
      - { name: DecayCurve, desc: "Controls the curvature of both the decay and release phases. 0.0 = steep exponential, 1.0 = gentler curve closer to linear. There is no separate release curve control - use TableEnvelope if you need to shape the release independently. [2]($FORUM_REF.8306$)", range: "0.0 - 1.0", default: "0.0" }
  - label: Voice Mode
    params:
      - name: Monophonic
        desc: "Shares a single envelope across all voices instead of running one per voice"
        range: "Off / On"
        default: "(dynamic)"
        hints:
          - type: warning
            text: "Monophonic mode prevents the envelope from killing voices. The voice will never stop on its own. If you need voice killing alongside a monophonic envelope, add a second polyphonic envelope to the same sound generator. [1]($FORUM_REF.9608$)"
      - { name: Retrigger, desc: "Restarts the envelope from its current value when a new note arrives in monophonic mode. Has no effect in polyphonic mode", range: "Off / On", default: "On" }
  - label: Legacy
    params:
      - { name: EcoMode, desc: "This parameter has no effect. Downsampling is now controlled globally", range: "Off / On", default: "On" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: AttackTimeModulation, desc: "Multiplies the attack time in milliseconds. A value of 0.5 halves the attack time; 0.0 makes it instant", scope: "per-voice", constrainer: "VoiceStartModulator" }
  - { name: AttackLevelModulation, desc: "Multiplies the attack peak level as linear gain. A value of 0.5 roughly equals -6 dB", scope: "per-voice", constrainer: "VoiceStartModulator" }
  - { name: DecayTimeModulation, desc: "Multiplies the decay time in milliseconds", scope: "per-voice", constrainer: "VoiceStartModulator" }
  - { name: SustainLevelModulation, desc: "Multiplies the sustain level as linear gain", scope: "per-voice", constrainer: "VoiceStartModulator" }
  - { name: ReleaseTimeModulation, desc: "Multiplies the release time in milliseconds", scope: "per-voice", constrainer: "VoiceStartModulator" }
---
::

### Monophonic Behaviour

In monophonic mode, all voices share a single envelope. The first note always starts a fresh envelope from zero. Subsequent notes either restart from the current value (Retrigger on) or continue from the current state (Retrigger off).

### Visualisation with AHDSRGraph

The `AHDSRGraph` FloatingTile displays the envelope shape with an animated ball showing the current position. Customise the appearance using Global Look and Feel callbacks: `drawAhdsrBackground`, `drawAhdsrPath`, and `drawAhdsrBall`. These require a Global LAF - local LAF does not work for these callbacks.

**See also:** $MODULES.FlexAHDSR$ -- more flexible envelope with independent curve controls per stage, $MODULES.SimpleEnvelope$ -- lighter two-stage envelope when Hold, Decay, and curve shaping are not needed, $MODULES.TableEnvelope$ -- table-driven envelope shapes with separate attack and release curves, $UI.FloatingTiles.AHDSRGraph$ -- FloatingTile for displaying the envelope shape with animated playback position
