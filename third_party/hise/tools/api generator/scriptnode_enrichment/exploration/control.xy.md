# control.xy - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2747`
**Base class:** `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`
**Classification:** control_source

## Signal Path

X parameter -> output slot 0 (directly)
Y parameter -> output slot 1 (directly)

xy is a multi-output control node with 2 named outputs. Each input parameter maps directly to one output slot with no transformation.

## Gap Answers

### output-routing: Does X map directly to output 0 and Y to output 1?

Yes. `setX(double v)` calls `this->getParameter().template call<0>(v)` and `setY(double v)` calls `this->getParameter().template call<1>(v)`. Direct passthrough with no transformation. The outputs are registered as named outputs via `cppgen::CustomNodeProperties::addModOutput(getStaticId(), { "X", "Y" })`.

### y-range-difference: Why does Y have range [-1,1] while X has [0,1]?

From `createParameters()`: X has range `{ 0.0, 1.0 }` and Y has range `{ -1.0, 1.0 }`. The asymmetry is intentional -- Y uses a bipolar range suitable for panning, stereo width, or other bipolar controls. X uses a standard unipolar range. This matches common XY pad conventions where the horizontal axis is unipolar and the vertical axis is bipolar.

### ui-presentation: Does xy have a special UI?

The C++ class does not define any UI. However, the `addModOutput` call with named outputs `{ "X", "Y" }` and the node ID "xy" likely trigger a special 2D pad display in the scriptnode editor runtime (handled by the UI layer, not the DSP class).

## Parameters

- **X**: Horizontal axis value (0..1). Forwarded directly to output slot 0.
- **Y**: Vertical axis value (-1..1). Forwarded directly to output slot 1.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The `initialise()` method forces `numParameters` to 2 for dynamic parameter lists: `this->getParameter().numParameters.storeValue(2, n->getUndoManager())`. For static lists, it checks at compile time. The guard `getNumParameters() > 0` / `> 1` in setX/setY prevents calls before the parameter list is initialized.
