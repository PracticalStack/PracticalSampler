# container.oversample - C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:414` (OversampleNode<-1>)
**Wrapper source:** `hi_dsp_library/node_api/nodes/processors.h:956` (wrap::oversample)
**Base class:** `SerialNode` (interpreted container), wraps `wrap::oversample<-1, DynamicSerialProcessor>`
**Classification:** container

## Signal Path

Input audio is upsampled by the configured factor using JUCE's `dsp::Oversampling<float>`,
then the child node chain processes the upsampled signal serially (via DynamicSerialProcessor),
then the result is downsampled back to the original sample rate.

```
Input -> [JUCE Oversampler: upsample] -> child1 -> child2 -> ... -> childN -> [JUCE Oversampler: downsample] -> Output
```

The child chain is a standard serial chain (DynamicSerialProcessor iterates
`getNodeList()` sequentially, each child modifies the buffer in-place).

### Processing flow (processors.h:1011-1032)

1. Acquire read lock on `this->lock`
2. If oversampler is null, return (no processing)
3. Convert ProcessData to JUCE AudioBlock
4. `oversampler->processSamplesUp(audioBlock)` -- upsample to internal buffer
5. Build new ProcessData with upsampled channel pointers and `numSamples * oversamplingFactor`
6. Copy non-audio data (MIDI events) from original data
7. Call `obj.process(od)` -- serial child chain processes upsampled data
8. `oversampler->processSamplesDown(audioBlock)` -- downsample back to original buffer

Channel pointers are extracted into a `float* tmp[NUM_MAX_CHANNELS]` stack array (max 16 channels).

### Bypass behavior (NodeContainerTypes.cpp:297-313, 342-368)

When bypassed:
- `setBypassed()` re-prepares with original specs (originalBlockSize, originalSampleRate)
- `process()` calls `obj.getWrappedObject().process(d)` -- the DynamicSerialProcessor
  directly, bypassing the oversample wrapper entirely
- Children see original sampleRate and blockSize
- `getSampleRateForChildNodes()` returns `originalSampleRate * 1` (factor becomes 1)
- `getBlockSizeForChildNodes()` returns `originalBlockSize * 1`

When un-bypassed:
- `setBypassed()` re-prepares with original specs, which flows through the
  oversample wrapper's prepare and re-creates the oversampler

## Gap Answers

### child-dispatch-serial: Serial child processing confirmation

Yes, confirmed. The wrapper composition is `wrap::oversample<-1, SerialNode::DynamicSerialProcessor>`.
The DynamicSerialProcessor iterates `parent->getNodeList()` in a for-each loop
(see containers.md section 4). Children process serially within the oversampled context.
The `obj.process(od)` call at processors.h:1029 passes the upsampled ProcessData
to the DynamicSerialProcessor which iterates all children sequentially.

### oversampling-parameter-encoding: Oversampling parameter value mapping

The Oversampling parameter (P=0 for dynamic variant) has range 0-4, step 1. The value
is an exponent passed to `setOversamplingFactor(int factorExponent)` (processors.h:750-760):

```
factorExponent = jlimit(0, MaxOversamplingExponent, factorExponent)  // clamp to 0-4
oversamplingFactor = std::pow(2, factorExponent)                      // 1, 2, 4, 8, 16
```

Display names (NodeContainerTypes.h:491-501):
| Parameter Value | Exponent | Factor | Display Name |
|---|---|---|---|
| 0 | 0 | 1 (2^0) | "None" |
| 1 | 1 | 2 (2^1) | "2x" |
| 2 | 2 | 4 (2^2) | "4x" |
| 3 | 3 | 8 (2^3) | "8x" |
| 4 | 4 | 16 (2^4) | "16x" |

Default value is 1 (= 2x oversampling), set at NodeContainerTypes.h:504.

When Oversampling=0 (factor=1): The JUCE oversampler is still instantiated with
`log2(1) = 0` as the oversampling order. `dsp::Oversampling` with order 0 creates
a trivial passthrough (no filtering, no latency), but the upsample/downsample
calls still execute. This adds minimal overhead (buffer copies) compared to not
using the container at all.

### filter-type-names: Filter type display names

FilterType parameter (P=1 for dynamic, P=0 for fixed variants) has range 0-1, step 1.

Display names (NodeContainerTypes.h:513): `StringArray("Polyphase", "FIR")`

