# core.snex_osc - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:2362`
**Base class:** `snex_osc_base<T>`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

`snex_osc<NV, T>` wraps a user-defined oscillator type and manages frequency tracking, phase accumulation, and MIDI integration. The wrapper handles two processing paths:

**Frame processing** (`processFrame()` line 2414): calls `oscData.tick()` to advance phase, then `data[0] += oscType.tick(uptime)` -- adds the oscillator output to channel 0.

**Block processing** (`process()` line 2421): builds an `OscProcessData` struct with the current uptime, delta, and a reference to channel 0's buffer, then calls `oscType.process(op)`. After the user's process returns, advances uptime by `delta * numSamples`.

Output is additive (`data[0] += ...` in frame mode; the OscProcessData convention expects additive writing too).

## Gap Answers

### oscillator-callbacks: Required callbacks?

Two required callbacks in the user's SNEX code:
- `float tick(double uptime)`: generates one sample at the given phase position
- `void process(OscProcessData& d)`: generates a block of samples

`prepare()` is optional (forwarded via `prototypes::check`). The wrapper always calls `tick()` in frame mode and `process()` in block mode -- it does not choose between them.

### frequency-tracking: Frequency vs MIDI interaction?

`handleHiseEvent()` (line 2435) calls `setFrequency(e.getFrequency())` on note-on, which OVERWRITES the current frequency. The Frequency parameter sets a manual frequency that is overridden (not added to) by MIDI note-on. Last-set-wins: if Frequency parameter is changed after a note-on, it overrides the MIDI frequency.

### additive-output: Additive or replacing?

Additive. `processFrame()` line 2418: `data[0] += this->oscType.tick(uptime)`. The oscillator output is added to the existing signal on channel 0.

### user-parameters: Parameter offset behaviour?

Confirmed at line 2380: for parameter index P > 1, the wrapper calls `typed->oscType.template setParameter<P - 2>(value)`. So the user's parameter 0 maps to scriptnode parameter index 2, parameter 1 maps to index 3, etc. Built-in parameters 0=Frequency, 1=PitchMultiplier.

### template-argument-polyphonic: How does polyphony work?

The node is fully polyphonic. `PolyData<OscData, NumVoices> oscData` stores per-voice phase state. The `TemplateArgumentIsPolyphonic` property means the user's oscillator class receives NumVoices as a template parameter, enabling `PolyData` usage in user code for per-voice state. The wrapper handles voice dispatch automatically.

## Parameters

- **Frequency** (20-20000 Hz, default 220): Base oscillator frequency. Overridden by MIDI note-on.
- **PitchMultiplier** (1-16, integer, default 1): Frequency multiplier. Clamped to [0.01, 100].

## Polyphonic Behaviour

`PolyData<OscData, NumVoices> oscData` stores per-voice uptime, uptimeDelta, and multiplier. Each voice runs independently.

## CPU Assessment

baseline: low (depends on user SNEX code)
polyphonic: true
scalingFactors: []
