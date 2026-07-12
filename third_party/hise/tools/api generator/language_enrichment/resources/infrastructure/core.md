# Scriptnode Core Infrastructure Reference

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/snex_basics/snex_ProcessDataTypes.h`
- `hi_dsp_library/snex_basics/snex_FrameProcessor.h`
- `hi_dsp_library/snex_basics/snex_ArrayTypes.h`
- `hi_dsp_library/snex_basics/snex_Types.h`
- `hi_dsp_library/snex_basics/snex_ExternalData.h`
- `hi_dsp_library/node_api/nodes/Base.h`
- `hi_dsp_library/node_api/nodes/OpaqueNode.h`
- `hi_dsp_library/node_api/helpers/node_macros.h`
- `hi_dsp_library/node_api/helpers/node_ids.h`
- `hi_dsp_library/node_api/helpers/ParameterData.h`
- `hi_dsp_library/node_api/helpers/NodeProperty.h`

---

## 1. Audio Buffer Types: span and dyn

Scriptnode uses two fundamental array types from the `snex::Types` namespace.

### span<T, Size>

A fixed-size, compile-time array. 16-byte aligned by default. Supports:
- Range-based for loops: `for(auto& s : mySpan)`
- Subscript operator with bounds-checked integer or index types (wrapped, clamped)
- Arithmetic operators (+, -, *, /) that apply element-wise, with SSE/SIMD
  acceleration when T=float and Size is a multiple of 4
- `clear()`, `fill(value)`, `accumulate()`, `toSimd()`

Key specializations:
- `span<float, N>` -- an N-sample audio frame (one sample per channel)
- `span<float, 4>` aka `float4` -- SIMD-friendly 4-float vector
- `span<float*, N>` -- array of N channel pointers (used internally by ProcessData)

### dyn<T>

A dynamic-size, non-owning reference to memory owned by something else. The
scriptnode equivalent of a "view" or "slice". Key alias: `block = dyn<float>`.

- `referTo(container, newSize, offset)` -- point at existing data
- `referToRawData(ptr, size)` -- point at raw pointer
- Arithmetic operators (+, -, *, /) on `dyn<float>` use JUCE FloatVectorOperations
- `split<N>()` -- split into N equal sub-blocks
- `toSimd()` -- reinterpret as `dyn<float4>`
- `isEmpty()` / `size()` for bounds checking

### heap<T>

An owning, dynamically-allocated array. `setSize(n)` allocates. Used internally
for scratch buffers. Not commonly seen in node APIs.

---

## 2. Audio Data Flow: ProcessData

Audio data flows through nodes via `ProcessData<C>` (fixed channel count) or
`ProcessDataDyn` (dynamic channel count). Both inherit from `InternalData`.

### InternalData (base class)

Holds:
- `float** data` -- array of channel pointers
- `HiseEvent* events` -- pointer to MIDI event buffer
- `int numSamples` -- samples in this block (always <= PrepareSpecs.blockSize)
- `int numEvents` -- number of MIDI events
- `int numChannels` -- channel count

Key methods:
- `getNumSamples()` -- sample count for this block
- `toChannelData(channelPtr)` -- converts a ChannelPtr to a `dyn<float>` (block)
- `toEventData()` -- returns `dyn<HiseEvent>` for MIDI event iteration
- `toAudioSampleBuffer()` / `toAudioBlock()` -- JUCE interop

### ProcessData<C> (fixed channels)

Template parameter C is the compile-time channel count. `NumChannels = C`.

Iteration patterns:
```cpp
// Channel-based: iterate channels, then samples
for(auto& ch : data)
{
    for(auto& s : data.toChannelData(ch))
        s *= 0.5f;
}

// Direct channel access
dyn<float> left = data[0];
dyn<float> right = data[1];

// Frame-based: converts to interleaved processing
auto fd = data.toFrameData();
while(fd.next())
    processFrame(fd.toSpan());
