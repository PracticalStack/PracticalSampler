---
title: Script FX
moduleId: ScriptFX
type: Effect
subtype: MasterEffect
tags: [custom]
builderPath: b.Effects.ScriptFX
screenshot: /images/v2/reference/audio-modules/scriptfx.png
cpuProfile:
  baseline: "(depends on loaded network)"
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: HardcodedMasterFX, type: alternative, reason: "Compiled variant for exported plugins - loads the same networks but without the script editor" }
  - { id: PolyScriptFX, type: alternative, reason: "Per-voice effect variant for polyphonic processing" }
  - { id: ScriptnodeVoiceKiller, type: companion, reason: "Required for voice management when using scriptnode envelopes alongside this module" }
commonMistakes:
  - title: "Network not compiled before export"
    wrong: "Exporting a plugin with an uncompiled scriptnode network in the ScriptFX"
    right: "Compile the network to a DLL before exporting, or switch to a HardcodedMasterFX"
    explanation: "Scriptnode networks must be compiled to C++ and included in the project DLL for exported plugins. Uncompiled networks will not work outside the HISE IDE."
  - title: "Mixing up script and network mode"
    wrong: "Writing processBlock script code and also loading a network"
    right: "Use one mode or the other - a loaded network always takes priority over script callbacks"
    explanation: "When a network is loaded, the processBlock HISEScript callback is ignored entirely. Remove unused script code to avoid confusion."
  - title: "Forgetting to enable extra mod chains"
    wrong: "Trying to add modulation chains to ScriptFX without setting the preprocessor definition"
    right: "Add HISE_NUM_SCRIPTNODE_FX_MODS=N to the Extra Definitions field in the project settings"
    explanation: "By default, ScriptFX has zero extra modulation chain slots. You must set the HISE_NUM_SCRIPTNODE_FX_MODS preprocessor macro to the desired count before compilation."
  - title: "Saved network XML crashes compiled plugin"
    wrong: "Leaving the scriptnode .xml network file in the project when exporting a compiled plugin"
    right: "Remove or do not save the network XML before export so only the compiled C++ code is active"
    explanation: "Including a saved .xml scriptnode network file alongside compiled C++ code can cause the plugin to crash on load in a DAW."
forumReferences:
  - id: 1
    title: "HISEScript processBlock mode is legacy and will be removed"
    summary: "Christoph Hart confirmed the HISEScript processBlock callback approach is deprecated; all new DSP work should use scriptnode networks."
    topic: 5813
  - id: 2
    title: "HISEScript per-sample DSP is at least 10x slower than scriptnode"
    summary: "Processing audio sample-by-sample via HISEScript is far more expensive than the equivalent logic in a scriptnode network or SNEX node."
    topic: 8672
  - id: 6
    title: "Swap ScriptFX and HardcodedMasterFX at compile time via preprocessor define"
    summary: "Maintain both module variants in the tree and use a #define flag to bypass the inactive set, enabling a clean development-to-export workflow."
    topic: 14343
llmRef: |
  Script FX (Effect/MasterEffect)

  Monophonic master effect that processes stereo audio through a scriptnode DSP network. Also supports HISEScript processBlock callbacks as a fallback when no network is loaded. All parameters are dynamic - they come from the loaded network.

  Signal flow:
    audio in (L/R) -> [network loaded?] -> ExtraModChains modulate network params -> network process -> audio out (L/R)
                                        -> script processBlock -> audio out (L/R)

  CPU: depends on loaded network, monophonic
    Framework overhead is negligible.

  Parameters:
    All parameters are dynamic and come from the loaded scriptnode network.
    No fixed parameters (offset = 0).

  Modulation chains:
    Extra Modulation Chains (default 0, configurable via HISE_NUM_SCRIPTNODE_FX_MODS) - see Audio Modules modulators reference for connection mode details

  Channel configuration:
    Standard scriptnode channel configuration (default stereo, up to 16). See Audio Modules sound-generators reference for details.

  Complex data types and parameter exposure:
    No fixed parameters (offset = 0). See Audio Modules index Custom section for complex data types, parameter exposure, and configuration table.

  When to use:
    Use ScriptFX to host a custom effect built in scriptnode during development. For exported plugins, switch to HardcodedMasterFX with the compiled network.

  Common mistakes:
    Forgetting to compile the network before export.
    Having both script and network code - network always takes priority.
    Forgetting to set HISE_NUM_SCRIPTNODE_FX_MODS when needing extra mod slots.
    Leaving .xml network file in the project when exporting crashes the compiled plugin.

  Tips:
    HISEScript processBlock mode is legacy and will be removed - use scriptnode networks instead.
    Only top-level network parameters (macro knobs) are addressable via processorId/parameterId.
    Use a preprocessor define to swap between ScriptFX (development) and HardcodedMasterFX (export).

  See also:
    alternative HardcodedMasterFX - compiled variant for exported plugins
    alternative PolyScriptFX - per-voice effect variant
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules with user-defined signal paths via scriptnode networks or HISEScript callbacks" }
---
::

