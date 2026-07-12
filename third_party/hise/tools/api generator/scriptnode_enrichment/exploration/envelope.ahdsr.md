# envelope.ahdsr - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h:672`
**Base class:** `pimpl::envelope_base<ParameterType>`, `pimpl::ahdsr_base`
**Classification:** control_source

## Signal Path

The AHDSR envelope multiplies the audio input by the envelope value per-sample. On each sample, `state_base::tick()` is called which advances the state machine and returns a value in [0, 1]. Each audio sample is multiplied by this value. The envelope value is also sent as CV modulation output via `postProcess()`. The envelope has dual outputs: CV (continuous envelope value) and Gate (binary on/off).

## Gap Answers

### envelope-state-machine: AHDSR state machine states and transitions

The state machine has 7 states defined in `state_base::EnvelopeState`:
- **IDLE:** Output = 0. No processing. Entered when release completes or sustain reaches zero.
- **ATTACK:** `current_value = attackBase + current_value * attackCoef`. Exponential rise using precomputed coefficients. Transitions to HOLD when `current_value >= attackLevel` (or to SUSTAIN directly if `attackLevel <= sustain`). If attack time is 0, jumps instantly to HOLD with `current_value = attackLevel`.
- **HOLD:** Holds at `attackLevel` for `holdTimeSamples`. Increments `holdCounter` each sample. Transitions to DECAY when counter expires.
- **DECAY:** `current_value = decayBase + current_value * decayCoef`. Exponential decay toward sustain level. Transitions to SUSTAIN when the difference from sustain is silent (<-90dB). If sustain is 0, goes to IDLE instead.
- **SUSTAIN:** `current_value = sustain * modValues[3]`. Holds at the modulated sustain level until note-off.
- **RETRIGGER:** A monophonic-only state for retriggering. By default (without `HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO`), immediately jumps to ATTACK and calls `tick()` recursively. With the define, ramps down at 0.005/sample until zero, then enters ATTACK.
- **RELEASE:** `current_value = releaseBase + current_value * releaseCoef`. Exponential decay toward zero. Transitions to IDLE when value is silent. If release time is 0, jumps instantly to IDLE with value 0.

Gate transitions: Gate > 0.5 triggers ATTACK (from IDLE) or RETRIGGER (from any other state). Gate <= 0.5 triggers RELEASE (from any non-IDLE state).

### attack-curve-mapping: AttackCurve parameter mapping

The `setAttackCurve()` method in `ahdsr_base` maps the 0..1 parameter to `attackBase`:
- **0.5 (linear):** `attackBase = 1.2` (nearly linear exponential approximation)
- **< 0.5 (logarithmic/fast start):** `attackBase = 1.0 / ((1.0 - value*2.0) * 100.0)` -- values approach 0, producing a logarithmic curve
- **> 0.5 (exponential/slow start):** `attackBase = (value - 0.5) * 2.0 * 100.0` -- large values produce steep exponential curves

The `attackBase` is then used in `calculateCoefficients()` to compute per-sample coefficients: `stateCoeff = pow(attackBase, 1/timeSamples)`.

### retrigger-behaviour: Retrigger parameter

When Retrigger is On (> 0.5), the `handleHiseEvent()` method for monophonic mode treats every note-on as a retrigger, even during legato playing. In `setParameter<Gate>`, if the envelope is not IDLE, it enters the RETRIGGER state. By default (without `HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO`), RETRIGGER immediately jumps to ATTACK and continues from the current value -- it does NOT restart from zero. The retrigger flag only affects monophonic mode; in polyphonic mode, each voice has its own state.

### gate-parameter-usage: Gate parameter for manual triggering

Yes, the Gate parameter can manually trigger the envelope. In `setParameter<Gate>`, when the value > 0.5, the state transitions to ATTACK (from IDLE) or RETRIGGER (from active). When <= 0.5, it transitions to RELEASE. This operates independently of MIDI events -- the Gate parameter provides a modulation-driven trigger mechanism. In `handleHiseEvent()`, note-on/off events call `setGate()` which calls `setParameter<Gate>`.

### display-buffer-ring-properties: Custom ring buffer properties

Yes. `ahdsr_base` overrides `registerPropertyObject()` to register `AhdsrRingBufferProperties`. This custom property object:
- Fixes buffer length to 9 samples (one per parameter value)
- Implements `transformReadBuffer()` to copy `uiValues[9]` for shape display
- Implements `createPath()` to draw the AHDSR envelope shape from parameter values
- Creates an `AhdsrGraph` component for visualization

### voice-reset-on-release-end: Voice reset via Gate output

Yes. When the release phase completes, `tick()` sets `current_state = IDLE` and `active = false`. The `postProcess()` method detects `thisActive != wasActive` and sends `Gate = 0` (call<1>(0.0)) and `CV = 0.0` (call<0>(0.0)). This gate-off signal is how the envelope communicates voice termination to downstream voice management.

## Parameters

- **Attack/Hold/Decay/Release:** Time in ms. Converted to per-sample coefficients via `calculateCoefficients()` or `calcCoef()`. Note: Attack, Decay, Release values are doubled (`v * 2.0f`) before passing to state setters because `modValues[]` are initialized to 0.5 (simulating modulation chain at half intensity).
- **AttackLevel:** Target level at end of attack phase (0..1). Default 1.0.
- **Sustain:** Sustain level (0..1). Affects both decay target and sustain hold value. Also recalculates release coefficients.
- **AttackCurve:** Controls exponential shape of attack segment (0=log, 0.5=linear, 1=exp).
- **Retrigger:** Boolean. When On, monophonic legato notes retrigger the envelope.
- **Gate:** Boolean. Manual gate trigger (>0.5 = on, <=0.5 = off).

## Polyphonic Behaviour

`PolyData<state_base, NumVoices> states` stores one complete envelope state per voice. In polyphonic mode, note-on/off directly triggers gate. In monophonic mode, `handleKeyEvent()` implements legato key counting with sustain pedal support.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

## Notes

The `modValues[5]` array in `state_base` supports per-voice modulation of internal chains (Attack time, Attack level, Decay time, Sustain level, Release time). In the scriptnode context, these are initialized to 0.5 for attack/decay/release (hence the `v * 2.0f` multiplication) and 1.0 for sustain/attack level. The ball position updater (`ExecutionLimiter`) throttles UI display updates to frame rate.
