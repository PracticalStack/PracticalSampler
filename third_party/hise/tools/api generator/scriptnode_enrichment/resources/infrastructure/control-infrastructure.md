# Control Node Infrastructure Reference

Distilled from C++ source for the scriptnode node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/dsp_nodes/CableNodeBaseClasses.h`
- `hi_dsp_library/dsp_nodes/CableNodes.h`
- `hi_dsp_library/node_api/helpers/parameter.h`
- `hi_dsp_library/node_api/helpers/ParameterData.h`
- `hi_dsp_library/node_api/helpers/modulation.h`
- `hi_scripting/scripting/scriptnode/api/DynamicProperty.h`
- `hi_scripting/scripting/scriptnode/dynamic_elements/DynamicParameterList.h`
- `hi_scripting/scripting/scriptnode/api/NodeBase.h`

Also consulted: `resources/infrastructure/core.md` (for base parameter/ModValue context)

---

## 1. What Is a Control Node?

A control node is a scriptnode node that does NOT process audio. Instead, it
transforms a value and sends it to one or more target parameters via a cable
connection. Control nodes live inside the signal path tree but are marked with
`PropertyIds::IsControlNode` and `PropertyIds::OutsideSignalPath` so the audio
engine skips them during signal processing.

Key characteristics:
- Inherit from `pimpl::parameter_node_base<ParameterType>` (holds the output)
- Inherit from `pimpl::no_processing` (stubs out all audio callbacks)
- Have a `setValue(double)` method that receives input, transforms it, and calls
  `this->getParameter().call(outputValue)` to send the result
- Template parameter `ParameterType` determines how the output is connected

---

## 2. Cable Base Classes (pimpl namespace)

All base classes live in `scriptnode::control::pimpl`.

### parameter_node_base<ParameterType>

The fundamental base for any control node. Holds the output parameter connection.

```cpp
template <class ParameterType> struct parameter_node_base
{
    parameter_node_base(const Identifier& id)
    {
        cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::IsControlNode);
    }

    template <int I, class T> void connect(T& t)
    {
        this->p.template getParameter<0>().template connect<I>(t);
    }

    auto& getParameter() { return p; }

    ParameterType p;
};
```

- Registers `IsControlNode` in the constructor (tells C++ generator to skip
  `wrap::mod` wrapper)
- `connect<I>(target)` -- compile-time connection to target parameter slot I
- `getParameter()` -- access the output parameter for calling or checking
  `isConnected()`

### no_processing

Stubs out all audio/MIDI callbacks. Also marks the node as `OutsideSignalPath`.

```cpp
struct no_processing
{
    no_processing(const Identifier& id)
    {
        cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::OutsideSignalPath);
    }

    static constexpr bool isPolyphonic() { return false; }
    static constexpr bool isNormalisedModulation() { return true; }
    bool handleModulation(double& d) { return false; }
    // SN_EMPTY_PROCESS, SN_EMPTY_PROCESS_FRAME, SN_EMPTY_RESET, etc.
};
```

Note: `isNormalisedModulation()` defaults to `true` here. Nodes that send
unnormalised values must override this.

### no_mod_normalisation

Base class for nodes that send raw (unnormalised) output values.

```cpp
struct no_mod_normalisation
{
    static constexpr bool isNormalisedModulation() { return false; }

