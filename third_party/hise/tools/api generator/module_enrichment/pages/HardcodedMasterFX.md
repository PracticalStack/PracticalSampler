---
title: Hardcoded Master FX
moduleId: HardcodedMasterFX
type: Effect
subtype: MasterEffect
tags: [custom]
builderPath: b.Effects.HardcodedMasterFX
screenshot: /images/v2/reference/audio-modules/hardcodedmasterfx.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors:
    - { parameter: "Loaded network", impact: "variable", note: "CPU cost depends entirely on the compiled scriptnode network" }
seeAlso:
  - { id: ScriptFX, type: alternative, reason: "Loads interpreted XML scriptnode networks instead of compiled C++ code" }
  - { id: HardcodedPolyphonicFX, type: alternative, reason: "Per-voice compiled effect for polyphonic processing" }
  - { id: SlotFX, type: disambiguation, reason: "Swaps between existing HISE effects; Hardcoded Master FX loads compiled scriptnode networks" }
  - { id: "LANG.cpp-dsp-nodes", type: guide, reason: "Complete callback interface and worked examples for writing custom C++ DSP nodes" }
forumReferences:
  - id: 1
    title: "restoreState() loads network and parameter values in one call"
    summary: "Rather than calling setEffect() and setting parameters individually, use exportState()/restoreState() to capture and restore the full module state including the loaded network name."
    topic: 7701
  - id: 2
    title: "getModuleList() and setEffect() require Synth.getSlotFX()"
    summary: "A reference obtained via Synth.getEffect() does not expose setEffect() or getModuleList(); use Synth.getSlotFX() for those and Synth.getEffect() only for exportState()/restoreState()."
    topic: 12498
  - id: 3
    title: "AllowCompilation must be enabled per-network before DLL build"
    summary: "Each scriptnode network must have AllowCompilation enabled individually; after building, use Tools > Show DLL info to confirm registration, and ensure the build configuration matches the HISE build."
    topic: 5911
commonMistakes:
  - title: "Missing compiled DLL"
    wrong: "Adding a Hardcoded Master FX without first compiling the scriptnode networks"
    right: "Use Export > Compile DSP Networks as DLL before loading a network in the module"
    explanation: "The network selector will be empty and the module will show 'No DLL loaded' until the project's scriptnode networks are compiled."
  - title: "Channel count mismatch"
    wrong: "Loading a stereo network when the routing matrix has a different number of active channels"
    right: "Ensure the routing matrix channel count matches the compiled network's channel count"
    explanation: "A channel mismatch disables processing and shows an error. Adjust the routing matrix to match the network."
  - title: "Using reserved parameter names in the network"
    wrong: "Naming a network parameter 'Bypassed', 'Type', 'ID', or 'Network'"
    right: "Choose parameter names that do not collide with HISE's reserved property names"
    explanation: "Reserved names cause a conflict with internal module properties and the module will show an error."
  - title: "DLL API version mismatch after HISE update"
    wrong: "Updating HISE and expecting existing compiled DLLs to continue working"
    right: "Recompile your DSP networks after every HISE update"
    explanation: "The internal API between HISE and the compiled DLL can change between HISE versions. A stale DLL will fail to load or cause undefined behaviour."
  - title: "Using Synth.getEffect() instead of Synth.getSlotFX()"
    wrong: "Calling setEffect() or getModuleList() on a reference obtained via Synth.getEffect()"
    right: "Use Synth.getSlotFX() for setEffect() and getModuleList(); use Synth.getEffect() only for exportState()/restoreState()"
    explanation: "Synth.getEffect() does not expose the slot-swapping API. Only Synth.getSlotFX() provides setEffect() and getModuleList()."
  - title: "getCurrentEffectId() returns the module ID, not the loaded network"
    wrong: "Expecting getCurrentEffectId() to return the name of the currently loaded scriptnode network"
    right: "Track the loaded network name externally or use exportState() to capture the full module state"
    explanation: "getCurrentEffectId() returns the module's own ID string, not the name of the compiled network currently loaded into it."
  - title: "Nesting inside a SlotFX"
    wrong: "Placing a Hardcoded Master FX inside a SlotFX module"
    right: "Use the Hardcoded Master FX directly - it already has built-in slot-swapping functionality"
    explanation: "Hardcoded Master FX is itself a slot-type module. Wrapping it in a SlotFX creates a double-slot arrangement that causes unexpected behaviour."
  - title: "UI knob range does not match network parameter range"
    wrong: "Setting a custom filmstrip knob range that differs from the parameter range declared in the scriptnode network"
    right: "Ensure the UI knob's min/max range exactly matches the parameter range in the compiled network"
    explanation: "A range mismatch causes the filmstrip to display incorrect positions even though the underlying value is set correctly."
