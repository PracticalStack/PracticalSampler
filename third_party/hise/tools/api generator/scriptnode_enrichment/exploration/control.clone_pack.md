# control.clone_pack - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1356`
**Base class:** `data::base`, `pimpl::no_processing`, `pimpl::parameter_node_base<ParameterType>`, `wrap::clone_manager::Listener`, `ComplexDataUIUpdaterBase::EventListener`
**Classification:** control_source

## Signal Path

SliderPack data * Value parameter -> per-clone output. For each clone i: `valueToSend = sliderData[i] * lastValue`. Sent via `callEachClone(i, valueToSend, false)`.

## Gap Answers

### slider-clone-mapping: Confirm slider-to-clone mapping formula

Confirmed from `setValue()` (line 1420): `auto numToIterate = jmin(sliderData.size(), numClones)`. Then for each i in [0, numToIterate): `auto valueToSend = sliderData[i] * lastValue; this->getParameter().callEachClone(i, valueToSend, false)`.

When slider pack has fewer entries than active clones: extra clones (beyond sliderData.size()) receive no update -- the loop bound is `jmin(sliderData.size(), numClones)`. When slider pack has more entries than clones: extra slider values are ignored -- the loop bound caps at numClones.

### content-change-optimization: Single-slider ContentChange optimization

Confirmed from `onComplexDataEvent()` (line 1388). When `EventType::ContentChange` fires with a specific changed index, clone_pack checks `isPositiveAndBelow(changedIndex, numClones)`, then reads the single slider value via `sp->getValue(changedIndex)`, multiplies by lastValue, and calls `callEachClone` for only that one clone index. So yes, editing a single slider only updates that specific clone, not all clones.

### normalisation-of-output: Confirm normalised output

clone_pack does NOT inherit from `no_mod_normalisation`. The `no_processing` base provides `isNormalisedModulation() = true`. Output values are normalised, meaning target parameter ranges are applied to the slider*value product. Since slider values are typically 0..1 and Value defaults to 1.0, the output is in [0,1].

### slider-pack-size-vs-clones: Does slider pack auto-resize?

No automatic resize. The slider pack size is set externally (by the user or another node like pack_resizer). clone_pack simply uses `jmin(sliderData.size(), numClones)` as the iteration bound. However, when `setNumClones()` is called (line 1437), only newly-added clones (from oldNumClones to newNumClones) receive updates, not all clones. This is an optimization: existing clones retain their previously-sent values.

## Parameters

- **NumClones** (P=0): Integer 1-16. Auto-synced from parent clone container.
- **Value** (P=1): 0.0-1.0, default 1.0. Global multiplier applied to all slider pack values before sending to clones.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: [{ "parameter": "NumClones", "impact": "linear", "note": "One multiply + callEachClone per active clone" }]

## Notes

clone_pack registers as both a `clone_manager::Listener` (for numClones auto-sync) and a `ComplexDataUIUpdaterBase::EventListener` (for slider change events). The `IsCloneCableNode` property is set in the constructor. Unlike clone_cable, clone_pack does not have a Mode property and does not process MIDI events.