    no_mod_normalisation(const Identifier& nodeId, const StringArray& unscaledInputParameterIds)
    {
        cppgen::CustomNodeProperties::addNodeIdManually(nodeId, PropertyIds::UseUnnormalisedModulation);

        for (const auto& s : unscaledInputParameterIds)
            cppgen::CustomNodeProperties::addUnscaledParameter(nodeId, s);
    }
};
```

- Registers `UseUnnormalisedModulation` property
- Also marks specific INPUT parameters as unscaled (e.g. "Value", "Add") so
  the UI knows not to apply range conversion on those inputs
- The `unscaledInputParameterIds` argument specifies which of the node's OWN
  input parameters receive values without range scaling

### no_parameter

Marker base for nodes with no output parameter slot at all.

### templated_mode

Base for nodes that have a compile-time-swappable mode selected via a `Mode`
property combobox. The text of the combobox maps to template arguments.

```cpp
struct templated_mode
{
    templated_mode(const Identifier& nodeId, const juce::String& modeNamespace)
    {
        cppgen::CustomNodeProperties::addNodeIdManually(nodeId, PropertyIds::HasModeTemplateArgument);
        cppgen::CustomNodeProperties::setModeNamespace(nodeId, modeNamespace);
    }
};
```

Used by: `converter` (namespace "conversion_logic"), `xfader` (namespace "faders"),
`smoothed_parameter` (namespace "smoothers").

### combined_parameter_base<DataType>

Base for nodes that combine multiple input parameters into one output. Stores a
`NormalisableRange<double>` and provides `getUIData()` for the UI display.
Used by `multi_parameter`.

### duplicate_parameter_node_base<ParameterType>

Extended `parameter_node_base` for clone-aware nodes. Also inherits from
`wrap::clone_manager::Listener` to receive `numClonesChanged()` notifications.

---

## 3. Normalised vs Unnormalised Modulation

This is the most important distinction in the control node system. It determines
how values flow through cable connections.

### Normalised modulation (default, isNormalisedModulation() == true)

- The source node outputs values in the 0..1 range
- The connection system applies the target parameter's range to convert 0..1
  to the target's actual value range
- Example: A control node outputs 0.5, target has range [20, 20000, skew=0.3],
  the target receives the range-converted value

Used by: `normaliser`, `cable_table`, `cable_pack`, `locked_mod`, `pma`,
`bipolar`, `intensity`, `smoothed_parameter`, `xfader`

### Unnormalised modulation (isNormalisedModulation() == false)

- The source node outputs raw values in arbitrary range
- The connection system passes the value through without range conversion
- The C++ generator ignores the target parameter's range

Used by: `unscaler`, `cable_expr`, `locked_mod_unscaled`, `converter`,
`pma_unscaled`, `smoothed_parameter_unscaled`, `bang`, `minmax`,
`clone_forward`, `blend`

### How normalisation interacts with `no_mod_normalisation`

`no_mod_normalisation` serves two roles:
1. Marks the NODE's output as unnormalised (via `UseUnnormalisedModulation`)
2. Marks specific INPUT parameters as unscaled (via `addUnscaledParameter`)

The input parameter marking is critical. When a control node's Value input is
marked as unscaled, the system knows not to apply the source's range to the
incoming value before passing it to `setValue()`. This prevents double-scaling.

Example from `cable_expr`:
```cpp
no_mod_normalisation(getStaticId(), { "Value" })
// "Value" input receives raw values, output is also raw
```

Example from `minmax`:
```cpp
no_mod_normalisation(getStaticId(), {})
// No unscaled input params -- Value input IS scaled (0..1)
// But output is unnormalised (raw range values)
```

---

## 4. Parameter Template Types (Compile-Time)

These classes in `scriptnode::parameter` define how a parameter connection
behaves at compile time (in C++ compiled code). At runtime in the interpreter,
the equivalent is the `dynamic_base` hierarchy (Section 5).

### parameter::empty

No-op parameter. Used when a container has no macro parameters. `call()` does
nothing. Every container requires a parameter class, so this serves as the
"null" option.

### parameter::plain<T, P>

Direct value forwarding with no conversion. Calls
`T::setParameterStatic<P>(obj, v)` with the raw value.

### parameter::inverted<T, P>

Inverts the normalised value: calls `setParameterStatic<P>(obj, 1.0 - v)`.

### parameter::from0To1<T, P, RangeType>

Converts input from 0..1 to target range using `RangeType::from0To1(v)`.
This is the standard compile-time connection from a normalised source to a
target parameter. The RangeType is declared with `DECLARE_PARAMETER_RANGE_XXX`
macros.

### parameter::from0To1_inv<T, P, RangeType>

Same as `from0To1` but the range is inverted. The implementation currently
looks identical to `from0To1` (both call `RangeType::from0To1(v)`) -- the
inversion is presumably handled by the RangeType itself via its `inv` flag.

### parameter::to0To1<T, P, RangeType>

The reverse direction: converts input FROM a real range TO 0..1 using
`RangeType::to0To1(v)`. Used for macro parameters that receive a knob value
in a real range and need to normalise before distributing to connections.

### parameter::expression<T, P, Expression>

Applies an arbitrary mathematical expression to the value before forwarding.
The Expression class must have an `op(double)` method. Created with the
`DECLARE_PARAMETER_EXPRESSION` macro.

### parameter::bypass<T, RangeType>

Special parameter for bypass toggling. Always targets parameter index 9000
(a sentinel value). With default (Identity) range: `v < 0.5` means enabled.
With a custom range: checks if value falls within the range bounds.

### parameter::chain<InputRange, Others...>

A parameter that fans out to multiple targets. When called:
1. Converts input to 0..1 using `InputRange::to0To1(v)`
2. Forwards the normalised value to each parameter in `Others...`
3. Each downstream parameter applies its own range conversion

This is the compile-time equivalent of `dynamic_chain`. The `connect<Index>()`
method wires up each slot.

### parameter::list<Parameters...>

A collection of independent parameters addressable by index. Unlike `chain`
(which broadcasts one value to all), `list` allows calling each parameter
individually via `call<P>(v)`.

Used by multi-output nodes (xfader, xy, branch_cable) where each output
sends a different value.

### parameter::empty_list

Placeholder for nodes that expect a list parameter but have no connections.

---

## 5. Dynamic Parameter System (Runtime)

At runtime in the scriptnode interpreter, connections are not known at compile
time. The `dynamic_base` hierarchy provides runtime-polymorphic parameter
connections.

### parameter::dynamic (ParameterData.h)

The type-erased callback. Stores a `void*` object pointer and a `Function`
(`void(*)(void*, double)`) pointer. This is the runtime equivalent of the
compile-time parameter templates.

```cpp
struct dynamic
{
    using Function = void(*)(void*, double);
    void call(double v) const;   // invokes f(obj, v)
    void referTo(void* p, Function f_);
    void* obj = nullptr;
    Function f = nullptr;
};
```

### parameter::dynamic_base (DynamicProperty.h)

Runtime-polymorphic base for parameter connections. Reference-counted.

```cpp
struct dynamic_base : public ReferenceCountedObject
{
    virtual void call(double value);            // send value to target
    virtual double getDisplayValue() const;     // last sent value
    virtual InvertableParameterRange getRange() const;
    virtual void updateRange(const ValueTree& v);

