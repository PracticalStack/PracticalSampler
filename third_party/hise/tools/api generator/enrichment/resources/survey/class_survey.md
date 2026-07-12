# HISE Scripting API Class Survey

87 scripting API classes analyzed across 6 groups.
Generated from scripting_api.json + C++ source declarations.

## Enrichment Prerequisites

Conceptual dependencies where understanding class A materially
improves the documentation of class B. These are NOT mere factory
relationships (Engine.get*() is not a prerequisite) -- they are cases
where A introduces concepts, conventions, or patterns that B's
documentation needs to reference.

| # | Enrich First | Then Enrich | Rationale |
|---|-------------|-------------|-----------|
| 1 | **GlobalRoutingManager** | GlobalCable | OSC address conventions, C++ interop, cable topology |
| 2 | **Content** | All ScriptComponent subclasses | Property system (set/get), component lifecycle, event model |
| 3 | **Synth** | Effect, Modulator, MidiProcessor, ChildSynth, SlotFX, AudioSampleProcessor, TableProcessor, RoutingMatrix | Module tree model, voice system, setAttribute/getAttribute pattern |
| 4 | **Synth** | Builder | Builder constructs what Synth.get*() looks up -- same module tree, different access pattern |
| 5 | **Sampler** | Sample, ComplexGroupManager | Sample map structure, region model, RR groups |
| 6 | **ExpansionHandler** | Expansion | Pack lifecycle, installation, credential model |
| 7 | **DspNetwork** | Node, Parameter, Connection | Graph model, node creation/lookup, processing chain |
| 8 | **ScriptPanel** | Graphics, ScriptShader | Graphics only exists inside paint callback; ScriptShader is set on a panel |
| 9 | **UserPresetHandler** | MacroHandler, MidiAutomationHandler | Preset save/load lifecycle -- macros and MIDI automation are preset state components |
| 10 | **FixObjectFactory** | FixObjectArray, FixObjectStack | Layout definition model -- factory defines the typed schema |
| 11 | **Server** | Download | HTTP request lifecycle -- Download is a handle to an in-progress request |
| 12 | **FileSystem** | File | Folder model and constants that File operations assume |
| 13 | **Table** | TableProcessor | Lookup table data model that TableProcessor wraps |
| 14 | **SliderPackData** | ScriptSliderPack | Slider array data model that the UI component displays |
| 15 | **ScriptLookAndFeel** | ScriptComponent subclasses (soft) | LAF paint functions reference component properties |
| 16 | **Rectangle** | Graphics | Layout slicing primitives (removeFromTop, reduced, etc.) that Graphics area parameters consume |

## Enrichment Groups

Classes organized by domain. Within each group, respect the
prerequisites above. Groups can be enriched in any order.

### module-tree (20 classes)
Audio engine, module tree navigation, and processor handles

