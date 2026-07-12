# container.repitch -- C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:544` (interpreted), `hi_dsp_library/node_api/nodes/processors.h` (wrap::repitch)
**Base class:** `SerialNode`
**Classification:** container

## Signal Path

Resamples the audio signal before and after child processing. Children see a
different effective sample rate depending on the RepitchFactor.

```
Input (numSamples at original SR)
  |
  Downsample: round(numSamples / ratio) internal samples
  |
  Children process internal buffer at effective SR
  |
  Upsample back to original numSamples
  |
Output (numSamples at original SR)
```

RepitchFactor > 1.0: more internal samples (children see lower effective pitch).
RepitchFactor < 1.0: fewer internal samples (children see higher effective pitch).
RepitchFactor = 1.0: no resampling (passthrough).

## Gap Answers

### resampling-flow: Confirm wrap::repitch processing flow

**Confirmed.** The `wrap::repitch` template (processors.h, per wrap-templates.md
section 3.10):

1. Compute internal sample count: `round(numSamples / ratio)`
2. Downsample input into internal buffer (per channel)
3. Call child's `process()` on the internal buffer
4. Upsample result back to original buffer

The internal buffer is allocated during `prepare()` with size
`blockSize * MaxDownsampleFactor` where `MaxDownsampleFactor = 2`
(so blockSize * 2).

The interpreted `RepitchNode::prepare()` (NodeContainerTypes.cpp:1390-1395)
calls `NodeBase::prepare(ps)`, `prepareNodes(ps)`, then `obj.prepare(ps)`.
The wrapper's prepare passes `blockSize * 2` to children.

### interpolation-types: Confirm three interpolation modes

**Confirmed.** `RepitchNode::createInternalParameterList()`
(NodeContainerTypes.cpp:1407-1429):

- Interpolation parameter uses `setParameterValueNames({"Cubic", "Linear", "None"})`
- Maps to `wrap::interpolators::dynamic` which supports:
  - 0 = Cubic (CatmullRom) -- highest quality, 4-point interpolation
  - 1 = Linear -- mid quality, 2-point interpolation
  - 2 = None (ZeroOrderHold) -- lowest quality, nearest sample

### channel-limitation: 1 or 2 channels only

**Confirmed.** The `wrap::repitch::process()` method dispatches only for 1 or 2
channels (per wrap-templates.md section 3.10, already logged as Issue 8):

```cpp
if(data.getNumChannels() == 1) processFixed<1>(data);
if(data.getNumChannels() == 2) processFixed<2>(data);
```

For > 2 channels, no processing occurs -- data passes through unmodified with
no assertion or error. This is a silent failure.

### frame-processing-limitation: Frame processing not supported

**Confirmed.** `RepitchNode::processFrame()` (NodeContainerTypes.h:583-586) is
an empty function body (no assertion, just does nothing). The wrapper's
`processFrame()` asserts false (per wrap-templates.md section 3.10).

Repitch containers cannot be used inside frame-based containers.

### repitch-factor-direction: Factor direction

**Confirmed.** RepitchFactor range is 0.5 to 2.0 with skew for centre at 1.0
(NodeContainerTypes.cpp:1412-1418):
- `p.setRange({0.5, 2.0})`
- `p.setSkewForCentre(1.0)`
- Default: 1.0 (no resampling)

Factor > 1.0 means `numSamples / ratio` produces FEWER internal samples, so
children process a shorter buffer. This effectively raises the pitch of the
processed signal (children see the signal compressed in time).

Factor < 1.0 means more internal samples, lowering the effective pitch (children
see the signal stretched in time).

## Parameters

- **RepitchFactor** (P=0): Resampling ratio. Range 0.5-2.0, default 1.0.
  Logarithmic skew centered at 1.0. One octave range in each direction.
- **Interpolation** (P=1): Interpolation quality. 0=Cubic, 1=Linear, 2=None.
  Default 0 (Cubic).

## CPU Assessment

baseline: medium
polyphonic: false
scalingFactors: [
  { "parameter": "Interpolation", "impact": "quality-dependent", "note": "Cubic uses 4-point interpolation per sample; None uses nearest-neighbor" }
]

The resampling operations (downsample + upsample) add per-sample cost
proportional to the interpolation quality. Children's CPU cost is also
affected by the changed block size.

## Notes

- `HasFixedParameters = true` -- parameters cannot be added/removed.
- Uses `SimpleReadWriteLock` for thread-safe ratio/interpolation changes
  (per wrap-templates.md section 5).
- `setBypassed()` is an empty override (NodeContainerTypes.h:598-601) --
  bypass state changes do not trigger re-preparation. This differs from
  OversampleNode and FixedBlockNode which re-prepare on bypass.
- The range is limited to one octave (0.5-2.0). Stacking multiple repitch
  containers can extend the range.
- The channel limitation (max 2) is already logged as Issue 8 in issues.md.