Mapping (processors.h:766-771):
| Value | FilterType enum | Display Name | Characteristics |
|---|---|---|---|
| 0 | filterHalfBandPolyphaseIIR | "Polyphase" | Lower latency, default |
| 1 | filterHalfBandFIREquiripple | "FIR" | Steeper rolloff |

These are the only 2 options. The `jlimit(0, 1, nt)` call ensures out-of-range
values are clamped.

### latency-reporting: Latency propagation to host

No. The `wrap::oversample` template and `OversampleNode` do not call
`getLatencyInSamples()` on the JUCE oversampler, nor do they report latency
to the DspNetwork or host. The JUCE `dsp::Oversampling` object does track its
own latency internally, but this information is not propagated.

There is no latency compensation mechanism in the oversample container. The
oversampling filter introduces a small amount of latency (depends on filter type
and factor), but this is not reported or compensated.

### bypass-reprepare: Children revert to original specs when bypassed

Yes, confirmed. `OversampleNode::setBypassed()` (NodeContainerTypes.cpp:297-313):

1. Calls `SerialNode::setBypassed(shouldBeBypassed)` to set the bypass state flag
2. Constructs a PrepareSpecs from `originalBlockSize`, `originalSampleRate`,
   `getCurrentChannelAmount()`, and `lastVoiceIndex`
3. Calls `prepare(ps)` which:
   - If bypassed: calls `obj.getWrappedObject().prepare(ps)` -- prepares DynamicSerialProcessor
     directly with original specs
   - If not bypassed: calls `obj.prepare(ps)` -- goes through oversample wrapper
     which multiplies sampleRate and blockSize by factor
4. Calls `getRootNetwork()->runPostInitFunctions()`

So yes, when bypassed, children see the original sampleRate and blockSize.

### dynamic-factor-change-behavior: Runtime factor change behavior

When the Oversampling parameter changes at runtime, `setOversamplingFactor()` is called
(processors.h:750-760):

1. Acquires `SimpleReadWriteLock::ScopedWriteLock` (blocks audio thread)
2. Computes new factor: `oversamplingFactor = pow(2, jlimit(0, 4, exponent))`
3. If specs are valid, calls `prepare(originalSpecs)` which:
   a. Also acquires write lock (re-entrant)
   b. Multiplies sampleRate and blockSize by new factor
   c. Calls child prepare with new specs
   d. Calls `rebuildOversampler()` which creates a NEW `dsp::Oversampling` object
      and swaps it in via `ScopedPointer::swapWith()`

The `rebuildOversampler()` (processors.h:703-719) creates a brand new JUCE
`Oversampling` object, calls `initProcessing()`, then atomically swaps it with
the old one. The old oversampler is destroyed.

The audio thread uses `ScopedReadLock` in `process()`. If the write lock is held
during factor change, the `ScopedReadLock` blocks briefly -- this causes a
momentary audio gap (silence or glitch) during the re-preparation. This is NOT
seamless. The factor change should be treated as a non-realtime operation.

The OversampleNode layer adds additional overhead: `setOversamplingFactor()` at
NodeContainerTypes.h:449-458 also calls `prepareNodes(lastSpecs)` to re-prepare
child nodes in the interpreted container graph.

### max-channel-count: Channel count limit

The `process()` method uses `float* tmp[NUM_MAX_CHANNELS]` (processors.h:1021)
where NUM_MAX_CHANNELS is 16 (a global scriptnode constant). This is a stack-allocated
array that holds pointers to the oversampled channel data.

The JUCE `dsp::Oversampling` constructor receives `numChannels` and allocates internal
buffers accordingly. There is no explicit channel count validation in the oversample
wrapper beyond the stack array size.

Practical limit: 16 channels (NUM_MAX_CHANNELS). Using more channels would write
past the stack array bounds.

## Parameters

### Oversampling (dynamic variant only, P=0)
Range 0-4 (integer step 1). Exponent for the oversampling factor (2^exponent).
Display values: "None", "2x", "4x", "8x", "16x". Default: 1 (2x).
Changes trigger full re-preparation with write lock.
Only present when `OversampleFactor == -1` (dynamic template parameter).

### FilterType (P=1 for dynamic, P=0 for fixed variants)
Range 0-1 (integer step 1). Selects the anti-aliasing filter.
Display values: "Polyphase", "FIR". Default: 0 (Polyphase).
Changes also trigger full re-preparation (processors.h:773-781), but only if the
filter type actually changed (early return on same value).

## Conditional Behaviour

### Dynamic vs Fixed template parameter

