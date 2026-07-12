---
title: Clone Cable
description: "Distributes different values to each clone using a selectable distribution algorithm."
factoryPath: control.clone_cable
factory: control
polyphonic: false
tags: [control, clone]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors:
    - { parameter: "NumClones", impact: "linear", note: "One distribution calculation per active clone" }
seeAlso:
  - { id: "control.clone_forward", type: alternative, reason: "Sends the same value to all clones instead of distributing" }
  - { id: "control.clone_pack", type: alternative, reason: "Uses a slider pack for per-clone values instead of a distribution formula" }
  - { id: "container.clone", type: companion, reason: "The clone container that this node controls" }
commonMistakes:
  - title: "MIDI-reactive modes need a MIDI context"
    wrong: "Using Harmonic or Nyquist mode outside a container that processes MIDI events"
    right: "Place the clone container inside a container.midichain or similar MIDI-aware context."
    explanation: "The Harmonic, Nyquist, and Fixed modes replace the Value parameter with the incoming note frequency on note-on. Without a MIDI context, no note-on events arrive and the modes fall back to the Value knob."
  - title: "Frequency target needs linear 0-20kHz range"
    wrong: "Connecting a MIDI-reactive mode to a frequency parameter with a skewed range"
    right: "Select the 'Linear 0 - 20kHz' preset in the range drop-down menu for the target parameter."
    explanation: "MIDI-reactive modes output a normalised frequency value (note frequency divided by 20000). The target parameter must use an unskewed linear range from 0 to 20000 Hz for correct pitch mapping."
llmRef: |
  control.clone_cable

  Distributes different values to each clone in a container.clone using a selectable distribution algorithm. Nine modes available: Spread, Scale, Triangle, Harmonics, Nyquist, Fixed, Ducker, Random, Toggle. Some modes respond to MIDI note-on frequency.

  Signal flow:
    Control node - no audio processing
    Value + Gamma -> mode algorithm -> per-clone output (normalised)

  CPU: negligible, monophonic

  Parameters:
    NumClones: 1 - 16 (integer, default 1)
      Auto-synced from the parent clone container.
    Value: 0.0 - 1.0 (default 0.0)
      Input value for the distribution function. Replaced by note frequency for MIDI-reactive modes.
    Gamma: 0.0 - 1.0 (default 0.0)
      Shape modifier. Effect varies by mode; ignored by Harmonics, Fixed, Random, Toggle.

  When to use:
    Per-clone parameter differentiation in a clone container: detuning, harmonic series, spatial spread, random variation, volume ducking.

  Common mistakes:
    MIDI-reactive modes need a MIDI context to receive note-on events.
    Frequency targets need the linear 0-20kHz range preset.

  See also:
    [alternative] control.clone_forward -- sends same value to all clones
    [alternative] control.clone_pack -- slider pack for per-clone values
    [companion] container.clone -- the clone container this node controls
---

The clone cable distributes different values to each clone inside a [container.clone]($SN.container.clone$). A Mode property selects the distribution algorithm, and the Value parameter controls the overall intensity of the distribution. The Gamma parameter adjusts the distribution curve shape for modes that support it.

Nine distribution modes are available:

| Mode | Description | Gamma effect | MIDI-reactive |
|------|-------------|-------------|---------------|
| Spread | Bipolar spread from the centre point | Blends linear to sine curve | No |
| Scale | Unipolar ramp from zero to Value | Power curve exponent | No |
| Triangle | V-shape peak at centre, dips at edges | Blends linear to sine-squared curve | No |
| Harmonics | Integer multiples of the input value | Ignored | Yes |
| Nyquist | Harmonic series with high-frequency rolloff | Rolloff steepness | Yes |
| Fixed | Same value to all clones | Ignored | Yes |
| Ducker | Gain compensation based on clone count | Reduces ducking amount | No |
| Random | Random offset around a spread-like base | Ignored | No |
| Toggle | Binary on/off threshold across clones | Ignored | No |

The Harmonics, Nyquist, and Fixed modes respond to MIDI note-on events. When a note-on is received, the note frequency replaces the Value parameter as the distribution input. This makes them suitable for additive synthesis, where each clone produces a different harmonic of the played note. The Random mode also re-randomises on note-on but does not use the frequency.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Input value fed to the distribution function"
      range: "0.0 - 1.0"
      default: "0.0"
    Gamma:
      desc: "Shape modifier for the distribution curve"
      range: "0.0 - 1.0"
      default: "0.0"
    NumClones:
      desc: "Number of active clones to address"
      range: "1 - 16"
      default: "1"
  functions:
    distribute:
      desc: "Applies the selected mode algorithm per clone"
---

```
// control.clone_cable - distributes values across clones
// control in -> per-clone control out (normalised)

onValueChange(Value) {
    for each clone [0..NumClones]:
        output[clone] = distribute(clone, NumClones, Value, Gamma)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: NumClones, desc: "Number of active clones. Automatically synchronised from the parent clone container.", range: "1 - 16", default: "1" }
  - label: Signal
    params:
      - { name: Value, desc: "Input value for the distribution function. For MIDI-reactive modes (Harmonics, Nyquist, Fixed), replaced by the note frequency on note-on.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Gamma, desc: "Shape modifier for the distribution curve. Adjusts how values are spread across clones. Ignored by Harmonics, Fixed, Random, and Toggle modes.", range: "0.0 - 1.0", default: "0.0" }
---
::

The NumClones parameter is automatically kept in sync with the parent clone container. Manual adjustment is possible but will be overwritten when the container's clone count changes. Toggle mode is an exception: it does not auto-update when the clone count changes. Output values are normalised (0-1), so target parameter ranges are applied automatically.

**See also:** $SN.control.clone_forward$ -- sends the same unscaled value to all clones, $SN.control.clone_pack$ -- per-clone values via a slider pack, $SN.container.clone$ -- the clone container
