# DspNetwork -- Project Context

## Project Context

### Real-World Use Cases
- **Effect plugin hosting**: A ScriptFX module calls `Engine.createDspNetwork()` to load a DSP graph that processes audio. The graph is built visually in the scriptnode editor and stored as XML. The script contains only the one-line creation call with empty `prepareToPlay`/`processBlock` callbacks - the hosting processor handles audio routing automatically.
- **Synthesizer DSP layer**: A polyphonic synth uses DspNetwork graphs for oscillator processing, filter chains, and modulation routing. Multiple networks may coexist (e.g., 25 networks in a wavetable synth covering filters, delays, and resonators), each loaded by a separate scriptnode processor in the module tree.
- **Compiled production pipeline**: Networks authored visually at design time are compiled to C++ via `StaticLibraryHostFactory` for release performance. The DspNetwork XML serves as the authoring format; the compiled `HardcodedMasterFX`/`HardcodedPolyphonicFX` replaces the interpreted graph at export. This eliminates the performance argument for writing C++ from scratch.
- **Custom C++ DSP integration**: ThirdParty C++ nodes (written as header files in `DspNetworks/ThirdParty/`) are loaded into scriptnode graphs alongside stock nodes. The DspNetwork graph provides routing and parameter control while C++ handles algorithms with no stock equivalent (granular engines, analog filter models, FDN reverbs).

### Complexity Tiers
1. **Hosting** (most common): Call `Engine.createDspNetwork("id")` in a ScriptFX processor, build the graph visually, leave callback stubs empty. This is how the vast majority of production plugins use DspNetwork.
2. **Parameter forwarding control**: Use `setForwardControlsToParameters()` to choose whether UI controls drive network parameters directly (for DAW automation) or route through script callbacks (for custom control logic). The default (`true`) is correct for most use cases.
3. **Programmatic graph construction**: Use `create()`, `createAndAdd()`, and `createFromJSON()` to build node trees from script. This is available for advanced scenarios like dynamically generated signal chains but is not used in typical production workflows where the visual editor is preferred.

### Practical Defaults
- Use the visual scriptnode editor to build graphs rather than programmatic node creation. The XML-based workflow provides immediate visual feedback and is how all production plugins are built.
- Leave `prepareToPlay` and `processBlock` callbacks empty when hosting a network in a ScriptFX processor. The hosting processor handles audio routing automatically.
- Keep `setForwardControlsToParameters(true)` (the default). This makes root node parameters visible to DAW automation. Only disable it when you need custom script-side parameter processing.
- Use `GlobalCable` nodes within the scriptnode graph to bridge data between HiseScript and the DSP network, rather than trying to call DspNetwork methods from audio callbacks.

### Integration Patterns
- `Engine.createDspNetwork()` -> ScriptFX/PolyScriptFX processor -- the network is hosted by a scriptnode-capable processor module in the HISE module tree. The processor type determines mono vs. polyphonic processing.
- `DspNetwork` root parameters -> DAW automation -- when forward controls is enabled, parameters defined on the root `container.chain` node are exposed to the host as automatable parameters.
- `DspNetwork` XML -> `StaticLibraryHostFactory` -> `HardcodedMasterFX` -- the compilation pipeline converts visual graphs to C++ for production performance without changing the authoring workflow.
- `GlobalCable`/`routing.global_cable` nodes -- the primary bridge between HiseScript logic and scriptnode DSP. Script sets cable values; nodes inside the graph receive them. This replaces direct DspNetwork method calls for runtime parameter control.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Writing custom `processBlock` logic to manually feed audio into a hosted network | Leave `processBlock` empty and let the ScriptFX processor handle audio routing | The hosting processor automatically connects the network to the audio stream. Manual `processBlock` is only needed for standalone buffer processing outside a ScriptFX context. |
| Creating nodes programmatically when the graph is static | Use the visual scriptnode editor and save as XML | Programmatic creation adds complexity without benefit for fixed signal chains. The visual editor provides immediate feedback and the XML format supports compilation to C++. |
| Using `setForwardControlsToParameters(false)` without a clear reason | Keep the default (`true`) for DAW automation support | Disabling parameter forwarding breaks DAW automation and requires manual parameter bridging through script callbacks. |