```

- `isSilent()` -- SIMD-accelerated silence detection (< -90dB). Only for <= 2 channels.
- Non-copyable by design (JUCE_DECLARE_NON_COPYABLE).

### ProcessDataDyn (dynamic channels)

Same interface as ProcessData<C> but channel count is runtime. Used by
`OpaqueNode::process()`. The `toFrameData<N>()` method requires a compile-time
template argument for the frame size.

### ChunkableProcessData

Utility for splitting a ProcessData into smaller chunks (sub-block processing).
Automatically slices MIDI events by timestamp and adjusts timestamps per chunk.

Usage:
```cpp
ChunkableProcessData<ProcessDataDyn> chunked(data);
while(chunked)
{
    auto chunk = chunked.getChunk(64);  // 64-sample chunks
    processChunk(chunk.toData());
}
auto remainder = chunked.getRemainder();  // must consume all samples
```

Destructor asserts that all samples were consumed.

---

## 3. Frame Processing: FrameProcessor

Provides interleaved (sample-by-sample) iteration over multichannel audio.
Created by `ProcessData<C>::toFrameData()`.

### How it works

The FrameProcessor reads one sample from each channel into a local
`span<float, NumChannels>` (the "frame"), processes it, then writes it back.

```cpp
void process(ProcessData<2>& data)
{
    auto frame = data.toFrameData();
    while(frame.next())    // loads next frame, returns false at end
        processFrame(frame.toSpan());
}