![Script FX screenshot](/images/v2/reference/audio-modules/scriptfx.png)

Script FX hosts a scriptnode DSP network as a monophonic master effect. It processes the full stereo output bus, making it suitable for insert effects like EQ, compression, distortion, or reverb built in scriptnode. Parameters declared in the network are automatically exposed to the module tree and can be controlled from HISEScript or connected to UI components.

The module also supports a legacy HISEScript mode with `prepareToPlay` and `processBlock` callbacks for simple audio processing without a scriptnode network. When a network is loaded, it always takes priority over the script callbacks.

## Signal Path

::signal-path
---
glossary:
  functions:
    loadNetwork:
      desc: "Loads a scriptnode DSP network from the project's DspNetworks folder"
    networkProcess:
      desc: "Processes the audio buffer through the scriptnode network's root node"
    scriptProcess:
      desc: "Fallback: processes audio through the HISEScript processBlock callback"
  modulations:
    ExtraModChains:
      desc: "Extra modulation chain slots that drive extra_mod nodes inside the network. Default count is 0, configurable via HISE_NUM_SCRIPTNODE_FX_MODS"
      scope: "monophonic"
---

```
// Script FX - scriptnode master effect container
// audio L/R in -> audio L/R out

process(left, right) {
    if (networkLoaded) {
        // Extra modulation chains drive network parameters
        networkParams *= ExtraModChains

        // Primary path: process through scriptnode network
        networkProcess(left, right)
    } else {
        // Fallback: HISEScript processBlock callback
        scriptProcess(left, right)
    }
}
```

::

### Loading a Network

Create a scriptnode network in the `onInit` callback by calling `Engine.createDspNetwork("NetworkName")`. This creates a new network or loads an existing one from the `DspNetworks/Networks/` folder, where networks are stored as `.xml` files.

The network operates in monophonic mode — it receives the full mixed audio bus and processes it as a single stream. Use monophonic containers in the network; polyphonic containers are not applicable here.

Switching networks at runtime is possible by calling `Engine.createDspNetwork()` again with a different name. However, this is a heavyweight operation that reinitialises the entire DSP graph and should not be done during audio playback. For preset-style switching, expose network parameters and change those instead. Alternatively, use the `SlotFX` interface for managed effect switching.

### Modulation Chain Configuration

This module has no built-in Gain or Pitch modulation chains. Extra modulation slots are controlled by `HISE_NUM_SCRIPTNODE_FX_MODS` (default: 0). See [Scriptnode Modulation Bridge](/v2/reference/audio-modules/modulators/#scriptnode-modulation-bridge) for how extra modulation slots connect to network parameters.

### Channel Configuration

This module supports the standard scriptnode channel configuration. See [Scriptnode and Hardcoded Module Channels](/v2/reference/audio-modules/sound-generators/#scriptnode-and-hardcoded-module-channels).

### Parameter Exposure and Complex Data

This module has no fixed parameters — all network parameters start at index 0. See [Custom module hosting](/v2/reference/audio-modules/#custom) for parameter exposure, complex data types, and the configuration table.

### Legacy Script Mode

The HISEScript `processBlock` mode is legacy and will be removed in a future version [1]($FORUM_REF.5813$). All new DSP work should use scriptnode networks. Processing audio on a per-sample basis via HISEScript is at least ten times slower than the equivalent logic in a scriptnode network or SNEX node [2]($FORUM_REF.8672$).

### Connecting Interface Controls to Network Parameters

Only parameters exposed at the top level of the network (via the macro knob section) are available in the processorId/parameterId picker [3]($FORUM_REF.13585$) [4]($FORUM_REF.12743$). Internal node parameters are not directly addressable from the Interface script. When scripted UI components are present in a Script FX interface, they connect to the top-level network parameters in the order the parameters were added [5]($FORUM_REF.12743$).

### Development-to-Export Workflow

A common pattern is to maintain both a Script FX (for development) and a HardcodedMasterFX (for export) version of each effect in the module tree, then use a preprocessor define to bypass the inactive set via `setBypassed()` after initialisation [6]($FORUM_REF.14343$). This enables a clean development-to-export workflow without restructuring the project.

**See also:** $MODULES.HardcodedMasterFX$ -- compiled variant for exported plugins, $MODULES.PolyScriptFX$ -- per-voice effect variant for polyphonic processing
