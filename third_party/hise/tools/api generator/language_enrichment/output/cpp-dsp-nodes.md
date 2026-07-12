---
title: C++ DSP Nodes
description: "Writing custom DSP processing nodes in C++ for the scriptnode signal graph"

guidance:
  summary: >
    Guide for writing custom C++ DSP nodes for scriptnode. Covers the complete
    node callback interface (prepare, reset, process, processFrame,
    handleHiseEvent, setExternalData, handleModulation), audio buffer types
    (ProcessData, span, dyn/block, FrameProcessor), parameter registration
    and dispatch, polyphonic voice state (PolyData), external data access,
    smoothed values (sfloat/sdouble), node macros, and manual node
    composition. Includes a complete worked example (Saturated Sine) that
    reuses existing HISE building blocks alongside a custom waveshaper node,
    with the same DSP implemented in SNEX and Faust for comparison.
  concepts:
    - C++ DSP
    - scriptnode
    - custom nodes
    - ThirdParty
    - DLL
    - process
    - processFrame
    - PrepareSpecs
    - ProcessData
    - span
    - dyn
    - block
    - FrameProcessor
    - parameters
    - createParameters
    - PolyData
    - polyphonic
    - ExternalData
    - sfloat
    - sdouble
    - ModValue
    - SNEX_NODE
    - node composition
  prerequisites:
    - scriptnode
  complexity: advanced
---

C++ DSP nodes let you write custom signal processing code that plugs directly into the scriptnode graph. Unlike $LANG.snex$ (a restricted C++ subset interpreted at runtime) or $LANG.faust$ (a functional DSP language compiled on save), C++ nodes give you full access to the language, the JUCE framework, and the entire library of existing scriptnode building blocks.

The main reasons to choose C++ over the other DSP languages:

- **Full C++ with existing code** - no language restrictions, full JUCE framework access, and the ability to wrap existing C++ DSP code. Any code that follows the standard prepare/process callback pattern - third-party DSP libraries, VST2 plugin code, custom algorithms - can be adapted to the node interface with minimal boilerplate. This includes reusing scriptnode's own building blocks (`core::oscillator`, `filters::svf`, and hundreds of other nodes) as C++ template members alongside your own code
- **IDE debugging** - C++ nodes compile in Visual Studio or Xcode, giving you breakpoints, step-through debugging, memory inspection, and profiling tools that are not available for SNEX, Faust, or HiseScript code
- **Direct hardcoded module loading** - C++ nodes can be loaded into hardcoded HISE modules without a scriptnode network wrapper, bypassing the interpreter entirely. SNEX, Faust, and RNBO nodes all require a network even when it just forwards parameters 1:1

During development, C++ nodes are compiled into a Project DLL that HISE loads at startup. When you export your plugin, the same code is compiled directly into the binary.

> [!Tip:Start with SNEX for prototyping] $LANG.snex$ uses a near-identical callback API and compiles instantly inside the IDE. Once your algorithm works, porting to C++ is mostly copy-paste. C++ becomes worthwhile when you need to compose multiple nodes or use features outside SNEX's subset.


## Example: Saturated Sine

This complete node implements a polyphonic synth voice: sine oscillator, tanh soft-clip saturation, SVF lowpass filter. It reuses two HISE building blocks (`core::oscillator` and `filters::svf`) and adds a custom waveshaper as an inner struct. The same DSP is implemented in $LANG.snex$ and $LANG.faust$ so you can compare approaches across languages.

The following sections unpack every part of this code in detail.