void processFrame(span<float, 2>& frame)
{
    auto& l = frame[0];
    auto& r = frame[1];
    // MS processing, filters, etc.
}
```

- `next()` -- loads the next sample from all channels into the frame buffer,
  writes the previous frame back. Returns false when all samples are processed.
- `toSpan()` -- returns `span<float, NumChannels>&` for direct access
- `size()` -- returns NumChannels (compile-time constant)
- Implicitly castable to `span<float, NumChannels>&`
- The compiler typically inlines everything, so there is no performance overhead
  vs. manual interleaving

### FrameConverters

Static utility class that bridges dynamic channel counts to fixed-size frame
processing. Used internally by container nodes.

- `forwardToFrameMono(ptr, data)` -- process as 1 channel
- `forwardToFrameStereo(ptr, data)` -- dispatches 1, 2, or 4 channels
- `forwardToFrame16(ptr, data)` -- dispatches 1-16 channels via switch
- `forwardToFixFrame16(ptr, frameData)` -- same but for frame data references
- Max frame container channels controlled by `HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS`

### OscProcessData

Specialized data structure for oscillator nodes (`snex_osc`):
- `dyn<float> data` -- mono output buffer to fill
- `double uptime` -- phase accumulator (since voice start)
- `double delta` -- phase increment per sample (from samplerate * frequency * ratio)
- `operator++()` -- post-increment: returns current uptime, advances by delta

---

## 4. PrepareSpecs

Passed to every node's `prepare()` method. Contains the processing context:

```cpp
struct PrepareSpecs
{
    double sampleRate;       // May differ from host if oversampled
    int blockSize;           // Maximum block size (actual may be smaller)
    int numChannels;         // Number of audio channels
    PolyHandler* voiceIndex; // Pointer to polyphonic voice handler (or nullptr)
};
```

- `operator bool()` -- returns true if all fields are valid (channels > 0,
  sampleRate > 0, blockSize > 0)
- `withBlockSize(n)` / `withNumChannels(n)` -- create modified copies
  (used by container nodes to adjust specs for children)
- `blockSize == 1` indicates frame-based processing context; `withBlockSize()`
  preserves this when `overwriteIfFrame` is true

The `voiceIndex` pointer is critical for polyphonic nodes. It points to the
`PolyHandler` that manages voice state. Monophonic nodes receive nullptr here.

---

## 5. Polyphonic State: PolyData and PolyHandler

### PolyHandler

Manages which polyphonic voice is currently being processed. Thread-aware:
returns the voice index only when called from the audio thread during voice
rendering.

States:
- `voiceIndex == -1` -- no specific voice active (iteration covers all voices)
- `voiceIndex >= 0` -- specific voice being rendered (iteration covers one voice)
- `enabled == 0` -- polyphony disabled (always returns voice 0)

Scoped helpers:
- `ScopedVoiceSetter(handler, voiceIndex)` -- sets active voice for duration
- `ScopedAllVoiceSetter(handler)` -- marks current thread as "all voices" thread
  (e.g., UI thread). getVoiceIndex() returns -1 for this thread.
- `ScopedNoReset(handler, voiceIndex)` -- prevents reset() from firing for a
  specific voice (used during voice start to avoid killing envelopes)

Also holds:
- `VoiceResetter*` -- callback interface for voice reset notifications
- `DllBoundaryTempoSyncer*` -- tempo/transport sync across DLL boundary

### PolyData<T, NumVoices>

A container that stores one T per voice. The key abstraction for per-voice state.

```cpp
template <int NV> struct my_node
{
    PolyData<float, NV> gain;

    void prepare(PrepareSpecs ps) { gain.prepare(ps); }

    void reset()
    {
        for(auto& g : gain)  // iterates one voice or all voices
            g = 0.0f;
    }

    template <typename PD> void process(PD& data)
    {
        auto& g = gain.get();  // returns current voice's value
        // use g...
    }

    template <int P> void setParameter(double v)
    {
        for(auto& g : gain)  // from UI: all voices. from mod: one voice.
            g = (float)v;
    }
};
```

Key behaviors:
- `prepare(ps)` -- stores the PolyHandler pointer from PrepareSpecs
- `get()` -- returns reference to current voice's data
- `begin()/end()` -- iterates one voice (during rendering) or all voices
  (outside rendering / from UI thread)
- `all()` -- always iterates all voices regardless of context
- `isFirst()` -- true if current voice is voice 0 (useful for one-time init)
- When `NumVoices == 1`, the compiler eliminates all overhead -- identical
  machine code to using a plain T variable

NumVoices must be a power of two. Typical values: 1 (mono) or
NUM_POLYPHONIC_VOICES (default 256).

### polyphonic_base

Base class for polyphonic nodes. Constructor registers the node ID with two
CustomNodeProperties: `IsPolyphonic` and (by default) `IsProcessingHiseEvent`.

```cpp
struct polyphonic_base
{
    polyphonic_base(const Identifier& id, bool addProcessEventFlag=true);
};
```

### VoiceDataStack

Manages the mapping between voice indices and note-on events for polyphonic
processing. Handles:
- Starting voices with `startVoice()` (stores voice-to-event mapping, calls reset)
- Routing note-off, CC, pitchwheel, aftertouch to correct voices
- All-notes-off handling

---

## 6. Node Class Hierarchy

### mothernode

The root base class for all scriptnode nodes.

```cpp
struct mothernode
{
    virtual ~mothernode() {};
    void setDataProvider(DataProvider* dp);
    DataProvider* getDataProvider() const;
};
```

Provides the data provider link (for external data routing). The `isBaseOf<T>()`
and `getAsBase(T&)` static helpers enable type-safe downcasting through wrapper
layers.

### HiseDspBase

Base class for DSP node implementations. Inherits from `ParameterHolder`.

```cpp
class HiseDspBase : public ParameterHolder
{
    bool isPolyphonic() const { return false; }
    virtual void initialise(ObjectWithValueTree* n) {}
    virtual void createParameters(ParameterDataList& data) {}
};
```

- `initialise()` -- called when the node is connected to the value tree
  (gives access to properties, undo manager)
- `createParameters()` -- inherited from ParameterHolder, used to register
  parameters during OpaqueNode::create()

### SingleWrapper<T>

Wraps any class T into a HiseDspBase, forwarding `initialise()` and
`handleHiseEvent()`.

### OpaqueNode

The type-erased wrapper that all nodes are stored as at runtime. Uses C-style
function pointers for all callbacks, avoiding template bloat.

```
OpaqueNode
  - processFunc      -> process(ProcessDataDyn&)
  - monoFrame        -> processFrame(span<float,1>&)
  - stereoFrame      -> processFrame(span<float,2>&)
  - prepareFunc      -> prepare(PrepareSpecs)
  - resetFunc        -> reset()
  - eventFunc        -> handleHiseEvent(HiseEvent&)
  - externalDataFunc -> setExternalData(ExternalData&, int)
  - modFunc          -> handleModulation(double&)
  - initFunc         -> initialise(ObjectWithValueTree*)
