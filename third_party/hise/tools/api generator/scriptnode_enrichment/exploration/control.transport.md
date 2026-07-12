# control.transport - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:651`
**Base class:** `transport_base<bool, NV>`, `polyphonic_base`
**Classification:** control_source

## Signal Path

DAW transport start/stop -> onTransportChange() sets `this->value = isPlaying` -> handleModulation() (from transport_base) detects change -> wrap::mod outputs 0 or 1.

transport is the simplest tempo-aware node. It outputs a boolean signal reflecting the DAW play/stop state.

## Gap Answers

### output-values: Confirm output polarity.

Confirmed. `onTransportChange(bool isPlaying, double ppqPosition)` sets `this->value = isPlaying`. The `handleModulation()` in transport_base returns `v = (double)value`, so: `1.0` when playing, `0.0` when stopped. The ppqPosition argument is ignored (parameter name prefixed with `/*`).

### no-iscontrolnode: How does the modulation output work?

transport does not have IsControlNode. It inherits from `transport_base` which provides `handleModulation()`. This returns true when `polyValue.get() != value` (per-voice state differs from current value), sets `v = (double)value`, and updates polyValue. The wrap::mod wrapper (applied at registration time) calls checkModValue() which consumes this. The node inherits from `clock_base` which marks it as `OutsideSignalPath` and provides the TempoListener registration in `prepare()`.

## Parameters

No parameters. No properties.

## Polyphonic Behaviour

Uses `PolyData<bool, NumVoices>` for per-voice state tracking in `transport_base`. Each voice independently detects the change from the shared `value` member, ensuring each voice receives the transport change notification exactly once.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
