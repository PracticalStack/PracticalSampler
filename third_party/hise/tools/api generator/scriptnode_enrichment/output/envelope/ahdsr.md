---
title: AHDSR Envelope
description: "A full AHDSR envelope with attack curve shaping, manual gate, and retrigger support."
factoryPath: envelope.ahdsr
factory: envelope
polyphonic: true
tags: [envelope, ahdsr, modulation]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "envelope.flex_ahdsr", type: alternative, reason: "Adds per-segment curve control, trigger/loop modes, and a draggable UI" }
  - { id: "envelope.simple_ar", type: alternative, reason: "Lighter two-stage envelope when hold, decay, and sustain are not needed" }
  - { id: "envelope.voice_manager", type: companion, reason: "Connect the Gate output here to manage voice lifecycle" }
  - { id: "AHDSR", type: module, reason: "Module-tree equivalent with identical envelope stages" }
commonMistakes:
  - title: "Connect Gate output for voice management"
    wrong: "Leaving the Gate modulation output unconnected and expecting voices to stop automatically."
    right: "Wire the Gate output to an envelope.voice_manager node so voices are released when the envelope finishes."
    explanation: "The envelope multiplies the audio to silence, but HISE still renders the voice unless it receives an explicit voice reset message. The Gate output provides the signal that tells voice_manager when to stop the voice."
  - title: "Retrigger only affects monophonic mode"
    wrong: "Enabling Retrigger and expecting polyphonic voices to restart their envelopes on legato notes."
    right: "Retrigger controls monophonic legato behaviour only. In polyphonic mode each voice already has its own independent envelope."
    explanation: "In polyphonic mode every note-on starts a fresh voice with its own envelope state. The Retrigger parameter is only relevant when the node operates monophonically."
  - title: "Monophonic mode disables voice killing"
    wrong: "Switching to monophonic mode and expecting the envelope to still kill voices when it finishes."
    right: "Add a second envelope in polyphonic mode dedicated to voice lifecycle, or keep the main envelope polyphonic and handle legato separately."
    explanation: "In monophonic mode the envelope loses its ability to terminate voices. The voice-kill detection that normally ensures an envelope is present in the signal chain does not work correctly with monophonic envelopes."
forumReferences:
  - id: 1
    title: "Midichain placement required for sample-accurate modulation"
    summary: "Both the envelope and its target must be inside a midichain container for per-event timing precision."
    topic: 7890
llmRef: |
  envelope.ahdsr

  Full AHDSR envelope that multiplies audio by the envelope value and sends CV + Gate modulation outputs. Equivalent to the HISE AHDSR envelope modulator.

  Signal flow:
    MIDI note-on/off -> state machine -> envelope value
    audio in * envelope value -> audio out
    envelope value -> CV output (0..1)
    voice active -> Gate output (0 or 1)

  CPU: low, polyphonic

  Parameters:
    Timing: Attack (0-10000 ms, default 10), Hold (0-10000 ms, default 20), Decay (0-10000 ms, default 300), Release (0-10000 ms, default 20)
    Levels: AttackLevel (0-1, default 1.0), Sustain (0-1, default 0.5)
    Shape: AttackCurve (0-1, default 0.5; 0=log, 0.5=linear, 1=exp)
    Control: Retrigger (Off/On, default Off; monophonic only), Gate (Off/On, default Off; manual trigger)

  Placement:
    Both envelope and target must be inside a midichain container for sample-accurate modulation.
    Cannot modulate container-level FX -- use Synth.setUseUniformVoiceHandler() for cross-generator modulation.

  When to use:
    Standard amplitude envelope for synth voices. Use flex_ahdsr if you need per-segment curve control or trigger/loop modes.

  Common mistakes:
    - Gate output must be connected to voice_manager for proper voice lifecycle
    - Retrigger only affects monophonic mode
    - Monophonic mode disables voice killing -- add a second polyphonic envelope for lifecycle

  See also:
    [alternative] envelope.flex_ahdsr -- per-segment curves and more modes
    [alternative] envelope.simple_ar -- lighter two-stage envelope
    [companion] envelope.voice_manager -- voice lifecycle from Gate output
    [module] AHDSR -- module-tree equivalent with identical envelope stages
---

The AHDSR envelope is the standard amplitude envelope for scriptnode synthesiser voices. It provides attack, hold, decay, sustain, and release stages with configurable timing, an adjustable attack curve, and support for monophonic retrigger. The node multiplies the input audio by the envelope value per sample, so it shapes amplitude directly without requiring a separate gain node.

In addition to shaping audio, the node sends two modulation outputs: a continuous CV signal (0 to 1) that tracks the envelope value, and a binary Gate signal that remains high while the envelope is active. The Gate output is typically connected to an [envelope.voice_manager]($SN.envelope.voice_manager$) node to ensure voices are stopped once the release phase completes. This node is a direct equivalent of the [AHDSR]($MODULES.AHDSR$) envelope modulator in the HISE module tree.

### Placement

Both the envelope and any node it modulates must be inside a [container.midichain]($SN.container.midichain$) for sample-accurate modulation. Placing either outside the midichain means only the value from the last MIDI message is applied to the entire buffer, losing per-event timing precision. [1]($FORUM_REF.7890$)