```

Created via `OpaqueNode::create<T>()` which:
1. Allocates storage (inline buffer of 128 bytes, or heap for larger objects)
2. Wires all function pointers via `prototypes::static_wrappers<T>`
3. Queries compile-time traits: isPolyphonic, hasTail, isSuspendedOnSilence,
   getFixChannelAmount, isNormalisedModulation, isProcessingHiseEvent
4. Calls `createParameters()` to populate the parameter list
5. Sets up mothernode pointer if applicable

Important fields:
- `numParameters` -- number of registered parameters (max 16)
- `numChannels` -- fixed channel count (-1 if dynamic)
- `numDataObjects[5]` -- count of each ExternalData type
- `shouldProcessHiseEvent` -- whether MIDI events are forwarded
- `hasTail_` / `canBeSuspended_` -- tail/silence behavior flags
- `isNormalised` -- whether modulation output is 0..1

---

## 7. External Data: ExternalData and data::base

Nodes access complex HISE data structures (tables, slider packs, audio files,
filter coefficients, display buffers) through the ExternalData system.

### ExternalData

A lightweight struct that wraps a pointer to any complex data type:

```cpp
struct ExternalData
{
    enum class DataType : int
    {
        Table,              // Lookup table (512 floats)
        SliderPack,         // Resizable float array
        AudioFile,          // Multichannel audio with metadata
        FilterCoefficients, // For filter display
        DisplayBuffer,      // Ring buffer for visualizations
        numDataTypes
    };

