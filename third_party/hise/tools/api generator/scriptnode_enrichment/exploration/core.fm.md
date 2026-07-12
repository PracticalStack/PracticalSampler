# core.fm - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:2013`, `CoreNodes.cpp:55`
**Base class:** `HiseDspBase`
**Classification:** audio_processor

## Signal Path

The FM node reads the input signal as a modulator and replaces it with a sine carrier output. Per-sample in `processFrame()` (line 2053):

1. Read `d[0]` as modulation value
2. Generate sine output: `d[0] = sinTable->getInterpolatedValue(od.tick())`
3. Apply FM: `od.uptime += modGain.get() * modValue`

The carrier is always a sine wave using a shared `SineLookupTable<2048>`. The FM is applied by adding the scaled modulator value directly to the phase accumulator after each sample. This is classical phase modulation (often called FM synthesis).

`process()` dispatches to mono frame processing via `FrameConverters::forwardToFrameMono()`.

## Gap Answers

### fm-algorithm: What is the FM synthesis algorithm?

Phase modulation with a sine carrier. The carrier uses `sinTable->getInterpolatedValue()` for lookup. The modulator signal on channel 0 is scaled by `modGain` and added to the carrier's `uptime` (phase accumulator) after each sample tick. This is equivalent to classic Chowning FM synthesis.

### signal-routing: How are channels handled?

Mono processing only -- `forwardToFrameMono()` processes channel 0 only. The input on channel 0 is read as the modulator, then replaced with the carrier sine output.

### freq-multiplier-role: What does FreqMultiplier multiply?

The carrier frequency. `setFreqMultiplier()` (CoreNodes.cpp:104) sets `oscData.multiplier`. In `OscData::tick()`, uptime advances by `uptimeDelta * multiplier`. So FreqMultiplier scales the carrier's phase increment. It does NOT affect the modulator.

### not-polyphonic: Is the polyphonic classification correct?

The preliminary JSON says isPolyphonic=false, but the C++ code declares `isPolyphonic() const { return true; }` at line 2041, and uses `PolyData<OscData, NUM_POLYPHONIC_VOICES>`. The node IS polyphonic. Also, `isProcessingHiseEvent()` returns true (line 2038), and `handleHiseEvent()` sets frequency on note-on (CoreNodes.cpp:98). The preliminary JSON classification is incorrect.

### gate-behaviour: What does Gate=0 do?

`setGate()` (CoreNodes.cpp:129): sets `enabled` and multiplies uptime by 0.0 (resetting phase) when turned off. When enabled=0, `process()` returns early (line 2047). Gate must be set manually -- there is no automatic MIDI gate control.

## Parameters

- **Frequency** (20-20000 Hz, default 20): Carrier frequency. Overridden by MIDI note-on.
- **Modulator** (0-1, default 0): FM modulation depth. Internally scaled by `sinTable->getTableSize() * 0.05`.
- **FreqMultiplier** (1-12, integer): Carrier frequency multiplier (harmonic ratio).
- **Gate** (0/1, default 1): Enables/disables output. Off resets phase to 0.

## Polyphonic Behaviour

`PolyData<OscData, NUM_POLYPHONIC_VOICES> oscData` and `PolyData<double, NUM_POLYPHONIC_VOICES> modGain` store per-voice state. Always uses NUM_POLYPHONIC_VOICES (not templated on NV like other nodes).

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

## Notes

The preliminary JSON incorrectly classifies this node as monophonic with no MIDI handling. The C++ source clearly shows it is polyphonic and handles MIDI note-on for pitch tracking.
