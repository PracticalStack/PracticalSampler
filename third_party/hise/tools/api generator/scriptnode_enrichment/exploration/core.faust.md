# core.faust - C++ Exploration

**Source:** `hi_faust_jit/FaustJitNode.h:13` (base class), `hi_faust_jit/FaustJitNode.h:120` (template)
**Base class:** `faust_jit_node_base` (inherits `WrapperNode`, `FaustManager::FaustListener`)
**Classification:** audio_processor

## Signal Path

The faust node compiles Faust DSP code at runtime (JIT or interpreter) and delegates all audio processing to the compiled Faust DSP instance. The signal flow is:

1. Faust source code is compiled via LLVM JIT (`HISE_FAUST_USE_LLVM_JIT`) or Faust interpreter.
2. A `faust_jit_wrapper<NV>` wraps one `::faust::dsp` instance per voice.
3. In `process()`, input audio is copied to a separate buffer (Faust may not support in-place processing), then `fdsp->compute(nFrames, inputs, outputs)` is called.
4. The Faust UI system (`faust_ui`) maps Faust sliders/buttons/bargraphs to scriptnode parameters and modulation outputs.

The `faust_base_wrapper::process()` method handles the actual Faust compute call: it copies input channels to an internal buffer, then calls `compute()` with separate input/output pointers. Channel count validation ensures Faust output channels match HISE channels, and Faust input channels can be fewer than HISE channels.

## Gap Answers

### faust-integration

Faust code is **compiled at runtime**. Two backends are supported:
- **LLVM JIT** (`HISE_FAUST_USE_LLVM_JIT`): Uses `::faust::createDSPFactoryFromString()` to compile Faust code to native machine code via LLVM.
- **Interpreter** (default): Uses `::faust::createInterpreterDSPFactoryFromString()` for interpreted execution.

The signal flow: Faust source -> factory creation -> DSP instance creation per voice -> `buildUserInterface()` maps parameters -> `init(sampleRate)` -> `compute()` called each block.

The `faust_jit_node_base` manages the source code, file I/O, class ID property, and parameter synchronisation. The templated `faust_jit_node<NV>` implements all processing callbacks by forwarding to the `faust_jit_wrapper<NV>`.

### faust-parameters

Yes, parameters are **defined dynamically by the Faust code**. When Faust code is compiled, `buildUserInterface()` is called, populating `faust_ui::parameters` with UI elements (sliders, nentry, button). These are then mapped to scriptnode parameters in `setupFaustParameters()`:

1. Each Faust parameter creates a `parameter::data` with the label and range from the Faust UI element.
2. A `parameter::dynamic_base_holder` wraps each parameter to allow safe reconnection during recompilation.
3. Parameter values are stored in the ValueTree and survive recompilation.
4. The Faust zone pointers are connected to the parameter callbacks so that scriptnode parameter changes directly update the Faust DSP's control values.

Modulation outputs are also dynamic: `hbargraph` and `vbargraph` Faust elements become modulation outputs via `faust_ui::modoutputs` and `parameter::dynamic_list`.

### faust-description-missing

The base JSON has an empty description. Based on the class comment and functionality, the correct description is: "A Faust JIT node that compiles and runs Faust DSP code." (The `SN_NODE_ID("faust")` is in the base class `faust_jit_node_base`.)

### faust-polyphony

The node **supports polyphony** via the NV template parameter. `faust_jit_node<NV>` inherits from `faust_jit_node_base` which inherits from `WrapperNode`. The `faust_base_wrapper` stores `PolyData<::faust::dsp*, NumVoices> faustDsp` -- one Faust DSP instance per voice. The `faust_ui` handles MIDI events: `handleHiseEvent()` is forwarded to the UI which manages MIDI zones (`anyMidiZonesActive` flag). The `isProcessingHiseEvent()` method returns `faust->ui.anyMidiZonesActive` -- it is dynamic based on whether the Faust code defines MIDI zones. External data is not directly supported through the standard scriptnode ComplexData system -- data is managed by the Faust runtime.

### faust-compilation

Faust support is conditional on HISE being built with the Faust module (`hi_faust_jit` or `hi_faust`). The `HISE_INCLUDE_FAUST` preprocessor flag controls availability. The Faust compiler libraries must be available at build time (either the interpreter library or LLVM). The `FaustVersionChecker` verifies minimum Faust version 2.74.6. Faust library include paths are resolved via `getFaustLibraryPaths()`. No external Faust installation is needed at runtime if the interpreter backend is used; LLVM backend requires LLVM libraries.

## Parameters

No fixed parameters. All parameters are dynamically created from the Faust code's UI elements (sliders, buttons, nentry). Modulation outputs come from bargraph elements. The `classId` NodeProperty selects the Faust source file.

## Conditional Behaviour

- **Bypass**: All processing callbacks check `isBypassed()` and return early if true.
- **MIDI processing**: `isProcessingHiseEvent()` returns dynamically based on whether the Faust code defines MIDI zones (`anyMidiZonesActive`).
- **Modulation output**: `isUsingNormalisation()` returns true when `getNumFaustModulationOutputs() > 0` (bargraph elements present in the Faust code).
- **Channel validation**: If Faust output channel count does not match HISE channel count, an `IllegalFaustChannelCount` error is thrown.

## Polyphonic Behaviour

Templated with NV for voice count. Each voice gets its own `::faust::dsp` instance. The `faust_ui` handles per-voice zone pointer updates via `ScopedZoneSetter`. MIDI events are forwarded to the Faust UI for voice management.

## CPU Assessment

baseline: variable (depends entirely on the Faust code)
polyphonic: true
scalingFactors:
  - parameter: NumVoices, impact: high, note: "Each voice instantiates a full Faust DSP object"
  - parameter: FaustCodeComplexity, impact: high, note: "CPU depends on the Faust algorithm"

The wrapper overhead includes: input buffer copy (Faust may not support in-place), read lock acquisition, and channel validation. The actual DSP cost is determined by the Faust code.

## Notes

The `faust_jit_node_base` is a `WrapperNode` (inherits `NodeBase`) rather than a `HiseDspBase`, which means it has access to the full DspNetwork infrastructure including the value tree. The `compileFaustCode()` virtual method handles file-based compilation. The `classId` is stored as a `NodePropertyT<String>` and triggers recompilation on change. Thread safety is managed via `SimpleReadWriteLock jitLock` -- processing acquires a read lock, compilation acquires a write lock.