    DataType dataType;
    int numSamples;         // Length of float array
    int numChannels;        // Usually 1 unless AudioFile
    int isXYZAudioData;     // True if multisample set
    void* data;             // Untyped pointer to actual data
    ComplexDataUIBase* obj;  // For UI updates and locking
    double sampleRate;       // For audio files
};
```

Key methods:
- `isEmpty()` / `isNotEmpty()` -- check if data is valid
- `referBlockTo(block&, channelIndex)` -- point a block at channel data
- `setDisplayedValue(double)` -- send playback position to UI
- `isXYZ()` -- true if pointing to a multisample set (XYZ = note/velocity/RR)

### data::base

Base class for nodes that use external data. Stores a copy of ExternalData:

```cpp
namespace data {
    struct base {
        ComplexDataUIBase* getUIPointer();
        virtual void setExternalData(const ExternalData& d, int index);
        void sendDisplayUpdateMessage(double v);
        ExternalData externalData;  // local copy
    };
}
```

### Specialized data base classes

- **`data::filter_base`** -- for filter nodes. Inherits `base` and
  `FilterDataObject::Broadcaster`. Requires implementing
  `getApproximateCoefficients()` for display.

- **`data::display_buffer_base<EnableBuffer>`** -- for nodes that write to a
  ring buffer for visualization. Template parameter enables/disables buffer
  writing. Manages `SimpleRingBuffer` ownership and writing:
  - `updateBuffer(double v, int numSamples)` -- write single value
  - `updateBuffer(float* values, int numSamples)` -- write array
  - `registerPropertyObject(rb)` -- register custom ring buffer properties
  - Handles prepare() to resize ring buffer to match channel count

### Data locking

- `DataReadLock(data::base*)` -- RAII read lock on the external data
- `DataTryReadLock(data::base*)` -- non-blocking try-read lock (use on audio thread)
- `DataWriteLock(data::base*)` -- RAII write lock (for data modification)
- All support `operator bool()` for scoped `if(auto lock = ...)` pattern

### ExternalDataHolder / ExternalDataProviderBase

- `ExternalDataHolder` -- interface for objects that own complex data slots.
  Provides `getTable(index)`, `getSliderPack(index)`, `getAudioFile(index)`,
  `getFilterData(index)`, `getDisplayBuffer(index)`.

- `ExternalDataProviderBase` -- interface for nodes that consume external data.
  Requires `getNumRequiredDataObjects(type)` and `setExternalData(data, index)`.

### Data routing templates

In namespace `data::external`:
- `table<Index>`, `sliderpack<Index>`, `audiofile<Index>`, `filter<Index>`,
  `displaybuffer<Index>` -- route external data slot at `Index` to the node

In namespace `data::embedded`:
- `table<DataClass>`, `sliderpack<DataClass>`, `audiofile<DataClass>` --
  embed data directly in the node (no external slot needed)

`data::matrix<MatrixType>` handles complex routing with multiple data slots
per type, including embedded data via index offset (>= 1000 = embedded).

---

## 8. Parameters: Definition and Connection

### parameter::data

The primary parameter descriptor, used at runtime:

```cpp
struct data {
    pod info;           // Lightweight POD with name, range, default
    dynamic callback;   // Function pointer to the setter
};
```

### parameter::pod

Plain-old-data struct holding parameter metadata:
- `char parameterName[32]` -- parameter name
- `char pageName[16]` -- optional page grouping
- `char subGroupName[16]` -- optional sub-group within page
- `float min, max, defaultValue, skew, interval` -- range specification
- `bool inverted` -- range inversion flag
- `TextValueConverters textConverter` -- display format enum:
  Undefined, Frequency, Time, TempoSync, Pan, NormalizedPercentage,
  Decibel, Semitones

### parameter::dynamic

Type-erased parameter callback. Stores a `void*` object pointer and a
`Function` (void(*)(void*, double)) pointer. Created from `parameter::inner<T,P>`
which binds to `T::setParameterStatic<P>()`.

### parameter::inner<T, P>

Compile-time binding of a parameter callback to object T, parameter index P.
Calls `T::setParameterStatic<P>(obj, value)`.

### InvertableParameterRange

Wraps `juce::NormalisableRange<double>` plus an `inv` (inversion) flag.
- `convertFrom0to1(input, applyInversion)` -- denormalize
- `convertTo0to1(input, applyInversion)` -- normalize
- Range fields: min, max, interval, skew, inverted

### How parameters are registered

Nodes implement `createParameters(ParameterDataList& data)`:

```cpp
void createParameters(ParameterDataList& data)
{
    parameter::data p("Frequency");
    p.setRange({20.0, 20000.0, 0.1, 0.3});  // min, max, interval, skew
    p.setDefaultValue(1000.0);
    registerCallback<(int)Parameters::Frequency>(p);  // binds to setParameterStatic<0>
    data.add(std::move(p));
}
```

The `DEFINE_PARAMETERDATA(ClassName, ParameterName)` macro simplifies this.

### Parameter encoding

Parameters can be serialized to a compact binary format via `parameter::encoder`.
The `SNEX_METADATA_ENCODED_PARAMETERS(N)` macro declares a `span<unsigned int, N>`
that stores encoded parameter data. This is used for C++ compiled nodes and
JIT-compiled SNEX nodes.

### PageInfo

Parameters can be organized into pages and sub-groups for UI layout:
- `PageInfo::Tree` builds a hierarchical structure from flat parameter lists
- Pages and groups are stored in the pod's `pageName` and `subGroupName` fields

---

## 9. CustomNodeProperties (cppProperties)

A global registry that associates compile-time node properties with node IDs.
Stored as a `SharedResourcePointer<Data>` (singleton pattern).

### How properties are registered

Nodes register properties in their constructors or via macros:

```cpp
// In constructor (e.g., polyphonic_base):
cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::IsPolyphonic);