```cpp
#pragma once
#include <JuceHeader.h>

namespace project
{
using namespace juce;
using namespace hise;
using namespace scriptnode;

template <int NV> struct my_synth_voice : public data::base
{
    // Metadata Definitions ---------------------------------------------------

    SNEX_NODE(my_synth_voice);

    struct MetadataClass
    {
        SN_NODE_ID("my_synth_voice");
    };

    static constexpr bool isModNode()    { return false; };
    static constexpr bool isPolyphonic() { return NV > 1; };
    static constexpr bool hasTail()      { return false; };

    static constexpr bool isSuspendedOnSilence() { return false; };
    static constexpr int getFixChannelAmount()   { return 2; };

    static constexpr int NumTables         = 0;
    static constexpr int NumSliderPacks    = 0;
    static constexpr int NumAudioFiles     = 0;
    static constexpr int NumFilters        = 0;
    static constexpr int NumDisplayBuffers = 0;

    // Custom inner node ------------------------------------------------------

    struct my_shaper
    {
        hmath Math;

        PolyData<sfloat, NV> drive;

        void prepare(PrepareSpecs ps)
        {
            for (auto& d : drive)
                d.prepare(ps.sampleRate, 20.0);  // 20ms smoothing time
        }

        void reset()
        {
            for (auto& d : drive)
                d.reset();
        }

        template <typename T> void processFrame(T& data)
        {
            auto& d = drive.get();
            auto driveValue = d.advance();

            for (auto& s : data)
                s = Math.tanh(s * driveValue);
        }

        void setDrive(double v)
        {
            for (auto& d : drive)
                d.set((float)v);
        }
    };

    // Scriptnode Callbacks ---------------------------------------------------

    void prepare(PrepareSpecs ps)
    {
        osc.prepare(ps);
        shaper.prepare(ps);
        filter.prepare(ps);
    }

    void reset()
    {
        osc.reset();
        shaper.reset();
        filter.reset();
    }

    template <typename T> void process(T& data)
    {
        static constexpr int NumChannels = getFixChannelAmount();
        auto& fixData = data.template as<ProcessData<NumChannels>>();

        osc.process(fixData);

        auto fd = fixData.toFrameData();

        while (fd.next())
            shaper.processFrame(fd.toSpan());

        filter.process(fixData);
    }

    template <typename T> void processFrame(T& data)
    {
        osc.processFrame(data);
        shaper.processFrame(data);
        filter.processFrame(data);
    }

    void handleHiseEvent(HiseEvent& e)
    {
        osc.handleHiseEvent(e);
        filter.handleHiseEvent(e);
    }

    void setExternalData(const ExternalData& d, int index) {}

    int handleModulation(double& value) { return 0; }

    // Hardcoded Module Modulation --------------------------------------------

    void createExternalModulationInfo(OpaqueNode::ModulationProperties& info)
    {
        modulation::ParameterProperties::ConnectionList list;

        modulation::ConnectionInfo slot;
        slot.connectedParameterIndex = 2;  // Cutoff
        slot.modColour = HiseModulationColours::ColourId::FX;
        slot.modulationMode = modulation::ParameterMode::ScaleOnly;
        list.push_back(slot);

        info.fromConnectionList(list);
    }

    // Parameter Functions ----------------------------------------------------

    template <int P> void setParameter(double v)
    {
        if (P == 0) osc.template setParameter<1>(v);     // Frequency
        if (P == 1) shaper.setDrive(v);                   // Drive
        if (P == 2) filter.template setParameter<0>(v);   // Cutoff
    }

    void createParameters(ParameterDataList& data)
    {
        {
            parameter::data p("Frequency", { 20.0, 20000.0 });
            registerCallback<0>(p);
            p.setDefaultValue(440.0);
            data.add(std::move(p));
        }
        {
            parameter::data p("Drive", { 0.0, 10.0 });
            registerCallback<1>(p);
            p.setDefaultValue(1.0);
            data.add(std::move(p));
        }
        {
            parameter::data p("Cutoff", { 20.0, 20000.0 });
            registerCallback<2>(p);
            p.setDefaultValue(1000.0);
            data.add(std::move(p));
        }
    }

    // Member variables -------------------------------------------------------

    core::oscillator<NV>  osc;
    my_shaper             shaper;
    filters::svf<NV>      filter;
};
}
```

The custom DSP lives in the `my_shaper` inner struct - just the essentials: state, `prepare`, `reset`, `processFrame`, and a setter method. No boilerplate macros, metadata, or empty callbacks. The outer `my_synth_voice` handles all the registration and runtime integration.

Note the `.template` disambiguator in `setParameter` calls (e.g., `osc.template setParameter<1>(v)`). This is required because the member types depend on the template parameter `NV`, so the compiler needs the hint that `setParameter` is a template.


## Project Setup

C++ node source files live in `DspNetworks/ThirdParty/` inside your HISE project. Each node is a `.h` file containing a single template struct.

To create a new node, use **Tools > Create Third Party C++ Node** in the HISE menu bar. This generates a `.h` file with the complete node template, pre-filled with empty callbacks and a sample parameter.

### File structure

```
YourProject/
  DspNetworks/
    ThirdParty/
      my_synth_voice.h     <-- your custom node
```

