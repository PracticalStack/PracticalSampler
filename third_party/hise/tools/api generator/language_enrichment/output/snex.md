---
title: SNEX
description: "HISE's JIT-compiled C++ subset for writing custom DSP code inside scriptnode"

guidance:
  summary: >
    Language reference for SNEX (Scriptnode Expression Language), HISE's JIT-compiled
    C++ subset for writing custom DSP nodes. Covers the full snex_node callback
    interface (prepare, reset, process, processFrame, handleHiseEvent,
    setExternalData, handleModulation), available syntax (variables, types,
    control flow, structs, enums, auto, range-based for, operator overloading),
    audio buffer types (ProcessData, span, dyn/block), parameter registration,
    polyphonic voice state (PolyData), the SNEX preprocessor, debugging tools
    (Console.print, Console.stop, live variable watch), and a categorised
    differences-from-C++ table. Includes a complete worked example (Saturated
    Sine) with the same DSP implemented in C++ and Faust for comparison.
  concepts:
    - SNEX
    - JIT
    - scriptnode
    - snex_node
    - process
    - processFrame
    - PrepareSpecs
    - ProcessData
    - span
    - dyn
    - block
    - FrameProcessor
    - parameters
    - PolyData
    - polyphonic
    - ExternalData
    - sfloat
    - sdouble
    - ModValue
    - Math
    - hmath
    - Console
    - debugging
    - preprocessor
  prerequisites:
    - scriptnode
  complexity: intermediate
---

SNEX (Scriptnode Expression Language) is HISE's JIT-compiled C++ subset for writing custom DSP code inside scriptnode. It compiles to native machine code at runtime, giving you near-C++ performance with instant iteration - no external IDE, no DLL compilation, no restart.

The main reasons to choose SNEX:

- **Instant compilation** - code compiles in the HISE editor and runs immediately. No build step, no waiting
- **100% subset of the C++ node API** - SNEX is valid C++ (minus the `Console` debug calls). Porting to $LANG.cpp-dsp-nodes$ is a direct copy-paste, not a rewrite
- **Built-in debugging** - `Console.print()`, breakpoints, and live variable watch directly in the HISE IDE

The tradeoffs vs C++ DSP nodes:

- No access to existing scriptnode building blocks (you hand-roll all DSP)
- No JUCE framework access
- No hardcoded module loading (SNEX nodes must live inside a scriptnode network)
- A restricted C++ subset (no STL, no `switch`, no virtual functions, limited templates)

> [!Tip:Graduate to C++ when needed] When your SNEX node outgrows the language subset - you need existing nodes, JUCE utilities, or hardcoded module loading - porting to $LANG.cpp-dsp-nodes$ is straightforward because the callback API is the same.


## Example: Saturated Sine

This complete `snex_node` implements a polyphonic synth voice: sine oscillator with a hand-rolled phase accumulator, tanh soft-clip saturation, and a one-pole lowpass filter. The same DSP is implemented in $LANG.cpp-dsp-nodes$ and $LANG.faust$ so you can compare approaches across languages.

The following sections unpack every part of this code in detail.

