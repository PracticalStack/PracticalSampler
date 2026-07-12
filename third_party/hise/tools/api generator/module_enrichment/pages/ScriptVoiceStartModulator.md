---
title: Script Voice Start Modulator
moduleId: ScriptVoiceStartModulator
type: Modulator
subtype: VoiceStartModulator
tags: [custom]
builderPath: b.Modulators.ScriptVoiceStartModulator
screenshot: /images/v2/reference/audio-modules/scriptvoicestartmodulator.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors:
    - { parameter: "User script complexity", impact: "variable", note: "Executes once per voice start - typically low overhead" }
seeAlso:
  - { id: ScriptProcessor, type: companion, reason: "General-purpose HiseScript MIDI processor with the full callback and API set" }
  - { id: Velocity, type: alternative, reason: "Built-in velocity-to-modulation converter - use when a simple velocity curve is sufficient" }
  - { id: KeyNumber, type: alternative, reason: "Built-in key number-to-modulation converter - use when a simple key tracking curve is sufficient" }
commonMistakes:
  - title: "Forgetting to return a value from onVoiceStart"
    wrong: "Writing logic in onVoiceStart without a return statement"
    right: "Always return a value between 0.0 and 1.0 from onVoiceStart"
    explanation: "The return value of onVoiceStart becomes the per-voice modulation value. Without a return statement, the modulation value defaults to 0.0, which silences the voice if the modulator is in a gain chain."
  - title: "Relying on the voiceIndex in onVoiceStop"
    wrong: "Using the voiceIndex parameter in onVoiceStop to identify which voice stopped"
    right: "Use onVoiceStop only for general cleanup tasks that do not depend on the voice index"
    explanation: "The voiceIndex parameter in onVoiceStop is always 0 regardless of which voice actually stopped. It does not reflect the true stopping voice."
  - title: "Expecting the full ScriptProcessor API"
    wrong: "Trying to use Server, FileSystem, or Timer functions inside a Script Voice Start Modulator"
    right: "Use a ScriptProcessor for operations that need the full API - the Script Voice Start Modulator has a reduced API surface"
    explanation: "This module only provides Message, Engine, Synth, Console, and Content objects. Advanced APIs like Server, FileSystem, Threads, and timer functions are not available."
llmRef: |
  Script Voice Start Modulator (Modulator/VoiceStartModulator)

  Computes a per-voice modulation value at note-on using a HiseScript callback. Use for custom velocity curves, round-robin logic, or any scripted per-voice modulation that the built-in modulators cannot express.

  Signal flow:
    noteOn -> onVoiceStart(voiceIndex) -> return value (0.0-1.0) -> modulation output

  CPU: negligible (framework), polyphonic. Executes once per voice start, not per sample.

  Callbacks:
    onInit - runs once at compile time, declare variables, create UI
    onVoiceStart(voiceIndex) - fires at voice start, MUST return 0.0-1.0 modulation value
    onVoiceStop(voiceIndex) - fires on note-off (voiceIndex always 0)
    onController - fires on CC events, access via Message object
    onControl(number, value) - fires on UI component change

  API objects available (reduced set):
    Message, Engine, Synth, Console, Content

  When to use:
    When built-in VoiceStartModulators (Velocity, KeyNumber) cannot express the modulation logic you need. Common uses: custom velocity curves with conditional logic, round-robin selection, random modulation with custom distribution.

  Common mistakes:
    Forgetting to return a value from onVoiceStart (defaults to 0.0 = silence in gain chain).
    Relying on voiceIndex in onVoiceStop (always 0).
    Expecting full ScriptProcessor API (reduced set here).

  See also:
    companion ScriptProcessor - full HiseScript MIDI processor with all callbacks and APIs
    alternative Velocity - built-in velocity-to-modulation converter
    alternative KeyNumber - built-in key number-to-modulation converter
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules that run user-defined logic via HiseScript callbacks" }
---
::

![Script Voice Start Modulator screenshot](/images/v2/reference/audio-modules/scriptvoicestartmodulator.png)

The Script Voice Start Modulator computes a per-voice modulation value at note-on using a HiseScript callback. Place it in any modulation chain where a [Velocity]($MODULES.Velocity$) or [KeyNumber]($MODULES.KeyNumber$) modulator would go, but use it when the built-in modulators cannot express the logic you need. The `onVoiceStart` callback receives the voice index, has access to the incoming MIDI event via the `Message` object, and must return a value between 0.0 and 1.0 that becomes the modulation output for that voice.

