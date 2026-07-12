---
title: Envelope Follower
description: "A polyphonic envelope follower that tracks input amplitude with configurable attack and release."
factoryPath: dynamics.envelope_follower
factory: dynamics
polyphonic: true
tags: [dynamics, envelope, follower, analysis, modulation]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: "voice count", impact: "linear", note: "Per-voice envelope calculation" }
seeAlso:
  - { id: "dynamics.comp", type: companion, reason: "Compressor for dynamics processing driven by similar detection" }
  - { id: "dynamics.gate", type: companion, reason: "Gate for dynamics processing driven by similar detection" }
commonMistakes:
  - title: "ProcessSignal On replaces audio output"
    wrong: "Enabling ProcessSignal and expecting audio to pass through unchanged"
    right: "Leave ProcessSignal Off for analysis-only mode. Enable it only when the envelope signal itself is the desired output."
    explanation: "When ProcessSignal is On, all audio channels are replaced with the envelope value. This produces a DC-like signal following the input amplitude, not usable as audio output."
llmRef: |
  dynamics.envelope_follower

  Polyphonic envelope follower that tracks the peak amplitude of the input signal. Each voice maintains its own envelope state. Outputs the tracked envelope as a normalised modulation signal (0..1).

  Signal flow:
    audio in -> peak detect -> attack/release follower -> modulation out (envelope 0..1)
    if ProcessSignal=On: audio replaced with envelope value

  CPU: low, polyphonic (scales linearly with voice count)

  Parameters:
    Attack: 0 - 1000 ms (default 20). Envelope attack time.
    Release: 0 - 1000 ms (default 20). Envelope release time.
    ProcessSignal: Off / On (default Off). Off = pass audio through; On = replace audio with envelope.

  When to use:
    Tracking input amplitude for modulation routing. Commonly paired with math.mul or core.gain for sidechain-style ducking or level-dependent effects. Use dynamics.comp/gate/limiter when gain reduction is needed directly.

  Common mistakes:
    ProcessSignal On replaces audio with envelope DC signal.

  See also:
    [companion] dynamics.comp - compressor for dynamics processing
    [companion] dynamics.gate - gate for dynamics processing
---

The only polyphonic node in the dynamics factory, this envelope follower tracks the peak amplitude of the input signal. Each voice maintains its own envelope state with independent attack and release smoothing, making it suitable for per-voice amplitude modulation in synthesiser patches. The tracked envelope is output as a normalised modulation signal (0..1), which can be routed to other parameters for amplitude-dependent processing such as ducking, auto-gain, or visual metering.

By default, audio passes through the node unchanged while the envelope is tracked in the background. Enabling ProcessSignal replaces the audio output with the envelope value itself.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Attack:
      desc: "Envelope attack time"
      range: "0 - 1000 ms"
      default: "20"
    Release:
      desc: "Envelope release time"
      range: "0 - 1000 ms"
      default: "20"
    ProcessSignal:
      desc: "Off: pass audio through; On: replace audio with envelope value"
      range: "Off / On"
      default: "Off"
  functions:
    peakDetect:
      desc: "Computes the maximum absolute sample value across all channels"
    followEnvelope:
      desc: "Smooths the detected peak with per-voice attack and release timing"
---

```
// dynamics.envelope_follower - amplitude tracker
// audio in -> audio out + modulation out

process(input) {
    peak = peakDetect(input)
    envelope = followEnvelope(peak, Attack, Release)

    if (ProcessSignal == On)
        output = envelope    // replace audio with envelope value
    else
        output = input       // pass audio through unchanged

    modulation = envelope    // 0..1 normalised
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Envelope
    params:
      - { name: Attack, desc: "How quickly the envelope rises to follow increasing amplitude.", range: "0 - 1000 ms", default: "20" }
      - { name: Release, desc: "How quickly the envelope decays when amplitude decreases.", range: "0 - 1000 ms", default: "20" }
  - label: Configuration
    params:
      - { name: ProcessSignal, desc: "Off passes audio through unchanged (analysis only). On replaces all audio channels with the envelope value.", range: "Off / On", default: "Off" }
---
::

### Detection Method

The envelope follower uses peak detection (not RMS), taking the maximum absolute sample value across all channels per frame. For RMS-based detection, use [dynamics.updown_comp]($SN.dynamics.updown_comp$) with its RMS toggle. The attack and release range (0--1000 ms) is wider than [dynamics.comp]($SN.dynamics.comp$), [dynamics.gate]($SN.dynamics.gate$), and [dynamics.limiter]($SN.dynamics.limiter$) (0--250 ms), allowing slower envelope shapes for gentle amplitude tracking.

**See also:** $SN.dynamics.comp$ -- compressor for dynamics processing, $SN.dynamics.gate$ -- gate for dynamics processing