All `.h` files in `ThirdParty/` are automatically included by the build system. No manual `#include` directives are needed.

### Development workflow

1. Write or edit your `.h` file in `ThirdParty/`
2. **Export > Compile DSP Networks as DLL** to compile the Project DLL
3. HISE automatically hot-loads the new DLL - your nodes are available immediately
4. Repeat from step 1 as you iterate

The Project DLL build system handles factory registration automatically. Polyphonic nodes (where `isPolyphonic()` returns `NV > 1`) are registered in both mono and poly variants.

When you export the final plugin, the DLL code is compiled directly into the plugin binary. No DLL ships with the product.

> [!Tip:Treat the DLL as a throwaway build artifact] The Project DLL should be recompiled whenever your HISE source code changes. Do not check it into version control or share it between machines - it is a local build cache, not a distributable.

### Using your node

Once the DLL is compiled, there are two ways to use your node:

**Hardcoded module** (recommended) - Add a hardcoded module to the HISE module tree and select your node from the dropdown. The node's callbacks are wired directly via function pointers - no scriptnode interpreter involved. This is the most efficient path and the primary reason to write C++ nodes.

| Module type | Base processor | Use case |
| --- | --- | --- |
| `HardcodedMasterFX` | Master effect | Stereo bus effects (reverb, limiter, EQ) |
| `HardcodedPolyphonicFX` | Voice effect | Per-voice effects (filter, distortion) |
| `HardcodedSynthesiser` | Synth | Polyphonic sound generators |
| `HardcodedTimeVariantModulator` | Time-variant modulator | LFOs, step sequencers |
| `HardcodedEnvelopeModulator` | Envelope modulator | Per-voice envelopes (ADSR, custom shapes) |

**Inside a scriptnode network** - Your node also appears as `project.my_synth_voice` in the scriptnode node browser. This is useful when you want to combine custom C++ processing with visual scriptnode editing, but the node still runs through the scriptnode interpreter's dispatch layer.


## The Node Callback Interface

Every C++ DSP node must provide the callbacks listed below. The runtime calls them through type-erased function pointers, so the signatures must match exactly.

### prepare

```cpp
void prepare(PrepareSpecs ps)
```

Called when the processing context changes (sample rate, block size, channel count, or voice assignment). Use it to initialise smoothing ramps, allocate scratch buffers, and propagate specs to sub-nodes. The `PrepareSpecs` struct contains:

| Field | Type | Description |
| --- | --- | --- |
| `sampleRate` | `double` | May differ from the host rate if the node sits inside an oversampling container |
| `blockSize` | `int` | Maximum samples per block (actual blocks may be smaller) |
| `numChannels` | `int` | Number of audio channels |
| `voiceIndex` | `PolyHandler*` | Voice manager pointer (nullptr for monophonic contexts) |

The runtime always calls `reset()` immediately after `prepare()`. You do not need to call `reset()` from within your `prepare()` implementation.

In the example, the inner `my_shaper` struct initialises its smoothing ramp in `prepare`:

```cpp
void prepare(PrepareSpecs ps)
{
    for (auto& d : drive)
        d.prepare(ps.sampleRate, 20.0);
}
```

The outer `my_synth_voice` propagates `prepare` to all sub-nodes:

```cpp
void prepare(PrepareSpecs ps)
{
    osc.prepare(ps);
    shaper.prepare(ps);
    filter.prepare(ps);
}
```

### reset

```cpp
void reset()
```

Called at voice start (polyphonic) or when the node is un-bypassed. Clear any internal state - filter history, delay buffers, phase accumulators - so the node starts from silence.

`reset()` is also called after `prepare()`, so the node is always in a clean state when processing begins.

### process

```cpp
template <typename T> void process(T& data)
```

Block-based processing. Called once per audio block with all channels. The template parameter `T` is `ProcessDataDyn` at runtime; inside the function you cast to a fixed channel count.

The canonical pattern (from the HISE-generated template) is:

```cpp
template <typename T> void process(T& data)
{
    static constexpr int NumChannels = getFixChannelAmount();
    auto& fixData = data.template as<ProcessData<NumChannels>>();

    auto fd = fixData.toFrameData();

    while (fd.next())
        processFrame(fd.toSpan());
}
```

