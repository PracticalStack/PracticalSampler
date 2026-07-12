---
title: Modulators
description: All HISE modulator modules - signal model, targets, parameters, and module list
---

Modulators generate control signals that scale parameters such as gain, pitch, and effect amounts. They are grouped into three subtypes based on their timing behaviour.

## Modulation Signal

All modulators produce a value in the normalised range **0.0 to 1.0**. How this value is applied depends on the chain mode:

- **Gain mode** - the modulation value is multiplied with the target. A value of 1.0 means no change, 0.0 means silence. Multiple modulators in the same chain are multiplied together.
- **Pitch mode** - the modulation value is converted to a bipolar range and added to the pitch signal.
- **Pan mode** - the modulation value is converted to a bipolar range and controls stereo panning.

The exact formula for each mode is shown in the `applyIntensity` function in the pseudocode blocks below.

Modulation signals in HISE are downsampled by a configurable factor (`HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`, default: 8). This means modulators update once every 8 samples rather than at full audio rate, significantly reducing CPU usage. For complex modulation synth projects where higher resolution is needed, this can be set to 1 for sample-accurate modulation.

## Modulation Targets

Modulators are placed inside **modulation chains** that belong to a parent module:

- **Gain Modulation** - scales the output volume. Uses multiplicative (Gain) mode.
- **Pitch Modulation** - modulates the pitch of all voices. Uses additive (Pitch) mode with bipolar range.
- **Effect parameter chains** - some effects expose modulatable parameters with their own modulation chains.
- **Global modulators** - modulators placed inside a [Global Modulator Container]($MODULES.GlobalModulatorContainer$) can be shared across multiple targets using Global Voice Start, Global Time Variant, or Global Envelope proxy modules.
- **Extra modulation chains** - scriptnode and hardcoded modules can expose additional modulation slots beyond the standard Gain and Pitch chains. See [Scriptnode Modulation Bridge](#scriptnode-modulation-bridge) below.

### Scriptnode Modulation Bridge

Scriptnode and hardcoded modules can expose extra modulation chain slots that bridge the HISE modulator system into the scriptnode network. The number of extra slots is configured per module type via preprocessor macros in the project settings' **Extra Definitions** field:

| Module Type | Preprocessor Macro | Default |
|---|---|---|
| Script FX | `HISE_NUM_SCRIPTNODE_FX_MODS` | 0 |
| Polyphonic Script FX | `HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS` | 0 |
| Scriptnode Synthesiser | `HISE_NUM_SCRIPTNODE_SYNTH_MODS` | 2 |
| Hardcoded Master FX | `NUM_HARDCODED_FX_MODS` | 0 |
| Hardcoded Polyphonic FX | `NUM_HARDCODED_POLY_FX_MODS` | 0 |
| Hardcoded Synthesiser | `NUM_HARDCODED_SYNTH_MODS` | 2 |

Once extra slots are configured, traditional HISE modulators (LFOs, envelopes, velocity modulators, etc.) can be placed in these slots. The modulation signals are produced at control rate (downsampled by a factor of 8) and delivered to the scriptnode network. Each extra slot supports one of two mutually exclusive connection modes:

**Direct parameter modulation** — Enable the *External Modulation* property on a root parameter in the scriptnode editor and select a modulation mode (Combined, Scale, or Add). The modulation chain drives the parameter directly at the resolution defined by the network's modulation block size. No node is needed inside the network — the parameter receives the modulated value automatically. This is the simpler approach when you just need a HISE modulator to scale or offset a network parameter.

**extra_mod node** — Place a `core::extra_mod` node in the network and set its **Index** parameter to match the modulation slot. The root parameter is rerouted to control the **base value** of the modulation chain rather than the network parameter directly. The `extra_mod` node picks up the full modulation signal inside the network, where it can be routed to any target, processed by other nodes, or split to multiple destinations. The root parameter still controls the parameter from the user's perspective — changing it adjusts the base value that the modulation signal is applied to. This also means a UI knob connected to the root parameter will display the modulation signal.

> [!Warning:Mutually exclusive modes] These two modes cannot be combined on the same slot. If a root parameter has External Modulation enabled and an `extra_mod` node is also connected to the same index, a warning icon will appear. Use one or the other per slot.

In addition to extra modulation slots, sound generators' built-in **Pitch Modulation** chain is bridged to the network via `core::pitch_mod` nodes. Any HISE pitch modulators placed in the pitch chain are automatically routed to these nodes.

Modulation that does not need to be exposed to the parent module tree can be handled entirely within the scriptnode graph using `container.modchain`, modulation cables, and `control.*` nodes. These operate within the network's own sample rate and do not require extra modulation slots.

## Common Parameters

All modulators share these properties from the `Modulation` base class.

> [!Warning:Scripting API] Intensity and Bipolar are **not** regular parameters - they do not go through the `setAttribute()` system. In scripts, use `Modulator.setIntensity()` and `Modulator.setBipolar()` instead.

::parameter-table
---
groups:
  - label: Modulation
    params:
      - { name: Intensity, desc: "Controls the modulation depth. The effect depends on the chain mode (see applyIntensity in pseudocode below). Set via `setIntensity()`.", range: "0 - 100% (Gain) or -12 - +12 st (Pitch)", default: "100% / 1 st" }
      - { name: Bipolar, desc: "Converts the output from unipolar (0 to 1) to bipolar (-1 to +1). Automatically enabled in Pitch chains. Set via `setBipolar()`.", range: "Off / On", default: "Off (Gain) / On (Pitch)" }
---
::

## Voice Start Modulators

Compute a single value at note-on that remains constant for the lifetime of the voice. Use them for velocity-to-volume mapping, per-note randomisation, or any modulation that does not need to change after the note starts.

::signal-path
---
glossary:
  functions:
    calculate:
      desc: "Computes the raw modulation value (0.0 to 1.0) from input data. Implementation varies per module (e.g. velocity lookup, random number, note number mapping)."
    applyIntensity:
      desc: "Applies the Intensity parameter to the raw value. The formula depends on the chain's Modulation::Mode."
      detail: |
        GainMode:   (1 - intensity) + (intensity * value)
        PitchMode:  intensity * value
        PanMode:    intensity * value
        GlobalMode: intensity * value
---

```
// Voice Start Modulator
// polyphonic, computed once at note-on

onNoteOn(voice):
    rawValue = calculate(noteNumber, velocity)
    value = applyIntensity(rawValue)
    // value is fixed for the lifetime of this voice
```

::

#### Modules

- [Array Modulator]($MODULES.ArrayModulator$): Creates a modulation signal from a slider pack array indexed by MIDI note number, allowing per-note modulation values.
- [Constant]($MODULES.Constant$): Creates a constant modulation signal (1.0) that can be used as a fixed gain offset or modulation source.
- [Event Data Modulator]($MODULES.EventDataModulator$): Creates a modulation value based on event data written through the global routing manager, allowing external control data to modulate voices.
- [Global Static Time Variant Modulator]($MODULES.GlobalStaticTimeVariantModulator$): Captures the current value of a global TimeVariantModulator at voice start, creating a constant per-voice modulation based on the LFO/envelope state at note-on.
- [Global Voice Start Modulator]($MODULES.GlobalVoiceStartModulator$): Connects to a global VoiceStartModulator in a GlobalModulatorContainer, allowing voice-start modulation to be shared across multiple targets.
- [Notenumber Modulator]($MODULES.KeyNumber$): Creates a modulation value based on the MIDI note number, with optional table mapping for custom response curves.
- [Random Modulator]($MODULES.Random$): Generates a random value at each voice start, with optional table mapping for custom probability distributions.
- [Script Voice Start Modulator]($MODULES.ScriptVoiceStartModulator$): Computes a per-voice modulation value at note-on using a HiseScript callback, for custom velocity curves or scripted voice logic.
- [Velocity Modulator]($MODULES.Velocity$): Creates a modulation value from the MIDI velocity of incoming note messages, with optional table mapping and decibel conversion.

## Time Variant Modulators

Produce a continuous monophonic signal that updates every audio block, shared across all voices. Use them for LFOs, MIDI CC mapping, and macro parameters where per-voice independence is not needed.

::signal-path
---
glossary:
  functions:
    calculate:
      desc: "Computes the raw modulation value (0.0 to 1.0) for this audio block. Implementation varies per module (e.g. LFO phase, CC value, macro position)."
    applyIntensity:
      desc: "Applies the Intensity parameter to the raw value. The formula depends on the chain's Modulation::Mode."
      detail: |
        GainMode:   (1 - intensity) + (intensity * value)
        PitchMode:  intensity * value
        PanMode:    intensity * value
        GlobalMode: intensity * value
---

```
// Time Variant Modulator
// monophonic, updates every audio block

process(blockSize):
    rawValue = calculate(blockSize)
    value = applyIntensity(rawValue)
    // same value applied to all active voices
```

::

#### Modules

- [Global Time Variant Modulator]($MODULES.GlobalTimeVariantModulator$): Shares a global TimeVariantModulator signal across multiple targets, allowing real-time continuous modulation from a single source.
- [Hardcoded Time Variant Modulator]($MODULES.HardcodedTimevariantModulator$): Runs a compiled C++ DSP network as a monophonic time-variant modulator with dynamic parameters.
- [LFO Modulator]($MODULES.LFO$): Generates a periodic modulation signal with multiple waveform types, tempo sync, and an optional step sequencer mode.
- [Macro Modulator]($MODULES.MacroModulator$): A modulator controlled by a macro controller slot, allowing real-time automation and MIDI learn functionality.
- [Midi Controller]($MODULES.MidiController$): Creates a modulation signal from MIDI CC messages with adjustable smoothing and optional table mapping for custom response curves.
- [Pitch Wheel Modulator]($MODULES.PitchWheel$): Creates a monophonic modulation signal from the pitch wheel, with smoothing to reduce stepping artifacts.
- [Script Time Variant Modulator]($MODULES.ScriptTimeVariantModulator$): Generates a continuous monophonic modulation signal from a scriptnode DSP network or HiseScript timer callback.

## Envelope Modulators

Produce per-voice signals that evolve over time, typically with attack/release stages. Envelope modulators control voice lifetime - when the envelope returns to zero after release, the voice is terminated. Every sound generator needs at least one envelope in its Gain chain.

::signal-path
---
glossary:
  functions:
    startEnvelope:
      desc: "Initialises the envelope state for a voice and begins the attack stage."
    startRelease:
      desc: "Transitions the envelope to its release stage, beginning the fade-out."
    calculate:
      desc: "Computes the raw envelope value (0.0 to 1.0) for this voice and audio block. Implementation varies per module (e.g. AHDSR stages, table lookup, scriptnode network)."
    applyIntensity:
      desc: "Applies the Intensity parameter to the raw value. The formula depends on the chain's Modulation::Mode."
      detail: |
        GainMode:   (1 - intensity) + (intensity * value)
        PitchMode:  intensity * value
        PanMode:    intensity * value
        GlobalMode: intensity * value
    inRelease:
      desc: "Returns true if the voice's envelope is in the release stage."
    killVoice:
      desc: "Terminates the voice. Called when the envelope has finished its release and the output is silent."
---

```
// Envelope Modulator
// polyphonic, updates every audio block per voice

onNoteOn(voice):
    startEnvelope(voice)

onNoteOff(voice):
    startRelease(voice)

process(voice, blockSize):
    rawValue = calculate(voice, blockSize)
    value = applyIntensity(rawValue)

    if value == 0 and inRelease(voice):
        killVoice(voice)
```

::

Envelope modulators have two additional parameters:

::parameter-table
---
groups:
  - label: Voice Management
    params:
      - { name: Monophonic, desc: "Processes a single shared envelope for all voices instead of one envelope per voice.", range: "Off / On", default: "Off" }
      - { name: Retrigger, desc: "Restarts the envelope from the beginning when a new note is played while another is held. Only applies when Monophonic is enabled.", range: "Off / On", default: "On" }
---
::

#### Modules

- [AHDSR Envelope]($MODULES.AHDSR$): An AHDSR envelope with adjustable curve shapes, optional downsampling for CPU savings, and per-voice modulation of all time parameters.
- [EventData Envelope]($MODULES.EventDataEnvelope$): An envelope modulator for time-varying event data slots, with smoothing for continuous modulation changes.
- [Flex AHDSR Envelope]($MODULES.FlexAHDSR$): A more complex AHDSR envelope with draggable curves and multiple playback modes.
- [Global Envelope Modulator]($MODULES.GlobalEnvelopeModulator$): Connects to a global EnvelopeModulator in a GlobalModulatorContainer, allowing envelope modulation to be shared across multiple targets.
- [Hardcoded Envelope Modulator]($MODULES.HardcodedEnvelopeModulator$): Runs a compiled C++ DSP network as a polyphonic envelope modulator with per-voice state and voice management.
- [MPE Modulator]($MODULES.MPEModulator$): Creates per-voice modulation from MPE pressure, slide, or glide gestures with adjustable smoothing and default values.
- [Matrix Modulator]($MODULES.MatrixModulator$): Combines multiple global modulators into a single modulation source with a base value and smoothed output.
- [Script Envelope Modulator]($MODULES.ScriptEnvelopeModulator$): Generates a polyphonic envelope signal from a scriptnode DSP network, with per-voice state and voice kill detection.
- [Scriptnode Voice Killer]($MODULES.ScriptnodeVoiceKiller$): Monitors a scriptnode envelope's gate signal and terminates voices when the gate closes, required for voice management in scriptnode-based envelopes.
- [Simple Envelope]($MODULES.SimpleEnvelope$): A lightweight two-stage envelope with attack and release, supporting linear or exponential curves.
- [Table Envelope]($MODULES.TableEnvelope$): An envelope with fully customizable attack and release shapes drawn as lookup tables.