// Via template helper:
CustomNodeProperties::setPropertyForObject(obj, PropertyIds::IsControlNode);
```

### Key property IDs

From `PropertyIds` namespace (node_ids.h):

| Property ID | Meaning |
|---|---|
| `IsPolyphonic` | Node supports polyphonic voice processing |
| `IsProcessingHiseEvent` | Node receives MIDI events |
| `IsControlNode` | Node is a control/modulation source, not signal |
| `HasModeTemplateArgument` | Node has a mode selector (template argument) |
| `IsCloneCableNode` | Node operates on clone cables |
| `IsRoutingNode` | Node handles signal routing |
| `IsFixRuntimeTarget` | Fixed runtime target connection |
| `IsDynamicRuntimeTarget` | Dynamic runtime target connection |
| `IsPublicMod` | Modulation output is publicly accessible |
| `UseUnnormalisedModulation` | Mod output is not 0..1 normalized |
| `AllowPolyphonic` | Polyphonic version exists |
| `AllowCompilation` | Can be compiled to C++ |
| `UncompileableNode` | Cannot be compiled |
| `HasTail` | Node produces output after input stops |
| `SuspendOnSilence` | Node can be suspended when input is silent |
| `UseRingBuffer` | Node uses a display ring buffer |
| `HasFixedParameters` | Parameters cannot be added/removed at runtime |
| `NeedsModConfig` | Needs modulation configuration |
| `IsOptionalSnexNode` | Optional SNEX implementation |
| `OutsideSignalPath` | Node is a cable/control, not in audio signal |

### Mode namespaces

Some nodes have a template mode argument. The mode namespace is registered via:
```cpp
CustomNodeProperties::setModeNamespace(nodeId, "modes::namespace");
```
Retrieved with `getModeNamespace(nodeId)`.

### Unscaled parameters

When a control node forwards values without range scaling:
```cpp
CustomNodeProperties::addUnscaledParameter(nodeId, "Value");
```
The UI uses this to detect range mismatches.

---

## 10. Node Macros

### Identity macros

- `SN_NODE_ID(id)` -- defines `getStaticId()` returning a JUCE Identifier
- `SN_POLY_NODE_ID(id)` -- adds `isPolyphonic()` based on template NumVoices

### Object access macros

- `SN_GET_SELF_AS_OBJECT(x)` -- for leaf nodes: getObject() and
  getWrappedObject() both return *this. Also defines ObjectType,
  WrappedObjectType, and SN_REGISTER_CALLBACK.
- `SN_SELF_AWARE_WRAPPER(x, ObjClass)` -- for wrappers that want to be visible
  as themselves but delegate getWrappedObject() to the inner obj.
- `SN_OPAQUE_WRAPPER(x, ObjClass)` -- for transparent wrappers that delegate
  both getObject() and getWrappedObject() to inner obj.

### Parameter macros

- `DEFINE_PARAMETERS` -- declares `setParameterStatic<P>(void*, double)`
- `DEF_PARAMETER(Name, Class)` -- if constexpr dispatch to `setName(value)`
- `DEFINE_PARAMETERDATA(Class, Name)` -- creates parameter::data and registers callback
- `SN_FORWARD_PARAMETER_TO_MEMBER(Class)` -- forwards setParameterStatic to
  setParameter<P>(value)
- `SN_ADD_SET_VALUE(Class)` -- shortcut for nodes with single "Value" parameter

### Empty callback macros

For nodes that don't need certain callbacks:
- `SN_EMPTY_PREPARE`, `SN_EMPTY_RESET`, `SN_EMPTY_PROCESS`,
  `SN_EMPTY_PROCESS_FRAME`, `SN_EMPTY_HANDLE_EVENT`,
  `SN_EMPTY_SET_EXTERNAL_DATA`, `SN_EMPTY_MOD`, `SN_EMPTY_INITIALISE`
- `SN_NO_PARAMETERS` -- combines SN_EMPTY_CREATE_PARAM + SN_EMPTY_SET_PARAMETER

### Default forwarding macros

For wrapper nodes that delegate to inner `obj`:
- `SN_DEFAULT_PREPARE`, `SN_DEFAULT_RESET`, `SN_DEFAULT_PROCESS`,
  `SN_DEFAULT_PROCESS_FRAME`, `SN_DEFAULT_HANDLE_EVENT`,
  `SN_DEFAULT_SET_EXTERNAL_DATA`, `SN_DEFAULT_MOD`, `SN_DEFAULT_INIT`
- These use `if constexpr` with `prototypes::check::` traits to safely
  forward only if the inner type actually implements the callback

### SNEX metadata macros

For compiled SNEX nodes:
- `SNEX_NODE(className)` -- all-in-one: Math member, self-as-object, node ID,
  parameter forwarding, empty initialise
- `SNEX_METADATA_ID(x)` -- static ID for metadata subclass
- `SNEX_METADATA_NUM_CHANNELS(x)` -- fixed channel count
- `SNEX_METADATA_ENCODED_PARAMETERS(N)` -- encoded parameter data literal

---

## 11. NodeProperty: Runtime-Configurable Properties

Node properties are non-realtime settings stored in the ValueTree and editable
in the scriptnode UI. They differ from parameters (which are realtime-safe).

### NodeProperty (base)

```cpp
struct NodeProperty
{
    NodeProperty(const Identifier& baseId, const var& defaultValue, bool isPublic);
    bool initialise(ObjectWithValueTree* o);
    virtual void postInit() = 0;
    Identifier getValueTreePropertyId() const;
};
```

- `initialise()` must be called both in the node's `initialise()` method AND
  in `createParameters()` with nullptr. This two-phase init handles the case
  where the property ID needs the node's value tree prefix (for hardcoded nodes).
- `isPublic` controls whether the property appears in the UI.
- The actual ValueTree property ID may be prefixed with the node ID when the
  node is embedded in a hardcoded container.

### NodePropertyT<T>

Typed subclass that manages a specific value type (int, String, bool):

```cpp
template <class T> struct NodePropertyT : public NodeProperty
{
    void storeValue(const T& newValue, UndoManager* um);
    T getValue() const;
    void setAdditionalCallback(const PropertyCallback& c);
};
```

- Listens for ValueTree property changes and updates the cached value
- `setAdditionalCallback()` allows nodes to react to property changes
- Explicit template instantiations exist for int, String, and bool

### StaticProperty<T, Value>

Compile-time constant property. `getValue()` always returns the template Value.
Used when a property is fixed at compile time but the API expects the property
interface.

### Common property IDs used by nodes

From PropertyIds (node_ids.h), properties commonly set on individual nodes:
- `Mode` -- operational mode selector (string)
- `BlockSize` -- sub-block processing size
- `SampleIndex` -- audio file sample index
- `File` -- file reference
- `FillMode` -- how data is filled
- `IsVertical` -- UI layout orientation
- `UseResetValue` / `ResetValue` -- voice reset behavior

---

## 12. Smoothed Values: sfloat and sdouble

Helper types for parameter smoothing to avoid zipper artifacts:

```cpp
sfloat gain;   // single precision
sdouble freq;  // double precision