    static dynamic_base::Ptr createFromConnectionTree(const ValueTree& v, ...);
};
```

Key method: `createFromConnectionTree()` -- factory that reads a ValueTree
connection definition and creates the appropriate dynamic_base subclass.

### parameter::dynamic_base_holder

Wraps a `dynamic_base::Ptr` with thread-safe swapping. When a connection is
changed at runtime, the holder atomically swaps the inner pointer using
`SimpleReadWriteLock`.

```cpp
struct dynamic_base_holder : public dynamic_base
{
    void call(double v) final override
    {
        setDisplayValue(v);
        SimpleReadWriteLock::ScopedReadLock sl(connectionLock);
        if (base != nullptr)
            base->call(v);
    }

    void setParameter(ObjectWithValueTree* n, dynamic_base::Ptr b);

    dynamic_base::Ptr base;
    SimpleReadWriteLock connectionLock;
};
```

Notable behavior in `setParameter()`: if the incoming `dynamic_base::Ptr` is
itself a `dynamic_base_holder` with `allowForwardToParameter == true`, the
system "bypasses" the intermediate holder and connects directly to its inner
target. This avoids unnecessary indirection in chained connections.

The `allowForwardToParameter` flag (default: true) controls this bypass
behavior. Set to false when a holder should NOT be transparently forwarded
through.

### parameter::dynamic_chain<ScaleInput>

Runtime equivalent of `parameter::chain`. Holds up to 32 target parameters
and broadcasts a value to all of them.

```cpp
template <bool ScaleInput> struct dynamic_chain : public dynamic_base
{
    static constexpr int NumMaxSlots = 32;

    void addParameter(dynamic_base::Ptr p, bool isUnscaled);

    void call(double v)
    {
        auto nv = ScaleInput ? getRange().convertTo0to1(v, true) : v;

        for (auto& t : targets)
        {
            auto isUnscaled = (double)unscaleValue[index];
            auto tv = ScaleInput ? t->getRange().convertFrom0to1(nv, true) : v;
            auto valueToSend = isUnscaled * v + (1.0 - isUnscaled) * tv;
            t->call(valueToSend);
        }
    }

    dynamic_base::List targets;
    bool unscaleValue[NumMaxSlots];
};
```

The `ScaleInput` template parameter controls whether the input value is
normalised to 0..1 before distribution:
- `dynamic_chain<true>` -- normalise input, then denormalise per-target
- `dynamic_chain<false>` -- pass through without conversion

The `isUnscaled` per-target flag handles mixed connections where some targets
want the raw value and some want range-converted values. The formula
`isUnscaled * v + (1.0 - isUnscaled) * tv` smoothly selects between raw (v)
and converted (tv) values using the boolean as a 0/1 multiplier.

---

## 6. Dynamic Parameter List (Runtime Multi-Output)

The `parameter::dynamic_list` class provides runtime multi-output parameter
management. It is the runtime equivalent of `parameter::list<...>`.

```cpp
struct dynamic_list
{
    NodePropertyT<int> numParameters;
    OwnedArray<MultiOutputSlot> targets;
    Array<double> lastValues;