The envelope cannot modulate parameters on effects placed at the container (root) level, because containers have no concept of voices. To use an envelope as a global modulator across multiple sound generators, enable a uniform voice handler via `Synth.setUseUniformVoiceHandler()` so that voices are allocated synchronously.

### Modulation Outputs

The envelope has two modulation outputs. **CV** sends the continuous envelope value (0 to 1). **Gate** sends 1 while the envelope is active and drops to 0 when the release phase completes and the voice becomes idle. Connect the Gate output to an [envelope.voice_manager]($SN.envelope.voice_manager$) node for proper voice lifecycle management.

### Limitations

- When Retrigger is enabled in monophonic mode, the envelope continues from its current value rather than restarting from zero. This avoids clicks during legato playing.
- If the Sustain level is set to 0, the envelope transitions directly to idle after the decay phase without entering a sustain hold.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Attack:
      desc: "Time for the envelope to rise from zero to AttackLevel"
      range: "0 - 10000 ms"
      default: "10.0"
    AttackLevel:
      desc: "Peak level at the end of the attack phase"
      range: "0.0 - 1.0"
      default: "1.0"
    Hold:
      desc: "Time the envelope holds at AttackLevel before decaying"
      range: "0 - 10000 ms"
      default: "20.0"
    Decay:
      desc: "Time for the envelope to fall from AttackLevel to Sustain"
      range: "0 - 10000 ms"
      default: "300.0"
    Sustain:
      desc: "Level held while the note is down"
      range: "0.0 - 1.0"
      default: "0.5"
    Release:
      desc: "Time for the envelope to fall from Sustain to zero after note-off"
      range: "0 - 10000 ms"
      default: "20.0"
    AttackCurve:
      desc: "Shape of the attack segment (0 = logarithmic, 0.5 = linear, 1 = exponential)"
      range: "0.0 - 1.0"
      default: "0.5"
    Retrigger:
      desc: "When enabled, legato notes restart the envelope in monophonic mode"
      range: "Off / On"
      default: "Off"
    Gate:
      desc: "Manual gate trigger, independent of MIDI"
      range: "Off / On"
      default: "Off"
  functions:
    stateMachine:
      desc: "Advances through AHDSR stages per sample using exponential curves"
    multiply:
      desc: "Applies the envelope value to each audio sample"
---

```
// envelope.ahdsr - AHDSR amplitude envelope
// audio + MIDI in -> audio out, CV out, Gate out

onNoteOn() {
    stateMachine.start()                // IDLE -> ATTACK
}

process(input) {
    value = stateMachine.tick()         // advance one sample
    output = input * value              // apply envelope to audio
    cvOut = value                       // continuous 0..1
}

onNoteOff() {
    stateMachine.release()              // -> RELEASE -> IDLE
    if (stateMachine == IDLE)
        gateOut = 0                     // signal voice done
}

// State sequence:
// IDLE -> ATTACK -> HOLD -> DECAY -> SUSTAIN -> RELEASE -> IDLE
// Attack ramps to AttackLevel using AttackCurve shape
// Decay falls toward Sustain with exponential curve
// Release falls toward zero with exponential curve
```

::

## Parameters

::parameter-table
---
groups:
  - label: Timing
    params:
      - { name: Attack, desc: "Rise time from zero to AttackLevel. Skewed toward lower values for fine control of short attacks.", range: "0 - 10000 ms", default: "10.0" }
      - { name: Hold, desc: "Duration at AttackLevel before the decay phase begins.", range: "0 - 10000 ms", default: "20.0" }
      - { name: Decay, desc: "Fall time from AttackLevel toward the Sustain level.", range: "0 - 10000 ms", default: "300.0" }
      - { name: Release, desc: "Fall time from the current level to zero after note-off.", range: "0 - 10000 ms", default: "20.0" }
  - label: Levels
    params:
      - { name: AttackLevel, desc: "Peak level reached at the end of the attack phase.", range: "0.0 - 1.0", default: "1.0" }
      - { name: Sustain, desc: "Level held while the note is down, and the target for the decay phase.", range: "0.0 - 1.0", default: "0.5" }
  - label: Shape
    params:
      - { name: AttackCurve, desc: "Curvature of the attack segment. 0 = logarithmic (fast start), 0.5 = linear, 1 = exponential (slow start).", range: "0.0 - 1.0", default: "0.5" }
  - label: Control
    params:
      - { name: Retrigger, desc: "When enabled, every note-on restarts the envelope in monophonic mode. Has no effect in polyphonic mode, where each voice has its own state.", range: "Off / On", default: "Off" }
      - { name: Gate, desc: "Manual gate trigger, independent of MIDI note events. Useful for modulation-driven triggering.", range: "Off / On", default: "Off" }
---
::

**See also:** $SN.envelope.flex_ahdsr$ -- per-segment curve control and trigger/loop modes, $SN.envelope.simple_ar$ -- lighter two-stage envelope, $SN.envelope.voice_manager$ -- voice lifecycle management, $MODULES.AHDSR$ -- module-tree equivalent with identical envelope stages
