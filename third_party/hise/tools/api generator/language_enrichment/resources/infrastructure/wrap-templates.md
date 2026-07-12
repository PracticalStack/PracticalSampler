# Wrapper Templates Infrastructure Reference (wrap:: namespace)

Distilled from C++ source for the scriptnode node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/node_api/nodes/processors.h` (primary -- all wrapper templates)
- `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h` (container usage)
- `hi_dsp_library/node_api/helpers/node_macros.h` (wrapper macros)
- `hi_tools/Macros.h` (HISE_EVENT_RASTER definition)

---

## 1. Overview: What Wrappers Do

The `wrap::` namespace contains C++ template classes that wrap around a node type
`T` and modify its processing context. Each wrapper holds an instance of T as a
member and forwards most callbacks while intercepting specific ones to transform
the processing environment.

Wrappers serve as the mechanism by which container nodes set up the processing
context for their children. When you place a node inside a `container.frame2_block`,
the container internally wraps its child processor chain with `wrap::frame<2, T>`,
which composes `wrap::fix<2, wrap::frame_x<T>>`.

### Wrapper Transparency

Wrappers come in two flavors based on the macro they use:

| Macro | Behavior | Used By |
|---|---|---|
| `SN_OPAQUE_WRAPPER` | Transparent: `getObject()` and `getWrappedObject()` both delegate to inner obj. The wrapper is invisible to type queries. | `fix`, `skip`, `frame_x`, `event`, `control_rate`, `sidechain`, `no_process`, `no_midi`, `offline` |
| `SN_SELF_AWARE_WRAPPER` | Self-aware: `getObject()` returns `*this`, but `getWrappedObject()` delegates inward. The wrapper itself is addressable. | `init`, `data`, `no_data`, `mod`, `repitch`, `dynamic_blocksize`, `node` |

Self-aware wrappers need to be addressable because they either:
- Have their own parameters (mod, repitch, dynamic_blocksize)
- Need to intercept `setExternalData` (data, no_data)
- Forward `setParameter` themselves (mod, repitch)

---

## 2. Processing Context Modification

Each wrapper modifies specific fields of PrepareSpecs before forwarding to the
child node. This table summarizes what changes:

| Wrapper | Channels | Block Size | Sample Rate | Notes |
|---|---|---|---|---|
| `fix<C, T>` | Set to C | Unchanged | Unchanged | Compile-time fixed channels |
| `frame_x<T>` | Unchanged | Set to 1 | Unchanged | Per-sample processing |
| `frame<C, T>` | Set to C | Set to 1 | Unchanged | Alias for `fix<C, frame_x<T>>` |
| `fix_block<B, T>` | Unchanged | Set to B (max) | Unchanged | Sub-block chunking |
| `dynamic_blocksize<T>` | Unchanged | Set to param (max) | Unchanged | Runtime block size selection |
| `oversample<F, T>` | Unchanged | Multiplied by F | Multiplied by F | Upsamples before, downsamples after |
| `control_rate<T>` | Set to 1 | Divided by RASTER | Divided by RASTER | Mono control signal |
| `event<T>` | Unchanged | Varies (per-event) | Unchanged | Splits at MIDI timestamps |
| `sidechain<T>` | Doubled | Unchanged | Unchanged | Adds empty sidechain channels |
| `repitch<T, I>` | Unchanged | Unchanged* | Unchanged | Resamples signal by ratio |
| `skip<T>` | N/A | N/A | N/A | All callbacks are no-ops |
| `no_data<T>` | Unchanged | Unchanged | Unchanged | Blocks setExternalData |
| `no_process<T>` | Unchanged | Unchanged | Unchanged | process/processFrame are no-ops |
| `no_midi<T>` | Unchanged | Unchanged | Unchanged | handleHiseEvent is no-op |

*repitch passes `blockSize * 2` (MaxDownsampleFactor) to child prepare().

---

## 3. Wrapper Details

### 3.1 fix<C, T> -- Fixed Channel Count

Template: `template <int C, class T> class fix`

Sets the channel count to C at compile time. In `prepare()`, overrides
`ps.numChannels = C` before forwarding. In `process()`, casts the incoming
ProcessDataType to `ProcessData<C>` via `data.as<ProcessData<C>>()`.

**Key behaviors:**
- Child node receives `ProcessData<C>` instead of `ProcessDataDyn`
- If incoming data has more than C channels, leftover channels are not processed
- If incoming data has fewer than C channels: undefined behavior (jassert fires)
- `processFrame()` similarly casts to `span<float, C>`

**Container usage:** Every `container.fixN_block` (e.g. `container.fix2_block`)
wraps its internal serial processor with `wrap::fix<N, ...>`.

The `SingleSampleBlock<N>` (i.e. `container.frameN_block`) handles channel
mismatch gracefully by allocating a leftover buffer for extra channels.

### 3.2 frame_x<T> -- Frame-Based Processing (Dynamic Channels)

Template: `template <class T> class frame_x`

Converts block-based processing to per-sample (frame) processing. In `prepare()`,
sets `ps.blockSize = 1`. In `process()`, uses `FrameConverters::processFix<C>()`
for fixed-channel data or `FrameConverters::forwardToFrame16()` for dynamic
channel data.

The `FrameConverters` utility iterates sample-by-sample, interleaving channel
data into a `span<float, C>` frame, calling `processFrame()` on the child,
then de-interleaving back.

**Container usage:** `container.framex_block` wraps with just `frame_x<T>`,
supporting any channel count at runtime.

### 3.3 frame<C, T> -- Frame-Based Processing (Fixed Channels)

Definition: `template <int C, typename T> using frame = fix<C, frame_x<T>>;`

This is a type alias, not a separate class. It composes `fix` and `frame_x`:
1. `fix<C, ...>` fixes the channel count to C
2. `frame_x<T>` converts to per-sample processing

**Container usage:** `container.frame2_block` wraps with `wrap::frame<2, T>`.

### 3.4 fix_block<B, T> -- Fixed Sub-Block Size

Template: `template <int BlockSize, class T> struct fix_block`

Inherits from `fix_blockx<T, static_functions::fix_block<BlockSize>>`.

Splits incoming audio into chunks of at most B samples. The core logic is in
`static_functions::fix_block<BlockSize>`:

- `prepare()`: Calls child prepare with `ps.withBlockSizeT<BlockSize>(true)`,
  which sets blockSize = min(BlockSize, originalBlockSize)
- `process()`: If `numSamples < BlockSize`, forwards directly (no splitting).
  Otherwise, uses `ChunkableProcessData` to iterate in BlockSize-sized chunks.
  HiseEvents are forwarded to each chunk (timestamp-aware splitting).

**Container usage:** `container.fix8_block`, `container.fix32_block`, etc.
These containers show `getBlockSizeForChildNodes() = FixedBlockSize` unless
bypassed or already in frame mode (blockSize == 1).

### 3.5 dynamic_blocksize<T> -- Runtime Block Size Selection

Template: `template <class T> struct dynamic_blocksize`

Like fix_block but the block size is a runtime parameter (P=0). Has a single
parameter that selects from a fixed set of block sizes:

| Index | Block Size |
|---|---|
| 0 | 1 (frame mode) |
| 1 | 8 |
| 2 | 16 |
| 3 | 32 |
| 4 | 64 (default) |
| 5 | 128 |
| 6 | 256 |
| 7 | 512 |

When index 0 is selected (blockSize = 1), processing switches to frame mode
using `FrameConverters::processFix<C>()` instead of ChunkableProcessData.

Uses a `SimpleReadWriteLock` for thread-safe block size changes. On parameter
change, calls `prepare()` again to propagate the new block size to children.

**Constraint:** `isPolyphonic()` always returns false.

### 3.6 oversample<F, T> -- Oversampling

Template: `template <int OversampleFactor, class T, class InitFunctionClass> class oversample`

Inherits from `oversample_base`. Upsamples the signal before processing the
child, then downsamples afterward. Uses JUCE's `dsp::Oversampling<float>`.

**Static oversampling (F > 0):**
- Factor is fixed at compile time (F must be a power of 2)
- PrepareSpecs: `sampleRate *= F`, `blockSize *= F`
- Only the FilterType parameter (P=0) is exposed

**Dynamic oversampling (F == 0, registered as OversampleFactor == -1):**
- Factor is set via parameter P=0 (exponent: 0=1x, 1=2x, 2=4x, 3=8x, 4=16x)
- Parameter P=1 controls filter type
- `MaxOversamplingExponent = 4` (max 16x)

**Filter types:**
- 0: `filterHalfBandPolyphaseIIR` (default) -- lower latency
- 1: `filterHalfBandFIREquiripple` -- steeper rolloff

**Processing flow:**
1. `oversampler->processSamplesUp(audioBlock)` -- upsample
2. Create new ProcessData with upsampled buffer (numSamples * factor)
3. Call child's `process()`
4. `oversampler->processSamplesDown(audioBlock)` -- downsample back

**Constraints:**
- `isPolyphonic()` always returns false (throws `Error::IllegalPolyphony` if
  voiceIndex is enabled during prepare)
- `processFrame()` asserts false -- oversampling only works at block level
- Uses `SimpleReadWriteLock` for thread-safe factor/filter changes

**Container usage:** `container.oversample2x`, `container.oversample4x`, etc.
for fixed factors. `container.oversampleX` for dynamic.

### 3.7 control_rate<T> -- Control Rate Downsampling

Template: `template <class T> class control_rate`

Downsamples the processing rate by `HISE_EVENT_RASTER` and forces mono (1 channel).
Designed for modulation source chains that don't need full audio rate.

**HISE_EVENT_RASTER values:**
- Default: 8 (instrument plugins)
- `FRONTEND_IS_PLUGIN == 1`: 1 (effect plugins -- no downsampling)

**prepare() behavior:**
- If already in frame mode (blockSize == 1): only sets numChannels = 1
- Otherwise: divides both sampleRate and blockSize by HISE_EVENT_RASTER,
  sets numChannels = 1, allocates control buffer

**process() behavior:**
- Computes `numToProcess = numSamples / HISE_EVENT_RASTER`
- Creates a mono ProcessData<1> using the internal controlBuffer
- Clears the buffer, then calls child's process()

**processFrame() behavior:**
- Creates a single-sample mono frame `{0.0f}` and calls child's processFrame()
- Comment in source: "must always be wrapped into a fix<1> node"

**Container usage:** `container.modchain` wraps with
`wrap::fix<1, wrap::control_rate<T>>`, composing both fixed mono channel
and control-rate downsampling.

`isModulationSource` is explicitly set to `false` (the control_rate wrapper
itself is not a mod source; the nodes inside it may be).

### 3.8 event<T> -- MIDI Event-Aware Processing

Template: `template <class T> class event`

Splits audio processing at MIDI event timestamps so that `handleHiseEvent()`
is called at sample-accurate positions within the block.

**Processing flow (in static_functions::event::process):**
1. Get event list from data via `toEventData()`
2. If no events: forward directly to child's `process()`
3. If events exist:
   - Use `ChunkableProcessData` (without HiseEvent forwarding in chunks)
   - For each non-ignored event, ordered by timestamp:
     a. Process audio samples from last position to event position
     b. Call `handleHiseEvent()` on the event
   - Process any remaining samples after the last event

**Key detail:** Sets `isProcessingHiseEvent() = true` (always), regardless of
the wrapped type's setting. This is how `container.midichain` enables MIDI
processing for its children.

**Container usage:** `container.midichain` wraps with `wrap::event<T>`.

### 3.9 sidechain<T> -- Sidechain Channel Doubling

Template: `template <class T> class sidechain`

Doubles the channel count to provide sidechain input channels. The extra
channels are allocated as a zeroed buffer.

**prepare():**
- Allocates sideChainBuffer if not in frame mode
- Calls child prepare with `ps.numChannels *= 2`

**process():**
- Creates a new channel pointer array of size `numChannels * 2`
- First half: original audio channel pointers
- Second half: zeroed sidechain buffer (one block per channel)
- Creates new ProcessData with doubled channels and forwards to child

**processFrame():** Asserts false -- not supported in frame mode.

**Container usage:** `container.sidechain` wraps with `wrap::sidechain<T>`.

### 3.10 repitch<T, InterpolatorType> -- Pitch-Based Resampling

Template: `template <class T, class InterpolatorType> class repitch`

Resamples the audio signal to process at a different effective sample rate.
Has two parameters:
- P=0: RepitchFactor (ratio, clamped to 0.5..2.0)
- P=1: Interpolation type (only for `interpolators::dynamic`)

**Interpolator types:**
- `interpolators::cubic` (CatmullRom)
- `interpolators::linear`
- `interpolators::none` (ZeroOrderHold)
- `interpolators::dynamic` -- runtime selectable between above three

**Processing flow:**
1. Compute internal sample count: `round(numSamples / ratio)`
2. Downsample input into internal buffer (per channel)
3. Call child's `process()` on internal buffer
4. Upsample result back to original buffer

**prepare():** Passes `blockSize * MaxDownsampleFactor` (= blockSize * 2) to child.

**Constraints:**
- Only works with 1 or 2 channels (hardcoded switch in process())
- processFrame() asserts false
- Uses SimpleReadWriteLock for thread-safe ratio changes

**Container usage:** `container.repitch` wraps with
`wrap::repitch<T, wrap::interpolators::dynamic>`.

### 3.11 mod<ParameterClass, T> -- Modulation Source

Template: `template <class ParameterClass, class T> struct mod`

Wraps a node that produces modulation output and connects it to parameter targets.
Does NOT modify PrepareSpecs -- it only adds post-processing modulation dispatch.

**Modulation check points:**
After each of these callbacks, `checkModValue()` is called:
- `process()`
- `processFrame()`
- `reset()`
- `handleHiseEvent()`

**checkModValue() logic:**
1. Call `handleModulation(modValue)` on the wrapped object
2. If it returns true, call `p.call(modValue)` on the ParameterClass

The wrapped node must implement `bool handleModulation(double& value)` which:
- Sets value to the current modulation output
- Returns true if the value should be sent to targets
- Returns false to skip redundant updates

**Connection API:**
```cpp
template <int I, class TargetType> void connect(TargetType& t)
{
    p.getParameter<0>().connect<I>(t);
}
```

**Container usage:** Used in `StaticNodeWrappers.h` as
`bypass::simple<wrap::mod<parameter::dynamic_base_holder, OpaqueNode>>`.

### 3.12 skip<T> -- Complete Bypass

Template: `template <class T> class skip`

All processing callbacks are empty no-ops: prepare, reset, process, processFrame,
handleHiseEvent. Only `initialise()` is forwarded. The wrapped object is
constructed but never processes audio.

Used by the code generator to represent bypassed nodes in compiled output.

### 3.13 no_data<T> -- External Data Blocker

Template: `template <class T> class no_data`

Forwards all callbacks normally except `setExternalData()`, which is a no-op.
This prevents the wrapped node from receiving external data connections.

Also forwards `connectToRuntimeTarget()` and parameter access.

### 3.14 no_process<T> -- Audio Processing Blocker

Template: `template <class T> struct no_process`

Forwards prepare, reset, init, and parameters. Blocks process, processFrame,
handleHiseEvent, and handleModulation (all empty). Used for nodes that should
receive prepare/reset lifecycle but not participate in audio processing.

### 3.15 no_midi<T> -- MIDI Event Blocker

Template: `template <class T> struct no_midi`

Forwards everything normally except `handleHiseEvent()`, which is a no-op.

**Container usage:** `container.no_midi` wraps with `wrap::no_midi<T>`.

### 3.16 init<T, Initialiser> -- Custom Initialization

Template: `template <class T, class Initialiser> class init`

Adds a custom initializer that runs during construction and initialise(). The
Initialiser class takes a `T&` in its constructor and can set up default values
and parameter connections.

Used as a base for `wrap::data`. The initialiser's `initialise(n)` is called
after the wrapped object's `initialise()`.

### 3.17 data<T, DataHandler> -- External Data Handler

Template: `template <class T, class DataHandler> struct data`

Extends `wrap::init<T, DataHandler>` with external data routing. The DataHandler
must define:
- Static constants: `NumTables`, `NumSliderPacks`, `NumAudioFiles`, `NumFilters`,
  `NumDisplayBuffers`
- `void setExternalData(T& obj, const ExternalData& data, int index)`

The `setExternalData()` call is routed through the DataHandler, which distributes
data slots to child nodes.

Also inherits from `data::pimpl::provider_base` and connects to the mothernode
data provider system if applicable.

### 3.18 node<T> -- SNEX/C++ Node Wrapper

Template: `template <class T> struct node`

The outermost wrapper for compiled SNEX or C++ custom nodes. Provides the bridge
between the raw node implementation (with metadata class) and the OpaqueNode
system.

**Key behaviors:**
- Reads `NumChannels`, `NumTables`, etc. from `T::metadata`
- Enforces channel count: `prepare()` throws `Error::ChannelMismatch` if
  `ps.numChannels != NumChannels`
- Casts ProcessDataDyn to ProcessData<NumChannels> for processing
- Handles modulation output, external data, runtime targets
- Creates parameters from the encoded parameter metadata

### 3.19 offline<T> -- Offline Rendering

Template: `template <class T> class offline`

A wrapper for nodes that render to external data objects (not realtime).
process(), processFrame(), and handleHiseEvent() are all empty. Only
prepare(), reset(), and initialise() are forwarded.

### 3.20 illegal_poly<T> -- Polyphony Error Guard

Template: `template <typename T> struct illegal_poly`

Throws `Error::IllegalPolyphony` in prepare(). All processing callbacks are
empty. Used as a replacement type for nodes that cannot operate in polyphonic
contexts. Displays "(not available in a poly network)" as description.

### 3.21 Clone Wrappers

Type aliases for clone-based processing:

```
clonechain<T, N>     = clone_base<clone_data<T, yes, N>, Serial>
clonesplit<T, N>     = clone_base<clone_data<T, yes, N>, Parallel>
clonecopy<T, N>      = clone_base<clone_data<T, yes, N>, Copy>
fix_clonechain<T, N> = clone_base<clone_data<T, no, N>, Serial>
fix_clonesplit<T, N> = clone_base<clone_data<T, no, N>, Parallel>
fix_clonecopy<T, N>  = clone_base<clone_data<T, no, N>, Copy>
```

The `options::yes` vs `options::no` controls whether the clone count is dynamic
(runtime adjustable) or fixed at compile time.

---

## 4. Wrapper Composition (Chaining)

Wrappers compose by nesting: the outermost wrapper processes first, modifies
the context, then forwards to the next wrapper inward. The innermost type is
the actual processing node or container.

### Reading Order

In C++ template syntax, wrappers are read inside-out:
```cpp
wrap::fix<2, wrap::frame_x<T>>
```
This means: `fix<2>` is the outer wrapper, `frame_x` is the inner wrapper, `T`
is the node. Processing order:
1. `fix<2>::process()` -- casts to 2-channel ProcessData
2. `frame_x::process()` -- converts to per-sample iteration
3. `T::processFrame()` -- processes one sample at a time

### PrepareSpecs Propagation

Each wrapper modifies PrepareSpecs before passing it inward:
```
Original:        {sr=44100, bs=512, ch=2}
  fix<1>:        {sr=44100, bs=512, ch=1}       -- channels overridden
    control_rate: {sr=5512, bs=64, ch=1}         -- sr/bs divided by 8
      T::prepare({sr=5512, bs=64, ch=1})