    template <int P> void call(double v)
    {
        lastValues.set(P, v);
        targets[P]->p.call(v);
    }

    void callWithRuntimeIndex(int index, double v)
    {
        if (isPositiveAndBelow(index, getNumParameters()))
        {
            lastValues.set(index, v);
            targets[index]->p.call(v);
        }
    }

    int getNumParameters() const;
    void initialise(ObjectWithValueTree* n);
};
```

### MultiOutputSlot

Each output slot wraps a `dynamic_base_holder` and a `ConnectionSourceManager`.
The ConnectionSourceManager handles the ValueTree-based connection system:
adding/removing connections, listening for node deletion, and rebuilding
callbacks when the connection topology changes.

```cpp
struct MultiOutputSlot : public ConnectionSourceManager
{
    ValueTree switchTarget;
    NodeBase::Ptr parentNode;
    parameter::dynamic_base_holder p;
};
```

### How nodes use dynamic_list

Nodes like `xfader`, `xy`, and `branch_cable` use `dynamic_list` as their
ParameterType when running in the interpreter. They call:
- `call<P>(value)` for compile-time-known output indices
- `callWithRuntimeIndex(index, value)` for runtime-determined indices

The `numParameters` property is stored in the ValueTree and determines how
many output slots are available. Nodes can modify this at init time (e.g.
`xy` forces it to 2).

---

## 7. ConnectionSourceManager

Manages the runtime wiring between a parameter source and its targets.
Lives in NodeBase.h.

```cpp
struct ConnectionSourceManager
{
    ConnectionSourceManager(DspNetwork* n, ValueTree connectionsTree);

    var addTarget(NodeBase::Parameter* p);

    // Subclasses override this to rebuild the parameter callback
    virtual void rebuildCallback() = 0;

    struct CableRemoveListener { ... };  // auto-removes on node deletion
};
```

When connections change (added/removed in the ValueTree), `connectionChanged()`
fires, which calls `rebuildCallback()`. The subclass (e.g. `MultiOutputSlot`)
then reconstructs its `dynamic_base` chain from the connection tree.

`CableRemoveListener` listens for node deletion on both source and target
sides and automatically removes the connection ValueTree entry.

---

## 8. Control Node Architecture Patterns

### Pattern 1: Simple pass-through (normaliser, locked_mod)

Receives a value, sends it through unchanged. The range conversion happens
in the connection layer.

```cpp
void setValue(double input)
{
    if (this->getParameter().isConnected())
        this->getParameter().call(input);
}
```

### Pattern 2: Value transformation (cable_table, cable_pack, cable_expr)

Receives a value, transforms it (table lookup, expression eval, slider pack
lookup), sends the transformed value.

```cpp
void setValue(double input)
{
    lastValue = input;
    IndexType index(input);
    auto v = tableData[index];      // lookup
    if (this->getParameter().isConnected())
        this->getParameter().call(v);
}
```

### Pattern 3: Multi-input combination (pma, bipolar, minmax, intensity)

Multiple input parameters are combined into one output value. Uses the
`multi_parameter` template with a `multilogic::*` DataType class.

```cpp
// In multilogic::pma:
double getValue() const
{
    dirty = false;
    return jlimit(0.0, 1.0, value * mulValue + addValue);
}