```cpp
template <int NV> struct my_synth_voice
{
    SNEX_NODE(my_synth_voice);

    // All per-voice state lives in a single struct wrapped in PolyData.
    // This ensures every voice has independent state — without the wrapper,
    // variables like phaseDelta would be shared and a new note would
    // change the pitch of every active voice.
    struct VoiceState
    {
        double phase = 0.0;
        double phaseDelta = 0.0;
        sfloat drive;
        sfloat cutoffCoeff;
        float filterState = 0.0f;
        double cutoffHz = 1000.0;

        // Any parameter conversion that depends on sample rate needs a
        // dedicated update function — otherwise the coefficient goes stale
        // when the sample rate changes (a common source of subtle glitches).
        void updateFilter(double sampleRate)
        {
            auto fc = cutoffHz / sampleRate;
            cutoffCoeff.set((float)(Math.PI * 2.0 * fc));
        }

        void prepare(double sampleRate)
        {
            drive.prepare(sampleRate, 20.0);
            cutoffCoeff.prepare(sampleRate, 20.0);
            updateFilter(sampleRate);
        }

        void reset()
        {
            drive.reset();
            cutoffCoeff.reset();
            phase = 0.0;
            filterState = 0.0f;
        }
    };

    PolyData<VoiceState, NV> voiceState;
    double sr = 44100.0;

    void prepare(PrepareSpecs ps)
    {
        sr = ps.sampleRate;

        for (auto& vs : voiceState)
        {
            vs.prepare(ps.sampleRate);
            vs.reset();
        }
    }

    void reset()
    {
        for (auto& vs : voiceState)
            vs.reset();
    }

    template <typename ProcessDataType> void process(ProcessDataType& data)
    {
        auto fd = data.toFrameData();

        while (fd.next())
            processFrame(fd.toSpan());
    }

    template <int C> void processFrame(span<float, C>& data)
    {
        auto& vs = voiceState.get();

        // 1. Sine oscillator
        auto sample = Math.sin(vs.phase * Math.PI * 2.0);
        vs.phase += vs.phaseDelta;

        // 2. Tanh saturation
        auto d = vs.drive.advance();
        sample = Math.tanh(sample * d);

        // 3. One-pole lowpass filter
        auto coeff = vs.cutoffCoeff.advance();
        vs.filterState = vs.filterState + coeff * ((float)sample - vs.filterState);

        for (auto& s : data)
            s = vs.filterState;
    }

    void handleHiseEvent(HiseEvent& e)
    {
        if (e.isNoteOn())
        {
            auto& vs = voiceState.get();
            vs.phaseDelta = e.getFrequency() / sr;
        }
    }

    void setExternalData(const ExternalData& d, int index) {}

    template <int P> void setParameter(double v)
    {
        if (P == 0)
        {
            for (auto& vs : voiceState)
                vs.drive.set((float)v);
        }
        if (P == 1)
        {
            for (auto& vs : voiceState)
            {
                vs.cutoffHz = v;
                vs.updateFilter(sr);
            }
        }
    }
};
```

Compared to the $LANG.cpp-dsp-nodes$ version of this example, the SNEX code is more concise (no `#pragma once`, no `#include`, no namespace block, no `MetadataClass`, no `NumTables` constants) but all DSP is hand-rolled - there are no `core::oscillator` or `filters::svf` building blocks to reuse. The nested `VoiceState` struct pattern shown here is the recommended way to manage polyphonic state: grouping all per-voice data into a single `PolyData` member keeps the code organised and prevents accidental state sharing between voices.


## The Language

SNEX supports a subset of C++17 syntax. This section covers what is available.

### Variables and types

```cpp
int count = 0;
float gain = 0.5f;
double phase = 0.0;
bool active = true;
```

The `auto` keyword works for type inference:

```cpp
auto sample = Math.sin(phase);  // deduced as double
```

> [!Warning:No strings or text types] SNEX operates on numeric data only. There is no string type, no character type, and no text manipulation. Text output for debugging uses `Console.print()`, which accepts numeric types and `HiseEvent` directly.

SNEX provides two built-in container types instead of STL containers or dynamic arrays:

```cpp
span<float, 512> table;    // fixed-size array of 512 floats
dyn<float> buffer;          // non-owning view into a float buffer (alias: block)
```