void prepare(PrepareSpecs ps)
{
    gain.prepare(ps.sampleRate, 20.0);  // 20ms smoothing time
}

void process(...)
{
    while(...)
    {
        auto g = gain.advance();  // get current value, step forward
        sample *= g;
    }
}

// In parameter callback:
gain.set(newValue);  // starts ramp to new value
gain.reset();        // jump to target immediately
```

- `prepare(sampleRate, timeMs)` -- calculate ramp step count
- `set(target)` -- start ramp to new target value
- `advance()` -- return current value and step
- `get()` -- return current value without stepping
- `isActive()` -- true if still ramping
- `reset()` -- jump to target value immediately

---

## 13. ModValue

Used by `wrap::mod` nodes to communicate modulation output:

```cpp
struct ModValue
{
    void setModValue(double v);              // always set + flag
    bool setModValueIfChanged(double v);     // set only if different
    bool getChangedValue(double& d);         // consume: clears flag
    double getModValue() const;
    void reset();

    int changed = false;   // flag (int for atomic safety)
    float modValue = 0.0f; // current value
};
```

- `setModValue()` -- for periodic modulation (always flags as changed)
- `setModValueIfChanged()` -- for event-driven modulation (avoids redundant updates)
- `getChangedValue()` -- used in `handleModulation()` callback: returns true
  and fills value if changed, then clears the flag

---

## 14. Tempo and Transport

### DllBoundaryTempoSyncer

Bridges tempo/transport information across DLL boundaries. Implements
`TempoListener` and maintains a list of registered listeners.

- `tempoChanged(bpm)` -- broadcast BPM changes
- `onTransportChange(isPlaying, ppqPosition)` -- broadcast play state
- `onResync(ppqPos)` -- broadcast position resync
- `registerItem(listener)` / `deregisterItem(listener)` -- manage listeners
- `getCurrentPPQPosition(timestamp)` -- get PPQ at sample offset (uses
  ppqFunction if available for sample-accurate position)
- Also holds pointers to: `publicModValue` (for public_mod nodes),
  `additionalEventStorage`, `uuidManager`, `uuidRequestFunction`

Accessed via `PolyHandler::getTempoSyncer()`.