// In multi_parameter::sendPending():
if (d.dirty && this->getParameter().isConnected())
    this->getParameter().call(d.getValue());
```

The `dirty` flag is set whenever any input parameter changes. `sendPending()`
checks the flag and sends the combined output. For polyphonic contexts, it only
sends when a voice is actively rendering (voiceIndex != -1).

### Pattern 4: Multi-output (xfader, xy, branch_cable)

One input value produces different values on multiple output slots.

```cpp
// xfader: distributes fade values across outputs
void setValue(double v)
{
    callFadeValue<0>(v);  // each output gets its fade coefficient
    callFadeValue<1>(v);
    // ...
}

// branch_cable: sends to one output selected by index
void setValue(double newValue)
{
    this->getParameter().callWithRuntimeIndex(index, newValue);
}
```

### Pattern 5: Event-driven (midi_cc, voice_bang)

Triggered by MIDI events rather than parameter changes.

```cpp
// voice_bang: fires on note-on
void handleHiseEvent(HiseEvent& e)
{
    if (e.isNoteOn())
        if (this->getParameter().isConnected())
            this->getParameter().call(value);
}
```

### Pattern 6: Smoothed output (smoothed_parameter)

Uses `wrap::mod` pattern instead of direct parameter output. Writes to a
ModValue on each process() call, which is consumed by handleModulation().

```cpp
template <typename ProcessDataType> void process(ProcessDataType& d)
{
    modValue.setModValueIfChanged(value.advance());
}

bool handleModulation(double& v)
{
    return modValue.getChangedValue(v);
}
```

This is the only control node pattern that runs during audio processing
(process/processFrame are not empty). It does NOT inherit from no_processing
for the audio callbacks, only for the signal path marking.

---

## 9. multi_parameter Template

The `multi_parameter` template is the primary vehicle for control nodes with
multiple input parameters. It combines a `multilogic::*` DataType class with
a ParameterType output.

```cpp
template <int NV, class ParameterType, typename DataType>
struct multi_parameter : public mothernode,
                         public polyphonic_base,
                         public pimpl::combined_parameter_base<DataType>,
                         public pimpl::parameter_node_base<ParameterType>,
                         public pimpl::no_processing
