---
title: EventData Envelope
moduleId: EventDataEnvelope
type: Modulator
subtype: EnvelopeModulator
tags: [routing]
builderPath: b.Modulators.EventDataEnvelope
screenshot: /images/v2/reference/audio-modules/eventdataenvelope.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: EventDataModulator, type: companion, reason: "Sample-and-hold version that reads the event data slot once at note-on instead of polling continuously" }
  - { id: MPEModulator, type: alternative, reason: "Per-voice continuous modulation from MPE controller data instead of custom event data slots" }
commonMistakes:
  - title: "EventData Envelope does not kill voices"
    wrong: "Using EventData Envelope as the only envelope modulator in a sound generator"
    right: "Always pair it with another envelope (such as AHDSR) that handles voice termination"
    explanation: "EventData Envelope always reports itself as active and never signals the end of a voice. Without a companion envelope to release voices, they accumulate indefinitely."
  - title: "SmoothingTime of 0 means instant jumps"
    wrong: "Expecting smooth transitions with the default SmoothingTime of 0 ms"
    right: "Set SmoothingTime to a non-zero value (e.g. 20 ms) to avoid audible clicks when slot values change"
    explanation: "At 0 ms the value snaps immediately to the new target with no interpolation, which can produce clicks on gain-mapped parameters."
  - title: "Script must write slot data before or during the note"
    wrong: "Adding EventData Envelope without writing any event data from script, expecting automatic values"
    right: "Use the GlobalRoutingManager API to write data to the target slot before or during the note"
    explanation: "The envelope only reads from an event data slot. If no script writes to that slot, every note receives the DefaultValue."
customEquivalent:
  approach: hisescript
  moduleType: ScriptEnvelopeModulator
  complexity: simple
  description: "Poll a custom variable or event data in the onControl callback of a ScriptEnvelopeModulator with a Timer for continuous updates"
llmRef: |
  EventData Envelope (Modulator/EnvelopeModulator)

  Continuously polls an event data slot every audio block and outputs the value as a per-voice modulation signal. Unlike EventDataModulator which snapshots once at note-on, this envelope re-reads the slot throughout the voice's lifetime, picking up mid-note changes. A linear ramp smoother prevents clicks when values change.

  Signal flow:
    each audio block: read slot[SlotIndex] for this voice's event ID -> if changed, ramp to new value -> modulation output

  CPU: low, polyphonic (one slot read per block + per-sample linear ramp during transitions)

  Parameters:
    Monophonic (Off/On, default dynamic) - restricts output to a single shared voice
    Retrigger (Off/On, default On) - re-reads the slot when a new note arrives in monophonic mode
    SlotIndex (0-16, default 0) - which event data slot to read (0-15 valid; 16 wraps to 0)
    DefaultValue (0 - 100%, default 0%) - fallback when the slot has not been written
    SmoothingTime (0 - 2000 ms, default 0 ms) - linear ramp time for value transitions

  When to use:
    Continuous per-voice modulation driven by scripted event data. Typical uses include per-note expression curves, scripted velocity-to-brightness envelopes, or any scenario where a script updates modulation data while a voice is sounding.

  Common mistakes:
    Does not kill voices - must be paired with another envelope like AHDSR.
    SmoothingTime defaults to 0 ms (instant jumps) - set a non-zero value to avoid clicks.
    Script must write data to the slot via GlobalRoutingManager before or during the note.

  Custom equivalent:
    hisescript via ScriptEnvelopeModulator, simple complexity.

  See also:
    companion EventDataModulator - sample-and-hold version (reads once at note-on)
    alternative MPEModulator - per-voice continuous modulation from MPE instead of custom data
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![EventData Envelope screenshot](/images/v2/reference/audio-modules/eventdataenvelope.png)

The EventData Envelope continuously reads a value from an event data slot and outputs it as a per-voice modulation signal. It polls the slot every audio block, so mid-note changes written by script are picked up automatically. This makes it suitable for continuous per-voice modulation driven by scripted data - for example, custom expression curves or per-note parameter automation.

