# control.sliderbank - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:92`
**Base class:** `data::base`, `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`, `ComplexDataUIUpdaterBase::EventListener`
**Classification:** control_source

## Signal Path

Value parameter -> setValue() -> for each output slot i: output[i] = Value * sliderPack[i] -> sent via multi-output parameter list.

sliderbank multiplies the input Value by each slider pack element and sends the result to the corresponding output parameter slot. It supports up to 8 outputs (hardcoded unrolled loop).

## Gap Answers

### scaling-formula: How does the slider pack scale the input?

Confirmed: for each output index P, `callSlider<P>(v)` sends `v * b[P]` where `v` is the input Value and `b[P]` is the slider pack element at index P. The formula is simple multiplication: `output[i] = Value * sliderPack[i]`.

### num-parameters-sync: How does NumParameters interact with slider pack size?

For static list (compile-time): if `d.numSamples != ParameterClass::getNumParameters()`, the slider pack is resized to match the parameter count via `sp->setNumSliders()`. For dynamic list (runtime): `callSlider<P>()` checks `P < b.size() && P < this->getParameter().getNumParameters()` before calling. The NumParameters property is initialised via `this->p.initialise(n)` in `initialise()`.

### multi-output-pattern: Does sliderbank use dynamic_list?

Yes. The ParameterClass template is `dynamic_list` at runtime. The `initialise()` method calls `this->p.initialise(n)` which sets up the dynamic parameter list. Each output slot is a separate connection target. The `setValue()` method calls `callSlider<0>` through `callSlider<7>` (unrolled), and the slider pack content change listener (`onComplexDataEvent`) calls individual slider updates via the same mechanism.

## Parameters

- **Value**: Input value multiplied by each slider pack element. Range 0..1.

## Conditional Behaviour

When a slider pack element changes (via `onComplexDataEvent`), only the corresponding output is updated (not all outputs). The changed index is received as `(int)data` and dispatched via a switch statement (0-7).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

Maximum 8 outputs due to hardcoded unrolled loop. The node listens for `ComplexDataUIUpdaterBase::EventType::ContentChange` events on the slider pack to react to individual slider changes. The slider pack is referenced as a block `b` via `d.referBlockTo(b, 0)`.
