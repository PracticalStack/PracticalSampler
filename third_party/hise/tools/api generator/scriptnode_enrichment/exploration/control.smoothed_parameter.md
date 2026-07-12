# control.smoothed_parameter - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2823`
**Base class:** `pimpl::templated_mode`, `polyphonic_base`, `smoothed_parameter_base`
**Classification:** control_source

## Signal Path

Value parameter -> SmootherClass::set(newValue) -> process()/processFrame() calls value.advance() each block/sample -> ModValue::setModValueIfChanged() -> handleModulation() consumed by wrap::mod -> normalised output.

smoothed_parameter sits inside the signal path (no OutsideSignalPath, no IsControlNode). It uses the wrap::mod pattern with non-empty process() and processFrame() methods that advance the smoother each call.

## Gap Answers

### smoothing-modes: What smoothing modes are available?

From the `smoothers` namespace in `logic_classes.h`, three modes:

1. **linear_ramp**: Uses `sdouble` (linear ramp). Linearly interpolates from current to target value over the smoothing time. Uses `currentBlockRate` (sampleRate/blockSize) for timing.
2. **low_pass**: Uses a `Smoother` (exponential low-pass filter). Exponentially approaches the target. Checks `isSmoothing` to skip processing when settled.
3. **no**: No smoothing. `advance()` returns the current value immediately. `set()` updates instantly.

The mode namespace is `"smoothers"`. All modes inherit from `smoothers::base`.

### processing-pattern: Confirm it runs in the audio callback.

Confirmed. `process()` and `processFrame()` both call `modValue.setModValueIfChanged(value.advance())`. The `advance()` method steps the smoother forward by one sample/block. The `handleModulation()` method returns the latest value via `modValue.getChangedValue(v)`. The wrap::mod wrapper calls checkModValue() after each process/processFrame call.

### enabled-bypass: What happens when Enabled=false?

When `setEnabled(false)` is called on the smoother base class, `enabled` is set to false. In `linear_ramp::advance()`: returns `state.get().targetValue` directly (no interpolation). In `low_pass::advance()`: returns `state.get().target` directly. In both cases, the target value passes through immediately with no smoothing. The node does NOT stop outputting -- it outputs the target value instantly.

## Parameters

- **Value**: Target value for the smoother. Range 0..1. Calls `value.set(newValue)` which starts the ramp.
- **SmoothingTime**: Smoothing time in milliseconds. Range 0.1-1000, default 100. Updates the smoother's time constant.
- **Enabled**: On/Off toggle. When off, smoothing is bypassed (instant parameter changes). Default On.

## Polyphonic Behaviour

Uses `PolyData` inside the SmootherClass (e.g., `PolyData<sdouble, NumVoices>` for linear_ramp, `PolyData<State, NumVoices>` for low_pass). Each voice has independent smoothing state. On voice reset, `value.reset()` jumps to target and `modValue.setModValue(value.get())` forces a change flag to ensure the modulation fires on voice start.

## CPU Assessment

baseline: low (per-sample or per-block advance operation)
polyphonic: true
scalingFactors: [{ "parameter": "SmoothingTime", "impact": "negligible", "note": "Only affects ramp duration, not per-sample cost" }]

## Notes

The `smoothed_parameter_pimpl` template has an `IsScaled` boolean. When true (smoothed_parameter), `isNormalisedModulation()` returns true. When false (smoothed_parameter_unscaled), it returns false and registers UseUnnormalisedModulation. The static ID is determined by IsScaled: "smoothed_parameter" or "smoothed_parameter_unscaled".