llmRef: |
  Hardcoded Master FX (Effect/MasterEffect)

  Runs a compiled C++ scriptnode network or custom C++ DSP node as a monophonic master effect. All parameters, tables, slider packs, and audio files are defined by the loaded network.

  Signal flow:
    audio in -> [compiled C++ processing] -> audio out

  Modulation: extra slots only (NUM_HARDCODED_FX_MODS, default 0), no built-in Gain or Pitch chains. See modulators parent page.
  Channels: fixed at compile time, must match routing matrix. See sound-generators parent page.
  Parameters: all from network, offset 0. See index parent page.
  Complex data: slot counts baked at compile time. See index parent page.
  Custom C++ nodes: see cpp-dsp-nodes language guide.

  CPU: negligible framework overhead, actual cost depends on loaded network, monophonic.

  Common mistakes:
    Must compile DSP networks as DLL before the module can load anything.
    Channel count between routing matrix and network must match.
    Avoid reserved parameter names (Bypassed, Type, ID, Network).
    Must recompile DLL after HISE updates.
    Use Synth.getSlotFX() for setEffect()/getModuleList(); Synth.getEffect() does not expose slot API.
    getCurrentEffectId() returns the module ID, not the loaded network name.
    Do not nest inside a SlotFX - the module already has built-in slot-swapping.
    UI knob ranges must exactly match network parameter ranges for correct filmstrip display.

  See also:
    alternative ScriptFX - loads interpreted XML scriptnode networks
    alternative HardcodedPolyphonicFX - per-voice compiled effect
    disambiguation SlotFX - swaps between stock HISE effects, not compiled networks
    guide cpp-dsp-nodes - C++ DSP node callback interface and examples
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules that run user-defined DSP logic via scriptnode networks, compiled C++ code, or HiseScript callbacks" }
---
::

![Hardcoded Master FX screenshot](/images/v2/reference/audio-modules/hardcodedmasterfx.png)

The Hardcoded Master FX runs compiled C++ code as a monophonic master effect. It can load compiled scriptnode networks (visual graphs exported to C++) or custom C++ DSP nodes (hand-written algorithms), both delivered through the same DLL compilation pipeline. Unlike $MODULES.ScriptFX$ which interprets an XML scriptnode network at runtime, this module loads pre-compiled code for better performance.

The module has no fixed parameters of its own. When a compiled network or custom node is loaded, its parameters, tables, slider packs, and audio files appear automatically in the module's interface. Modulation chains can be connected to network parameters, allowing the HISE module tree to automate the compiled network's controls.

## Signal Path

::signal-path
---
glossary:
  functions:
    selectNetwork:
      desc: "Choose which compiled C++ scriptnode network to load from the project"
    processNetwork:
      desc: "Run the compiled network's audio processing on the input buffer"
  modulations:
    ExtraModChains:
      desc: "Additional modulation chains driving compiled network parameters at control rate (downsampled by factor of 8)"
      scope: "monophonic"
---

```
// Hardcoded Master FX - compiled scriptnode network as master effect
// audio L/R in -> audio L/R out

process(left, right) {
    // Select the compiled network from the project DLL
    network = selectNetwork("NetworkName")

    // Network parameters are set from the UI or via scripting
    // Extra mod chains modulate network parameters per block chunk
    networkParams *= ExtraModChains

    // Process audio through the compiled C++ network
    processNetwork(left, right)
}
```

::

### What Can Be Loaded

The Hardcoded Master FX can load two kinds of content, both delivered through the same DLL compilation pipeline:

**Compiled scriptnode networks** are visual graphs designed in the scriptnode editor, then exported to C++. This is the standard workflow for most projects: design and iterate visually, then compile for production performance. The compilation step collapses the entire node graph into a single optimised function, eliminating per-node overhead.

