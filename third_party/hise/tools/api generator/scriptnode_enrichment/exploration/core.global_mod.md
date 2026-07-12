# core.global_mod - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/ModulationNodes.h:563`
**Base class:** `indexable_mod_base` -> `mod_base<NV, IndexClass, RuntimeTarget::GlobalModulator, ConfigClass>`
**Classification:** audio_processor (modulation bridge)

## Signal Path

Picks up a modulation signal from the GlobalModulatorContainer and exposes it inside scriptnode. On note-on, queries the SignalSource for the selected modulator slot's EventData (which may be VoiceStart constant, TimeVariant block, or Envelope block). During process/processFrame, reads the modulation signal, applies the selected mode formula (Gain/Unipolar/Bipolar) with Value and Intensity parameters, then outputs via ModValue for downstream parameter modulation. When ProcessSignal is enabled, also writes the result into audio channel 0.

The signal flow is: HISE GlobalModulatorContainer -> runtime_target connection -> SignalSource -> EventData query per voice -> mode formula application -> ModValue output (and optionally audio channel 0).

## Gap Answers

### mode-formulas: Confirm the exact formula for each mode

Confirmed in `applyModulation()` (mod_base, line 149):

- **Gain (0):** `output = clamp(baseValue - intensity + intensity * signal * baseValue, 0, 1)`. This means at intensity=1, value=1: output = signal. At intensity=0.5, value=1: output = 0.5 + 0.5 * signal.
- **Unipolar (1):** `output = clamp(baseValue + signal * intensity, 0, 1)`. Additive offset from base.
- **Bipolar (2):** `output = clamp(baseValue + (2*signal - 1) * intensity, 0, 1)`. Signal is centered around 0.5, scaled bipolar by intensity, added to base.

All modes clamp output to [0, 1] via `jlimit` or `FloatVectorOperations::clip`.

### value-parameter-role: Role of Value parameter in different modes

Value is the `baseValue` smoothed float in the formulas above. In Gain mode with Value=1.0 and Intensity=1.0, the output equals the raw modulation signal. In Unipolar/Bipolar modes, Value serves as the center point around which the modulation is added. The `setValue()` method limits to [-1, 1] and sets it on the per-voice smoothed float.

### intensity-negative: What does negative intensity do?

Negative intensity inverts the modulation direction. In Gain mode: `base - (-intensity) + (-intensity) * signal * base` = `base + intensity - intensity * signal * base`, which inverts the modulation curve. In Unipolar: `base + signal * (-intensity)` subtracts instead of adds. In Bipolar: `base + (2*signal-1) * (-intensity)` flips the bipolar direction. The range is clamped to [-1, 1] by `setIntensity()`.

### index-to-modulator-mapping: How do indices map to the container's children?

The Index parameter (0-16) is passed to `SignalSource::getEventData(slotIndex, event, wantsPolyphonic)`. Per the runtime-target infrastructure, each slot in the SignalSource maps to a child modulator in the GlobalModulatorContainer's gain chain, in order (index 0 = first child). The `setIndex()` method calls `jlimit(-1, NumMaxModulationSources, ...)` so -1 is technically accepted but the createParameters range is [0, 16].

### description-missing: Correct short description

No SN_DESCRIPTION macro is present for global_mod (unlike pitch_mod and matrix_mod). A suitable description: "Picks up a modulation signal from the GlobalModulatorContainer".

### signal-rate-interpolation: How does upsampling affect signal quality?

In `processFrame()`, audio-rate upsampling uses linear interpolation between control-rate samples via `Interpolator::interpolateLinear(v1, v2, alpha)` where alpha increments by 1/HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR each sample. In `process()`, the `ModBufferExpansion::expand()` utility performs the same linear ramp (AlignedRamper). This produces smooth transitions but with a one-block latency. At typical 512-sample blocks with factor 8 downsampling, each control-rate value spans 8 audio samples -- adequate for most modulation but may produce audible stepping for very fast LFOs at low sample rates.

## Parameters

- **Index** (0-16, step 1, default 0): Selects the modulator slot in the GlobalModulatorContainer. Maps to child position in the gain chain.
- **Value** (0-1, default 1.0): Base value for the modulation formula. Acts as modulation center/ceiling depending on mode.
- **ProcessSignal** (Disabled/Enabled, default Disabled): When enabled, writes the modulation signal to audio channel 0.
- **Mode** (Gain/Unipolar/Bipolar, default Gain): Selects the modulation formula.
- **Intensity** (-1 to 1, default 1.0): Modulation depth. Negative values invert the modulation direction.

## Conditional Behaviour

Mode parameter selects three distinct formulas in `applyModulation()`:
- Gain: Multiplicative scaling relative to baseValue
- Unipolar: Additive offset from baseValue
- Bipolar: Bipolar additive offset (signal centered at 0.5)

ProcessSignal toggles between:
- Disabled: Single-sample evaluation per block, smoothing rates adjusted to per-block (`sampleRate / blockSize`)
- Enabled: Full buffer written to channel 0, per-sample smoothing

## Polyphonic Behaviour

Per-voice state is stored in `PolyData<Data, NV>`. Each voice has:
- `eventData` (EventData): the modulation signal reference for this voice
- `baseValue` / `intensity` (sfloat): smoothed parameter values per voice
- `lastRampValue` (float): last interpolation anchor for audio-rate upsampling
- `uptime` (int): read position in the modulation signal buffer
- `controlRateSubIndex` (int): sub-sample counter for frame-based interpolation

On note-on, `handleHiseEvent()` calls `state.get().startVoice()` which queries the SignalSource for voice-specific EventData.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: [{"parameter": "ProcessSignal", "impact": "medium", "note": "Enabled mode processes full audio buffer; disabled mode evaluates single sample per block"}]

## Notes

- global_mod registers `NeedsModConfig` and `IsFixRuntimeTarget` properties.
- The display buffer uses a downsampling factor of 32 and sets BufferLength to 2048 samples.
- When the SignalSource is not connected or EventData type is Void, the output defaults to the base value (no modulation applied).
- Missing SN_DESCRIPTION -- could be flagged as an issue.
