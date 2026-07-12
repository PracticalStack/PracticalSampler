# core.snex_node - C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/snex_nodes/SnexNode.h:50`
**Base class:** `SnexSource`
**Classification:** audio_processor

## Signal Path

The snex_node is a generic wrapper that delegates all audio processing to user-written SNEX (JIT-compiled C++) code. The node itself performs no DSP -- it resolves SNEX callbacks at compile time and forwards all scriptnode callbacks to the JIT-compiled function pointers via `NodeCallbacks`.

The flow is: SNEX source code -> JIT compilation -> function pointer resolution -> all scriptnode callbacks (prepare, reset, process, processFrame, handleHiseEvent) forwarded to JIT code.

Processing uses `ProcessDataDyn` (dynamic channel count). The `process(ProcessData<C>&)` template overload asserts false -- only the dynamic variant is used at runtime.

## Gap Answers

### callback-set

Confirmed from `recompiledOk()`: **five required callbacks** must all be resolved or compilation fails:
1. `prepare(PrepareSpecs ps)`
2. `reset()`
3. `handleHiseEvent(HiseEvent& e)`
4. `process(ProcessDataDyn& data)` (or templated ProcessData)
5. `processFrame(span<float, C>& data)`

**Three optional callbacks** are checked separately:
- `handleModulation(double& value)` -- returns int (1=changed, 0=not). Only active if resolved.
- `setExternalData(const ExternalData& d, int index)` -- managed by SnexSource base class.
- `getPlotValue(int getMagnitude, double freqNormalised)` -- returns double. Requires the SNEX class to derive from `data::filter_node_base` and call `SNEX_INIT_FILTER`.

### parameter-discovery

Parameters are defined by the SNEX code via the `setParameter<P>(double v)` template method pattern. Parameter discovery uses the `ParameterHelpers` system which detects `setXxx(double)` named methods. The maximum parameter count is `OpaqueNode::NumMaxParameters` = 16. The node itself declares `SN_EMPTY_CREATE_PARAM` -- no parameters are registered from the C++ side. All parameter definitions come from the SNEX code's metadata.

### complex-data-dynamic

Yes, complex data slots are dynamically determined by the SNEX code. The base JSON correctly shows zero slots because the C++ wrapper declares no fixed data requirements. The SNEX code can request external data by implementing `setExternalData(const ExternalData& d, int index)`. The actual slot counts are discovered when the SNEX code is compiled and its `getNumRequiredDataObjects()` is queried via the `ExternalDataProviderBase` interface.

### modulation-output

Yes, snex_node supports modulation output. The `isNormalisedModulation()` returns true, confirming the output is normalised to 0..1. The `handleModulation()` method checks if `modDefined` is true (set during compilation if `handleModulation` is found in the SNEX code) and forwards to the JIT function. The SNEX function signature is `int handleModulation(double& value)` -- return 1 if changed, 0 if not. The editor shows a modulation dragger when `modDefined` is true.

### polyphony-limitation

Confirmed. `isPolyphonic()` returns `false` (constexpr). The snex_node class is not templated on voice count and does not inherit from `polyphonic_base`. PolyData cannot be used in snex_node SNEX code because the monophonic context provides no voice indexing. For polyphonic SNEX use cases, `snex_osc` supports polyphony via its NV template parameter.

## Parameters

No fixed parameters. All parameters are defined dynamically by the user's SNEX code.

## Conditional Behaviour

- **modDefined**: When the SNEX code defines `handleModulation()`, the node acts as a modulation source and shows a mod dragger in the UI.
- **plotDefined**: When the SNEX code defines `getPlotValue()` and derives from `data::filter_node_base`, filter frequency response display is enabled.
- **isProcessingHiseEvent** is always true (constexpr) -- MIDI events are always forwarded to the SNEX code's `handleHiseEvent()`.

## CPU Assessment

baseline: variable (depends entirely on user SNEX code)
polyphonic: false
scalingFactors: []

The wrapper overhead is negligible (function pointer calls with read lock). Actual CPU depends on the user's SNEX implementation.

## Notes

The `preprocess()` method exists for code preprocessing before compilation. The `rebuildCallbacksAfterChannelChange()` call in `prepare()` handles channel count changes by recompiling if needed. Thread safety is managed via `SimpleReadWriteLock` -- the `ScopedCallbackChecker` RAII pattern acquires a read lock before calling any JIT function, while recompilation acquires a write lock.