| Class | Role | Base Class | Brief |
|-------|------|-----------|-------|
| AudioSampleProcessor | handle | ConstScriptingObject | Script handle to a module that plays a single audio file (AudioLoopPlayer, etc.), providing file loa |
| Builder | factory | ConstScriptingObject | Programmatic module tree construction tool that creates synths, effects, and modulators by type/ID,  |
| ChildSynth | handle | ConstScriptingObject | Script handle to a child sound generator module (synth, sampler, etc.) within a SynthGroup or SynthC |
| ComplexGroupManager | handle | ConstScriptingObject | Advanced multi-layer round-robin group controller for Samplers that provides per-layer/per-group vol |
| Effect | handle | ConstScriptingObject | Script handle to an audio effect module in the signal chain (filter, reverb, delay, etc.) that provi |
| Engine | factory | ScriptingObject, ApiClass | Central factory and utility class that provides object creation (Broadcasters, Timers, MidiLists, Mo |
| Message | event | ScriptingObject, ApiClass | Transient reference to the current MIDI event inside onNoteOn/onNoteOff/onController callbacks, prov |
| MessageHolder | container | ConstScriptingObject | Persistent, copyable container for a MIDI event that can be stored across callbacks, used in arrays  |
| MidiPlayer | handle | MidiPlayerBaseType, ConstScriptingObject, SuspendableTimer | Script handle to a MIDI Player module that provides MIDI file loading, sequence playback control (pl |
| MidiProcessor | handle | ConstScriptingObject, AssignableObject | Generic script handle to any MIDI processor module in the signal chain, providing attribute get/set, |
| Modulator | handle | ConstScriptingObject, AssignableObject | Script handle to a modulator in the signal chain (LFO, envelope, constant, etc.) that provides attri |
| RoutingMatrix | handle | ConstScriptingObject | Script handle to a processor's channel routing matrix that manages source-to-destination channel con |
| Sample | handle | ConstScriptingObject, AssignableObject | Handle to a single sampler sound within a sample map, allowing get/set of per-sample properties (roo |
| Sampler | handle | ConstScriptingObject | Script handle to a ModulatorSampler module that provides sample map loading/saving, round-robin grou |
| ScriptModulationMatrix | service | ConstScriptingObject, ControlledObject, UserPresetStateManager | Dynamic modulation routing system that manages many-to-many connections between global modulator sou |
| Settings | service | ApiClass, ScriptingObject | Audio device and standalone application settings manager that provides get/set access to audio drive |
| SlotFX | handle | ConstScriptingObject | Script handle to a dynamic effect slot that can load and swap different effect types at runtime, pro |
| Synth | processor | ScriptingObject, ApiClass | Script handle to the parent synthesiser that provides MIDI note generation, voice control (pitch/vol |
| TableProcessor | handle | ConstScriptingObject | Script handle to any module that contains lookup tables (table envelopes, table modulators, etc.), p |
| WavetableController | handle | ConstScriptingObject, ControlledObject, WeakErrorHandler | Script handle to a WavetableSynth module that provides wavetable loading from audio files or buffers |

### ui (23 classes)
UI components, rendering, and visual tools

| Class | Role | Base Class | Brief |
|-------|------|-----------|-------|
| Colours | utility | ApiClass | Static utility namespace for colour manipulation -- converting between ARGB/HSL/vec4 formats, mixing |
| ContainerChild | component | ConstScriptingObject | A reference to a child component inside a ScriptDynamicContainer, providing property access, hierarc |
| Content | factory | ScriptingObject | Top-level factory for creating and managing all UI components on the script interface, including lay |
| Graphics | utility | ConstScriptingObject | 2D drawing context passed to paint routines for rendering shapes, text, images, paths, gradients, la |
| MarkdownRenderer | utility | ConstScriptingObject | Markdown text renderer that parses markdown syntax into styled text with headings, lists, and inline |
| Path | utility | ConstScriptingObject | Vector path object for defining reusable 2D shapes (arcs, bezier curves, polygons, stars, etc.) that |
| Rectangle | utility | RectangleDynamicObject | Mutable rectangle utility for layout calculations with slicing (removeFromTop/Left/Bottom/Right), re |
| ScriptAudioWaveform | component | ComplexDataScriptComponent | Audio waveform display component for visualizing and interacting with audio samples, connected to Au |
| ScriptButton | component | ScriptComponent | Toggle or momentary button component with filmstrip/image support, radio group assignment, and popup |
| ScriptComboBox | component | ScriptComponent | Drop-down list component for selecting from a set of named items, with dynamic item addition and tex |
| ScriptDynamicContainer | container | ScriptComponent | Container component that dynamically creates and manages child components from a data model, support |
| ScriptFloatingTile | component | ScriptComponent | Wrapper component that embeds pre-built HISE floating tile widgets (keyboard, preset browser, wavefo |
| ScriptImage | component | ScriptComponent | Static image display component for showing PNG/filmstrip images with offset and alpha control, witho |
| ScriptLabel | component | ScriptComponent | Text display and input component for showing labels, editable text fields, and text overlays on the  |
| ScriptLookAndFeel | service | ConstScriptingObject | Customizable look-and-feel object for overriding the rendering of standard UI components (buttons, s |
| ScriptMultipageDialog | component | ScriptComponent | Multi-page wizard/dialog component for building guided workflows with pages, navigation, modal overl |
| ScriptPanel | component | ScriptComponent | Scriptable panel component with custom paint routines, mouse/touch interaction, timer callbacks, ani |
| ScriptShader | utility | ConstScriptingObject | OpenGL fragment shader wrapper for GPU-accelerated visual effects, supporting uniform variables, pre |
| ScriptSlider | component | ScriptComponent | Numeric value slider/knob component with configurable range, step size, mid-point skew, multiple dis |
| ScriptSliderPack | component | ComplexDataScriptComponent | Multi-slider array component for editing a pack of discrete values, commonly used for step sequencer |
| ScriptTable | component | ComplexDataScriptComponent | Curve editor component backed by a Table data object, used for shaping envelopes, velocity curves, a |
| ScriptWebView | component | ScriptComponent | Embedded web browser component for rendering HTML/CSS/JS content within the plugin interface, with b |
| ScriptedViewport | component | ScriptComponent | Scrollable viewport component that doubles as a sortable data table with configurable columns, row d |

### data (16 classes)
Containers, complex data references, and DSP utilities

| Class | Role | Base Class | Brief |
|-------|------|-----------|-------|
| Array | container | juce::var (built-in JavaScript engine type) | Dynamic-size general-purpose container that holds any type of element (numbers, strings, objects, ne |
| AudioFile | handle | ScriptComplexDataReferenceBase (ConstScriptingObject, ComplexDataUIUpdaterBase::EventListener) | Scriptable reference to an audio file slot in a processor, providing file loading, sample data acces |
| Buffer | container | VariantBuffer (built-in JavaScript engine type) | Fixed-size float array optimized for audio sample data, supporting DSP operations like normalization |
| DisplayBuffer | handle | ScriptComplexDataReferenceBase (ConstScriptingObject, ComplexDataUIUpdaterBase::EventListener) | Scriptable reference to a processor's ring buffer, used for real-time waveform/spectrum visualizatio |
| FFT | processor | ConstScriptingObject | Fast Fourier Transform processor that analyses audio buffers into frequency-domain magnitude/phase d |
| FixObjectArray | container | fixobj::Array (LayoutBase, AssignableObject, ObjectWithJSONConverter, ConstScriptingObject) | Fixed-size array of typed objects with named properties and contiguous memory layout, created via Fi |
| FixObjectFactory | factory | fixobj::Factory (LayoutBase, ConstScriptingObject) | Factory that defines a typed memory layout from a JSON prototype and creates FixObjectArray, FixObje |
| FixObjectStack | container | fixobj::Stack (inherits fixobj::Array) | Variable-occupancy stack of typed objects (up to a fixed capacity) with insert/remove operations, cr |
| LorisManager | processor | ConstScriptingObject | Interface to the Loris partial-tracking library for analysis, manipulation, and resynthesis of audio |
| MidiList | container | ConstScriptingObject | Exactly 128 integer slots (indexed 0-127) for storing per-note values like velocity curves, key swit |
| NeuralNetwork | processor | ConstScriptingObject | Machine-learning inference engine supporting PyTorch, TensorFlow, NAM, and ONNX models for real-time |
| SliderPackData | handle | ScriptComplexDataReferenceBase (ConstScriptingObject, ComplexDataUIUpdaterBase::EventListener) | Scriptable reference to a slider pack's data array (multiple discrete float values), used to control |
| String | utility | juce::String (built-in JavaScript engine type) | Built-in string type with methods for searching, splitting, transforming, encrypting, and parsing te |
| Table | handle | ScriptComplexDataReferenceBase (ConstScriptingObject, ComplexDataUIUpdaterBase::EventListener) | Scriptable reference to a lookup table (curve editor data) used by modulators and effects, supportin |
| ThreadSafeStorage | container | ConstScriptingObject | Lock-based container for safely passing arbitrary data (JSON, arrays, objects) between the audio thr |
| UnorderedStack | container | ConstScriptingObject | Fast insert/remove container for up to 128 float numbers or HISE events, without preserving order -- |

### event (8 classes)
Asynchronous events, callbacks, timing, and routing

| Class | Role | Base Class | Brief |
|-------|------|-----------|-------|
| BackgroundTask | handle | ConstScriptingObject | A task handle for running long operations (downloads, processing, OS processes) on a background thre |
| Broadcaster | event | ConstScriptingObject | A flexible event router that attaches to component values, properties, module parameters, complex da |
| ErrorHandler | service | ConstScriptingObject | Intercepts system-level error events (missing samples, audio dropouts, license issues) and routes th |
| GlobalCable | handle | ConstScriptingObject | A named value bus for routing normalised or ranged values between scripts, modules, macro controls,  |
| GlobalRoutingManager | factory | ConstScriptingObject | A singleton manager for the global cable routing system that creates and retrieves named GlobalCable |
| Threads | utility | ApiClass | Provides thread-awareness utilities for querying thread identity, lock state, and safely executing f |
| Timer | utility | ConstScriptingObject | A periodic timer that fires a callback on the message thread at a configurable millisecond interval, |
| TransportHandler | event | ConstScriptingObject | Registers callbacks for host transport events including tempo changes, play/stop state, time signatu |

### scriptnode (6 classes)
ScriptNode DSP graph system

| Class | Role | Base Class | Brief |
|-------|------|-----------|-------|
| Connection | handle | ConstScriptingObject | A modulation or signal connection between a source node and a target parameter within a ScriptNode g |
| DspModule | processor | ConstScriptingObject | Legacy API for loading and controlling an external DSP module instance from a DspFactory, predating  |
| DspNetwork | container | ConstScriptingObject | The top-level container that manages a ScriptNode DSP graph, providing methods to create, retrieve,  |
| NetworkTest | utility | ConstScriptingObject | A test harness for verifying ScriptNode DSP network output by running processing with configurable s |
| Node | processor | ConstScriptingObject | A processing unit within a ScriptNode DSP network that can be connected, bypassed, and configured wi |
| Parameter | handle | ConstScriptingObject | A named parameter of a ScriptNode Node that holds a value with range properties and can receive modu |

### services (14 classes)
File I/O, networking, preset model, and standalone utilities

| Class | Role | Base Class | Brief |
|-------|------|-----------|-------|
| BeatportManager | service | ConstScriptingObject | Beatport DRM integration service for validating product access through the Beatport SDK. |
| Console | utility | ApiClass | Debugging utility for printing messages, benchmarking, assertions, and automated callback testing. |
| Date | utility | ApiClass | Date/time utility providing system time access and ISO-8601 string conversion. |
| Download | handle | ConstScriptingObject | Handle to an active or completed file download with progress tracking, pause/resume, and abort capab |
| Expansion | handle | ConstScriptingObject | Handle to a single installed expansion pack, providing access to its sample maps, audio files, image |
| ExpansionHandler | factory | ConstScriptingObject | Factory/manager for loading, installing, and managing expansion packs with encryption and credential |
| File | handle | ConstScriptingObject | Handle to a specific file or directory on disk, with methods for reading, writing, and manipulating  |
| FileSystem | factory | ApiClass | Namespace providing access to special folder locations, file browsing dialogs, and RSA encryption ut |
| MacroHandler | service | ConstScriptingObject | Service for reading and writing macro control connection data with change notification callbacks. |
| Math | utility | ApiClass | Standard math library with trigonometric, logarithmic, rounding, clamping, and range conversion func |
| MidiAutomationHandler | service | ConstScriptingObject | Service for managing MIDI CC-to-parameter automation mappings with popup configuration and change ca |
| Server | service | ApiClass | HTTP client for GET/POST requests and file downloads, with queue management and server activity call |
| Unlocker | service | ConstScriptingObject | RSA key-based product license manager for registration validation, key file handling, and expansion  |
| UserPresetHandler | service | ConstScriptingObject | Service for managing user preset load/save lifecycle, custom automation, host parameter mapping, and |

## Factory Relationships (createdBy)

Mechanically derived from method return types. Most Engine.create*()
and Engine.get*() relationships are trivial factory links, not
conceptual dependencies.

- **Array** <- String
- **AudioFile** <- AudioSampleProcessor, Engine
- **AudioSampleProcessor** <- Builder, Synth
- **BackgroundTask** <- Engine
- **BeatportManager** <- Engine
- **Broadcaster** <- Engine
- **Buffer** <- Buffer, DisplayBuffer, SliderPackData, UnorderedStack
- **Builder** <- Synth
- **ChildSynth** <- Builder, ChildSynth, Synth
- **ComplexGroupManager** <- Sampler
- **Connection** <- Node, Parameter
- **ContainerChild** <- ContainerChild
- **DisplayBuffer** <- Engine
- **Download** <- Server
- **DspNetwork** <- Engine
- **Effect** <- Builder, SlotFX, Synth
- **ErrorHandler** <- Engine
- **Expansion** <- ExpansionHandler
- **ExpansionHandler** <- Engine
- **FFT** <- Engine
- **File** <- Download, Expansion, File, FileSystem, Unlocker
- **FixObjectArray** <- FixObjectFactory
- **FixObjectFactory** <- Engine
- **FixObjectStack** <- FixObjectFactory
- **GlobalCable** <- GlobalRoutingManager
- **GlobalRoutingManager** <- Engine
- **MacroHandler** <- Engine
- **MarkdownRenderer** <- Content
- **MessageHolder** <- Engine
- **MidiAutomationHandler** <- Engine
- **MidiList** <- Engine
- **MidiPlayer** <- MidiProcessor, Synth
- **MidiProcessor** <- Builder, MidiPlayer, Synth
- **Modulator** <- Builder, ChildSynth, Effect, Modulator, Synth
- **NetworkTest** <- DspNetwork
- **NeuralNetwork** <- Engine
- **Node** <- DspNetwork
- **Parameter** <- Node
- **Path** <- Content
- **RoutingMatrix** <- ChildSynth, Synth
- **Sample** <- Sample, Sampler
- **Sampler** <- Builder, ChildSynth, Synth
- **ScriptAudioWaveform** <- Content
- **ScriptButton** <- Content
- **ScriptComboBox** <- Content
- **ScriptDynamicContainer** <- Content
- **ScriptFloatingTile** <- Content
- **ScriptImage** <- Content
- **ScriptLabel** <- Content
- **ScriptLookAndFeel** <- Content
- **ScriptModulationMatrix** <- Engine
- **ScriptMultipageDialog** <- Content
- **ScriptPanel** <- Content
- **ScriptShader** <- Content
- **ScriptSlider** <- Content
- **ScriptSliderPack** <- Content
- **ScriptTable** <- Content
- **ScriptWebView** <- Content
- **ScriptedViewport** <- Content
- **SliderPackData** <- Engine
- **SlotFX** <- Builder, Synth
- **Table** <- Engine, TableProcessor
- **TableProcessor** <- Builder, Synth
- **Timer** <- Engine
- **TransportHandler** <- Engine
- **UnorderedStack** <- Engine
- **UserPresetHandler** <- Engine
- **WavetableController** <- Synth
