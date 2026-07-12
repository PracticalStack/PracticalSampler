# core.phasor_fm - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:1387`
**Base class:** `phasor_base<NV, true>`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Identical to core.phasor except the `useFM` template parameter is `true`. In `processFrameInternal()` (line 1257), when `useFM` is true, the input signal on channel 0 is read as a modulator BEFORE being overwritten with the ramp output:

1. Read `data[0]` as FM modulation input
2. Compute `delta = uptimeDelta * multiplier * data[0]`
3. Add delta to uptime (phase modulation)
4. Wrap phase and write ramp to `data[0]`

The input signal on channel 0 serves as both the FM source and the output destination. The FM modulation is actually phase modulation -- the input signal scales the phase increment per sample.

## Gap Answers

### fm-signal-usage: How does input modulate the phase?

Phase modulation. At line 1259-1261: `delta = uptimeDelta * multiplier; delta *= (double)data[0]; uptime += delta;`. The input sample multiplies the base phase increment. A value of 1.0 means normal speed, 2.0 means double speed, 0.0 means frozen. Negative values reverse the phase direction.

### processing-order: Read input then replace output?

Yes. The input on channel 0 is read first (as FM modulator), then the ramp output overwrites channel 0. This means the FM source must be placed before the phasor_fm in the signal chain.

### gate-midi-interaction: Does MIDI control Gate?

No, same as core.phasor. `handleHiseEvent()` only sets frequency on note-on.

### output-range: Still 0..1?

The ramp itself is still wrapped to 0..<1 via `phase -= bitwiseOrZero(phase)`. However, with extreme FM modulation, the phase can advance multiple cycles per sample, so the output may appear to jump erratically despite always being in [0, 1).

## Parameters

Same as core.phasor: Gate, Frequency, Freq Ratio, Phase.

## Polyphonic Behaviour

Same as core.phasor: `PolyData<OscData, NumVoices> voiceData`.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