This casts to fixed channels, creates a `FrameProcessor`, and forwards each sample frame to `processFrame()`. For nodes that benefit from block-level processing (e.g., applying a gain ramp to an entire channel at once), you can process the block directly instead of forwarding to frames. See the Audio Buffer Types section for both patterns.

### processFrame

```cpp
template <typename T> void processFrame(T& data)
```

Per-sample processing. `T` is `span<float, NumChannels>` - a fixed-size array with one sample per channel. This is where most custom DSP logic lives.

In the example, `my_shaper::processFrame` applies the tanh saturation:

```cpp
template <typename T> void processFrame(T& data)
{
    auto& d = drive.get();
    auto driveValue = d.advance();

    for (auto& s : data)
        s = Math.tanh(s * driveValue);
}
```

The `drive.get()` call retrieves the current voice's smoothed value, and `advance()` steps the smoother by one sample.

### handleHiseEvent

```cpp
void handleHiseEvent(HiseEvent& e)
```

Called for each MIDI event (note-on, note-off, controller, pitch bend, aftertouch). If your node does not process MIDI, leave the body empty but keep the method defined. When composing sub-nodes, forward events to each one so they maintain correct MIDI state.

### setExternalData

```cpp
void setExternalData(const ExternalData& d, int index)
```

Called when external data (Tables, SliderPacks, AudioFiles, FilterCoefficients, DisplayBuffers) is assigned to the node. The `index` is a global counter across all data types. See the External Data section for details.

If your node does not use external data, leave the body empty.

### handleModulation (optional)

```cpp
int handleModulation(double& value)
```

Called after every `process` and `processFrame` block. If your node produces a modulation signal, write the value into the reference parameter and return `1`. Return `0` if no new value is available.

To enable the modulation dragger in the UI, set `isModNode()` to return `true`.


## Audio Buffer Types

Scriptnode provides several buffer types for different processing contexts. Understanding how they relate is key to writing correct `process` and `processFrame` implementations.

### ProcessData\<C\> and ProcessDataDyn

`ProcessData<C>` wraps a multichannel audio block with a compile-time channel count `C`. It provides:

- `operator[]` - access a channel as `dyn<float>` (a contiguous float buffer)
- `toChannelData(ch)` - convert a channel iterator to `dyn<float>`
- `toFrameData()` - create a `FrameProcessor` for per-sample iteration
- `getNumSamples()` - sample count for this block
- `toEventData()` - access MIDI events as `dyn<HiseEvent>`

`ProcessDataDyn` is the dynamic-channel variant used by the runtime dispatch layer. Inside your `process()` callback, cast it to a fixed channel count:

```cpp
auto& fixData = data.template as<ProcessData<2>>();
```

### span\<T, N\>

A fixed-size array. In DSP code, `span<float, N>` represents an audio frame - one sample per channel. Used as the argument to `processFrame()`.

```cpp
// Access individual channels in a stereo frame
auto& left  = data[0];
auto& right = data[1];
```

### dyn\<T\> and block

`dyn<T>` is a non-owning view into a contiguous buffer. The alias `block` means `dyn<float>` - a single channel's sample buffer.

```cpp
// Channel-based processing: iterate channels, then samples
for (auto& ch : fixData)
{
    for (auto& s : fixData.toChannelData(ch))
        s *= 0.5f;
}
```

### FrameProcessor

Bridges block-based and frame-based processing. Created by `ProcessData<C>::toFrameData()`, it reads one sample from each channel into a `span<float, C>`, lets you process the frame, then writes it back.

```cpp
auto fd = fixData.toFrameData();

while (fd.next())
    processFrame(fd.toSpan());
```

This is the pattern used in the HISE-generated template and in the example's `process` callback. It is zero-overhead - the compiler inlines everything into a tight loop equivalent to manual interleaving.

### When to use block vs frame processing

| Pattern | Best for | Example |
| --- | --- | --- |
| FrameProcessor (forward to `processFrame`) | Per-sample algorithms, filters, waveshapers | The shaper in the example - applies tanh per sample |
| Channel iteration in `process` | Block-level operations, SIMD, applying gain ramps | Multiplying an entire channel buffer by a constant |

For most custom nodes, the FrameProcessor pattern is simpler and the performance difference is negligible. Use direct channel iteration only when you need block-level operations like JUCE `FloatVectorOperations`.


## Parameters

Parameters connect your node to the scriptnode UI and modulation system. Each node can have up to 16 parameters.

