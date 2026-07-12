# Buffer - Phase 1A C++ Exploration

## Consulted resource files
- `enrichment/resources/survey/class_survey.md`
- `enrichment/resources/survey/class_survey_data.json`

No prerequisite class context entry applies for Buffer in the Enrichment Prerequisites table.

## Primary C++ locations

### API-facing wrapper declaration
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:171`
  - `class ScriptingObjects::ScriptBuffer : public ConstScriptingObject`
  - Declares Buffer API methods for documentation / API class metadata.

### Runtime implementation of Buffer behavior
- `HISE/hi_tools/hi_tools/VariantBuffer.h:74`
  - `class VariantBuffer : public DynamicObject`
  - Actual data container and method host used by scripting runtime.
- `HISE/hi_tools/hi_tools/VariantBuffer.cpp:172`
  - `VariantBuffer::addMethods()` installs all callable Buffer methods with `setMethod(...)`.

### Var runtime type integration
- `HISE/customized_JUCE_files/juce_Variant.h:32`
  - Forward declaration and API (`var(VariantBuffer*)`, `isBuffer()`, `getBuffer()`).
- `HISE/customized_JUCE_files/juce_Variant.cpp:309`
  - `var::VariantType_Buffer` implementation.

### Factory registration into script globals
- `HISE/hi_scripting/scripting/ScriptProcessorModules.cpp`
  - `registerNativeObject("Buffer", new VariantBuffer::Factory(64));`
  - Appears in multiple script processor types (MidiProcessor, PolyphonicEffect, MasterEffect, TimeVariantModulator, EnvelopeModulator, Synthesiser).

## Architectural split: ScriptBuffer vs VariantBuffer

Buffer has a dual-layer architecture:

1) API class shell (`ScriptBuffer`)
- Declared in `ScriptingApiObjects.h` with method signatures and object name `"Buffer"`.
- Constructor in `ScriptingApiObjects.cpp:7233` is currently a stub with `jassertfalse`.
- No method registration (`ADD_API_METHOD_*`) in `ScriptBuffer` constructor.

2) Runtime object (`VariantBuffer`)
- Inherits `DynamicObject` and is wrapped as a custom `juce::var` type (`VariantType_Buffer`).
- Real callable methods are injected dynamically in `VariantBuffer::addMethods()` using `setMethod`.
- This means Buffer method behavior is defined in `hi_tools`, not in `ScriptingApiObjects.cpp` wrappers.

Implication for downstream method analysis:
- Do not expect `ADD_API_METHOD_*` metadata for Buffer methods.
- Type forcing via `ADD_TYPED_API_METHOD_*` does not exist for Buffer.
- Method parameter constraints come from `var::NativeFunctionArgs` checks and conversions inside lambdas.

## Inheritance and type chain

### Script-side type identity
- `ScriptBuffer` base: `ConstScriptingObject` (API identity layer).
- `VariantBuffer` base: `DynamicObject` (actual callable object).
- `juce::var` has dedicated buffer type (`VariantType_Buffer`) with:
  - `isBuffer() == true`
  - `getBuffer()` cast path to `VariantBuffer*`

### Data ownership model
- `VariantBuffer` can own data (`VariantBuffer(int samples)`) or reference external data (`VariantBuffer(float* externalData, int size)` / `referToData(...)` / `referToOtherBuffer(...)`).
- `referencedBuffer` keeps source lifetime alive when a slice/reference is created.

## Constructor and registration patterns

## ScriptBuffer constructor
- `ScriptingObjects::ScriptBuffer::ScriptBuffer(ProcessorWithScriptingContent* p, int size)`
  - Calls `ConstScriptingObject(p, 0)`
  - Immediately `jassertfalse`.
  - No constants.
  - No `ADD_API_METHOD_*`.

## VariantBuffer constructors
- `VariantBuffer(float* externalData, int size_)`
  - Refers to external memory if non-null.
  - Calls `addMethods()`.
- `VariantBuffer(VariantBuffer* otherBuffer, int offset, int numSamples)`
  - References region of another buffer.
  - Calls `addMethods()`.
- `VariantBuffer(int samples)`
  - Allocates 1-channel internal `AudioSampleBuffer`, zeroes content.
  - Calls `addMethods()`.

## VariantBuffer::Factory
- `Factory(int stackSize_)` preallocates `stackSize` zero-length buffers into `sectionBufferStack`.
- Exposes methods:
  - `create(size)` -> owning buffer.
  - `referTo(buffer, offset?, numSamples?)` -> reference buffer pulled from preallocated pool.
- Pool reuse condition (`getFreeVariantBuffer()`): only returns entries with refcount `== 2`.
- Failure path: throws `"Buffer stack size reached!"` when no reusable slot is available.

## Factory / obtainedVia topology

## Primary acquisition path
- Global native object registration:
  - `scriptEngine->registerNativeObject("Buffer", new VariantBuffer::Factory(64));`
- Practical creation forms:
  - `Buffer.create(numSamples)`
  - `Buffer.referTo(sourceBuffer, offset, length)`

## Secondary acquisition paths (class_survey_data createdBy)
- `DisplayBuffer` (`ScriptRingBuffer`) -> returns Buffer via:
  - `getReadBuffer()` -> wraps ring buffer channel pointer in `VariantBuffer`.
  - `getResizedBuffer(...)` -> new `VariantBuffer(numDestSamples)`.
- `SliderPackData` -> `getDataAsBuffer()` returns underlying `SliderPackData::dataBuffer` as var.
- `UnorderedStack` -> `asBuffer(getAllElements)` returns
  - `elementBuffer` (references active stack elements)
  - or `wholeBf` (128-slot full backing view).
- `Buffer` itself -> `referTo(...)` and methods returning new buffers (`resample`, `trim`, `applyMedianFilter`, etc.).

## Upstream provider chain analysis

### Chain A: Script runtime global -> Buffer factory
Provider -> dependency -> API class:
- Script processor registration (`ScriptProcessorModules.cpp`)
  -> `VariantBuffer::Factory(64)`
  -> script global `Buffer` object.

Observed across multiple processor classes, so availability is broad in scripting contexts, not tied to a single module type.

### Chain B: ExternalData display infrastructure -> DisplayBuffer -> Buffer
Provider -> dependency -> API class:
- `ProcessorWithExternalData` publishes `SimpleRingBuffer` as `ExternalData::DataType::DisplayBuffer`
  -> `ScriptRingBuffer` (`ScriptComplexDataReferenceBase` resolves holder/index)
  -> `getReadBuffer()` / `getResizedBuffer()` produce Buffer var objects.

Locking context:
- `ScriptRingBuffer::copyReadBuffer()` uses `SimpleReadWriteLock::ScopedReadLock` and `ScopedLock` on ring buffer locks.
- `getReadBuffer()` directly wraps read pointer and does not clone by default.

### Chain C: SliderPack model -> SliderPackData -> Buffer
Provider -> dependency -> API class:
- `SliderPackData` owns `VariantBuffer::Ptr dataBuffer`
  -> `SliderPackData::getDataArray()` returns `var(dataBuffer.get())`
  -> `ScriptSliderPackData::getDataAsBuffer()` exposes it to script.

This is reference semantics to slider data model storage.

### Chain D: UnorderedStack numeric storage -> Buffer views
Provider -> dependency -> API class:
- `ScriptUnorderedStack` owns fixed stack storage (`UnorderedStack<float, 128> data`)
  -> `elementBuffer->referToData(data.begin(), data.size())` on updates
  -> `asBuffer(false)` returns dynamic-length view, `asBuffer(true)` returns full 128-length view.

Event-mode caveat:
- If stack is event mode, `asBuffer` emits script error (`Can't use asBuffer on a stack for events`).

