---
title: Control Nodes
description: "The control factory contains utility nodes for modulation connection manipulation, value transformation, and control signal routing."
factory: control
---

The control factory contains nodes that modify values sent through modulation and parameter connections. Most nodes in this factory do not process the audio signal (the placement in the network does not matter). Instead they have a modulation source and a Value parameter that is intended to be modulated, so you can inject the node into a cable connection to transform or route the control signal before it reaches its target.

> Be aware that the modulation update rate is determined by the container that contains **all** nodes of a cable connection. If you enable CPU profiling, it will also show you the modulation update rate of each cable so you can spot irregularities.

### Scaled and Unscaled

By default, parameter modulation in scriptnode uses the target parameter's range to convert values. A normalised source value of 0.0 maps to the range minimum and 1.0 maps to the maximum, with skew applied. This means you do not need to convert between different ranges manually.

Some nodes bypass this range conversion and send raw values directly -- these are the **unscaled** nodes. A valid use case is [$SN.control.tempo_sync$]($SN.control.tempo_sync$): when the tempo changes it sends an exact millisecond value regardless of the target's range. You can identify unscaled parameters by the small **U** icon next to them in the interface.

> Be aware that even if a node has an unscaled Value parameter, other parameters on the same node may still be scaled. For example, [$SN.control.smoothed_parameter_unscaled$]($SN.control.smoothed_parameter_unscaled$) has a scaled SmoothingTime parameter.

## Nodes

### Value Processing

| Node | Description |
|------|-------------|
| [$SN.control.pma$]($SN.control.pma$) | Multiply-add with 0--1 clamping. Normalised output. |
| [$SN.control.pma_unscaled$]($SN.control.pma_unscaled$) | Multiply-add without clamping. Raw unnormalised output. |
| [$SN.control.minmax$]($SN.control.minmax$) | Maps normalised 0--1 input to a custom range with skew, step, and polarity. |
| [$SN.control.bipolar$]($SN.control.bipolar$) | Centres modulation around 0.5 with configurable scale and gamma curve. |
| [$SN.control.intensity$]($SN.control.intensity$) | HISE intensity formula: crossfades between 1.0 (no effect) and the input value. |
| [$SN.control.normaliser$]($SN.control.normaliser$) | Pass-through that marks the output as normalised for range conversion. |
| [$SN.control.converter$]($SN.control.converter$) | 14-mode unit converter (ms to Hz, dB to gain, MIDI to Hz, etc.). |
| [$SN.control.change$]($SN.control.change$) | Filters duplicate values. Only forwards when the value actually changes. |
| [$SN.control.compare$]($SN.control.compare$) | Two-input comparator with 8 modes (EQ, GT, LT, MIN, MAX, etc.). |
| [$SN.control.logic_op$]($SN.control.logic_op$) | Boolean logic (AND, OR, XOR) with a 0.5 threshold. |
| [$SN.control.unscaler$]($SN.control.unscaler$) | Pass-through that bypasses target range conversion (unnormalised output). |

### Signal Routing

| Node | Description |
|------|-------------|
| [$SN.control.bang$]($SN.control.bang$) | Stores a value and emits it when triggered (Bang > 0.5). |
| [$SN.control.blend$]($SN.control.blend$) | Linear interpolation between two values using an Alpha parameter. |
| [$SN.control.input_toggle$]($SN.control.input_toggle$) | Hard switch between two values based on an Input selector. |
| [$SN.control.branch_cable$]($SN.control.branch_cable$) | Demultiplexer: routes a value to one of N output slots. |
| [$SN.control.xfader$]($SN.control.xfader$) | Distributes fade coefficients across N outputs using selectable curve modes. |
| [$SN.control.cable_expr$]($SN.control.cable_expr$) | Custom SNEX expression transform on a control value. |
| [$SN.control.cable_table$]($SN.control.cable_table$) | Lookup table with linear interpolation for value remapping. |
| [$SN.control.cable_pack$]($SN.control.cable_pack$) | Slider pack lookup with discrete step selection. |
| [$SN.control.delay_cable$]($SN.control.delay_cable$) | Delays a control value by a configurable number of samples. |

