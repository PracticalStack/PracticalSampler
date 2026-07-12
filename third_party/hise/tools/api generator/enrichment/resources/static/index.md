---
title: Scripting API
---

The HISE Scripting API provides 87 classes across six functional domains. Every class is available as a global object or is created by a factory method - there is no `new` keyword in HiseScript.

Classes are tagged by **domain group** (what area of the engine they belong to) and **role** (what kind of object they are). Click any tag on a class page to jump to its category below.

## Domain Groups

An overview of all scripting API classes grouped by functional domain.

---

### Module Tree

Audio engine, module tree navigation, and processor handles.

- [AudioSampleProcessor]($API.AudioSampleProcessor$): Script handle to a module that plays a single audio file
- [Builder]($API.Builder$): Programmatic module tree construction tool
- [ChildSynth]($API.ChildSynth$): Script handle to a child sound generator module
- [ComplexGroupManager]($API.ComplexGroupManager$): Advanced multi-layer round-robin group controller
- [Effect]($API.Effect$): Script handle to an audio effect module
- [Engine]($API.Engine$): Central factory and utility namespace for object creation and system queries
- [Message]($API.Message$): Transient reference to the current MIDI event in callbacks
- [MessageHolder]($API.MessageHolder$): Persistent, copyable container for a MIDI event
- [MidiPlayer]($API.MidiPlayer$): Script handle to a MIDI Player module
- [MidiProcessor]($API.MidiProcessor$): Script handle to any MIDI processor module
- [Modulator]($API.Modulator$): Script handle to a modulator in the signal chain
- [RoutingMatrix]($API.RoutingMatrix$): Script handle to a processor's channel routing matrix
- [Sample]($API.Sample$): Handle to a single sampler sound within a sample map
- [Sampler]($API.Sampler$): Script handle to a ModulatorSampler module
- [ScriptModulationMatrix]($API.ScriptModulationMatrix$): Dynamic modulation routing system
- [Settings]($API.Settings$): Audio device and standalone application settings manager
- [SlotFX]($API.SlotFX$): Script handle to a dynamic effect slot
- [Synth]($API.Synth$): Script handle to the parent synthesiser
- [TableProcessor]($API.TableProcessor$): Script handle to a module that contains lookup tables
- [WavetableController]($API.WavetableController$): Script handle to a WavetableSynth module

---

### UI

UI components, rendering, and visual tools.

- [Colours]($API.Colours$): Colour manipulation utilities
- [Content]($API.Content$): Top-level factory for creating and managing UI components
- [Graphics]($API.Graphics$): 2D drawing context for paint routines
- [MarkdownRenderer]($API.MarkdownRenderer$): Markdown text renderer for styled text display
- [Path]($API.Path$): Vector path object for defining reusable 2D shapes
- [Rectangle]($API.Rectangle$): Mutable rectangle utility for layout calculations
- [ScriptAudioWaveform]($API.ScriptAudioWaveform$): Audio waveform display component
- [ScriptButton]($API.ScriptButton$): Toggle or momentary button component
- [ScriptComboBox]($API.ScriptComboBox$): Drop-down list component
- [ScriptDynamicContainer]($API.ScriptDynamicContainer$): Container that dynamically creates child components from a data model
- [ScriptFloatingTile]($API.ScriptFloatingTile$): Wrapper for pre-built HISE floating tile widgets
- [ScriptImage]($API.ScriptImage$): Static image display component
- [ScriptLabel]($API.ScriptLabel$): Text display and input component
- [ScriptLookAndFeel]($API.ScriptLookAndFeel$): Customizable look-and-feel for overriding component rendering
- [ScriptMultipageDialog]($API.ScriptMultipageDialog$): Multi-page wizard/dialog component
- [ScriptPanel]($API.ScriptPanel$): Scriptable panel with custom paint, mouse, and timer callbacks
- [ScriptShader]($API.ScriptShader$): OpenGL fragment shader wrapper
- [ScriptSlider]($API.ScriptSlider$): Numeric value slider/knob component
- [ScriptSliderPack]($API.ScriptSliderPack$): Multi-slider array component
- [ScriptTable]($API.ScriptTable$): Curve editor component
- [ScriptWebView]($API.ScriptWebView$): Embedded web browser component
- [ScriptedViewport]($API.ScriptedViewport$): Scrollable viewport and data table component
- [ContainerChild]($API.ContainerChild$): Reference to a child component inside a ScriptDynamicContainer

---

### Scriptnode

ScriptNode DSP graph system.

- [Connection]($API.Connection$): Modulation or signal connection between nodes
- [DspModule]($API.DspModule$): Legacy API for external DSP module instances
- [DspNetwork]($API.DspNetwork$): Top-level container for a ScriptNode DSP graph
- [NetworkTest]($API.NetworkTest$): Test harness for verifying DSP network output
- [Node]($API.Node$): Processing unit within a ScriptNode network
- [Parameter]($API.Parameter$): Named parameter of a ScriptNode Node

---

### Services

File I/O, networking, preset model, and standalone utilities.

- [BeatportManager]($API.BeatportManager$): Beatport DRM integration service
- [Console]($API.Console$): Debugging utility for printing and benchmarking
- [Date]($API.Date$): Date/time utility
- [Download]($API.Download$): Handle to an active or completed file download
- [Expansion]($API.Expansion$): Handle to a single installed expansion pack
- [ExpansionHandler]($API.ExpansionHandler$): Factory for loading and managing expansion packs
- [File]($API.File$): Handle to a file or directory on disk
- [FileSystem]($API.FileSystem$): Special folder locations and file browsing dialogs
- [MacroHandler]($API.MacroHandler$): Macro control connection data manager
- [Math]($API.Math$): Standard math library
- [MidiAutomationHandler]($API.MidiAutomationHandler$): MIDI CC-to-parameter automation manager
- [Server]($API.Server$): HTTP client for requests and downloads
- [Unlocker]($API.Unlocker$): RSA key-based product license manager
- [UserPresetHandler]($API.UserPresetHandler$): User preset load/save lifecycle manager

---

## Roles

Classes are also tagged by their architectural role, describing what kind of object they are regardless of domain.

---

### Handle

Script handle to a module or resource managed elsewhere. These objects are obtained via factory methods and provide get/set access to the underlying engine object.

---

### Factory

Creates and manages other API objects. Factories are typically singletons or global namespaces that produce handles, components, or containers.

---

### Container

Stores and organizes data or child elements. Containers hold collections of values, events, or typed objects with various access patterns.

---

### Component

Visual UI component on the script interface. Components are created by `Content.add*()` and support properties, values, and look-and-feel customization.

---

### Utility

Stateless helper functions and conversions. Utilities provide math operations, colour manipulation, path drawing, string processing, and similar tools.

---

### Service

Singleton manager for a subsystem. Services handle preset management, automation, licensing, error handling, and other cross-cutting concerns.

---

### Processor

Performs DSP, analysis, or event processing. Processors transform audio data, run neural networks, or handle MIDI event streams.

---

### Event

Routes or dispatches asynchronous events. Event objects connect sources to targets through structured messages with change detection.