Unlike the [EventData Modulator]($MODULES.EventDataModulator$), which snapshots the slot value once at note-on and holds it for the voice's lifetime, this envelope re-reads the slot on every audio block. A configurable linear ramp smoother prevents clicks when the value changes. The envelope never terminates a voice on its own, so it must always be paired with another envelope (such as AHDSR) that handles voice lifecycle.

## Signal Path

::signal-path
---
glossary:
  parameters:
    SlotIndex:
      desc: "Which event data slot (0-15) to read"
      range: "0 - 16"
      default: "0"
    DefaultValue:
      desc: "Fallback value when the slot has not been written"
      range: "0 - 100%"
      default: "0%"
    SmoothingTime:
      desc: "Linear ramp time for smoothing value transitions"
      range: "0 - 2000 ms"
      default: "0 ms"
  functions:
    readSlot:
      desc: "Reads the current value from the event data slot for this voice's event ID"
    linearRamp:
      desc: "Per-sample linear interpolation toward the target value over SmoothingTime"
  modulations: {}
---

```
// EventData Envelope - continuous event data to modulation
// event data slot in -> modulation out (per voice, per block)

onNoteOn() {
    value = readSlot(SlotIndex) or DefaultValue
}

onAudioBlock() {
    target = readSlot(SlotIndex) or DefaultValue

    if (target != currentTarget)
        linearRamp(target, SmoothingTime)

    // output: ramped value each sample -> modulation
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Voice Mode
    params:
      - { name: Monophonic, desc: "Restricts envelope output to a single shared voice. All subsequent voices copy the result from the first rendered voice.", range: "Off / On", default: "(dynamic)" }
      - name: Retrigger
        desc: "When enabled in monophonic mode, a new note re-reads the slot using the new event's data. The ramp continues from its current position toward the new value."
        range: "Off / On"
        default: "On"
        hints:
          - type: info
            text: "Only has an effect when Monophonic is enabled."
  - label: Event Data
    params:
      - name: SlotIndex
        desc: "Which event data slot to read. Slots are shared with EventData Modulator via the same storage."
        range: "0 - 16"
        default: "0"
        hints:
          - type: warning
            text: "Valid slot indices are 0-15. Index 16 wraps to slot 0."
      - { name: DefaultValue, desc: "The value used when the event data slot has not been written for this event", range: "0 - 100%", default: "0%" }
  - label: Smoothing
    params:
      - name: SmoothingTime
        desc: "Duration of the linear ramp applied when the slot value changes. Higher values produce smoother transitions but add latency to value changes."
        range: "0 - 2000 ms"
        default: "0 ms"
        hints:
          - type: warning
            text: "At 0 ms (default), value changes are instant with no interpolation. This can cause audible clicks when modulating gain or filter parameters. Set a small value (10-50 ms) for smooth transitions."
---
::

### Writing Event Data

The envelope reads from slots managed by the [GlobalRoutingManager]($API.GlobalRoutingManager$). A script must write values to the target slot for the envelope to produce meaningful output. Since the envelope polls the slot every audio block, values written mid-note are picked up on the next block boundary.

The 16 event data slots are shared between EventData Envelope and [EventData Modulator]($MODULES.EventDataModulator$). Both modules read from the same storage, so a single script write can drive both a continuous envelope and a voice-start snapshot on different slots (or even the same slot for different purposes).

### Voice Management

The EventData Envelope always reports itself as playing and never signals that a voice should stop. This is by design - it is a data reader, not a traditional attack-release envelope. If it is the only envelope modulator in a sound generator, voices will accumulate indefinitely and never release.

Always pair it with at least one other envelope that handles voice termination, such as an AHDSR. The EventData Envelope continues to read and output values through the release phase of the companion envelope, with no value freezing at note-off.

**See also:** $MODULES.EventDataModulator$ -- sample-and-hold companion that reads the event data slot once at note-on, $MODULES.MPEModulator$ -- per-voice continuous modulation from MPE controller data instead of custom event data slots