### Defining parameters

Parameters are registered in `createParameters()`. Each parameter needs a name, range, default value, and a callback binding:

```cpp
void createParameters(ParameterDataList& data)
{
    {
        parameter::data p("Drive", { 0.0, 10.0 });
        registerCallback<0>(p);
        p.setDefaultValue(1.0);
        data.add(std::move(p));
    }
}
```

The `registerCallback<0>(p)` call binds the parameter to `setParameter<0>()`. The template argument must match the index you check in `setParameter`.

### Range specification

The `parameter::data` constructor accepts `{ min, max }`. For more control, use `setRange()`:

```cpp
parameter::data p("Frequency", { 20.0, 20000.0 });
p.setRange({ 20.0, 20000.0, 0.1, 0.3 });  // min, max, interval, skew
```

The skew value controls the curve of the parameter slider. A skew of 0.3 puts the midpoint near 1000 Hz - useful for frequency parameters.

### Receiving parameter changes

The `setParameter` template dispatches parameter values to your node. The template argument `P` is the parameter index:

```cpp
template <int P> void setParameter(double v)
{
    if (P == 0)
    {
        for (auto& d : drive)
            d.set((float)v);
    }
}
```

The `if (P == 0)` pattern (rather than `if constexpr`) is the standard idiom used by the HISE template generator. When multiple parameters exist, add an `if` block for each index.

> [!Warning:Parameter callbacks run on any thread] Parameter changes can arrive from the audio thread (modulation), the UI thread (knob drag), or the scripting thread. Never allocate memory, acquire locks, or perform I/O inside `setParameter`. Use `sfloat`/`sdouble` for smoothing - these are safe for concurrent access.

### Forwarding parameters to sub-nodes

When composing nodes, forward parameter values to sub-node parameters by calling their `setParameter` method:

```cpp
template <int P> void setParameter(double v)
{
    if (P == 0) osc.template setParameter<1>(v);     // -> oscillator Frequency
    if (P == 1) shaper.setDrive(v);                     // -> shaper Drive
    if (P == 2) filter.template setParameter<0>(v);   // -> SVF Frequency
}
```

The sub-node parameter indices correspond to the parameter order defined in that node's `createParameters()`. For HISE built-in nodes, consult the scriptnode node reference for parameter indices.

### Display formatting

Set a text converter on the `parameter::data` to control how the value displays in the UI:

```cpp
p.info.textConverter = parameter::TextValueConverters::Frequency;  // shows "440.0 Hz"
```

Available converters: `Frequency`, `Time`, `TempoSync`, `Pan`, `NormalizedPercentage`, `Decibel`, `Semitones`.


## Polyphonic Nodes

Polyphonic nodes maintain independent state for each voice. The template parameter `NV` controls the voice count: `1` for monophonic, or `NUM_POLYPHONIC_VOICES` (typically 256) for polyphonic.

### PolyData\<T, NV\>

The central abstraction for per-voice state. Stores one `T` per voice and automatically selects the correct voice based on the current processing context.

```cpp
PolyData<sfloat, NV> drive;
```

Key operations:

| Method | Context | Behaviour |
| --- | --- | --- |
| `get()` | During `process`/`processFrame` | Returns reference to the current voice's data |
| Range-based `for` | During voice rendering | Iterates the current voice only |
| Range-based `for` | From UI/parameter thread | Iterates all voices |
| `prepare(ps)` | In `prepare()` | Stores the voice handler pointer from `PrepareSpecs` |

This context-sensitive iteration is why `setParameter` and `reset` use `for (auto& d : drive)` - when called from the parameter thread, this sets all voices; when called during rendering, it sets only the active voice.

When `NV == 1` (monophonic), the compiler eliminates all voice management overhead. The generated machine code is identical to using a plain `T` member.

### Making a node polyphonic

The HISE template handles this automatically through the `NV` parameter. In the example, the outer node declares `isPolyphonic()` based on `NV`, and the inner `my_shaper` uses `PolyData<sfloat, NV>` for its per-voice drive state:

```cpp
struct my_shaper
{
    PolyData<sfloat, NV> drive;  // NV inherited from enclosing template

    void prepare(PrepareSpecs ps)
    {
        for (auto& d : drive)
            d.prepare(ps.sampleRate, 20.0);
    }
};
```

