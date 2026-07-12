# SNEX JIT Overview for Scriptnode

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_snex/snex_library/snex_CallbackCollection.h/.cpp`
- `hi_snex/snex_library/snex_ExternalObjects.h/.cpp`
- `hi_snex/snex_library/snex_jit_ApiClasses.h`
- `hi_snex/snex_public/snex_jit_JitCompiledNode.h`
- `hi_snex/snex_components/snex_WorkbenchData.cpp`
- `hi_dsp_library/snex_basics/snex_Math.h`
- `hi_dsp_library/snex_basics/snex_Types.h`
- `hi_dsp_library/dsp_nodes/CoreNodes.h` (snex_shaper, snex_osc)
- `hi_dsp_library/dsp_nodes/CableNodes.h` (cable_expr)
- `hi_dsp_library/dsp_nodes/MathNodes.h` (expression_base/expr)
- `hi_scripting/scripting/scriptnode/snex_nodes/SnexNode.h/.cpp`
- `hi_scripting/scripting/scriptnode/snex_nodes/SnexShaper.cpp`
- `hi_scripting/scripting/scriptnode/snex_nodes/SnexOscillator.cpp`
- `hi_scripting/scripting/scriptnode/snex_nodes/SnexSource.h`
- Infrastructure reference: `scriptnode_enrichment/resources/infrastructure/core.md`

---

## 1. What is SNEX?

SNEX (Supercool Node Expression) is HISE's JIT-compiled C++ subset for writing
custom DSP code inside scriptnode. It provides:

- A restricted C++ syntax that compiles to native machine code at runtime
- Direct access to scriptnode's audio buffer types (ProcessData, span, block)
- Integration with scriptnode's parameter, external data, and MIDI systems
- The ability to export SNEX code as compilable C++ for final plugin builds

SNEX code is written inside the HISE IDE's code editor. When you create a SNEX
node, a `.h` file is created in the project's `DspNetworks/code_library/`
directory, organized by node type:

```
DspNetworks/
  code_library/
    snex_nodes/      -- snex_node code files
    snex_shaper/     -- snex_shaper code files
    snex_timer/      -- snex_timer code files
```

An accompanying `.xml` metadata file stores parameter definitions, ranges, and
ExternalData configuration.

SNEX uses either the asmjit backend (x86/x64 JIT compilation) or the MIR
backend (`SNEX_MIR_BACKEND` preprocessor flag) for code generation.

---

## 2. SNEX Node Types in Scriptnode

There are three primary SNEX node types in the `core` factory, plus two
expression-based nodes that use SNEX's JIT compiler internally.

### 2.1 core.snex_node -- Full Custom DSP Node

The most comprehensive SNEX node. Provides the complete callback set for
arbitrary DSP processing.

**C++ class:** `scriptnode::core::snex_node` (inherits `SnexSource`)
**Source:** `hi_scripting/scripting/scriptnode/snex_nodes/SnexNode.h`

Key properties:
- `isPolyphonic()` returns false (monophonic only)
- `isProcessingHiseEvent()` returns true (always receives MIDI)
- `isNormalisedModulation()` returns true (mod output 0..1)
- Supports optional `handleModulation()` callback for modulation output
- Supports optional `getPlotValue()` callback for filter display (requires
  deriving from `data::filter_node_base` and calling `SNEX_INIT_FILTER`)

#### Required Callbacks

These five callbacks must all be defined. The node will fail to compile if
any is missing (the `recompiledOk` method checks all are resolved):

| Callback | Signature | Purpose |
|----------|-----------|---------|
| `prepare` | `void prepare(PrepareSpecs ps)` | Initialize processing state |
| `reset` | `void reset()` | Reset state (voice start, unbypass) |
| `handleHiseEvent` | `void handleHiseEvent(HiseEvent& e)` | Process MIDI events |
| `process` | `template <typename T> void process(T& data)` | Block-based processing |
| `processFrame` | `template <int C> void processFrame(span<float, C>& data)` | Frame-based processing |

#### Optional Callbacks

| Callback | Signature | Purpose |
|----------|-----------|---------|
| `setExternalData` | `void setExternalData(const ExternalData& d, int index)` | Receive external data (tables, audio files, etc.) |
| `handleModulation` | `int handleModulation(double& value)` | Output modulation value. Return 1 if changed, 0 if not. |
| `getPlotValue` | `double getPlotValue(int getMagnitude, double freqNormalised)` | Filter frequency response display |

#### Parameters

Parameters are defined via template:
```cpp
template <int P> void setParameter(double v)
{
    // P is the parameter index (0, 1, 2, ...)
}
```

Parameter discovery uses naming convention: any method named `setXxx(double)`
is detected as a parameter named "Xxx" by the `ParameterHelpers` system.

#### Default Code Template

When creating a new snex_node, the IDE generates this template:

```cpp
template <int NV> struct MyNode
{
    SNEX_NODE(MyNode);

    void prepare(PrepareSpecs ps) { }
    void reset() { }
    template <typename ProcessDataType> void process(ProcessDataType& data) { }
    template <int C> void processFrame(span<float, C>& data) { }
    void handleHiseEvent(HiseEvent& e) { }
    void setExternalData(const ExternalData& d, int index) { }
    template <int P> void setParameter(double v) { }
};
```

The `SNEX_NODE(className)` macro expands to:
- `hmath Math;` member variable (access math functions via `Math.sin()` etc.)
- `SN_GET_SELF_AS_OBJECT(className)` (object access pattern)
- `SN_NODE_ID(className)` (static identifier)
- `SN_FORWARD_PARAMETER_TO_MEMBER(className)` (parameter forwarding)
- `SN_EMPTY_INITIALISE` (empty initialise callback)

### 2.2 core.snex_shaper -- Waveshaper Node

A specialized SNEX node for waveshaping algorithms. Simpler than snex_node
because it focuses on sample-level transformation with a display showing the
transfer function.

**C++ class:** `scriptnode::core::snex_shaper<ShaperType>` (template wrapper)
**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:593`