## Internal helper / utility dependencies used by Buffer methods

- `FloatVectorOperations` for vector math and copies.
- `FloatSanitizers` for sanitizing written values.
- `Decibels::decibelsToGain` in `normalise`.
- `juce::Interpolators::{WindowedSinc,Lagrange,CatmullRom,Linear,ZeroOrderHold}` in `resample`.
- `MedianFilter` (`SiTraNoConverter.h`) in `applyMedianFilter`.
- `PitchDetection::detectPitch(...)` in `detectPitch` (guarded by preprocessor).
- `SiTraNoConverter` and nested `ConfigData` in `decompose`.
- `MemoryBlock` base64 encode/decode for `toBase64` / `fromBase64`.

## Enum / constant-like behavioral tracing

Buffer has no `addConstant()` registration, but has string mode selectors and config toggles that behave like enums.

## `resample(..., interpolationType, wrapAround)` mode selector

Defined list in `VariantBuffer.cpp`:
- `"WindowedSinc"`
- `"Lagrange"`
- `"CatmullRom"`
- `"Linear"`
- `"ZeroOrderHold"`

Decision points and behavior:
- String lookup: `interpolatorTypes.indexOf(quality)`.
- If lookup fails (`idx == -1`): throws error with supported list.
- Switch by `idx` selects interpolator class and calls `.process(ratio, in, out, numOut, numIn, wrapAround)`.

Behavioral consequences per mode value:
- `WindowedSinc`: highest quality, higher CPU.
- `Lagrange`: polynomial interpolation, medium quality/cost.
- `CatmullRom`: cubic interpolation profile.
- `Linear`: lower quality, cheaper default path.
- `ZeroOrderHold`: sample-and-hold, lowest quality, cheapest.

