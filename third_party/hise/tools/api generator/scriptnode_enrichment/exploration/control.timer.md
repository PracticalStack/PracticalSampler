# control.timer - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/EventNodes.h:180`
**Base class:** `timer_base<TimerType>`, `polyphonic_base`, `pimpl::templated_mode`
**Classification:** control_source

## Signal Path

process()/processFrame() -> TimerInfo::tick() counts samples -> when samplesLeft <= 0, calls tType.getTimerValue() -> ModValue set -> handleModulation() consumed by wrap::mod -> normalised output.

timer sits inside the signal path (no OutsideSignalPath, no IsControlNode). It uses the wrap::mod pattern with non-empty process() and processFrame() methods that count samples against the configured interval.

## Gap Answers

### mode-variants: What Mode values are available?

From the `timer_logic` namespace in `logic_classes.h`:

1. **ping**: Returns `1.0` every interval. Simple periodic impulse.
2. **random**: Returns `hmath::randomDouble()` every interval. New random value each tick.
3. **toggle**: Alternates between 0.0 and 1.0 each interval. Uses per-voice `PolyData<double, NV>` state.

The mode namespace is `"timer_logic"`.

### processing-pattern: Confirm it runs in the audio callback.

Confirmed. In `process()`: `thisInfo.tick(numSamples)` decrements `samplesLeft` by block size. When <= 0, fires `getTimerValue()` and resets. In `processFrame()`: `thisInfo.tick()` decrements by 1 per sample. The timer is block-accurate in process() and sample-accurate in processFrame(). After processing, `sendPending()` is NOT called here (it is in multi_parameter but timer is not a multi_parameter). Instead, `handleModulation()` returns the value from `t.get().getChangedValue(value)`.

### snex-custom-logic: SNEX custom timer logic.

The `IsOptionalSnexNode` property is set. The `setParameter()` dispatch allows parameters beyond index 1 to be forwarded to the TimerType: `typed->tType.template setParameter<P - 2>(value)` (guarded by `#if HISE_INCLUDE_SNEX`). Custom SNEX classes must provide: `getTimerValue() -> double`, and optionally `initialise()`, `prepare()`, `reset()`, and `setParameter<P>()`.

### interval-units: Is the timer sample-accurate?

The interval is stored in samples: `samplesBetweenCallbacks = roundToInt(timeMs * 0.001 * sr)`. In `processFrame()`, it decrements one sample at a time (sample-accurate). In `process()`, it decrements by block size (block-accurate). When the timer fires, `samplesLeft` wraps around by adding `samplesBetweenCallbacks` (not resetting to zero), which accumulates fractional offsets for timing accuracy.

## Parameters

- **Active**: On/Off toggle. When off, processing is skipped entirely (`if (!thisInfo.active) return`). Default On.
- **Interval**: Time between triggers in milliseconds (0-2000, step 0.1). Default 500ms. Converted to samples internally.

## Polyphonic Behaviour

Uses `PolyData<TimerInfo, NumVoices>`. Each voice has its own `samplesLeft` counter, `active` flag, and `lastValue` ModValue. On reset, each voice's counter resets to `samplesBetweenCallbacks` and the ModValue is set to the current timer value.

## CPU Assessment

baseline: low (sample counting per block/sample)
polyphonic: true
scalingFactors: [{ "parameter": "Active", "impact": "eliminates processing when off", "note": "Early return in process/processFrame" }]

## Notes

When Active is toggled on, the counter resets to `samplesBetweenCallbacks` and the ModValue fires immediately with the current timer value. The toggle mode uses PolyData for its state, making it truly per-voice in polyphonic contexts.