The `snex_shaper` template wraps a user-defined `ShaperType` class:
- Forwards `process()`, `processFrame()`, `prepare()`, `reset()` to the inner type
- Forwards `setExternalData()` only if the inner type implements it (via
  `prototypes::check::setExternalData`)
- Forwards `setParameter<P>()` to the inner type
- Does NOT handle MIDI events (`SN_EMPTY_HANDLE_EVENT`)
- Sets `TemplateArgumentIsPolyphonic` property
- Not polyphonic itself (`isPolyphonic()` returns false)

#### Required Callbacks for Shaper Code

| Callback | Signature | Purpose |
|----------|-----------|---------|
| `process` | `template <typename T> void process(T& data)` | Block-based processing |
| `processFrame` | `template <typename T> void processFrame(T& data)` | Frame-based processing |
| `prepare` | `void prepare(PrepareSpecs ps)` | Setup (validated on compile) |
| `reset` | `void reset()` | Reset state (validated on compile) |

The key simplification: the default template provides a `getSample(float input)`
method that `process` and `processFrame` forward to. For stateless waveshapers,
you only need to implement `getSample`:

```cpp
template <int NumVoices> struct MyShaper
{
    SNEX_NODE(MyShaper);

    float getSample(float input)
    {
        return input;  // implement waveshaping here
    }

    template <typename T> void process(T& data)
    {
        for(auto ch: data)
            for(auto& s: data.toChannelData(ch))
                s = getSample(s);
    }

    template <typename T> void processFrame(T& data)
    {
        for(auto& s: data)
            s = getSample(s);
    }

    void reset() { }
    void prepare(PrepareSpecs ps) { }
};
```

The waveshaper display evaluates the transfer function by passing 128 linearly
spaced samples from -1.0 to 1.0 through the process callback and plotting the
output.

### 2.3 core.snex_osc -- Oscillator Node

A specialized SNEX node for oscillator algorithms. Handles frequency tracking,
pitch multiplication, and MIDI note-on events automatically.

