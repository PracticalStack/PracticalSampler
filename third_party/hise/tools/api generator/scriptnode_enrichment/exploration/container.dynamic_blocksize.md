# container.dynamic_blocksize -- C++ Exploration

**Source:** `hi_dsp_library/node_api/nodes/processors.h:1288` (wrap::dynamic_blocksize), `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:733` (DynamicBlockSizeNode)
**Base class:** `SerialNode` (interpreted)
**Classification:** container

## Signal Path

Serial container whose block size is controlled by a modulatable parameter.
Like fixN_block, splits audio into fixed-size chunks. Unlike fix_blockx where
block size is a design-time property, here it is a runtime parameter that can
be modulated by control nodes.

```
Input -> BlockSize parameter selects N -> fix_block<N>::process() or FrameConverters -> children -> Output
```

Special case: when BlockSize index = 0 (blockSize = 1), switches to frame-based
processing via FrameConverters instead of ChunkableProcessData.

## Gap Answers

### parameter-to-blocksize-mapping: Index-to-blocksize mapping and edge cases

The mapping is defined in `wrap::dynamic_blocksize::setBlockSize()` (processors.h:1368-1385):

```cpp
static const std::array<int, 8> BlockSizes = { 1, 8, 16, 32, 64, 128, 256, 512 };
auto idx = roundToInt(newBlockSize);
if(isPositiveAndBelow(idx, BlockSizes.size()))
{
    auto nb = BlockSizes[idx];
    if(nb != currentBlockSize)
    {
        currentBlockSize = nb;
        SimpleReadWriteLock::ScopedWriteLock sl(lock);
        this->prepare(lastSpecs);
    }
}
```

| Index | Block Size |
|-------|-----------|
| 0 | 1 (frame mode) |
| 1 | 8 |
| 2 | 16 |
| 3 | 32 |
| 4 | 64 (default) |
| 5 | 128 |
| 6 | 256 |
| 7 | 512 |

Fractional values are rounded via `roundToInt()`. Negative values or values >= 8
fail the `isPositiveAndBelow()` check and are silently ignored (no change).

The interpreted `DynamicBlockSizeNode::setBlockSize()` (NodeContainerTypes.cpp:1521-1534)
adds an extra layer: it calls `obj.setParameter<0>(s)` which reaches the
wrap::dynamic_blocksize::setBlockSize(), then separately checks if the block
size actually changed and re-prepares with the network connection lock.

The `createInternalParameterList()` (NodeContainerTypes.cpp:1588-1603) sets up the
parameter with `setParameterValueNames({ "1", "8", "16", "32", "64", "128", "256", "512" })`
and default value 4.0 (= index 4 = block size 64).

### thread-safety-mechanism: Lock behavior during block size changes

`wrap::dynamic_blocksize` uses `SimpleReadWriteLock` (processors.h:1405).

**On parameter change** (setBlockSize): acquires a **write lock** and calls
`prepare()` on children with the new block size. This blocks audio processing.

**During process()** (processors.h:1341): uses `ScopedTryReadLock`:
```cpp
if(auto sl = SimpleReadWriteLock::ScopedTryReadLock(lock))
{
    // process with current block size
}
```

If the try-read fails (write lock held during block size change), the entire
process() call is skipped -- output silence for that buffer. This means block
size changes can cause a single buffer of silence (a brief click/dropout),
but the audio thread never blocks.

The interpreted `DynamicBlockSizeNode::setBlockSize()` additionally acquires
the network's connection lock (a broader write lock) for the re-prepare.

### frame-mode-switch: BlockSize=1 (frame mode) behavior

When `currentBlockSize == 1`, `process()` (processors.h:1345-1354) switches
to FrameConverters instead of ChunkableProcessData:

```cpp
case 1:
{
    constexpr int C = ProcessDataType::getNumFixedChannels();
    if constexpr (ProcessDataType::hasCompileTimeSize())
        FrameConverters::processFix<C>(&obj, data);
    else
        FrameConverters::forwardToFrame16(&obj, data);
    break;
}
```

This is functionally identical to `wrap::frame_x<T>`. Children see blockSize=1
in their PrepareSpecs (set in `prepare()`: `ps.blockSize = jmin(ps.blockSize, currentBlockSize)`).

The transition happens via the same setBlockSize path: write lock, prepare with
blockSize=1, release lock. The next process() call uses the frame path.

### compilation-behaviour: Compiled output

The compiled `wrap::dynamic_blocksize<T>` retains the full switch statement
over all 8 block sizes (processors.h:1339-1366). It does NOT bake to a single
block size like fix_blockx does. The switch is present at runtime even in
compiled output because the parameter is modulatable.

This is slightly more overhead than fix_blockx compiled output (which becomes
a static fix_block<N>), but the switch is one branch per process() call -- negligible.

### hise-event-raster-interaction: BlockSize=1 vs HISE_EVENT_RASTER

BlockSize=1 (frame mode) IS supported despite being below HISE_EVENT_RASTER=8.
The event raster restriction only applies to intermediate values (2, 4) which
are excluded from the BlockSizes array entirely.

When blockSize=1, MIDI events are NOT split by ChunkableProcessData (frame mode
uses FrameConverters, not ChunkableProcessData). Events are handled normally via
handleHiseEvent() which is forwarded separately. Inside a frame container, child
nodes that need MIDI events receive them through the standard handleHiseEvent
callback before processing starts.

Block sizes 2 and 4 are excluded because they would interact poorly with the
HISE_EVENT_RASTER (default 8) used by modulation chains. At those sizes,
control-rate processing (which divides blockSize by 8) would produce 0-sample
blocks, causing division-by-zero or no-op processing.

## Parameters

| Parameter | Index | Range | Default | Display Values |
|-----------|-------|-------|---------|---------------|
| BlockSize | 0 | 0-7, step 1 | 4 (=64) | "1", "8", "16", "32", "64", "128", "256", "512" |

HasFixedParameters = true: the parameter list cannot be modified at runtime.

## Conditional Behaviour

1. **BlockSize=1:** Switches from ChunkableProcessData to FrameConverters
   (frame-based processing). Children see blockSize=1 in PrepareSpecs.
2. **BlockSize >= 8:** Uses ChunkableProcessData with the selected chunk size.
   Same chunking behaviour as fixN_block.
3. **Bypass:** `getBlockSizeForChildNodes()` returns originalBlockSize when
   bypassed. However, `setBypassed()` is empty (NodeContainerTypes.h:771) --
   no re-prepare occurs on bypass toggle. The bypass check is only in
   `getBlockSizeForChildNodes()`.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: [{"parameter": "BlockSize", "impact": "linear", "note": "Smaller block sizes = more chunk iterations per host buffer"}]

## Notes

- `wrap::dynamic_blocksize` declares `isPolyphonic() = false` (processors.h:1293).
  This container cannot be used in polyphonic contexts.
- The compiled template holds 7 `static_functions::fix_block<N>` members
  (processors.h:1395-1401) as b8, b16, b32, b64, b128, b256, b512. These are
  unused -- the BLOCKSIZE_CASE macro calls static_functions directly. The members
  appear to be vestigial from an earlier implementation.
- DynamicBlockSizeNode::prepare() (NodeContainerTypes.cpp:1563-1576) calls
  `obj.prepare(ps)` twice -- once conditionally (line 1570-1573) and once
  unconditionally (line 1575). This appears to be a minor redundancy bug.
