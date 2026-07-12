# container.modchain -- C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:71` (interpreted)
**Base class:** `SerialNode`
**Classification:** container

## Signal Path

A serial chain optimized for modulation sources. Wraps children in
`wrap::fix<1, wrap::control_rate<DynamicSerialProcessor>>`, which forces mono
processing and downsamples the processing rate by HISE_EVENT_RASTER.

The modchain does NOT modify the parent's audio signal. The `control_rate`
wrapper creates a separate internal mono control buffer. Children process this
buffer, not the parent's audio. The parent's audio passes through unmodified.

```
Parent audio --> [passthrough, unmodified] --> Parent output
                    |
                    +-- modchain internal:
                        mono controlBuffer (zeroed)
                        sampleRate / HISE_EVENT_RASTER
                        blockSize / HISE_EVENT_RASTER
                        Children process controlBuffer serially
                        (modulation outputs sent via wrap::mod)
```

## Gap Answers

### control-rate-downsampling: Confirm wrap::fix<1, wrap::control_rate<T>> composition

**Confirmed.** `ModulationChainNode` wraps `DynamicSerialProcessor` in
`wrap::fix<1, wrap::control_rate<...>>` (NodeContainerTypes.h:97).

The two-layer composition:
1. `wrap::fix<1>` forces numChannels = 1 (mono)
2. `wrap::control_rate` divides sampleRate and blockSize by HISE_EVENT_RASTER

For a typical 44100Hz / 512 block setup with HISE_EVENT_RASTER = 8:
- Children see: sampleRate = 5512.5 Hz, blockSize = 64, numChannels = 1

Confirmed in `getBlockSizeForChildNodes()` (NodeContainerTypes.cpp:255-258):
`jmax(1, originalBlockSize / HISE_EVENT_RASTER)` (unless frame mode).

Confirmed in `getSampleRateForChildNodes()` (lines 260-263):
`originalSampleRate / (double)HISE_EVENT_RASTER` (unless frame mode).

### audio-isolation: Confirm modchain does not modify parent audio

**Confirmed.** The `control_rate` wrapper (processors.h, per wrap-templates.md
section 3.7) allocates an internal `controlBuffer` and creates a mono
`ProcessData<1>` from it. Children process this separate buffer. The parent's
audio data is never read or written by the modchain's processing.

`ModulationChainNode::process()` (NodeContainerTypes.cpp:218-227) calls
`obj.process(data)` but the `wrap::fix<1, wrap::control_rate<>>` composition
ignores the incoming audio data's content -- it processes its own internal buffer.

### frame-mode-interaction: Behavior when nested in frame-based container

**Confirmed.** `ModulationChainNode::prepare()` (lines 229-240) checks
`ps.blockSize == 1` and sets `isProcessingFrame = true`. When in frame mode:

- `getBlockSizeForChildNodes()` returns `originalBlockSize` (not divided)
- `getSampleRateForChildNodes()` returns `originalSampleRate` (not divided)
- The `control_rate` wrapper's `prepare()` skips downsampling when already
  in frame mode (only forces mono)

This means modchain inside a frame-based container runs at full audio rate,
not control rate. The CPU savings from downsampling are lost.

### hise-event-raster-fx-plugins: FX plugin behavior

**Confirmed.** When `FRONTEND_IS_PLUGIN == 1`, `HISE_EVENT_RASTER = 1`
(hi_tools/Macros.h, per wrap-templates.md section 3.7). This means:
- sampleRate is divided by 1 (unchanged)
- blockSize is divided by 1 (unchanged)
- modchain in FX context runs at full audio rate
- Only the mono channel restriction (wrap::fix<1>) still applies

The CPU savings from control-rate downsampling are not available in FX plugins.

## Parameters

None. Modchain has no parameters of its own.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: [{ "parameter": "HISE_EVENT_RASTER", "impact": "divisor", "note": "instrument plugins process at 1/8 rate; FX plugins at full rate" }]

The reduced processing rate (1/8 for instruments) is the primary CPU optimization.

## Notes

- The `prepare()` method calls `DspHelpers::setErrorIfNotOriginalSamplerate(ps, this)`
  (line 235), validating that modchain is not nested inside a resampled context.
- The modchain itself is NOT a modulation source (`isModulationSource = false`).
  Nodes inside it (like `core.peak`) produce the actual modulation output.
- Container color: `Colour(0xffbe952c)` (amber/orange).
- The description in base data uses lowercase 'a' at start.