**C++ class:** `scriptnode::core::snex_osc<NV, T>` (wraps `snex_osc_base<T>`)
**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:2362`

Key architecture:
- `snex_osc_base<T>` holds the user's oscillator type and forwards optional
  `initialise`, `setExternalData`, and `prepare` calls
- `snex_osc<NV, T>` adds polyphonic support, frequency/pitch parameters, and
  the oscillator data management (uptime, delta)
- Inherits from `polyphonic_base` -- supports polyphonic voices
- `isProcessingHiseEvent()` returns true -- handles note-on for frequency
- Sets `TemplateArgumentIsPolyphonic` property

#### Built-in Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Frequency | 20.0 - 20000.0 Hz (skew center 1000) | 220.0 | Base oscillator frequency |
| PitchMultiplier | 1.0 - 16.0 (integer steps) | 1.0 | Pitch ratio multiplier |

Additional user-defined parameters (index >= 2) are forwarded to the inner
oscillator type's `setParameter<P-2>()`.

#### Required Callbacks for Oscillator Code

| Callback | Signature | Purpose |
|----------|-----------|---------|
| `tick` | `float tick(double uptime)` | Generate one sample at given phase |
| `process` | `void process(OscProcessData& d)` | Generate a block of samples |

The `prepare` callback is optional (forwarded via `prototypes::check`).

#### OscProcessData Structure

```cpp
struct OscProcessData
{
    dyn<float> data;    // mono output buffer to fill with samples
    double uptime;      // current phase accumulator
    double delta;       // phase increment per sample
};
```

- `operator++()` returns current uptime and advances by delta (post-increment)
- The `data` buffer should be filled additively (the wrapper adds osc output
  to existing signal: `data[0] += oscType.tick(uptime)`)

#### Default Code Template

```cpp
template <int NumVoices> struct MyOsc
{
    SNEX_NODE(MyOsc);

    float tick(double uptime)
    {
        return Math.fmod(uptime, 1.0);
    }

    void process(OscProcessData& d)
    {
        for (auto& s : d.data)
        {
            s = tick(d.uptime);
            d.uptime += d.delta;
        }
    }

    void prepare(PrepareSpecs ps) { }
};
```

#### How Frequency Tracking Works

The `snex_osc` wrapper manages the oscillator state:
1. `handleHiseEvent()` catches note-on events and calls `setFrequency()`
2. `setFrequency()` computes `cyclesPerSample = frequency / sampleRate`
   and stores it as `uptimeDelta` in the per-voice `OscData`
3. In `process()`, the wrapper sets up `OscProcessData` with the current
   `uptime` and `delta * multiplier`, then calls the user's `process()`
4. After the user's process returns, the wrapper advances uptime by
   `delta * numSamples`
5. In `processFrame()`, the wrapper calls `oscData.tick()` to get the current
   uptime and passes it to the user's `tick()` method

---

## 3. Expression Nodes Using SNEX JIT

Two node types use SNEX's JIT compiler for lightweight expression evaluation
rather than full node callbacks.

### 3.1 math.expr -- Audio-Rate Expression

**C++ class:** `scriptnode::math::expr<NV, ExpressionClass>`
**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:805`

This is an `OpNode` wrapping `expression_base<ExpressionClass>`. The expression
class must provide a static `op(float sample, float value)` method that
transforms each sample using the current Value parameter.

In the SNEX/JIT context, the `ExpressionClass` is a JIT-compiled class whose
`op` function contains the user's expression. The expression has access to two
variables:
- `input` (or the sample `s`) -- the current audio sample
- `value` -- the current Value parameter

Processing paths:
- Block: iterates channels, then samples, calling `ExpressionClass::op(s, value)`
- Frame: iterates samples in frame, calling `ExpressionClass::op(s, value)`

### 3.2 control.cable_expr -- Control-Rate Expression

