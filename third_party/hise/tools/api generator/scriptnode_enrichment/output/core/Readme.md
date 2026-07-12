---
title: Core Nodes
factory: core
---

The core factory contains the fundamental DSP building blocks for scriptnode: oscillators, gain staging, signal analysis, file playback, and custom code nodes. These are the primary audio-generating and audio-processing primitives that form the backbone of most networks.

Core nodes handle tasks that do not fit neatly into the more specialised factories (filters, math, routing, control). They range from simple utilities like gain and peak metering to complex processors like the granular synthesiser and timestretching player. Many core nodes are polyphonic and respond to MIDI events for pitch tracking.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.core.oscillator$]($SN.core.oscillator$) | A tone generator with multiple waveforms |
| [$SN.core.phasor$]($SN.core.phasor$) | A naive 0-1 ramp oscillator for waveshaping pipelines |
| [$SN.core.phasor_fm$]($SN.core.phasor_fm$) | A ramp oscillator with FM modulation from the input signal |
| [$SN.core.fm$]($SN.core.fm$) | An FM operator that uses the signal input as modulator |
| [$SN.core.ramp$]($SN.core.ramp$) | A free-running ramp signal generator for modulation |
| [$SN.core.clock_ramp$]($SN.core.clock_ramp$) | A tempo-synced ramp signal generator |
| [$SN.core.gain$]($SN.core.gain$) | A gain module with decibel range and parameter smoothing |
| [$SN.core.smoother$]($SN.core.smoother$) | A one-pole lowpass filter for smoothing channel 0 |
| [$SN.core.fix_delay$]($SN.core.fix_delay$) | A non-interpolating delay line |
| [$SN.core.table$]($SN.core.table$) | A symmetrical lookup table based waveshaper |
| [$SN.core.peak$]($SN.core.peak$) | Creates a modulation signal from the absolute input magnitude |
| [$SN.core.peak_unscaled$]($SN.core.peak_unscaled$) | Creates a raw modulation signal from the input |
| [$SN.core.mono2stereo$]($SN.core.mono2stereo$) | Converts a mono signal to stereo (L to L+R) |
| [$SN.core.recorder$]($SN.core.recorder$) | Records the signal input into an audio file slot |
| [$SN.core.file_player$]($SN.core.file_player$) | A file player with multiple playback modes |
| [$SN.core.stretch_player$]($SN.core.stretch_player$) | A buffer player with timestretching |
| [$SN.core.granulator$]($SN.core.granulator$) | A granular synthesiser |
| [$SN.core.snex_node$]($SN.core.snex_node$) | A generic SNEX node with the complete callback set |
| [$SN.core.snex_shaper$]($SN.core.snex_shaper$) | A custom waveshaper using SNEX |
| [$SN.core.snex_osc$]($SN.core.snex_osc$) | A custom oscillator node using SNEX |
| [$SN.core.faust$]($SN.core.faust$) | A Faust DSP node |
| [$SN.core.extra_mod$]($SN.core.extra_mod$) | An extra modulation source |
| [$SN.core.global_mod$]($SN.core.global_mod$) | A global modulation source |
| [$SN.core.pitch_mod$]($SN.core.pitch_mod$) | Picks up the pitch modulation signal from the parent sound generator |
| [$SN.core.matrix_mod$]($SN.core.matrix_mod$) | A modulator that connects to the modulation matrix |