Common uses include custom velocity curves with conditional logic (e.g. different curves per key range), round-robin group selection, scripted random modulation with custom distributions, and any per-voice calculation that depends on multiple input values. Since the callback executes only once per voice start rather than per sample, the CPU overhead is minimal even with moderately complex scripts.

## Signal Path

::signal-path
---
glossary:
  functions:
    onInit:
      desc: "Runs once when the script compiles. Declare variables, create UI components"
    onVoiceStart:
      desc: "Fires when a new voice starts. Must return a modulation value between 0.0 and 1.0"
    onVoiceStop:
      desc: "Fires on note-off events. Used for cleanup tasks"
    onController:
      desc: "Fires on CC events. Access the event via the Message object"
    onControl:
      desc: "Fires when a UI component value changes. Receives the component reference and new value"
---

```
// Script Voice Start Modulator - per-voice modulation via HiseScript
// noteOn in -> onVoiceStart -> modulation out (0.0-1.0, per voice)

onInit() {
    // Declare variables, create UI components
}

onVoiceStart(voiceIndex) {
    // Access: Message.getNoteNumber(), Message.getVelocity()
    // Access: Synth.getModulatorValue(), Engine.getHostBpm()

    // Example: custom velocity curve
    local vel = Message.getVelocity();
    local curve = Math.pow(vel / 127.0, 2.0);

    return curve;  // MUST return 0.0 - 1.0
}

onVoiceStop(voiceIndex) {
    // Cleanup, bookkeeping
    // Note: voiceIndex is always 0
}

onController() {
    // Handle CC events
    // Store values for use in onVoiceStart
}

onControl(number, value) {
    // Handle UI component changes
}
```

::

### onInit

The `onInit` callback runs once each time the script is compiled. Use it to declare variables and create UI components. The API surface is more limited than in a [Script Processor]($MODULES.ScriptProcessor$) - you have access to `Content`, `Engine`, `Synth`, `Console`, and `Message`, but not `Server`, `FileSystem`, `Threads`, or timer functions.

### onVoiceStart

The most important callback. Fires each time a new voice starts (triggered by a note-on event). Receives one parameter:

- `voiceIndex` (int) - the index of the starting voice

The `Message` object contains the triggering note-on event, so you can read `Message.getNoteNumber()`, `Message.getVelocity()`, `Message.getChannel()`, and other properties.

**This callback must return a value between 0.0 and 1.0.** The return value becomes the per-voice modulation output:
- **1.0** = no modulation (full pass-through)
- **0.0** = full attenuation (silence, if used in a gain chain)

:::{.warning}
If `onVoiceStart` does not contain a `return` statement, the modulation value defaults to 0.0. In a gain modulation chain, this silences the voice entirely.
:::

Before the callback executes, the voice's gain and pitch values are reset to 1.0, ensuring a clean starting state for each calculation.

### onVoiceStop

Fires when a note-off event is received. Receives one parameter:

- `voiceIndex` (int) - always 0 in the current implementation, regardless of which voice actually stopped

Use this callback only for general bookkeeping tasks (e.g. decrementing a counter, updating a display) that do not depend on knowing which specific voice has stopped.

### onController

Fires on MIDI controller events. The `Message` object provides access to the controller number and value. A common pattern is to store controller values in a variable during `onController` and then read them in `onVoiceStart` to influence the modulation calculation.

### onControl

Fires when a UI component's value changes. Receives two parameters:
- `number` - a reference to the component that changed
- `value` - the new value

Works identically to the `onControl` callback in [Script Processor]($MODULES.ScriptProcessor$).

### Available API Objects

The Script Voice Start Modulator provides a reduced API surface compared to the Script Processor:

- **Message** - read the current MIDI event
- **Synth** - access the parent synthesiser
- **Engine** - global engine properties
- **Console** - debug output
- **Content** - UI component creation

The following APIs are **not** available: Server, FileSystem, Threads, Date, Settings, Colours, Sampler, Buffer. If you need these, use a [Script Processor]($MODULES.ScriptProcessor$) to perform the work and store results in a variable that the modulator can read.

**See also:** $MODULES.ScriptProcessor$ -- general-purpose HiseScript MIDI processor with the full callback and API set, $MODULES.Velocity$ -- built-in velocity-to-modulation converter for simple curves, $MODULES.KeyNumber$ -- built-in key number-to-modulation converter for simple key tracking
