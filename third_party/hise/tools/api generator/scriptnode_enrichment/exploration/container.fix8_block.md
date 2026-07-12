# container.fix8_block -- C++ Exploration

**Source:** `hi_dsp_library/node_api/nodes/processors.h:1284` (compiled `wrap::fix_block`), `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:610` (interpreted `FixedBlockNode`)
**Base class:** `wrap::fix_blockx<T, static_functions::fix_block<8>>` (compiled), `SerialNode` (interpreted)
**Classification:** container

## Signal Path

Serial container that splits incoming audio into fixed-size chunks of 8 samples.
Children process each chunk sequentially (same as container.chain), but see a
maximum block size of 8 samples instead of the host block size.

```
Input (512 samples) -> [chunk 0..7] -> children.process() ->
                       [chunk 8..15] -> children.process() ->
                       ...
                       [chunk 504..511] -> children.process() -> Output
```

MIDI events are timestamp-split across chunks. Each chunk only receives events
whose timestamps fall within that chunk's sample range.

When the incoming buffer has fewer than 8 samples, no chunking occurs -- the
buffer is forwarded directly to children as-is.

## Gap Answers

### sub-block-chunking-behaviour: Remainder handling for non-multiple-of-8 buffers

The last chunk CAN be smaller than 8 samples. `static_functions::fix_block<8>::process()`
(processors.h:151-172) uses `ChunkableProcessData` in a while loop:

```cpp
while (cpd)
{
    int numThisTime = jmin(BlockSize, cpd.getNumLeft());
    auto c = cpd.getChunk(numThisTime);
    pf(obj, &c.toData());
}
```

`jmin(BlockSize, cpd.getNumLeft())` ensures the last chunk gets only the
remaining samples. For example, a 100-sample buffer with BlockSize=8 produces
12 chunks of 8 and one final chunk of 4.

`ChunkableProcessData`'s destructor (snex_ProcessDataTypes.h:420-424) asserts
that ALL samples were consumed (`jassert(numSamples == 0)`), confirming no
zero-padding occurs -- children see the actual remainder size.

Children must handle variable block sizes gracefully. The PrepareSpecs blockSize
is a maximum guarantee, not an exact promise.

### midi-event-splitting: MIDI event distribution across chunks

Yes, `ChunkableProcessData` splits MIDI events by timestamp. The template
parameter `IncludeHiseEvents` is set to `true` in fix_block (processors.h:161):

```cpp
static constexpr bool IncludeHiseEvents = true;
ChunkableProcessData<ProcessDataType, IncludeHiseEvents> cpd(data);
```

The `ScopedChunk` constructor (snex_ProcessDataTypes.h:444-483) slices the event
buffer per chunk:
1. Iterates events to find those with timestamps in [numProcessed, numProcessed + chunkSize)
2. Creates a `dyn<HiseEvent>` slice pointing at matching events
3. Adjusts timestamps by subtracting `numProcessed` so they are chunk-relative
4. On destruction, restores original timestamps

This provides sub-block-accurate MIDI timing. A note-on at sample 5 in the
original buffer will appear at timestamp 5 in the first 8-sample chunk. A
note-on at sample 12 will appear at timestamp 4 in the second chunk.

### bypass-revert-behaviour: Bypass mechanism and child re-preparation

When bypassed, `FixedBlockNode` re-prepares children with the original block size.
The mechanism is a full re-prepare, not just skipping the chunking.

`FixedBlockNode::setBypassed()` (NodeContainerTypes.cpp:436-451):
```cpp
SerialNode::setBypassed(shouldBeBypassed);
PrepareSpecs ps;
ps.blockSize = originalBlockSize;
ps.sampleRate = originalSampleRate;
// ...
prepare(ps);
getRootNetwork()->runPostInitFunctions();
```

`FixedBlockNode::prepare()` (NodeContainerTypes.cpp:454-464):
```cpp
if (isBypassed())
    obj.getObject().prepare(ps);  // original block size
else
    obj.prepare(ps);              // fix_block prepare (sets blockSize=8)
```

And `FixedBlockNode::process()` (NodeContainerTypes.cpp:388-406):
```cpp
if (isBypassed())
    obj.getObject().process(d);   // direct passthrough (no chunking)
else
    obj.process(d);               // fix_block chunking
```

So when bypassed:
- Children are re-prepared with original block size
- Processing skips the fix_block wrapper entirely
- Children see the full host buffer

This enables A/B comparison: bypass toggle switches between fixed-8 and
host-buffer processing contexts.

### block-size-vs-incoming: Behavior when host block size < 8

Confirmed: when `numSamples < BlockSize`, the buffer is forwarded directly
without chunking (processors.h:155-156):

```cpp
if (numToDo < BlockSize)
    pf(obj, &data);
```

No buffering across callbacks occurs. Children see the smaller buffer as-is.
This also means children were prepared with `min(8, hostBlockSize)` via
`withBlockSizeT<8>(true)`, so they know the actual maximum.

### frame-mode-passthrough: Behavior when already in frame mode (blockSize==1)

Yes, fix_block preserves frame mode. `static_functions::fix_block<8>::prepare()`
(processors.h:145-149) uses:

```cpp
auto ps_ = ps.withBlockSizeT<BlockSize>(true);
```

`withBlockSizeT<8>(true)` (snex_Types.h:916-921) means "overwrite if frame":
```cpp
copy.blockSize = (overwriteIfFrame && copy.blockSize == 1) ? 1 : BlockSize;
```

When `blockSize == 1` (frame mode) and `overwriteIfFrame == true`, the block
size stays at 1. Children continue in frame mode and the fix_block chunking
is irrelevant (since `numSamples` will always be < BlockSize in frame context).

The interpreted `FixedBlockNode::getBlockSizeForChildNodes()` also handles this:
```cpp
return (isBypassed() || originalBlockSize == 1) ? originalBlockSize : FixedBlockSize;
```

## Parameters

None. FixedBlockNode has no parameters.

## Conditional Behaviour

Two conditions affect processing:

1. **Bypass state:** When bypassed, children process with original block size
   (full re-prepare). When active, children process with BlockSize chunks.

2. **Incoming buffer size < BlockSize:** No chunking occurs; buffer forwards
   directly. This is a runtime condition per process() call, not a mode switch.

3. **Frame mode (blockSize==1):** Fix_block becomes a no-op; frame mode is
   preserved through to children.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

The chunking overhead is minimal -- `ChunkableProcessData` does pointer arithmetic
and event timestamp adjustment per chunk. No buffer copies. The per-chunk function
call overhead is the main cost, which is proportional to (hostBlockSize / 8).

With BlockSize=8, a 512-sample host buffer produces 64 chunk iterations. This is
the highest iteration count of the fixN_block family, making fix8_block the most
expensive variant in terms of call overhead (but still negligible vs. actual DSP).

## Notes

- Template instantiations are explicit in NodeContainerTypes.cpp:479-484 for
  B = 8, 16, 32, 64, 128, 256. No fix512_block exists as a container node
  (512 is only available via fix_blockx and dynamic_blocksize).
- The `fix_block` compiled template (processors.h:1284-1286) inherits from
  `fix_blockx<T, static_functions::fix_block<BlockSize>>`, which is a generic
  wrapper that delegates prepare/process to a FixBlockClass member. This design
  allows both static (fix_block) and dynamic (fix_blockx with DynamicBlockProperty)
  variants to share the same wrapper infrastructure.
- The NodeProfiler reports the fixed block size (B) as the "effective samples"
  when active, and `d.getNumSamples()` when bypassed. This affects CPU meter
  display but not actual processing.