**C++ class:** `scriptnode::control::cable_expr<ExpressionClass, ParameterClass>`
**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1066`

A control node (no audio processing) that transforms a control value using a
SNEX expression before forwarding it to a connected parameter.

Key traits:
- Inherits `pimpl::no_processing` -- registered as non-audio (`OutsideSignalPath`)
- Inherits `pimpl::no_mod_normalisation` -- the "Value" parameter is unscaled
- Has a single `setValue(double input)` method that:
  1. Calls `obj.op(input)` to transform the value
  2. Forwards the result to the connected parameter

The `ExpressionClass` must provide `double op(double input)`.

### 3.3 JitExpression -- The Underlying Evaluator

Both expression nodes ultimately use `snex::JitExpression` for compilation.

**Source:** `hi_snex/snex_library/snex_CallbackCollection.h:145`

`JitExpression` wraps a SNEX expression string into a compilable function:

For `cable_expr` (double precision):
```cpp
double get(double input){ return <EXPRESSION>; }
```

For `math.expr` with value input (float precision):
```cpp
float get(float input, float value){ return <EXPRESSION>; }
```

The expression can use:
- The `input` variable (incoming value)
- The `value` variable (current parameter, float variant only)
- `Math.sin()`, `Math.PI`, etc. (converted to `hmath::` internally)
- Ternary operator: `input > 0.5 ? 1.0 : 0.0`
- Arithmetic: `input * 2.0 + 1.0`
- Type casts: `(double)8`, `(float)x`

If the expression fails to compile, `JitExpression::getValue()` returns the
input value unchanged (passthrough). The `isValid()` and `getErrorMessage()`
methods allow checking compilation status.

For C++ export, `JitExpression::convertToValidCpp()` replaces `Math.` with
`hmath::` in the expression string.

---

## 4. Available Types in SNEX

### Primitive Types

| Type | Description |
|------|-------------|
| `float` | 32-bit floating point (primary audio type) |
| `double` | 64-bit floating point (parameters, phase accumulators) |
| `int` | 32-bit integer |

### Audio Buffer Types

| Type | Description |
|------|-------------|
| `block` | Alias for `dyn<float>` -- dynamic-size float buffer |
| `span<float, N>` | Fixed-size array of N floats (audio frame) |
| `dyn<float>` | Non-owning view into float data |
| `ProcessData<C>` | Fixed C-channel audio block with MIDI events |
| `ProcessDataDyn` | Dynamic-channel audio block |
| `OscProcessData` | Oscillator-specific: mono buffer + uptime + delta |

### Struct Types

| Type | Description |
|------|-------------|
| `PrepareSpecs` | sampleRate, blockSize, numChannels, voiceIndex |
| `HiseEvent` | MIDI event with extended metadata |
| `ExternalData` | Handle to table/slider pack/audio file/filter/display buffer |

### Index Types

Safe array access with defined out-of-bounds behavior:
- `index::wrapped<UpperLimit, isFloat>` -- wraps around
- `index::clamped<UpperLimit, isFloat>` -- clamps to bounds
- `index::unsafe<UpperLimit, isFloat>` -- no bounds checking (fastest)
- `index::lerp<IndexType>` -- linear interpolation between elements
- Parameterized: `index::unscaled<T, IndexType>` and `index::normalised<T, IndexType>`

### Special Types

| Type | Description |
|------|-------------|
| `sfloat` / `sdouble` | Smoothed value with automatic ramping |
| `PolyData<T, NV>` | Per-voice state container (see core.md section 5) |
| `ModValue` | Modulation output helper |
| `float4` | Alias for `span<float, 4>` -- SIMD-friendly |

---

## 5. Math Functions (hmath / Math)

SNEX provides a `Math` object (instance of `hmath` struct) with these functions.
All functions have both float and double overloads unless noted.

### Constants

| Name | Value |
|------|-------|
| `Math.PI` | 3.14159265358979... |
| `Math.E` | 2.71828182845904... |
| `Math.SQRT2` | 1.41421356237309... |

### Basic Math

| Function | Description |
|----------|-------------|
| `Math.abs(x)` | Absolute value |
| `Math.sign(x)` | Sign: +1.0 or -1.0 |
| `Math.min(a, b)` | Minimum of two values |
| `Math.max(a, b)` | Maximum of two values |
| `Math.range(v, lo, hi)` | Clamp value to [lo, hi] |
| `Math.round(x)` | Round to nearest integer |
| `Math.ceil(x)` | Round up |
| `Math.floor(x)` | Round down |
| `Math.fmod(x, y)` | Floating-point modulo |
| `Math.wrap(v, limit)` | Wrap value into [0, limit) |
| `Math.sqr(x)` | Square (x*x) |
| `Math.sqrt(x)` | Square root |
| `Math.pow(base, exp)` | Power |

### Trigonometric

| Function | Description |
|----------|-------------|
| `Math.sin(x)` | Sine |
| `Math.cos(x)` | Cosine |
| `Math.tan(x)` | Tangent |
| `Math.asin(x)` | Arc sine |
| `Math.acos(x)` | Arc cosine |
| `Math.atan(x)` | Arc tangent |

### Hyperbolic

| Function | Description |
|----------|-------------|
| `Math.sinh(x)` | Hyperbolic sine |
| `Math.cosh(x)` | Hyperbolic cosine |
| `Math.tanh(x)` | Hyperbolic tangent |
| `Math.atanh(x)` | Inverse hyperbolic tangent |

### Fast Approximations (JUCE FastMathApproximations)

| Function | Description |
|----------|-------------|
| `Math.fastsin(x)` | Fast sine approximation |
| `Math.fastcos(x)` | Fast cosine approximation |
| `Math.fasttan(x)` | Fast tangent approximation |
| `Math.fasttanh(x)` | Fast tanh approximation |
| `Math.fastsinh(x)` | Fast sinh approximation |
| `Math.fastcosh(x)` | Fast cosh approximation |
| `Math.fastexp(x)` | Fast exp approximation |

### Logarithmic / Exponential

| Function | Description |
|----------|-------------|
| `Math.log(x)` | Natural logarithm |
| `Math.log10(x)` | Base-10 logarithm |
| `Math.exp(x)` | e^x |
| `Math.db2gain(x)` | Decibels to linear gain |
| `Math.gain2db(x)` | Linear gain to decibels |

### Signal Conversion

| Function | Description |
|----------|-------------|
| `Math.sig2mod(v)` | Signal [-1,1] to mod [0,1]: `v * 0.5 + 0.5` |
| `Math.mod2sig(v)` | Mod [0,1] to signal [-1,1]: `v * 2.0 - 1.0` |
| `Math.norm(v, min, max)` | Normalize to 0..1 range |
| `Math.map(norm, start, end)` | Map 0..1 to [start, end] |
| `Math.smoothstep(v, lo, hi)` | Hermite smoothstep interpolation |

### Validation

| Function | Description |
|----------|-------------|
| `Math.isinf(x)` | Returns 1 if infinite |
| `Math.isnan(x)` | Returns 1 if NaN |
| `Math.sanitize(x)` | Replace NaN/inf with 0.0 |

### Random

| Function | Description |
|----------|-------------|
| `Math.random()` | Random float in [0, 1) |
| `Math.randomDouble()` | Random double in [0, 1) |
| `Math.randInt(lo, hi)` | Random integer in [lo, hi) |

### Block Operations (block/dyn<float> only)

| Function | Description |
|----------|-------------|
| `Math.peak(block)` | Peak absolute value in buffer |
| `Math.vmul(b1, b2)` | Element-wise multiply |
| `Math.vadd(b1, b2)` | Element-wise add |
| `Math.vsub(b1, b2)` | Element-wise subtract |
| `Math.vmov(b1, b2)` | Copy b2 into b1 |
| `Math.vmuls(b, s)` | Multiply by scalar |
| `Math.vadds(b, s)` | Add scalar |
| `Math.vmovs(b, s)` | Fill with scalar |
| `Math.vclip(b, lo, hi)` | Clip all values to range |
| `Math.vabs(b)` | Absolute value of all elements |
| `Math.min(b, s)` | Element-wise min with scalar |
| `Math.max(b, s)` | Element-wise max with scalar |
| `Math.range(b, lo, hi)` | Clip block to range |

---

## 6. Console Functions

SNEX provides a `Console` object for debugging (only active when debug mode
is enabled in the GlobalScope):

| Function | Description |
|----------|-------------|
| `Console.print(value, lineNumber)` | Print int/float/double/HiseEvent |
| `Console.dump()` | Dump all variables |
| `Console.clear()` | Clear console |
| `Console.stop(condition)` | Breakpoint (pauses execution if condition true) |

---

## 7. SNEX Integration with Scriptnode

### SnexSource Base Class

All SNEX nodes in scriptnode inherit from `SnexSource` which manages:
- Code compilation via `WorkbenchData`
- Parameter handler (`ParameterHandlerLight` / `ParameterHandler`)
- External data (tables, slider packs, audio files)
- Compile listeners (`SnexSourceListener`)
- Thread-safe callback execution via `SimpleReadWriteLock`

The `CallbackHandlerBase` inner class provides:
- `ScopedCallbackChecker` RAII guard for thread-safe callback invocation
- `getFunctionAsObjectCallback()` to resolve JIT-compiled member functions
- Read/write lock protection on all callback function pointers

### ScriptnodeCallbacks Enum

The `ScriptnodeCallbacks` struct defines the callback ID ordering:

```
PrepareFunction      = 0
ResetFunction        = 1
HandleEventFunction  = 2
ProcessFunction      = 3
ProcessFrameFunction = 4
OptionalOffset       = 5  (array length for required callbacks)
HandleModulation     = 6  (optional)
SetExternalData      = 7  (optional)
GetPlotValue         = 8  (optional)
numTotalFunctions    = 9
```

### Callback Signatures (from getPrototype)

| ID | Name | Return | Arguments |
|----|------|--------|-----------|
| 0 | `prepare` | void | `PrepareSpecs` (by value) |
| 1 | `reset` | void | (none) |
| 2 | `handleHiseEvent` | void | `HiseEvent&` (by reference) |
| 3 | `process` | void | `ProcessData<C>&` (by reference, C = channel count) |
| 4 | `processFrame` | void | `span<float, C>&` (by reference) |
| 6 | `handleModulation` | int | `double&` (by reference) |
| 7 | `setExternalData` | void | `const ExternalData&, int index` |
| 8 | `getPlotValue` | double | `int getMagnitude, double freqNormalised` |

### JitCompiledNode Runtime

`JitCompiledNode` (`hi_snex/snex_public/snex_jit_JitCompiledNode.h`) is the
runtime wrapper for a compiled SNEX class:
- Holds function pointers for all ScriptnodeCallbacks
- Supports up to `OpaqueNode::NumMaxParameters` parameters (16)
- Parameter functions are discovered by name convention (`setXxx`)
- `prepare()` calls both PrepareFunction and ResetFunction
- Implements `ExternalDataProviderBase` for external data routing

### CallbackCollection (Legacy Callbacks)

The older `CallbackCollection` system (used by some SNEX contexts outside
scriptnode nodes) supports three processing callbacks with automatic selection:

| Callback | Signature | Purpose |
|----------|-----------|---------|
| `processChannel` | `void processChannel(block b, int channel)` | Per-channel block |
| `processFrame` | `void processFrame(block b)` | Per-frame |
| `processSample` | `float processSample(float s)` | Per-sample |

Selection priority for **frame processing**: processFrame > processSample > processChannel
Selection priority for **block processing**: processChannel > processFrame > processSample

These are NOT used by the three main SNEX node types (snex_node, snex_shaper,
snex_osc), which use the ScriptnodeCallbacks system instead.

---

## 8. Limitations and Constraints

### Language Restrictions

SNEX is a C++ subset. Key limitations vs. full C++:
- No STL containers or algorithms
- No dynamic memory allocation (no new/delete, no std::vector)
- No virtual functions or runtime polymorphism
- No exceptions (no try/catch/throw)
- No standard library includes
- Limited template support (basic templates work, no SFINAE/concepts)
- No raw pointers for audio data (use span/dyn/block instead)
- No multithreading primitives

### Polyphony

- `snex_node` is monophonic only (`isPolyphonic()` returns false)
- `snex_osc` supports polyphony (via `polyphonic_base` and the NV template)
- `snex_shaper` is monophonic but the shaper type receives the NumVoices
  template parameter (`TemplateArgumentIsPolyphonic`)
- For polyphonic state, use `PolyData<T, NumVoices>` in the SNEX class

### C++ Export

SNEX code can be exported to C++ for compiled plugins:
- The `SNEX_NODE(className)` macro ensures C++ compatibility
- `snex_shaper` wraps the user type in `core::snex_shaper<UserType>`
- `snex_osc` wraps in `core::snex_osc<NV, UserType>`
- Expression nodes convert `Math.` calls to `hmath::` via
  `JitExpression::convertToValidCpp()`
- Nodes marked `UncompileableNode` cannot be exported

### Threading

- All SNEX callbacks execute on the audio thread
- No blocking operations (locks, I/O, allocation) are permitted
- `SimpleReadWriteLock` protects callback function pointers during recompilation
- The `ScopedCallbackChecker` RAII pattern ensures thread-safe invocation

### External Data Access

SNEX nodes can access external data through `setExternalData()`:
- Tables (512-float lookup tables)
- SliderPacks (resizable float arrays)
- AudioFiles (multichannel audio with metadata)
- FilterCoefficients (for filter display)
- DisplayBuffers (ring buffers for visualization)

The data is accessed through the `ExternalData` struct. See core.md section 7
for full details on the external data system.