```

### Common Compositions in Container Nodes

| Container Factory ID | Wrapper Composition | Effect |
|---|---|---|
| `container.modchain` | `fix<1, control_rate<T>>` | Mono, downsampled control rate |
| `container.midichain` | `event<T>` | Sample-accurate MIDI splitting |
| `container.no_midi` | `no_midi<T>` | Block MIDI events |
| `container.sidechain` | `sidechain<T>` | Double channel count |
| `container.frame2_block` | `frame<2, T>` = `fix<2, frame_x<T>>` | 2ch per-sample |
| `container.framex_block` | `frame_x<T>` | Dynamic-ch per-sample |
| `container.fix8_block` | `fix_block<8, T>` | 8-sample sub-blocks |
| `container.fix32_block` | `fix_block<32, T>` | 32-sample sub-blocks |
| `container.oversample2x` | `oversample<2, T>` | 2x oversampling |
| `container.oversample4x` | `oversample<4, T>` | 4x oversampling |
| `container.oversampleX` | `oversample<0, T>` | Dynamic oversampling |
| `container.repitch` | `repitch<T, dynamic>` | Pitch resampling |
| `container.fixN_block` (compiled) | `fix<N, T>` | N-channel fixed |

### Container getBlockSizeForChildNodes() / getSampleRateForChildNodes()

These virtual methods on the container node class return the *effective* specs
that child nodes see. They mirror the wrapper's prepare() logic:

- `FixedBlockNode::getBlockSizeForChildNodes()`: returns FixedBlockSize (or
  originalBlockSize if bypassed or already frame mode)
- `SingleSampleBlock::getBlockSizeForChildNodes()`: returns 1 (or originalBlockSize
  if bypassed)
- `OversampleNode::getSampleRateForChildNodes()`: returns sampleRate * factor
- `OversampleNode::getBlockSizeForChildNodes()`: returns blockSize * factor

---

## 5. Threading and Safety Patterns

Several wrappers use `SimpleReadWriteLock` for thread-safe parameter changes:

| Wrapper | Lock Usage |
|---|---|
| `oversample` | Write lock during prepare/rebuild, read lock during process/reset |
| `repitch` | ScopedMultiWriteLock for ratio/interpolation, ScopedTryReadLock in process |
| `dynamic_blocksize` | Write lock when changing block size, ScopedTryReadLock in process |

The pattern is consistent: parameter changes acquire write locks and may call
`prepare()` again. Audio-thread processing uses try-read locks that silently
skip processing if the lock is held (avoids blocking the audio thread).

The `oversample` wrapper is explicitly monophonic (`isPolyphonic() = false`)
and throws `Error::IllegalPolyphony` if a PolyHandler is active during prepare.

---

## 6. Frame Mode Detection

Several wrappers check for `blockSize == 1` as a frame-mode indicator:

- `control_rate::prepare()`: Skips downsampling when already in frame mode
  (no division by HISE_EVENT_RASTER, no buffer allocation)
- `fix_block (static_functions)::prepare()`: Uses `withBlockSizeT<B>(true)`,
  where the `true` flag means "don't override if already frame mode"
- `dynamic_blocksize::process()`: When blockSize parameter == 1, switches to
  FrameConverters instead of ChunkableProcessData
- `sidechain::prepare()`: Skips sidechain buffer allocation when blockSize == 1

This means frame mode propagates correctly through nested wrappers. A
`container.frame2_block` that contains a `container.modchain` will result in
the modchain seeing blockSize=1, which disables the RASTER downsampling and
processes the control chain sample-by-sample.

---

## 7. Interpolators Namespace

The `scriptnode::interpolators` namespace (in processors.h) provides resampling
interpolators used by `wrap::repitch` and `wrap::oversample`:

| Type | JUCE Class | Quality |
|---|---|---|
| `cubic` | `Interpolators::CatmullRom` | Highest quality, 4-point |
| `linear` | `Interpolators::Linear` | Mid quality, 2-point |
| `none` | `Interpolators::ZeroOrderHold` | Lowest, nearest sample |
| `dynamic` | Runtime switch | Selectable via enum |

The `dynamic` interpolator holds all three types and switches between them
via `setType(Types t)`. The enum values are:
- `Types::Cubic` (0)
- `Types::Linear` (1)
- `Types::None` (2)

---

## 8. Key Constants

| Constant | Value | Source |
|---|---|---|
| `HISE_EVENT_RASTER` | 8 (instruments), 1 (FX plugins) | hi_tools/Macros.h |
| `MaxOversamplingExponent` | 4 (=> max 16x) | oversample_base |
| `MaxDownsampleFactor` (repitch) | 2 | repitch template |
| `NUM_MAX_CHANNELS` | 16 | Used in oversample process() |
| Dynamic blocksize options | 1, 8, 16, 32, 64, 128, 256, 512 | dynamic_blocksize |