```

Type aliases (bottom of CableNodes.h):
```
control::pma          = multi_parameter<NV, PT, multilogic::pma>
control::pma_unscaled = multi_parameter<NV, PT, multilogic::pma_unscaled>
control::bipolar      = multi_parameter<NV, PT, multilogic::bipolar>
control::minmax       = multi_parameter<NV, PT, multilogic::minmax>
control::intensity    = multi_parameter<NV, PT, multilogic::intensity>
control::bang         = multi_parameter<NV, PT, multilogic::bang>
control::change       = multi_parameter<NV, PT, multilogic::change>
control::blend        = multi_parameter<NV, PT, multilogic::blend>
control::delay_cable  = multi_parameter<NV, PT, multilogic::delay_cable>
control::logic_op     = multi_parameter<NV, PT, multilogic::logic_op>
control::compare      = multi_parameter<NV, PT, multilogic::compare>
```

### multilogic DataType contract

Each multilogic class must provide:
- `static constexpr bool isNormalisedModulation()` -- output normalisation
- `static constexpr bool needsProcessing()` -- if true, process()/processFrame()
  are forwarded to the DataType (most return false)
- `double getValue() const` -- compute output from current state, clear dirty flag
- `template <int P> void setParameter(double v)` -- set input parameter P
- `bool operator==(const X&) const` -- equality comparison
- `static void createParameters(ParameterDataList&, NodeType&)` -- register params
- `mutable bool dirty` -- changed flag

### Polyphonic behavior

`multi_parameter` uses `PolyData<DataType, NV>` for per-voice state. In
`sendPending()`:
- Polyphonic mode: only sends if voiceIndex != -1 (active rendering)
- Monophonic mode: sends immediately when dirty

`setParameterStatic<P>()` iterates all voices via `for(auto& s : typed->data)`,
then calls `sendPending()` which reads only the current voice's data.

---

## 10. Modulation System Context

Control nodes interact with the HISE modulation system when used inside
modulation chains. The modulation system defines:

### TargetMode (how modulation is applied)

| Mode | Behavior |
|------|----------|
| `Gain` | HISE intensity-scale modulation (default) |
| `Unipolar` | Adds modulation to base value |
| `Bipolar` | Adds/subtracts bipolarly |
| `Pitch` | Allows values > 1.0, no clamping |
| `Raw` | No transformation (for extra_mod) |
| `Aux` | Intensity-only scaling |

### ParameterMode (per-parameter modulation config)

| Mode | Description |
|------|-------------|
| `Disabled` | No modulation |
| `ScaleAdd` | Scale + unipolar/bipolar add |
| `ScaleOnly` | Scale only (like HISE gain mod) |
| `AddOnly` | Unipolar/bipolar offset only |
| `Pan` | Bipolar scale around zero |
| `Pitch` | Auto-converts to pitch factor 0.5..2.0 |

### modulation::config classes

These configure how a modulation target node behaves:

- `config::dynamic` -- runtime-switchable mode and process flag
- `config::pitch_config` -- fixed pitch mode, unnormalised output
- `config::extra_config` -- fixed Raw mode, for extra modulation slots
- `config::constant<UseProcess, Mode>` -- fully compile-time fixed

All config classes except `pitch_config` return `isNormalisedModulation() == true`.
`pitch_config` returns false because pitch modulation values are not 0..1.

---

## 11. The isConnected() Guard Pattern

Nearly every control node's `setValue()` method checks
`this->getParameter().isConnected()` before calling. This is important because:

1. At init time, the parameter may not be connected yet
2. For `dynamic_base_holder`, `isConnected()` always returns true (to allow
   display value updates even without a connection)
3. For compile-time parameters, `isConnected()` checks if the void* target
   pointer is non-null

The guard prevents null-pointer calls on unconnected compile-time parameters.
For runtime parameters (dynamic_base_holder), the guard is technically
redundant but harmless.

---

## 12. Constructor Macros

### SN_PARAMETER_NOSIGNAL_CONSTRUCTOR(ClassId, ParameterId)

Expands to a constructor that initializes both `parameter_node_base` and
`no_processing` with the node's static ID:

```cpp
ClassId() :
    control::pimpl::parameter_node_base<ParameterId>(getStaticId()),
    control::pimpl::no_processing(getStaticId())
{};
```

Used by simple control nodes that need no additional constructor logic.

### SN_ADD_SET_VALUE(Class)

Shortcut for nodes with a single "Value" parameter. Defines the parameter
dispatch to call `setValue()`.

---

## 13. Key Behavioral Summary for Documentation

When documenting a control.* node, determine these properties:

1. **Normalised or unnormalised?** Check `isNormalisedModulation()`. This
   determines whether connected target ranges are applied.

2. **Which inputs are unscaled?** Check the `no_mod_normalisation` constructor
   for the `unscaledInputParameterIds` list.

3. **Single or multi output?** Check if ParameterType is a single parameter
   or a list/dynamic_list.

4. **Polyphonic?** Check if NV template parameter is used and if the node
   inherits from `polyphonic_base`.

5. **Needs audio processing?** Check `needsProcessing()` on multilogic types,
   or whether the node has non-empty process/processFrame methods.

6. **Has mode selector?** Check for `templated_mode` inheritance and the
   mode namespace string.

7. **Uses external data?** Check for `data::base` inheritance (cable_table,
   cable_pack use Tables/SliderPacks).

### Value flow summary

```
[Source parameter or knob]
    |
    v
[Control node input parameter (may be range-scaled or unscaled)]
    |
    v
[Control node setValue() / multilogic::setParameter<P>()]
    |  (transformation: lookup, expression, multiply+add, etc.)
    v
[Control node output: getParameter().call(value)]
    |
    v  (if normalised: target range applied; if unnormalised: raw passthrough)
    |
[Target parameter callback]
```
