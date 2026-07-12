# envelope.simple_ar - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h:475`
**Base class:** `pimpl::envelope_base<ParameterType>`, `pimpl::simple_ar_base`
**Classification:** control_source

## Signal Path

simple_ar multiplies the audio input by the envelope value per-sample. The `State::tick()` method computes the current envelope value using a combination of a linear ramp and an `EnvelopeFollower::AttackRelease` smoother, blended by the curve parameter. The target value toggles between 0.0 and 1.0 based on gate state. After processing, `postProcess()` sends CV and Gate modulation outputs.

## Gap Answers

### ar-state-machine: State machine for simple_ar

simple_ar does not have a traditional state machine with named states. Instead it uses a continuous smoothing approach:
- **Gate on:** `targetValue = 1.0`, `smoothing = true`. The value ramps up using attack time.
- **Gate off:** `targetValue = 0.0`, `smoothing = true`. The value ramps down using release time.
- **Smoothing complete:** When `|targetValue - lastValue| < 0.0001`, `smoothing = false`.
- **Active state:** `active = smoothing || targetValue == 1.0`. The envelope is active during attack, sustain (targetValue=1 and smoothing done), and release.

Per-sample computation in `tick()`:
1. If not smoothing, return targetValue immediately.
2. Calculate linear ramp: `linearRampValue += upRampDelta` (attack) or `linearRampValue -= downRampDelta` (release), clamped to [0,1].
3. Calculate `curvedValue = env.calculateValue(targetValue)` from the AttackRelease follower.
4. Blend based on curve parameter (see below).

The envelope effectively has a sustain phase at 1.0 between attack completing and note-off.

### attack-curve-mapping: AttackCurve mapping (default 0.0)

The curve parameter (0..1) blends between different envelope shapes:
- **curve = 0.0:** Pure AttackRelease follower (exponential) -- `lastValue = curvedValue`
- **curve < 0.5:** Interpolation between exponential and linear: `alpha = 2.0 * curve`, `lastValue = lerp(curvedValue, linearRampValue, alpha)`
- **curve = 0.5:** Pure linear ramp -- `lastValue = linearRampValue`
- **curve > 0.5:** Interpolation between linear and power curve: `alpha = 2.0 * (curve - 0.5)`, `oneValue = pow(linearRampValue, PI)`, `lastValue = lerp(linearRampValue, oneValue, alpha)`
- **curve = 1.0:** Pure power curve -- `lastValue = pow(linearRampValue, PI)` (very slow start)

The default of 0.0 gives a pure exponential response, different from ahdsr's default of 0.5.

### gate-parameter-vs-midi: Gate parameter vs MIDI

Yes, the Gate parameter allows manual triggering. The `setGate()` method checks `v > 0.5` and calls `State::setGate()` on all voice states. Both MIDI and the Gate parameter control the same internal gate. In `handleHiseEvent()`:
- Polyphonic: note-on/off directly calls `setGate(1.0/0.0)`
- Monophonic: `handleKeyEvent()` implements legato counting and calls `setGate()` on first note-on / last note-off

The Gate parameter enables modulation-driven triggering without MIDI.

### sustain-level: Sustain level

There is no Sustain level parameter. The envelope always sustains at 1.0 (full level). The `targetValue` is either 0.0 or 1.0, and the smoothing ramps between these two extremes. There is no way to set an intermediate sustain level.

## Parameters

- **Attack:** Time in ms (0-1000). Sets both linear ramp delta and exponential follower attack time.
- **Release:** Time in ms (0-1000). Sets both linear ramp delta and exponential follower release time.
- **Gate:** Boolean (Off/On). Manual gate trigger. > 0.5 triggers attack, <= 0.5 triggers release.
- **AttackCurve:** Shape control (0-1, default 0.0). 0=exponential, 0.5=linear, 1=power curve.

## Polyphonic Behaviour

`PolyData<State, NumVoices> states` stores one State per voice with its own `EnvelopeFollower::AttackRelease` instance, linear ramp value, and smoothing flag.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The display buffer uses a custom `PropertyObject` that renders the envelope shape by simulating a gate-on/gate-off cycle at adjusted sample rate. Buffer length is fixed at 1024 samples. The `recalculateLinearAttackTime()` method computes ramp deltas from time values: `upRampDelta = 1.0 / attackTimeSamples`, `downRampDelta = 0.9 / releaseTimeSamples` (the 0.9 factor means release reaches 90% of the way to zero per release time).
