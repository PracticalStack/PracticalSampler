---
title: Scriptnode Voice Killer
moduleId: ScriptnodeVoiceKiller
type: Modulator
subtype: EnvelopeModulator
tags: [custom]
builderPath: b.Modulators.ScriptnodeVoiceKiller
screenshot: /images/v2/reference/audio-modules/scriptnodevoicekiller.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: ScriptEnvelopeModulator, type: companion, reason: "Scriptnode envelope that requires a ScriptnodeVoiceKiller for voice management" }
  - { id: HardcodedEnvelopeModulator, type: companion, reason: "Compiled scriptnode envelope that requires a ScriptnodeVoiceKiller for voice management" }
  - { id: AHDSR, type: alternative, reason: "Built-in envelope with integrated voice killing - does not need a separate voice killer" }
commonMistakes:
  - title: "Forgetting to add ScriptnodeVoiceKiller when using scriptnode envelopes"
    wrong: "Using a ScriptEnvelopeModulator or HardcodedEnvelopeModulator without a ScriptnodeVoiceKiller"
    right: "Add a ScriptnodeVoiceKiller to the Gain Modulation chain of the same sound generator"
    explanation: "Without a ScriptnodeVoiceKiller, voices started by the sound generator will never be terminated. Notes will pile up indefinitely, consuming CPU until the voice limit is reached."
  - title: "Placing the voice killer in the wrong modulation chain"
    wrong: "Adding the ScriptnodeVoiceKiller to a modulation chain other than Gain Modulation"
    right: "Place the ScriptnodeVoiceKiller in the Gain Modulation chain of the parent sound generator"
    explanation: "The module only registers itself as the voice killer when placed in the Gain Modulation chain. In any other chain, it does nothing."
llmRef: |
  Scriptnode Voice Killer (Modulator/EnvelopeModulator)

  A structural envelope modulator that monitors a scriptnode envelope's gate signal and terminates voices when the gate closes. Required whenever a ScriptEnvelopeModulator or HardcodedEnvelopeModulator is used for voice management. It produces a constant 1.0 modulation output and does not shape the signal.

  Signal flow:
    noteOn -> voice marked active -> constant 1.0 modulation output
    scriptnode gate close -> voice marked inactive -> voice killed by HISE

  CPU: negligible, polyphonic. Only performs atomic boolean operations and a buffer fill.

  Parameters:
    Monophonic (Off/On, default dynamic) - shares one envelope state across all voices
    Retrigger (Off/On, default On) - restarts on new notes in monophonic mode

  When to use:
    Add to the Gain Modulation chain of any sound generator that uses a ScriptEnvelopeModulator or HardcodedEnvelopeModulator. Without it, voices will never stop. Not needed for built-in envelopes like AHDSR, which handle voice killing internally.

  Common mistakes:
    Forgetting to add it causes voices to pile up indefinitely.
    Placing it outside the Gain Modulation chain renders it inert.

  See also:
    companion ScriptEnvelopeModulator - scriptnode envelope requiring this voice killer
    companion HardcodedEnvelopeModulator - compiled scriptnode envelope requiring this voice killer
    alternative AHDSR - built-in envelope with integrated voice killing
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules that run user-defined DSP logic via scriptnode networks, compiled C++ code, or HiseScript callbacks" }
---
::

![Scriptnode Voice Killer screenshot](/images/v2/reference/audio-modules/scriptnodevoicekiller.png)

The Scriptnode Voice Killer is a companion module for scriptnode-based envelopes. Built-in envelopes like $MODULES.AHDSR$ handle voice killing internally - when the envelope finishes its release phase, it signals HISE to stop the voice. Scriptnode envelopes ([ScriptEnvelopeModulator]($MODULES.ScriptEnvelopeModulator$) and [HardcodedEnvelopeModulator]($MODULES.HardcodedEnvelopeModulator$)) cannot do this on their own because the scriptnode network runs in a separate processing context. The Scriptnode Voice Killer bridges the two systems: it registers itself with the parent sound generator's scriptnode network, receives a callback when the envelope's gate closes, and marks the voice as inactive so HISE can reclaim it.

The module must be placed in the **Gain Modulation** chain of the same sound generator that hosts the scriptnode envelope. It produces a constant modulation value of 1.0 (unity gain), so it does not affect the signal level. Its only function is voice lifecycle management.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Monophonic:
      desc: "Shares one envelope state across all voices instead of per-voice tracking"
      range: "Off / On"
      default: "(dynamic)"
    Retrigger:
      desc: "Restarts the voice state when a new note arrives in monophonic mode"
      range: "Off / On"
      default: "On"
  functions:
    registerWithNetwork:
      desc: "Connects to the parent sound generator's scriptnode network to receive gate-close callbacks"
---

```
// Scriptnode Voice Killer - voice lifecycle bridge
// noteOn in -> constant 1.0 modulation out

// On initialisation:
registerWithNetwork(parentSynth)

onNoteOn() {
    voice.active = true
    output = 1.0     // constant unity, no signal shaping
}

// Called by the scriptnode network when the envelope gate closes:
onGateClose(voiceIndex) {
    voice.active = false
    // HISE detects inactive voice and kills it
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Voice Mode
    params:
      - name: Monophonic
        desc: "Shares a single voice state across all voices instead of tracking each voice independently"
        range: "Off / On"
        default: "(dynamic)"
      - name: Retrigger
        desc: "Restarts the voice tracking when a new note arrives in monophonic mode. Has no effect in polyphonic mode"
        range: "Off / On"
        default: "On"
---
::

### When to Use

Add a Scriptnode Voice Killer whenever the sound generator uses a scriptnode-based envelope as its primary amplitude envelope. This includes both interpreted networks ([ScriptEnvelopeModulator]($MODULES.ScriptEnvelopeModulator$)) and compiled networks ([HardcodedEnvelopeModulator]($MODULES.HardcodedEnvelopeModulator$)). Place it in the Gain Modulation chain of the same sound generator - it will not function in any other chain.

Built-in envelopes ($MODULES.AHDSR$, $MODULES.SimpleEnvelope$, $MODULES.FlexAHDSR$, $MODULES.TableEnvelope$) do not need a Scriptnode Voice Killer because they handle voice killing internally.

> **Warning:** If you forget to add a Scriptnode Voice Killer, voices will never be terminated. They will accumulate with each note played, steadily increasing CPU usage until the voice limit is reached. This is one of the most common setup mistakes when working with scriptnode envelopes.

**See also:** $MODULES.ScriptEnvelopeModulator$ -- scriptnode envelope that requires this module for voice management, $MODULES.HardcodedEnvelopeModulator$ -- compiled scriptnode envelope that requires this module for voice management, $MODULES.AHDSR$ -- built-in envelope with integrated voice killing
