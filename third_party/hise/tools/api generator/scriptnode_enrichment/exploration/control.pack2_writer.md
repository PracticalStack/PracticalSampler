# control.pack2_writer - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:755`
**Base class:** `data::base`, `pimpl::no_parameter`, `pimpl::no_processing`
**Classification:** utility

## Signal Path

Each ValueN parameter (N=1..NumValues) -> setParameter<P>(v) -> SliderPackData::setValue(P, v, sendNotificationAsync, false). This writes the value directly to the corresponding slider pack index. There is no output cable -- the node writes to ComplexData (SliderPack), not to parameter targets.

## Gap Answers

### write-mechanism: How packN_writer writes values into the SliderPack

In `setParameter<P>(double v)` (line 779): casts `this->externalData.obj` to `SliderPackData*`, acquires a `DataReadLock`, then calls `sp->setValue(P, v, sendNotificationAsync, false)`. The fourth argument `false` means the value is NOT normalized (it is written as-is). The `sendNotificationAsync` flag means UI update notifications are queued asynchronously, so the slider pack display updates but not synchronously on the audio thread.

### slider-pack-size-mismatch: What happens if pack has fewer entries than parameters

The `SliderPackData::setValue(index, value, ...)` method performs bounds checking internally. If the index exceeds the pack size, the write is silently dropped. So extra Value parameters beyond the pack size have no effect. Additionally, in `setExternalData()` (line 769), when a slider pack is connected, `sp->setNumSliders(NumValues)` is called, which automatically resizes the slider pack to match the number of Value parameters. So in normal usage, the pack is always the correct size.

### no-iscontrolnode-flag: Confirm no parameter_node_base inheritance

Confirmed. pack_writer inherits from `pimpl::no_parameter` (line 756), NOT from `pimpl::parameter_node_base`. It has no output parameter slot, no `getParameter()` method, and no modulation output. It writes to ComplexData via the `data::base` inheritance path. The `IsControlNode` property is NOT registered.

### value-range-handling: Are values clamped to 0..1?

The `setParameter<P>()` method passes the value directly to `sp->setValue()` without clamping. The parameter definitions in `createParameter<P>()` (line 788) specify range `{0.0, 1.0}`, but this range is enforced by the parameter system at the connection level, not within setParameter itself. If an unnormalised modulation source bypasses range conversion, values outside 0..1 could theoretically be written to the slider pack.

### concurrent-write-safety: Locking for slider pack writes

A `DataReadLock sl(this)` is acquired in `setParameter<P>()` (line 783). This is a read lock on the external data, which prevents the data from being swapped out during the write but does NOT prevent concurrent writes from multiple parameter callbacks. Since parameter callbacks are typically called from the same thread context, this is acceptable in practice, but there is no per-element write mutex.

## Parameters

Parameters are generated dynamically in `createParameters()` using the `ADD_PARAMETER(X)` macro pattern (line 797). For each index X from 0 to NumValues-1, a parameter named "Value{X+1}" is created with range {0.0, 1.0} and registered with callback index X. For pack2_writer: Value1 (index 0), Value2 (index 1).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The `setExternalData()` override auto-resizes the connected SliderPack to match NumValues via `sp->setNumSliders(NumValues)`. This means connecting a pack2_writer to a slider pack will resize it to 2 entries. The node ID is dynamically generated: `Identifier("pack" + String(NumValues) + "_writer")` (line 759), which is why there is no SN_NODE_ID macro.