**Custom C++ DSP nodes** are hand-written algorithms placed in `DspNetworks/ThirdParty/*.h` files, following the scriptnode node callback interface. This is the path for effect algorithms with no stock equivalent. See [Custom C++ Nodes](#custom-c-nodes) below.

Both types use the same internal dispatch mechanism and appear in the same module dropdown. Note that $MODULES.ScriptFX$ (interpreted) can also host custom C++ nodes via the project DLL — the difference is that the Hardcoded Master FX eliminates the XML interpretation overhead on top.

### Compiled vs Interpreted

The Hardcoded Master FX and $MODULES.ScriptFX$ both host scriptnode networks, but they load them differently. ScriptFX loads the network from an XML file and interprets the node graph at runtime, creating individual objects for each node. The Hardcoded Master FX loads the same network as compiled C++ code, where the entire graph has been flattened into a single optimised function. The network design workflow is identical — you build the network in HISE's scriptnode editor either way.

### Loading a Compiled Network

Compile your scriptnode networks or custom C++ nodes using **Export > Compile DSP Networks as DLL**. Then select the network from the module's dropdown — network parameters, tables, slider packs, and audio files appear automatically based on what the compiled code declares.

Custom C++ nodes from the `DspNetworks/ThirdParty/` folder appear in the same dropdown alongside compiled scriptnode networks. Both load identically from the module's perspective.

During development, the DLL hot-loads when recompiled — restart HISE or recompile to pick up changes. In exported plugins, compiled networks and custom nodes are built directly into the binary (no DLL needed).

### Modulation Chain Configuration

This module has no built-in Gain or Pitch modulation chains. Extra modulation slots are configured via `NUM_HARDCODED_FX_MODS` (default: 0). See [Scriptnode Modulation Bridge](/v2/reference/audio-modules/modulators/#scriptnode-modulation-bridge) for how extra modulation slots connect to network parameters.

### Channel Configuration

This module supports the standard scriptnode channel configuration. Channel count is fixed at compile time and must match the routing matrix — a mismatch disables processing and shows an error. See [Scriptnode and Hardcoded Module Channels](/v2/reference/audio-modules/sound-generators/#scriptnode-and-hardcoded-module-channels).

### Parameter Exposure and Complex Data

All parameters come from the compiled network, starting at index 0. Parameter names must not collide with reserved names (Bypassed, Type, ID, Network). For custom C++ nodes, parameters are registered via the `createParameters()` callback. Complex data slot counts are baked into the compiled code at compile time. See [Custom module hosting](/v2/reference/audio-modules/#custom) for the full parameter and complex data reference.

### Custom C++ Nodes

This module can load custom C++ DSP nodes from `DspNetworks/ThirdParty/*.h` in addition to compiled scriptnode networks. See the [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) guide for the complete callback interface, workflow, and worked examples.

### Export Workflow

There are two workflows depending on what you are loading:

**Compiled scriptnode network**: Design your effect network in the scriptnode editor. When ready for production, use **Export > Compile DSP Networks as DLL** to compile it. Add a Hardcoded Master FX module to your signal chain, select the network from the dropdown, and configure the routing matrix. On plugin export, the network is baked directly into the binary.

**Custom C++ node**: Write your `.h` file in `DspNetworks/ThirdParty/`. Compile the DLL — the node auto-loads in HISE and appears in the module dropdown. On plugin export, the C++ code is compiled directly into the binary alongside the rest of the plugin.

In both cases, the exported plugin contains no DLL and no XML — everything runs as native compiled code.

### Restoring State in a Single Call

Rather than calling `setEffect()` followed by individual parameter assignments, you can capture the entire module state with `exportState()` (or via **Edit > Create Base64 encoded state**) and restore it in one call with `restoreState()`. The Base64 string encodes both the loaded network name and all parameter values. [1]($FORUM_REF.7701$) [2]($FORUM_REF.12498$)

### Enabling Networks for Compilation

Each scriptnode network that should appear in the Hardcoded Master FX dropdown must have **AllowCompilation** enabled individually before the DLL build step. After building and restarting HISE, use **Tools > Show DLL info** to verify the compiled networks are registered. The build configuration (Debug/Release) must also match the HISE build -- a mismatch results in 'No DLL loaded'. [3]($FORUM_REF.5911$)

**See also:** $MODULES.ScriptFX$ -- loads interpreted XML scriptnode networks instead of compiled C++ code, $MODULES.HardcodedPolyphonicFX$ -- per-voice compiled effect for polyphonic processing, $MODULES.SlotFX$ -- swaps between existing HISE effects rather than compiled networks, [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) -- complete callback interface and worked examples for writing custom C++ DSP nodes