Other selectors in same method:
- `ratio` clamped to `[0.01, 1000.0]`.
- `wrapAround` forwarded to interpolator process, affecting boundary behavior.

## `decompose(sampleRate, configData)` config selector object

`configData` is parsed by `SiTraNoConverter::ConfigData(const var& obj)`.

Consumed keys:
- `SlowFFTOrder` (clamped 5..15)
- `FastFFTOrder` (clamped 5..15)
- `FreqResolution`
- `TimeResolution`
- `CalculateTransients` (bool)
- `SlowTransientTreshold` (array[2], clamped 0..1, auto-sorted descending)
- `FastTransientTreshold` (array[2], clamped 0..1, auto-sorted descending)

Behavioral tracing for `CalculateTransients`:
- In `SiTraNoConverter::process()`:
  - if true: executes fast FFT path and transient/noise split (`performFFT(true)`, `applyMedianFilters(true)`, `calculateTransientNoise`).
  - if false: skips second-stage transient split, noise path absorbs residual.
- Output correction stage routes reconstruction residual into:
  - `Transient` output when true.
  - `Noise` output when false.
- In Buffer method `decompose` return payload:
  - Always returns sinusoidal and noise buffers.
  - Adds transient buffer only when `calculateTransients` true.
  - Always appends noise grains array afterwards.

Note on spelling:
- Property names are implemented as `SlowTransientTreshold` / `FastTransientTreshold` ("Treshold" typo), and must match exactly for parser pickup.

## JSON schema construction patterns

### ConfigData serialization (`toVar`)
- Uses `DynamicObject` and `setProperty()` for all keys.
- Threshold arrays are constructed as `Array<var>` and then assigned.

This provides explicit schema source for docs and callback/object property tables.

### Base64 object format
- `toBase64`: serializes raw float bytes and prefixes result with literal `"Buffer"`.
- `fromBase64`: requires this prefix, decodes payload, resizes buffer, then copies float data.

## Preprocessor guards and build sensitivity

- `#if HISE_INCLUDE_PITCH_DETECTION` around `detectPitch` method registration.
  - If disabled, method is not installed in `VariantBuffer::addMethods()`.
- `USE_IPP_MEDIAN_FILTER` in `MedianFilter::createBestInstance(...)` chooses implementation backend (IPP vs fallback).

No `USE_BACKEND` gating in Buffer method installation itself.

## Threading and lifecycle constraints

No explicit `interfaceCreationAllowed()` style init-only checks are present for Buffer methods.

Lifecycle / safety-relevant patterns:
- Many methods allocate (`new VariantBuffer`, internal `setSize`, temporary arrays/strings).
- Some methods mutate in-place (`normalise`, arithmetic operators, `fromBase64`).
- Reference-returning methods (`getSlice`, `Buffer.referTo`, `DisplayBuffer.getReadBuffer`, `SliderPackData.getDataAsBuffer`, `UnorderedStack.asBuffer`) create aliasing to external storage.
- `ScriptRingBuffer::copyReadBuffer` demonstrates lock-based copy path when reading external display data.

Class-level implication for downstream callScope work:
- No init-only API restrictions.
- Mixed method characteristics: some methods are pure reads, many are allocation-heavy, and some expose shared backing memory.

## API method inventory visible in Buffer declaration

From `ScriptBuffer` declaration:
- `getMagnitude`
- `getRMSLevel`
- `normalise`
- `detectPitch`
- `applyMedianFilter`
- `decompose`
- `toBase64`
- `fromBase64`
- `indexOfPeak`
- `toCharString`
- `getPeakRange`
- `resample`
- `getSlice`
- `trim`
- `getNextZeroCrossing`

No class constants are added for Buffer via `addConstant()` in either `ScriptBuffer` or `VariantBuffer::Factory`.

## Constructor macro registration audit (Phase 1A required check)

- `ScriptBuffer` constructor: no `ADD_API_METHOD_*`, no `ADD_TYPED_API_METHOD_*`, no `addConstant()`.
- `VariantBuffer::Factory` uses `setMethod("create", ...)` and `setMethod("referTo", ...)`.
- `VariantBuffer` runtime methods are all registered via `setMethod(...)` in `addMethods()`.

Consequence for Phase 1B forced type map:
- No forced-parameter metadata from `ADD_TYPED_API_METHOD_*` exists for class Buffer.

## Related classes from survey data (for alternatives/cross refs)

`class_survey_data.json` lists:
- Array
- MidiList
- DisplayBuffer
- AudioFile
- FFT
- File
- ScriptShader

Distinction snippets indicate Buffer is the in-memory fixed-size float container, while others cover dynamic mixed types, MIDI-indexed integer tables, ring-buffer handles, disk file I/O, or frequency-domain processing.