Any state that varies per voice (filter coefficients, phase accumulators, smoothed parameters) should be wrapped in `PolyData`. Shared state (lookup tables, constant coefficients) can remain as plain members.

### Composing polyphonic sub-nodes

When your node contains HISE building blocks like `core::oscillator<NV>` or `filters::svf<NV>`, pass the same `NV` template argument. Each sub-node manages its own polyphonic state internally:

```cpp
core::oscillator<NV>  osc;      // handles per-voice frequency/phase
my_shaper             shaper;   // handles per-voice drive smoothing (via PolyData)
filters::svf<NV>      filter;   // handles per-voice filter state
```

Inner structs like `my_shaper` inherit `NV` from the enclosing template, so their `PolyData` members automatically use the correct voice count.


## External Data

Nodes can access complex data objects - Tables, SliderPacks, AudioFiles, FilterCoefficients, and DisplayBuffers - through the external data system. The data is owned by the scriptnode graph and passed to the node via `setExternalData()`.

### Declaring data slots

Set the static constants to declare how many slots of each type your node needs:

```cpp
static constexpr int NumTables         = 1;  // one lookup table
static constexpr int NumSliderPacks    = 0;
static constexpr int NumAudioFiles     = 0;
static constexpr int NumFilters        = 0;
static constexpr int NumDisplayBuffers = 0;
```

### Receiving data

The `setExternalData` callback receives each data object with a global index across all types:

```cpp
void setExternalData(const ExternalData& d, int index)
{
    if (index == 0)  // first Table
    {
        // Store the data reference for use in process callbacks
        base::setExternalData(d, index);
    }
}
```

### Data locking

External data may be modified from the UI thread while you read it on the audio thread. Use `DataReadLock` for RAII read access:

```cpp
template <typename T> void processFrame(T& data)
{
    if (auto lock = DataReadLock(this))
    {
        // Safe to read external data here
    }
}
```

For the audio thread, prefer `DataTryReadLock` which returns immediately if the lock is contended (avoiding priority inversion).

### Data types

| Type | Typical use | Access |
| --- | --- | --- |
| Table | Waveshaping curves, envelope shapes | 512-float lookup array |
| SliderPack | User-editable value sequences | Resizable float array |
| AudioFile | Impulse responses, samples | Multichannel audio with sample rate |
| FilterCoefficients | Filter display integration | Coefficient data for visualisation |
| DisplayBuffer | Oscilloscopes, modulation plotters | Ring buffer for real-time display |


## Smoothed Values and Modulation Output

### sfloat and sdouble

Use `sfloat` (single precision) or `sdouble` (double precision) for parameters that change during playback. These types ramp smoothly from the old value to the new one, preventing the zipper noise that occurs when a parameter jumps instantly.

```cpp
sfloat drive;

void prepare(PrepareSpecs ps)
{
    drive.prepare(ps.sampleRate, 20.0);  // 20ms ramp time
}

void reset()
{
    drive.reset();  // jump to target immediately (no ramp on voice start)
}

// In setParameter:
drive.set(newValue);  // starts a smooth ramp to the new value

// In processFrame:
auto currentDrive = drive.advance();  // returns current value, steps the ramp
```

Key methods:

| Method | Purpose |
| --- | --- |
| `prepare(sampleRate, timeMs)` | Set the ramp duration |
| `set(target)` | Start ramping to a new value |
| `advance()` | Return the current value and step forward by one sample |
| `get()` | Return the current value without stepping |
| `reset()` | Jump to the target value immediately |
| `isActive()` | True if still ramping |

Wrap smoothed values in `PolyData` for per-voice smoothing, as shown in the example's inner `my_shaper` struct.

### Modulation

How your node integrates with modulation depends on how it is loaded.

#### Hardcoded modules: createExternalModulationInfo

When loaded as a hardcoded module, your node hooks into HISE's module tree modulation system (modulator chains, matrix modulation) via the `createExternalModulationInfo` callback. This is called once at node creation to declare which parameters accept modulation:

```cpp
void createExternalModulationInfo(OpaqueNode::ModulationProperties& info)
{
    modulation::ParameterProperties::ConnectionList list;

    modulation::ConnectionInfo slot;
    slot.connectedParameterIndex = 2;  // modulate parameter P==2 (Cutoff)
    slot.modColour = HiseModulationColours::ColourId::FX;
    slot.modulationMode = modulation::ParameterMode::ScaleOnly;
    list.push_back(slot);

    info.fromConnectionList(list);
}
```

