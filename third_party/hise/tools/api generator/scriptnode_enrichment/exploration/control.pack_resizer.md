# control.pack_resizer - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:817`
**Base class:** `data::base`, `pimpl::no_parameter`, `pimpl::no_processing`
**Classification:** utility

## Signal Path

NumSliders parameter -> setParameter<0>(v) -> SliderPackData::setNumSliders(newNumSliders). The node resizes the connected slider pack to a new size. There is no output cable -- the node modifies ComplexData (SliderPack) dimensions, not parameter values.

## Gap Answers

### resize-mechanism: How pack_resizer resizes the SliderPack

In `setParameter<0>(double v)` (line 831): casts `this->externalData.obj` to `SliderPackData*`, acquires a `DataWriteLock sl(this)`, computes `newNumSliders = jlimit<int>(1, 128, roundToInt(v))`, then calls `sp->setNumSliders(roundToInt(newNumSliders))`. Note: the value is clamped to [1, 128] in the setParameter method, overriding the parameter range minimum of 0. SliderPackData::setNumSliders preserves existing values when increasing size (new entries get default values) and truncates when decreasing.

### zero-default-behaviour: What does NumSliders=0 mean?

The parameter range is {0.0, 128.0, 1.0} (line 846), but the implementation clamps to `jlimit<int>(1, 128, ...)` (line 837). So a value of 0 is clamped to 1 -- the slider pack always has at least 1 entry. The default of 0.0 in the parameter definition means the slider pack starts at size 1 (after clamping) until the parameter is set to a different value.

### embedded-data-initial: What does the EmbeddedData represent?

The class has a member `float something = 90.0f` (line 854). This appears to be a vestigial/unused field. The embedded data in the base JSON likely represents the serialized initial state of the SliderPack data slot (1 entry at default value), not a custom node configuration.

### realtime-safety: Is the resize operation realtime-safe?

A `DataWriteLock sl(this)` is acquired (line 835), which is a write lock on the external data. SliderPackData::setNumSliders may involve memory allocation (resizing the internal array), which is NOT realtime-safe. However, since pack_resizer is marked OutsideSignalPath and parameter changes are typically triggered from the UI thread or non-realtime parameter updates, this is acceptable in practice. If the NumSliders parameter were modulated at audio rate, it could cause allocation on the audio thread.

## Parameters

- **NumSliders** (P=0): Range 0-128, integer steps. Effective range 1-128 due to jlimit clamping. Controls the size of the connected slider pack.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

pack_resizer inherits from `pimpl::no_parameter` (not parameter_node_base), confirming it has no modulation output. The `float something = 90.0f` member on line 854 is unused in any method -- it appears to be a vestigial field. Unlike packN_writer, pack_resizer does NOT auto-resize the slider pack on connect (no setNumSliders call in setExternalData, which it inherits from data::base without override).
