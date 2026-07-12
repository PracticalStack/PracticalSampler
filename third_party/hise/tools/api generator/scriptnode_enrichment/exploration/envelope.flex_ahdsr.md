# envelope.flex_ahdsr - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h:977`
**Base class:** `mothernode`, `flex_ahdsr_base`, `data::base`, `pimpl::envelope_base<ParameterClass>`
**Classification:** control_source

## Signal Path

Like the standard AHDSR, flex_ahdsr multiplies audio input by the envelope value per-sample. The state machine uses `PolyState::calculateNewValue()` which computes a normalized position (counter/thisTime) within each stage, then applies curve shaping and level interpolation. The envelope outputs CV and Gate modulation signals via `postProcess()`.

## Gap Answers

### flex-envelope-engine: Internal computation engine

flex_ahdsr uses a fundamentally different algorithm from ahdsr. Instead of precomputed exponential coefficients (ahdsr's approach), flex_ahdsr uses a time-based counter system:

1. Each state has a `counter` that increments each sample
2. Progress `value = counter / thisTime` gives a normalized 0..1 position within the stage
3. The curve parameter shapes this linear ramp: curve=1.0 is linear, >1.0 applies `pow(value, curve)` for exponential shape, <1.0 applies `1 - pow(1-value, 1/curve)` for logarithmic shape
4. The shaped value interpolates between `prevLevel` (start) and the current stage's target level

States are: IDLE, ATTACK, HOLD, DECAY, SUSTAIN, RELEASE, DONE. The `bump()` method advances to the next state when the counter exceeds `thisTime`. States with zero time are skipped automatically.

### mode-variants: Three playback modes

The Mode parameter (0-2) with labels "Trigger", "Note", "Loop":
- **Trigger (0):** The envelope plays through all stages without waiting at sustain. At SUSTAIN, `bump()` is called immediately (since `m != Mode::Note`), advancing to RELEASE. The note-off has no effect on timing.
- **Note (1, default):** Standard AHDSR behavior. The envelope holds at SUSTAIN indefinitely until note-off triggers RELEASE. At SUSTAIN, `bump()` is not called because `m == Mode::Note`.
- **Loop (2):** When the envelope reaches DONE and gate is still active (`gateActive`), it loops back to IDLE and calls `bump()` to restart from ATTACK. Creates a repeating envelope cycle.

### curve-parameter-mapping: Curve parameter mapping

All three curve parameters (AttackCurve, DecayCurve, ReleaseCurve) use the same mapping. The `convert<ParameterType::Curve>()` function transforms the 0..1 parameter to an internal curve factor:
`curveValue = pow(2.0, (paramValue - 0.5) * 8.0)`

This gives: 0.0 -> pow(2,-4) = 0.0625, 0.5 -> 1.0 (linear), 1.0 -> pow(2,4) = 16.0.

In `calculateNewValue()`, the curve factor is applied:
- curve ~= 1.0: linear interpolation between prevLevel and target
- curve > 1.0: `pow(value, curve)` -- exponential (slow start, fast end)
- curve < 1.0: `1 - pow(1-value, 1/curve)` -- logarithmic (fast start, slow end)

This is different from ahdsr's AttackCurve which uses a coefficient-based approach.

### draggable-curves-ui: Draggable curves UI feature

Yes, the "draggable curves" description refers to an actual interactive UI. flex_ahdsr inherits from `flex_ahdsr_base` which defines a `FlexAhdsrGraph` component and a `DragHandler` system. The `handleUIDrag()` method routes drag interactions to `setParameter<>()` calls. The UI graph component allows direct manipulation of time, level, and curve parameters by dragging points on the envelope shape. The `Properties` ring buffer class provides the visualization data via `refreshUI()`.

### no-retrigger-gate-params: No Retrigger or Gate parameters

flex_ahdsr has no Retrigger or Gate parameters. Retriggering is handled differently depending on Mode. In `handleHiseEvent()`, note-on triggers `gate(true)` which resets the state to IDLE and calls `bump()` to begin ATTACK -- this always retriggers from the current `prevValue` (not from zero). Note-off triggers `gate(false)` which sets the state to RELEASE from whatever the current state is. There is no manual gate parameter; the envelope is always driven by MIDI note-on/off (or `handleKeyEvent()` in monophonic mode for legato).

## Parameters

- **Attack/Hold/Decay/Release:** Time in ms (0-30000). Converted to samples via `value * 0.001 * sampleRate`.
- **Sustain:** Sets both DECAY target level and SUSTAIN hold level simultaneously. Also constrains attack level (attack level cannot be less than sustain).
- **Mode:** Trigger/Note/Loop. Controls sustain and looping behavior.
- **AttackLevel:** Sets both ATTACK and HOLD target levels. Clamped to be >= sustain level.
- **AttackCurve/DecayCurve/ReleaseCurve:** Each 0..1, mapped via `pow(2, (v-0.5)*8)`. Controls shape of respective segments.

## Polyphonic Behaviour

`PolyData<PolyState, NV> state` stores one complete `PolyState` per voice. Each PolyState contains per-stage `Values` structs with smoothed level and curve parameters (`sfloat`), enabling glitch-free parameter changes during playback.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

## Notes

The `SN_EMPTY_MOD` macro means flex_ahdsr does not use `handleModulation()`/`ModValue`. Instead it sends modulation directly via `postProcess()` through the parameter_node_base system. The `sendBallUpdate` flag and `lastStartedVoiceIndex` manage UI ball position display showing which stage is active. The `DragHandler` template parameter allows custom drag behavior extensions.