Each `ConnectionInfo` entry declares one modulation slot:

| Field | Purpose |
| --- | --- |
| `connectedParameterIndex` | Which parameter (by `setParameter<P>` index) receives modulation |
| `modColour` | Visual colour in the module tree (`Gain`, `Pitch`, `FX`, `ExtraMod`, `Midi`, etc.) |
| `modulationMode` | How modulation is applied: `ScaleOnly`, `AddOnly`, `ScaleAdd`, `Pan`, or `Disabled` |

The default number of modulation slots per hardcoded module is limited. To increase it, set the preprocessor definitions `NUM_HARDCODED_FX_MODS` and `NUM_HARDCODED_POLY_FX_MODS` in your project settings.

#### Scriptnode networks: handleModulation and ModValue

When used inside a scriptnode network, a node can act as a modulation *source* (with a draggable output in the UI). Set `isModNode()` to return `true` and use `ModValue`:

```cpp
static constexpr bool isModNode() { return true; };

ModValue modValue;

template <typename T> void processFrame(T& data)
{
    auto peak = Math.abs(data[0]);
    modValue.setModValueIfChanged(peak);
}

int handleModulation(double& value)
{
    return modValue.getChangedValue(value) ? 1 : 0;
}
```

| Method | Use case |
| --- | --- |
| `setModValue(v)` | Periodic sources (LFOs, envelopes) - always flags as changed |
| `setModValueIfChanged(v)` | Event-driven sources - only flags when the value actually changes |
| `getChangedValue(v)` | Called by the runtime in `handleModulation` - returns true and fills the value if changed |

Modulation output values are normalised to 0.0-1.0 by default. To output raw (unnormalised) values, override `isNormalisedModulation()` to return `false`.

> [!Warning:handleModulation is ignored in hardcoded modules] The `handleModulation` callback and `ModValue` only work inside scriptnode networks. When loaded as a hardcoded module, use `createExternalModulationInfo` to hook into the HISE module tree modulation system instead.


## Node Macros Reference

The HISE-generated template uses several macros. Here is what each one does.

### Identity and metadata

| Macro / Pattern | Purpose |
| --- | --- |
| `SNEX_NODE(ClassName)` | All-in-one: declares a `Math` member (for `Math.tanh()` etc.), object access helpers, parameter forwarding, and an empty `initialise` callback |
| `struct MetadataClass { SN_NODE_ID("id"); };` | Nested struct declaring the node's string ID for the factory |

### Static trait methods

These `static constexpr` methods are queried at registration time. They control how the runtime handles your node.

| Method | Returns | Effect |
| --- | --- | --- |
| `isModNode()` | `bool` | Enables the modulation output dragger in the UI |
| `isPolyphonic()` | `bool` | `NV > 1` for polyphonic nodes; the factory registers both mono and poly variants |
| `hasTail()` | `bool` | If true, the node continues producing output after input goes silent |
| `isSuspendedOnSilence()` | `bool` | If true, the runtime skips processing when input is silent |
| `getFixChannelAmount()` | `int` | Fixed channel count; the runtime skips the dynamic channel dispatch. Use 2 for stereo nodes |

### External data slot constants

| Constant | Type |
| --- | --- |
| `NumTables` | Lookup tables (512-float arrays) |
| `NumSliderPacks` | Resizable float arrays |
| `NumAudioFiles` | Multichannel audio buffers |
| `NumFilters` | Filter coefficient objects |
| `NumDisplayBuffers` | Ring buffers for visualisation |

Set each to 0 if your node does not use that data type.

### Callback shortcut macros

For nodes that do not need certain callbacks, shortcut macros provide empty implementations:

- `SN_EMPTY_PREPARE`, `SN_EMPTY_RESET`, `SN_EMPTY_PROCESS`, `SN_EMPTY_PROCESS_FRAME`
- `SN_EMPTY_HANDLE_EVENT`, `SN_EMPTY_SET_EXTERNAL_DATA`, `SN_EMPTY_MOD`
- `SN_NO_PARAMETERS` - combines empty `createParameters` and `setParameter`

These are optional conveniences. The generated template writes out the methods explicitly, which is clearer for learning.


## Composing Nodes

The example shows the basic composition pattern: declare sub-nodes as members (both HISE building blocks and custom inner structs) and forward each callback manually.