### MIDI and Timing

| Node | Description |
|------|-------------|
| [$SN.control.midi$]($SN.control.midi$) | Converts MIDI note events to normalised modulation (Gate, Velocity, NoteNumber, Frequency, Random). |
| [$SN.control.midi_cc$]($SN.control.midi_cc$) | Listens for a specific MIDI CC, pitch bend, or aftertouch value. |
| [$SN.control.tempo_sync$]($SN.control.tempo_sync$) | Converts musical time values to milliseconds using DAW tempo. |
| [$SN.control.ppq$]($SN.control.ppq$) | Sends normalised position within a musical time window on transport start. |
| [$SN.control.transport$]($SN.control.transport$) | Outputs 1.0 (playing) or 0.0 (stopped) based on DAW transport state. |
| [$SN.control.timer$]($SN.control.timer$) | Periodic modulation generator with Ping, Random, and Toggle modes. |

### Smoothing

| Node | Description |
|------|-------------|
| [$SN.control.smoothed_parameter$]($SN.control.smoothed_parameter$) | Smoothes parameter changes with Linear Ramp or Low Pass modes. Normalised output. |
| [$SN.control.smoothed_parameter_unscaled$]($SN.control.smoothed_parameter_unscaled$) | Smoothes parameter changes with unnormalised output. |

### Clone Control

| Node | Description |
|------|-------------|
| [$SN.control.clone_cable$]($SN.control.clone_cable$) | Distributes per-clone values using 9 mathematical distribution modes. |
| [$SN.control.clone_forward$]($SN.control.clone_forward$) | Forwards the same raw value identically to all clones. |
| [$SN.control.clone_pack$]($SN.control.clone_pack$) | Uses a slider pack to control per-clone parameter values. |

### Slider Pack Writers

| Node | Description |
|------|-------------|
| [$SN.control.pack2_writer$]($SN.control.pack2_writer$) | Writes 2 parameter values into a slider pack. |
| [$SN.control.pack3_writer$]($SN.control.pack3_writer$) | Writes 3 parameter values into a slider pack. |
| [$SN.control.pack4_writer$]($SN.control.pack4_writer$) | Writes 4 parameter values into a slider pack. |
| [$SN.control.pack5_writer$]($SN.control.pack5_writer$) | Writes 5 parameter values into a slider pack. |
| [$SN.control.pack6_writer$]($SN.control.pack6_writer$) | Writes 6 parameter values into a slider pack. |
| [$SN.control.pack7_writer$]($SN.control.pack7_writer$) | Writes 7 parameter values into a slider pack. |
| [$SN.control.pack8_writer$]($SN.control.pack8_writer$) | Writes 8 parameter values into a slider pack. |
| [$SN.control.pack_resizer$]($SN.control.pack_resizer$) | Dynamically resizes a connected slider pack. |

### Miscellaneous

| Node | Description |
|------|-------------|
| [$SN.control.file_analyser$]($SN.control.file_analyser$) | Extracts pitch, duration, or peak level from an audio file on load. |
| [$SN.control.locked_mod$]($SN.control.locked_mod$) | Normalised modulation passthrough for locked containers. |
| [$SN.control.locked_mod_unscaled$]($SN.control.locked_mod_unscaled$) | Unnormalised modulation passthrough for locked containers. |
| [$SN.control.random$]($SN.control.random$) | Generates a uniform random 0--1 value on each input change. |
| [$SN.control.resetter$]($SN.control.resetter$) | Sends a 0-then-1 impulse pair to re-trigger gates. |
| [$SN.control.sliderbank$]($SN.control.sliderbank$) | Multi-output node that scales an input value by a slider pack. |
| [$SN.control.voice_bang$]($SN.control.voice_bang$) | Sends a stored value on each note-on event. Requires polyphonic context. |
| [$SN.control.xy$]($SN.control.xy$) | Two-axis controller with separate X and Y outputs. |
