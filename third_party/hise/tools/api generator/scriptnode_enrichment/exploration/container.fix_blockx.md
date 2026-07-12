# container.fix_blockx -- C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:644` (FixedBlockXNode), `hi_dsp_library/node_api/nodes/processors.h:1249` (wrap::fix_blockx)
**Base class:** `SerialNode` (interpreted)
**Classification:** container

## Signal Path

Serial container with a property-selectable block size. Identical chunking
behaviour to fixN_block, but the block size is a string NodeProperty rather
than a compile-time template parameter. At runtime, a switch statement
dispatches to the correct `static_functions::fix_block<N>::process()` for each
valid block size.

```
Input -> DynamicBlockProperty selects N -> fix_block<N>::process() -> children -> Output
```

## Gap Answers

### property-dispatch-mechanism: How runtime dispatch works

`FixedBlockXNode` uses `wrap::fix_blockx<DynamicSerialProcessor, DynamicBlockProperty>`
(NodeContainerTypes.h:730). The `DynamicBlockProperty` inner class contains both
the prepare and process dispatch logic.

**Process dispatch** (NodeContainerTypes.h:686-698):
```cpp
switch (blockSize)
{
case 8:   wrap::static_functions::fix_block<8>::process(obj, pf, data); break;
case 16:  wrap::static_functions::fix_block<16>::process(obj, pf, data); break;
case 32:  wrap::static_functions::fix_block<32>::process(obj, pf, data); break;
case 64:  wrap::static_functions::fix_block<64>::process(obj, pf, data); break;
case 128: wrap::static_functions::fix_block<128>::process(obj, pf, data); break;
case 256: wrap::static_functions::fix_block<256>::process(obj, pf, data); break;
case 512: wrap::static_functions::fix_block<512>::process(obj, pf, data); break;
}
```

Each case instantiates a separate compiled code path for that block size.
The switch itself adds negligible overhead (one branch per process() call,
not per sample).

**Prepare dispatch** (NodeContainerTypes.h:679-684):
```cpp
void prepare(void* obj, prototypes::prepare f, const PrepareSpecs& ps)
{
    originalSpecs = ps;
    auto ps_ = ps.withBlockSize(blockSize, true);
    f(obj, &ps_);
}
```

Uses `withBlockSize(blockSize, true)` -- the dynamic (non-template) version.
Same frame-mode preservation as fixN_block.

### invalid-blocksize-handling: Invalid value handling

`DynamicBlockProperty::updateBlockSize()` (NodeContainerTypes.h:662-677):
```cpp
blockSize = newValue.toString().getIntValue();
if (blockSize > 7 && isPowerOfTwo(blockSize))
{
    // valid: re-prepare
}
else
    blockSize = 64;
```

Validation requires: value > 7 AND isPowerOfTwo(value). Invalid values silently
fall back to 64. This means:
- Values 1-7: invalid (reset to 64)
- Values like 3, 5, 6, 10: invalid (not power of two)
- Value 512: valid (> 7 and power of two)
- Value 1024+: valid per this check but has no switch case, so process() would
  fall through the switch without processing (silent bug for very large values)

The UI combobox (FixBlockXComponent, NodeContainerTypes.cpp:1470-1488) only
offers { "8", "16", "32", "64", "128", "256" } -- not 512. So 512 is technically
valid but not accessible through the UI.

### bypass-behaviour: Bypass mechanism

Identical to fixN_block. `FixedBlockXNode::setBypassed()` (NodeContainerTypes.cpp:1495-1511)
re-prepares with original block size:
```cpp
SerialNode::setBypassed(shouldBeBypassed);
prepare(ps);  // with originalBlockSize
```

`getBlockSizeForChildNodes()` (NodeContainerTypes.h:723-726):
```cpp
return (isBypassed() || originalBlockSize == 1) ? originalBlockSize : obj.fbClass.blockSize;
```

Same A/B comparison workflow as fixN_block.

### compilation-baking: C++ export behaviour

At C++ export time, the current BlockSize property value is read from the
ValueTree and used to emit a `wrap::fix_block<N, ...>` template instantiation,
eliminating the runtime switch. The exported code is identical to using the
corresponding fixN_block node directly.

This is visible in the DynamicBlockProperty design: it stores `originalSpecs`
separately, which is only needed for runtime re-preparation -- compiled code
never needs this.

## Parameters

None. Block size is a property, not a parameter.

## Properties

| Property | Type | Default | Valid Values |
|----------|------|---------|-------------|
| BlockSize | String | "64" | "8", "16", "32", "64", "128", "256", "512" |
| IsVertical | boolean | true | UI layout only |

The BlockSize property is hidden by default in the UI. The FixBlockXComponent
combobox provides access with values { "8", "16", "32", "64", "128", "256" }.

## Conditional Behaviour

Same as fixN_block (bypass, incoming < blockSize, frame mode), plus:

- **Property change:** When BlockSize property changes, acquires the network's
  connection lock (write lock) and re-prepares the entire node including children.
  This is a non-realtime operation.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

One additional switch branch per process() call vs fixN_block. After C++ export,
identical to fixN_block.

## Notes

- The key workflow advantage over fixN_block: evaluate different block sizes
  during development without replacing the node, then compile to eliminate
  the switch overhead.
- The property change triggers a full re-prepare through the network's connection
  lock, which may cause a brief audio interruption if changed during playback.
- 512 is accepted by the validation but not shown in the UI combobox.