```cpp
core::oscillator<NV>  osc;
my_shaper             shaper;
filters::svf<NV>      filter;

void prepare(PrepareSpecs ps)
{
    osc.prepare(ps);
    shaper.prepare(ps);
    filter.prepare(ps);
}
```

This is serial processing - each node processes the full block in sequence, with the output of one feeding the input of the next. The buffer is modified in-place.

In the example, `process` mixes block-level and frame-level calls: the oscillator and filter process full blocks, while the shaper runs per-frame via the `FrameProcessor`:

```cpp
template <typename T> void process(T& data)
{
    static constexpr int NumChannels = getFixChannelAmount();
    auto& fixData = data.template as<ProcessData<NumChannels>>();

    osc.process(fixData);

    auto fd = fixData.toFrameData();
    while (fd.next())
        shaper.processFrame(fd.toSpan());

    filter.process(fixData);
}
```

### Forwarding MIDI events

Forward `handleHiseEvent` to sub-nodes that need MIDI state (note tracking, velocity, pitch bend). Inner structs that do not process MIDI can be skipped:

```cpp
void handleHiseEvent(HiseEvent& e)
{
    osc.handleHiseEvent(e);
    filter.handleHiseEvent(e);
}
```

### Container templates

This manual composition pattern is straightforward but verbose. Scriptnode's `container::chain` template can express the same structure more compactly, and this is in fact what the C++ code generator produces when you export a scriptnode network. If you are curious, look at the generated `.h` files after exporting a network - they show the template composition style with `container::chain`, `wrap::fix`, and compile-time parameter wiring.

For hand-written nodes, manual composition is recommended. It is easier to read, easier to debug, and does not require understanding the template metaprogramming layer. When loaded as a hardcoded module, the manual forwarding compiles to direct function calls with zero interpreter overhead.


## Differences from Standard C++

C++ DSP nodes are standard C++17 compiled with your platform's toolchain. However, the audio thread environment imposes constraints, and scriptnode provides its own types in place of some standard library facilities.

### Available

| Feature | Notes |
| --- | --- |
| C++17 language features | `if constexpr`, structured bindings, fold expressions, etc. |
| JUCE framework | Full access to `juce::` utilities, `FloatVectorOperations`, `dsp::` classes |
| `hmath` (via `Math` member) | `Math.tanh()`, `Math.sin()`, `Math.db2gain()`, block operations, etc. |
| Templates | Full template support including variadic templates and SFINAE |

### Restricted on the audio thread

| Restriction | Reason |
| --- | --- |
| No dynamic memory allocation (`new`, `malloc`, `std::vector::push_back`) | Allocators may block; unpredictable latency |
| No exceptions (`throw`, `try`/`catch`) | Exception handling is disabled in the DLL build for performance |
| No mutex/lock acquisition | Priority inversion can cause audio dropouts |
| No file I/O or network calls | Unbounded latency |

These restrictions apply only to code that runs on the audio thread (inside `process`, `processFrame`, `reset`, `handleHiseEvent`, `handleModulation`). Code in `prepare`, `setExternalData`, and `createParameters` runs on the message thread and has no restrictions.

### HISE-specific types

| HISE type | Replaces | Purpose |
| --- | --- | --- |
| `span<T, N>` | `std::array<T, N>` | Fixed-size array with DSP operations and safe indexing |
| `dyn<T>` / `block` | `std::span<T>` | Non-owning buffer view with SIMD-accelerated arithmetic |
| `PolyData<T, NV>` | (no equivalent) | Per-voice state with automatic voice selection |
| `sfloat` / `sdouble` | (manual ramping) | Smoothed value with automatic per-sample interpolation |
| `ProcessData<C>` | (no equivalent) | Multichannel audio block with MIDI events |
| `HiseEvent` | `juce::MidiMessage` | Extended MIDI event with voice index and timestamp |


## Limitations

- **Maximum 16 channels** in frame-based container processing (`HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS`)
- **128-byte inline buffer** - nodes smaller than 128 bytes are stored inline (no heap allocation). Larger nodes are heap-allocated, which is fine but slightly slower to create


**See also:** $LANG.snex$ -- HISE's JIT-compiled C++ subset (near-identical API, instant compilation), $LANG.faust$ -- functional DSP language integration, $LANG.cpp-raw$ -- the C++ Raw API for building full plugin architectures without scriptnode
