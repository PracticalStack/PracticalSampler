# dynamics.updown_comp - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/DynamicsNode.h:439-606` + `DynamicsNode.cpp:33-284`
**Base class:** `display_buffer_base<true>` (standalone, no shared template)
**Classification:** audio_processor (with modulation output)

## Signal Path

Input audio (fixed 2 channels) -> peak detection (max abs across channels) -> optional RMS smoothing (30ms window) -> custom attack/release envelope follower -> dual-threshold gain reduction (upward below LowThreshold, downward above HighThreshold, with soft knee) -> gain applied to audio -> modulation output (gain ratio 0..1).

Processing: `process()` casts to fixed 2-channel `ProcessData<2>`, converts to frame data, iterates per-sample via `processFrame()`. After block, updates display buffer.

`processFrame()`:
1. Peak detect: `peak = max(abs(s))` across both channels
2. RMS detector: if enabled, smooths peak via 30ms running-sum RMS window
3. Envelope follower: smooths the detected level with attack/release (skips smoothing for signals below LowThreshold on rising edges)
4. `getGainReduction(peak)`: applies dual-threshold transfer function, returns target output level
5. Computes gain ratio: `g = gainReduction / peak`, clamped to -24..+24 range
6. Sets modulation value: `jlimit(0.0, 1.0, g)`
7. Multiplies all audio channels by gain: `data *= g`

## Gap Answers

### signal-path-processing: How does the updown compressor process audio?

Fixed 2-channel stereo processing. Per-sample frame processing with peak detection, optional RMS, envelope following, and a 5-zone transfer function for gain reduction. The transfer function handles: below low threshold (upward compression), low knee zone, between thresholds (unity), high knee zone, and above high threshold (downward compression).

### low-high-threshold-interaction: How do the dual thresholds work together?

`getGainReduction()` implements a 5-zone piecewise transfer function operating on linear gain values (thresholds are converted from dB to linear via `hmath::db2gain()`):

1. **Low knee zone** (`|input - lo_t| < lo_w/2`): Soft knee transition using quadratic curve for upward compression
2. **Below low threshold** (`input < lo_t - lo_w/2`): Upward compression -- `lo_t + (input - lo_t) / lo_r`, boosting quiet signals. Includes noise floor fade-out below -82 dB to avoid boosting silence.
3. **High knee zone** (`|input - hi_t| < hi_w/2`): Soft knee transition using quadratic curve for downward compression
4. **Above high threshold** (`input > hi_t + hi_w/2`): Downward compression -- `hi_t + (input - hi_t) / hi_r`, clamped to 1.0 (0 dB)
5. **Between thresholds**: Unity gain (return input unchanged)

### knee-parameter: What does the Knee parameter control?

Knee (range 0..0.3) sets the width of the soft knee transition zones around both thresholds. Internally clamped to max `(hi_t - lo_t) * 0.5` -- the knee cannot be wider than half the distance between thresholds. Both low and high knee zones use the same width. At Knee=0, transitions are hard. The parameter is stored as a smoothed float (`sfloat`).

### rms-parameter: What does the RMS toggle do?

RMS parameter (Off/On): enables the `RMSDetector`. When enabled, the peak value is replaced by a running RMS calculation using a 30ms window (buffer size = `sampleRate * 0.03`). The RMS uses a circular buffer with running sum: `sum -= old; sum += new^2; rms = sqrt(sum * coeff)`. This smooths transients and gives a more average-level-based compression response.

### ratio-semantics: How do LowRatio and HighRatio work?

- **LowRatio** (0.2..100): Upward compression ratio. Value of 2 means 2:1 upward boost -- signals 6 dB below LowThreshold are boosted to 3 dB below. Default 1.0 = no upward compression.
- **HighRatio** (0.2..100): Downward compression ratio. Value of 4 means 4:1 reduction -- signals 12 dB above HighThreshold are reduced to 3 dB above. Default 1.0 = no downward compression.
- Both ratios are clamped to 0.2..100 range. The ratio is applied as division: `(input - threshold) / ratio`.

### modulation-output: Does the node have modulation output?

YES. `isModNode()` returns true. `handleModulation()` returns the gain value via `gainRed.getChangedValue(value)`. The modulation value is `jlimit(0.0, 1.0, g)` where `g = getGainReduction(peak) / peak`. Values below 1.0 indicate gain reduction (downward), values above could indicate gain boost (upward), but the clamp limits output to 0..1.

### display-buffer-content: What does the DisplayBuffer show?

The display buffer receives the modulation value: `updateBuffer(gainRed.getModValue(), numSamples)`. It also has a `calculateGraph()` method that computes the transfer curve (input vs output) for visual display.

## Parameters

- **LowThreshold:** Converted to linear gain via `db2gain()`. Sets the threshold for upward compression. Default -100 dB (effectively disabled). Stored as smoothed float.
- **LowRatio:** Upward compression ratio. Clamped 0.2..100. Default 1.0 (no compression). Stored as smoothed float.
- **HighThreshold:** Converted to linear gain via `db2gain()`. Sets the threshold for downward compression. Default 0 dB (effectively disabled). Stored as smoothed float.
- **HighRatio:** Downward compression ratio. Clamped 0.2..100. Default 1.0 (no compression). Stored as smoothed float.
- **Knee:** Soft knee width. Clamped 0..0.5 internally (parameter range 0..0.3). Default 0.15. Stored as smoothed float.
- **Attack:** Envelope follower attack time in ms. Range 0..1000. Default 50.
- **Release:** Envelope follower release time in ms. Range 0..1000. Default 50.
- **RMS:** Toggle for RMS detection. Off=peak, On=RMS (30ms window). Default On.

All threshold, ratio, and knee parameters use `sfloat` (smoothed float) with 50ms smoothing time, preventing clicks on parameter changes.

## Conditional Behaviour

### RMS Detection Mode
- **Off (peak):** Direct peak value used for envelope detection
- **On (RMS):** 30ms circular buffer RMS replaces peak value before envelope follower

### Envelope Follower Low-Threshold Skip
The custom `updown_envelope_follower` has special behaviour: when the input is below `lo_t` (LowThreshold) AND the signal is rising, the smoothing coefficient is zeroed. This means the envelope follower instantly jumps to the input value for signals below the low threshold on rising edges, preventing the attack time from slowing down the upward compression response.

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: [{"parameter": "RMS", "impact": "slight increase", "note": "RMS adds circular buffer read/write per sample"}]

## Notes

- Completely independent implementation from comp/gate/limiter -- does not use chunkware or dynamics_wrapper.
- Fixed 2-channel processing (`getFixChannelAmount() = 2`). Will not work correctly with mono or multi-channel setups.
- All gain-domain parameters (thresholds, ratios, knee) use smoothed floats, preventing zipper noise.
- The noise floor handling at -82 dB prevents the upward compressor from amplifying silence.
- The modulation output clamps to 0..1, which means upward gain boost (values > 1.0 from the transfer function) is not visible in the modulation signal.
- `calculateGraph()` allows the UI to display the transfer curve.