When `OversampleFactor == -1` (registered as `OversampleNode<-1>`, factory ID `oversample`):
- Constructor passes -1 to oversample_base, which sets `oversamplingFactor = jmax(1, -1) = 1`
- Both Oversampling and FilterType parameters are exposed
- `setParameter<0>` routes to `setOversamplingFactor()`, `setParameter<1>` routes to `setFilterType()`

When `OversampleFactor > 0` (e.g., 2, 4, 8, 16):
- Constructor passes the fixed factor
- Only FilterType parameter is exposed (no Oversampling parameter)
- `setParameter<0>` routes to `setFilterType()` directly
- Static assert enforces: `P == 0` only (no P=1 for fixed variants)

The static assert at processors.h:978-979:
```cpp
static_assert(P == 0 || P == 1, "illegal parameter index");
static_assert(OversamplingFactor == 0 || P == 0, "wrong filter type index for static oversampling");
```

Note: `OversamplingFactor == 0` in the template maps to the dynamic case (the
factory registers -1 as the template parameter, but the template condition checks
for 0 in the `setParameter` dispatch. Looking more carefully: `if constexpr(P == 0 && OversamplingFactor == 0)` -- this means the template uses 0 for the dynamic case
internally, but the factory registers `OversampleNode<-1>`. The mismatch is
resolved because the OversampleNode constructor and wrapper handle the -1 -> 0
mapping. Actually, looking at the code again:

- `OversampleNode<-1>` uses `wrap::oversample<-1, ...>`
- `wrap::oversample<-1, ...>` calls `oversample_base(-1)` which sets factor to `jmax(1,-1) = 1`
- The template condition `OversamplingFactor == 0` at line 981 is for the compiled
  path where 0 means "dynamic". But for interpreted containers, -1 is used.

The compiled static_asserts use `OversamplingFactor == 0` for dynamic detection. The
interpreted OversampleNode uses -1. Both work because the parameter routing logic
is in OversampleNode::setOversamplingFactor/setFilterType (not in the template's
setParameter<P>).

### Prepare constraints

Two runtime checks in `OversampleNode::prepare()`:
1. `DspHelpers::setErrorIfFrameProcessing(ps)` -- throws `Error::IllegalFrameCall`
   if blockSize == 1. Cannot be placed inside a frame container.
2. `DspHelpers::setErrorIfNotOriginalSamplerate(ps, this)` -- throws
   `Error::SampleRateMismatch` if sampleRate != root network's original. Cannot
   be nested inside another oversample container or any wrapper that changes sampleRate.

### Polyphony constraint

`oversample_base::prepare()` (processors.h:727-731):
```cpp
if (ps.voiceIndex != nullptr && ps.voiceIndex->isEnabled())
    scriptnode::Error::throwError(Error::IllegalPolyphony);
```

Also `wrap::oversample::isPolyphonic()` returns false (constexpr).
Oversampling is strictly monophonic.

## CPU Assessment

baseline: high
polyphonic: false
scalingFactors:
  - parameter: Oversampling, impact: multiplicative, note: "CPU scales linearly with oversampling factor. 16x = 16x the work for all child nodes plus upsample/downsample overhead."
  - parameter: FilterType, impact: minor, note: "FIR filter is slightly more expensive than Polyphase but the difference is small compared to the child processing cost."

## Notes

- The `hasFixedParameters()` override returns true (NodeContainerTypes.h:469), which means
  the OversampleNode parameters are "fixed" -- they cannot be removed or re-ordered.
  This also affects `setOversamplingFactor()` which early-returns if `!hasFixedParameters()`.

- The factory registration at NodeContainer.cpp:764-768 registers all 5 variants:
  `OversampleNode<2>`, `<4>`, `<8>`, `<16>`, `<-1>`.

- Template instantiations at NodeContainerTypes.cpp:370-373 explicitly instantiate
  only 2, 4, 8, 16 (not -1). The -1 variant is presumably instantiated elsewhere
  or by the registration code.

- In the backend (USE_BACKEND), the process() method calculates the profiler's
  effective sample count as `numSamples * oversamplingFactor` for accurate CPU
  profiling (NodeContainerTypes.cpp:354-359).

- The `processFrame()` method at NodeContainerTypes.h:539 asserts false -- frame-based
  processing is not supported. The prepare() check also enforces this by throwing
  IllegalFrameCall for blockSize==1.

- JUCE's `dsp::Oversampling` third constructor argument `false` means it does NOT
  use maximum quality settings (processors.h:713). This keeps the filter order
  reasonable for real-time use.