`span<T, N>` is a compile-time-sized array. `dyn<T>` (also called `block` when `T` is `float`) is a runtime-sized view into existing memory. Both support element-wise arithmetic, range-based for loops, and subscript access. See [Types](#types) for the full reference.

### Control flow

```cpp
if (value > 0.5)
    output = 1.0f;
else
    output = 0.0f;

for (int i = 0; i < 10; i++)
    buffer[i] = 0.0f;

while (phase > 1.0)
    phase -= 1.0;
```

Range-based for loops work with `span`, `dyn`, and `PolyData`:

```cpp
for (auto& s : data)
    s *= gain;
```

There is no `switch` statement. Use `if`/`else if` chains instead. This is intentional - complex branching is primarily used in parameter callbacks, where `if`/`else if` chains can be resolved more efficiently at compile time.

### Enums

Enums define named integer constants. The most common use case in SNEX is naming parameter indices so that `setParameter` reads clearly instead of using raw numbers:

```cpp
enum Parameters
{
    Drive = 0,
    Cutoff
};

template <int P> void setParameter(double v)
{
    if (P == Parameters::Drive)
    {
        for (auto& vs : voiceState)
            vs.drive.set((float)v);
    }
    if (P == Parameters::Cutoff)
    {
        for (auto& vs : voiceState)
        {
            vs.cutoffHz = v;
            vs.updateFilter(sr);
        }
    }
}
```

Enums also work well for oscillator modes, filter types, or any fixed set of options:

```cpp
enum Mode
{
    Sine = 0,
    Saw,
    Square
};
```

### Type aliases

```cpp
using FilterState = span<float, 2>;
```

> [!Tip:Nest type aliases for complex templates] Chain type aliases to make deeply nested template definitions readable:
> ```cpp
> using StereoFrame = span<float, 2>;
> using StereoBuffer = dyn<StereoFrame>;
> using MultiStereoBuffer = span<StereoBuffer, 6>;
> ```

### Structs

Every SNEX node is defined as a `template <int NV> struct`, where `NV` is the number of polyphonic voices. The compiler instantiates your node with `NV == 1` for monophonic use or `NV == NUM_POLYPHONIC_VOICES` (typically 256) for polyphonic containers. This gives a zero-overhead solution for creating monophonic and polyphonic variants of a node at compile time — when `NV == 1`, all voice management code is eliminated entirely.

Inside this outer template struct, nested structs are the primary way to organise related state and behaviour. The most important use case is grouping all per-voice data into a single struct that can be wrapped in `PolyData<T, NV>` — as demonstrated in the [Saturated Sine example](#example-saturated-sine) above. Give each struct its own `prepare()` and `reset()` methods so the outer node can delegate cleanly:

```cpp
struct FilterData
{
    float state = 0.0f;
    float coeff = 0.0f;

    void reset() { state = 0.0f; }

    float process(float input)
    {
        state += coeff * (input - state);
        return state;
    }
};
```

> [!Tip:Structs inside PolyData] Wrapping a struct in `PolyData<MyStruct, NV>` gives every voice its own independent copy of the entire struct. This is the recommended pattern for polyphonic nodes — see the `VoiceState` struct in the example at the top.

### Operator overloading

User-defined structs can overload arithmetic and comparison operators:

```cpp
struct MyVec
{
    float x = 0.0f;
    float y = 0.0f;

    MyVec operator+(const MyVec& other)
    {
        MyVec r;
        r.x = x + other.x;
        r.y = y + other.y;
        return r;
    }
};
```

### Visibility modifiers

`public` and `private` visibility modifiers are supported in structs, following C++ semantics.

### Bitwise operators

The full set of bitwise operators is available: `&`, `|`, `^`, `~`, `<<`, `>>`.

### Static constants

```cpp
static const int NumChannels = 2;
static const float DefaultGain = 0.5f;
```

> [!Tip:Use static constants in template definitions] Static constants can parameterise type aliases and spans, keeping magic numbers in one place:
> ```cpp
> static const int NumChannels = 7;
> using Frame = span<float, NumChannels>;
> ```

### Preprocessor

SNEX includes a limited preprocessor for compile-time configuration. The primary use case is toggling DSP stages during development — switching between a cheap placeholder and the real algorithm without deleting code:

```cpp
#define USE_SATURATION 1
#define TABLE_SIZE 512

#if USE_SATURATION
    s = Math.tanh(s * drive);
#else
    s = Math.range(s * drive, -1.0f, 1.0f);
#endif
```

Supported directives:

| Directive | Purpose |
| --- | --- |
| `#define NAME value` | Define a compile-time constant |
| `#if` / `#else` / `#endif` | Conditional compilation |
| `#include "filename.h"` | Include another SNEX file (for splitting code across files) |

The preprocessor translates directly to C++ preprocessor directives when SNEX code is exported for compiled plugin builds. This makes it useful for maintaining a single codebase that works in both JIT and compiled modes — a `#define` toggle can enable debug logging in the JIT environment and disable it for the export build.


## Types

### Primitive types

| Type | Size | Use |
| --- | --- | --- |
| `int` | 32-bit | Counters, indices, flags |
| `float` | 32-bit | Audio samples, UI parameters |
| `double` | 64-bit | Phase accumulators, high-precision parameters |
| `bool` | 8-bit | Flags |

### Audio buffer types

| Type | Description |
| --- | --- |
| `span<float, N>` | Fixed-size array of N floats. Used as an audio frame (one sample per channel) in `processFrame` |
| `dyn<float>` / `block` | Non-owning view into a contiguous float buffer. A single channel's sample data |
| `ProcessData<C>` | Fixed C-channel audio block with MIDI event access |
| `ProcessDataDyn` | Dynamic-channel variant (used by the runtime dispatch layer) |

`span` supports element-wise arithmetic (+, -, *, /), range-based for loops, and subscript access. `dyn<float>` (aliased as `block`) provides the same arithmetic operations accelerated by SIMD.

### Special types

| Type | Description |
| --- | --- |
| `sfloat` / `sdouble` | Smoothed value with automatic per-sample ramping. Prevents zipper noise on parameter changes |
| `PolyData<T, NV>` | Per-voice state container. Stores one `T` per voice, automatically selects the correct voice |
| `ModValue` | Modulation output helper for scriptnode modulation connections |
| `PrepareSpecs` | Processing context: `sampleRate`, `blockSize`, `numChannels`, `voiceIndex` |
| `HiseEvent` | Extended MIDI event with voice index, timestamp, and frequency helpers |
| `ExternalData` | Handle to tables, slider packs, audio files, filter coefficients, or display buffers |
| `OscProcessData` | Mono buffer + phase uptime + phase delta (used by the `snex_osc` specialised node) |

### Index types

Safe array access with defined out-of-bounds behaviour:

| Type | Behaviour |
| --- | --- |
| `index::wrapped<N, isFloat>` | Wraps around (modulo) |
| `index::clamped<N, isFloat>` | Clamps to bounds |
| `index::unsafe<N, isFloat>` | No bounds check (fastest) |
| `index::lerp<IndexType>` | Linear interpolation between adjacent elements |

Use index types with `span` subscript access for safe, optimised array lookups:

```cpp
span<float, 512> table;
index::wrapped<512, true> idx;

idx = normalised_position;      // 0.0 to 1.0 maps to 0..511
auto value = table[idx];        // safe wrapped access
```

### The Math object

The `SNEX_NODE` macro provides a `Math` member (instance of `hmath`) with mathematical functions. Key categories:

**Constants:** `Math.PI`, `Math.E`, `Math.SQRT2`

**Basic math:** `Math.abs()`, `Math.min()`, `Math.max()`, `Math.range()`, `Math.round()`, `Math.fmod()`, `Math.wrap()`, `Math.sqrt()`, `Math.pow()`, `Math.sqr()`

**Trigonometric:** `Math.sin()`, `Math.cos()`, `Math.tan()`, `Math.asin()`, `Math.acos()`, `Math.atan()`

**Hyperbolic:** `Math.tanh()`, `Math.sinh()`, `Math.cosh()`, `Math.atanh()`

**Fast approximations:** `Math.fastsin()`, `Math.fastcos()`, `Math.fasttan()`, `Math.fasttanh()`, `Math.fastexp()`

**Conversion:** `Math.db2gain()`, `Math.gain2db()`, `Math.sig2mod()`, `Math.mod2sig()`, `Math.norm()`, `Math.map()`, `Math.smoothstep()`

**Validation:** `Math.isnan()`, `Math.isinf()`, `Math.sanitize()`

**Random:** `Math.random()`, `Math.randomDouble()`, `Math.randInt(lo, hi)`

**Block operations** (on `block`/`dyn<float>` only): `Math.peak()`, `Math.vmul()`, `Math.vadd()`, `Math.vsub()`, `Math.vmov()`, `Math.vmuls()`, `Math.vadds()`, `Math.vmovs()`, `Math.vclip()`, `Math.vabs()`

All scalar functions have both `float` and `double` overloads.


## Usage in HISE

SNEX source files live in `DspNetworks/CodeLibrary/` inside your HISE project, organised by node type:

```
DspNetworks/
  CodeLibrary/
    snex_node/
      my_synth_voice.h
    snex_osc/
      my_oscillator.h
    snex_shaper/
      my_shaper.h
```

To create a new SNEX node, add a `snex_node` to your scriptnode network. HISE generates a `.h` file with the complete template, pre-filled with empty callbacks. Edit the code in the built-in HISE code editor - compilation happens automatically when you save.

SNEX code can be exported to C++ for compiled plugins. The `SNEX_NODE(className)` macro ensures the code is compatible with both the JIT compiler and the C++ build system.

### Callbacks

The `snex_node` generated template includes these callbacks. They are identical to the $LANG.cpp-dsp-nodes$ callback interface.

#### prepare

```cpp
void prepare(PrepareSpecs ps)
```

Called when the processing context changes. Initialise smoothing ramps, store the sample rate, and prepare any state that depends on the processing specs.

The runtime calls `reset()` immediately after `prepare()`.

#### reset

```cpp
void reset()
```

Called at voice start (polyphonic) or when the node is un-bypassed. Clear all internal state to start from silence.

#### process

```cpp
template <typename ProcessDataType> void process(ProcessDataType& data)
```

Block-based processing. The canonical pattern forwards to `processFrame` via the `FrameProcessor`:

```cpp
template <typename ProcessDataType> void process(ProcessDataType& data)
{
    auto fd = data.toFrameData();

    while (fd.next())
        processFrame(fd.toSpan());
}
```

#### processFrame

```cpp
template <int C> void processFrame(span<float, C>& data)
```

Per-sample processing. `C` is the channel count, `data` holds one sample per channel. This is where most DSP logic lives.

Note the SNEX-specific signature: `template <int C>` with `span<float, C>&`, compared to C++ DSP nodes which use `template <typename T>` with `T&`. The SNEX JIT compiler resolves the channel count at compile time.

> [!Warning:Forward declaration order in templates] In `template <int NV>` structs, methods must be defined before they are called by other methods. If `prepare()` calls `updateFilter()`, define `updateFilter()` first. This constraint does not apply to non-templated classes.

#### handleHiseEvent

```cpp
void handleHiseEvent(HiseEvent& e)
```

Called for each MIDI event. Use `e.isNoteOn()`, `e.isNoteOff()`, `e.getFrequency()`, `e.getVelocity()`, etc. to extract event data.

In the example, `handleHiseEvent` converts note-on events to phase increments. Because the voice context is active during event handling, `voiceState.get()` returns the correct voice:

```cpp
void handleHiseEvent(HiseEvent& e)
{
    if (e.isNoteOn())
    {
        auto& vs = voiceState.get();
        vs.phaseDelta = e.getFrequency() / sr;
    }
}
```

#### setExternalData

```cpp
void setExternalData(const ExternalData& d, int index)
```

Called when external data (Tables, SliderPacks, AudioFiles, etc.) is assigned. Leave empty if your node does not use external data.

#### handleModulation (optional)

```cpp
int handleModulation(double& value)
```

Not in the default template, but can be added. Makes your node a modulation source with a draggable output in the scriptnode UI. Write the modulation value into the reference parameter and return `1` if changed, `0` otherwise.

Use `ModValue` to manage the change detection:

```cpp
ModValue modValue;

int handleModulation(double& value)
{
    return modValue.getChangedValue(value) ? 1 : 0;
}
```


### Parameters

#### Receiving parameter changes

Parameters are dispatched via the `setParameter` template:

```cpp
template <int P> void setParameter(double v)
{
    if (P == 0) drive.set((float)v);
    if (P == 1)
    {
        auto fc = v / sr;
        cutoffCoeff.set((float)(Math.PI * 2.0 * fc));
    }
}
```

Each `if` block handles one parameter index. The `if (P == N)` pattern (rather than `if constexpr`) is the standard SNEX idiom.

> [!Warning:Recalculate sample-rate-dependent coefficients] If a parameter conversion depends on the sample rate (e.g. frequency to filter coefficient), store the raw value and extract the conversion into a function called from both `setParameter` and `prepare`. Otherwise the coefficient goes stale when the sample rate changes — see the `updateFilter()` method in the example above.

#### Defining parameter metadata

Parameters are configured in the scriptnode UI. The node editor lets you set names, ranges, skew, and default values for each parameter index. These settings are stored in the accompanying `.xml` metadata file.

For programmatic parameter registration, `createParameters()` is available but not part of the default template. Parameters added via the UI are the standard workflow.


### Polyphonic Nodes

The `template <int NV>` parameter controls voice count. `NV` is `1` for monophonic or `NUM_POLYPHONIC_VOICES` (typically 256) for polyphonic.

#### PolyData\<T, NV\>

Stores one `T` per voice, automatically selecting the correct voice based on the processing context. The recommended pattern is to group all per-voice state into a nested struct and wrap that in a single `PolyData` member:

```cpp
struct VoiceState
{
    double phase = 0.0;
    double phaseDelta = 0.0;
    sfloat drive;
    float filterState = 0.0f;

    void prepare(double sampleRate) { /* ... */ }
    void reset() { /* ... */ }
};

PolyData<VoiceState, NV> voiceState;
```

> [!Warning:Unwrapped variables are shared across voices] If any per-voice variable (e.g. `phaseDelta`) is accidentally left outside `PolyData`, it becomes shared across all voices. A new note will silently overwrite the value for every active voice — a subtle bug that only manifests in polyphonic playback.

For trivial cases with a single state variable, `PolyData<float, NV>` works directly.

| Method | Context | Behaviour |
| --- | --- | --- |
| `get()` | During `process`/`processFrame`/`handleHiseEvent` | Returns reference to the current voice's data |
| Range-based `for` | During voice rendering | Iterates the current voice only |
| Range-based `for` | From `prepare`, `reset`, or `setParameter` | Iterates all voices |

In `prepare` and `reset`, use `for` loops to initialise all voices:

```cpp
for (auto& vs : voiceState)
    vs.reset();
```

In `processFrame`, use `get()` to access the current voice's struct:

```cpp
auto& vs = voiceState.get();
```

In `setParameter`, the `for` loop adapts automatically: when called from within a voice rendering context, it iterates only the current voice; otherwise it iterates all voices.

```cpp
for (auto& vs : voiceState)
    vs.drive.set((float)v);
```

When `NV == 1`, the compiler eliminates all voice management overhead.

#### Polyphony note

The `snex_node` itself is registered as monophonic in the scriptnode factory. Polyphonic behaviour comes from placing the node inside a polyphonic container (e.g., inside a `container.poly` or a polyphonic synthesiser module). The `NV` template parameter and `PolyData` still work correctly - the container manages voice dispatch and the node's per-voice state tracks each voice.


### Debugging

SNEX provides debugging tools that work directly in the HISE IDE. All debug features are gated by the debug mode toggle - they are no-ops in release builds and add zero overhead when disabled.

Since SNEX code runs on the audio thread without access to an external IDE debugger, these tools are your primary way to inspect what your DSP code is doing at runtime.

#### Console output

The `Console` object prints values to the HISE console. A typical debugging workflow is to print a voice's state from inside `processFrame` to verify your DSP logic:

```cpp
template <int C> void processFrame(span<float, C>& data)
{
    auto& vs = voiceState.get();
    Console.print(vs.phase);       // watch the phase accumulator advance
    Console.print(vs.filterState); // verify the filter is responding
    // ... rest of DSP
}
```

`Console.print()` is overloaded for `int`, `float`, `double`, `HiseEvent`, and complex types (structs, spans). The line number is captured automatically at compile time.

#### Breakpoints

```cpp
Console.stop(condition);       // pauses execution when condition is true
```

When `Console.stop(true)` is hit, SNEX suspends the current thread and updates the variable watch display. Use the Resume button in the IDE to continue execution.

You can also set breakpoints by clicking the gutter (line number margin) in the SNEX code editor. Gutter breakpoints trigger recompilation with debug instrumentation.

#### Line-hit detection

```cpp
Console.blink();               // highlights this line in the editor when hit
```

Useful for verifying that a particular code path is being reached during playback.

#### Live variable watch

When debug mode is enabled, the ScriptWatchTable displays live values of all local variables and parameters. Variables update at each breakpoint or at the debug polling rate.

#### Enabling debug mode

Click the bug icon in the SNEX editor toolbar to toggle debug mode. When enabled:

- `Console.print()` calls produce output
- `Console.stop()` breakpoints are active
- The variable watch table updates with live values
- Gutter breakpoints are functional

When disabled, all debug calls compile to no-ops.

> [!Tip:Leave debug calls in your code] Because all `Console` calls compile to no-ops when debug mode is off, you can leave them in your code permanently. Toggle the bug icon to switch between debugging and full-performance modes without editing any code.


## Differences from C++

SNEX supports most C++ syntax you would use in DSP code: variables, primitive types, `if`/`else`, `for`, `while`, range-based `for`, `auto`, `enum`, `using`, nested structs with methods and operator overloads, bitwise operators, `static const`, visibility modifiers, basic templates (`template <int N>`, `template <typename T>`), and a preprocessor (`#define`, `#if`/`#else`/`#endif`, `#include`). The restrictions below explain what is different and why.

### Audio-thread safety

SNEX runs directly on the audio thread. These restrictions prevent operations that could cause dropouts or undefined behaviour in a real-time context.

| Restriction | Alternative |
| --- | --- |
| No dynamic allocation (`new`/`delete`/`malloc`) | All memory is statically allocated or provided by the host |
| No exceptions (`try`/`catch`/`throw`) | Use return values and assertions |
| No multithreading primitives | All SNEX code runs on the audio thread; the host manages threading |

### JIT sandbox

The SNEX JIT compiler produces self-contained machine code. It cannot link against external libraries or system headers.

| Restriction | Alternative |
| --- | --- |
| No pointer syntax (`*`) | Use `&` references only. This eliminates nullptr dereferences and read access violations from uninitialised pointers, which would crash the audio thread in JIT-compiled code. For audio buffers, use `span`, `dyn`, `block` |
| No STL containers or algorithms | Use `span`, `dyn`, `block` |
| No standard library includes (`#include <cmath>` etc.) | Use the `Math` object |
| No JUCE framework access | SNEX targets the scriptnode DSP layer only |
| No strings or text types | DSP code operates on numeric data only. Text output for debugging uses `Console.print()` |

### Intentional design choices

These restrictions simplify the JIT compiler and keep the language focused on DSP authoring.

| Restriction | Rationale |
| --- | --- |
| No `switch` statement | `if`/`else if` chains resolve more efficiently at compile time, especially in parameter callbacks |
| No virtual functions | Eliminates vtable indirection; use templates or `if`/`else` branching |
| No multiple inheritance | Single inheritance only — keeps the type system and JIT implementation tractable |
| No advanced templates (SFINAE, concepts, variadic) | Basic `template <int N>` and `template <typename T>` cover all DSP use cases |

### HISE-specific: Console

The `Console` object is the only SNEX-specific language addition that does not exist in the C++ ThirdParty node API. All other types used in SNEX (`span`, `dyn`, `PolyData`, `sfloat`, `ProcessData`, index types, `Math`) are shared API classes available to both SNEX and C++ nodes.

| Function | Description |
| --- | --- |
| `Console.print()` | Debug output (int, float, double, HiseEvent) |
| `Console.stop(condition)` | Conditional breakpoint |
| `Console.blink()` | Line-hit detection |
| `Console.dump()` | Variable state dump |

### Limitations

- **Must be used inside a scriptnode network** - SNEX nodes cannot be loaded directly into hardcoded HISE modules. C++ DSP nodes can.
- **No existing node reuse** - cannot compose `core::oscillator`, `filters::svf`, or other building blocks as members. All DSP must be hand-written.
- **No IDE debugger** - use `Console.print()`, `Console.stop()`, and the live variable watch instead of breakpoints in Visual Studio or Xcode.
- **JIT compilation time** - initial compilation takes a moment (typically under a second). Subsequent edits recompile incrementally.


**See also:** $LANG.cpp-dsp-nodes$ -- full C++ with existing node reuse, IDE debugging, and hardcoded module loading (same callback API - easy to port), $LANG.faust$ -- functional DSP language with a different paradigm
