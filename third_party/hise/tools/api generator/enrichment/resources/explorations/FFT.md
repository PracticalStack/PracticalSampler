# FFT -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` -- FFT entry (lines 835-864)
- `enrichment/resources/survey/class_tags.json` -- group: data, role: processor
- `enrichment/base/FFT.json` -- 12 API methods (note: `dumpSpectrum` in wrapper but not in base JSON methods list -- it IS present with 4 params)
- No prerequisites required for this class

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, line 752

```cpp
class ScriptFFT : public ConstScriptingObject,
                  public Spectrum2D::Holder
{
```

### Inheritance
- `ConstScriptingObject` -- standard HISE scripting API base class (provides `addConstant()`, `reportScriptError()`, etc.)
- `Spectrum2D::Holder` -- interface providing `getParameters()` for the Spectrum2D rendering system

### Object Name
```cpp
Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("FFT"); }
```

### Debug Component
Has `FFTDebugComponent` (forward-declared struct) that implements a popup visualization using `PooledUIUpdater::SimpleTimer`. Shows spectrum image or "Spectrum is disabled" text. If inverse is enabled, shows both input and output spectrums side by side.

## Factory / obtainedVia

Created via `Engine.createFFT()`:
```cpp
// ScriptingApi.cpp, line 2406
var ScriptingApi::Engine::createFFT()
{
    return new ScriptingObjects::ScriptFFT(getScriptProcessor());
}
```

No parameters needed for creation. The `Engine` class registers this as `ADD_API_METHOD_0(createFFT)`.

## Constructor

**File:** `ScriptingApiObjects.cpp`, line 8271

```cpp
ScriptingObjects::ScriptFFT::ScriptFFT(ProcessorWithScriptingContent* p) :
    ConstScriptingObject(p, WindowType::numWindowType),  // numWindowType = 7 constants
    phaseFunction(p, this, var(), 2),
    magnitudeFunction(p, this, var(), 2)
{
```

The second arg to `ConstScriptingObject` is the number of constants (7 window types).

### Constants (addConstant calls)

| Name | Enum Value | Type |
|------|-----------|------|
| `Rectangle` | `WindowType::Rectangle` (0) | int |
| `Triangle` | `WindowType::Triangle` (1) | int |
| `Hamming` | `WindowType::Hamming` (2) | int |
| `Hann` | `WindowType::Hann` (3) | int |
| `BlackmanHarris` | `WindowType::BlackmanHarris` (4) | int |
| `Kaiser` | `WindowType::Kaiser` (5) | int |
| `FlatTop` | `WindowType::FlatTop` (6) | int |

These map directly to `FFTHelpers::WindowType` enum defined in `hi_tools/hi_tools/MiscToolClasses.h:2435`.

### Method Registrations

```cpp
ADD_API_METHOD_1(setWindowType);
ADD_API_METHOD_2(prepare);
ADD_API_METHOD_1(setOverlap);
ADD_API_METHOD_1(process);
ADD_TYPED_API_METHOD_2(setMagnitudeFunction, VarTypeChecker::Function, VarTypeChecker::Number);
ADD_CALLBACK_DIAGNOSTIC(magnitudeFunction, setMagnitudeFunction, 0);
ADD_TYPED_API_METHOD_1(setPhaseFunction, VarTypeChecker::Function);
ADD_CALLBACK_DIAGNOSTIC(phaseFunction, setPhaseFunction, 0);
ADD_API_METHOD_1(setEnableSpectrum2D);
ADD_API_METHOD_1(setEnableInverseFFT);
ADD_API_METHOD_1(setSpectrum2DParameters);
ADD_API_METHOD_0(getSpectrum2DParameters);
ADD_API_METHOD_4(dumpSpectrum);
ADD_API_METHOD_1(setUseFallbackEngine);
ADD_API_METHOD_1(setUseSpectrumList);
```

### Typed Method Registrations (forced types)
- `setMagnitudeFunction`: param1 = `VarTypeChecker::Function`, param2 = `VarTypeChecker::Number`
- `setPhaseFunction`: param1 = `VarTypeChecker::Function`

### Callback Diagnostics
- `ADD_CALLBACK_DIAGNOSTIC(magnitudeFunction, setMagnitudeFunction, 0)` -- registers the magnitudeFunction WeakCallbackHolder for parse-time diagnostic (arg index 0)
- `ADD_CALLBACK_DIAGNOSTIC(phaseFunction, setPhaseFunction, 0)` -- same for phaseFunction

### Initialization
```cpp
spectrumParameters = new Spectrum2D::Parameters();
```

## Member Variables

```cpp
// Private members (ScriptingApiObjects.h:838-912)
bool useFallback = false;
AudioSampleBuffer windowBuffer;
bool convertMagnitudesToDecibel = false;
PrepareSpecs lastSpecs;
bool enableInverse = false;
bool enableSpectrum = false;
AudioSampleBuffer fullBuffer;
Image spectrum;
Image outputSpectrum;
Spectrum2D::Parameters::Ptr spectrumParameters;
SimpleReadWriteLock lock;
Array<WorkBuffer> scratchBuffers;
Array<var> thisProcessBuffer;
Array<var> outputData;
ScopedPointer<juce::dsp::FFT> fft;
WeakCallbackHolder magnitudeFunction;
WeakCallbackHolder phaseFunction;
WindowType currentWindowType = WindowType::Rectangle;
double overlap = 0.0;
int maxNumSamples = 0;
```

### WorkBuffer struct
```cpp
struct WorkBuffer
{
    VariantBuffer::Ptr chunkInput;
    VariantBuffer::Ptr chunkOutput;
    VariantBuffer::Ptr magBuffer;
    VariantBuffer::Ptr phaseBuffer;
};
```

One WorkBuffer per channel, allocated in `prepare()`.

### SpectrumList struct (private, inner)
```cpp
struct SpectrumList
{
    SpectrumList(int numItems);
    bool dump(const File& outputFile);
    bool setImage(int imageIndex, const Image& img);
    std::vector<Image> images;
};
```

Used by `setUseSpectrumList()` and `dumpSpectrum()` to batch-collect spectrum images for file export.

## FFTHelpers -- Window Type Infrastructure

**File:** `hi_tools/hi_tools/MiscToolClasses.h:2433`

```cpp
struct FFTHelpers
{
    enum WindowType
    {
        Rectangle,    // 0
        Triangle,     // 1
        Hamming,      // 2
        Hann,         // 3
        BlackmanHarris, // 4
        Kaiser,       // 5
        FlatTop,      // 6
        numWindowType // 7
    };
```

Key static methods:
- `applyWindow(WindowType t, AudioSampleBuffer& b, bool normalise=true, int channelIndex=0)` -- applies the window function to a buffer
- `toComplexArray(phaseBuffer, magBuffer, out)` -- reconstructs complex array from mag+phase (for inverse FFT)
- `toPhaseSpectrum(inp, out)` -- extracts phase from complex FFT result
- `toFreqSpectrum(inp, out)` -- extracts magnitude from complex FFT result
- `scaleFrequencyOutput(b, convertToDb, invert=false)` -- scales/unscales frequency data, optionally converting to/from decibels

## Spectrum2D Infrastructure

**File:** `hi_tools/hi_tools/MiscToolClasses.h:2478`

The `Spectrum2D` struct provides 2D spectrogram rendering. It takes an audio buffer and produces spectrum images.

### Spectrum2D::Parameters (JSON schema)

`getAllIds()` returns these identifiers (MiscToolClasses.cpp:3259):

| Parameter ID | Type | Default | Range/Values | Description |
|-------------|------|---------|--------------|-------------|
| `FFTSize` | int (order) | -- | 7-13 (128-8192 samples) | Log2 of FFT size |
| `DynamicRange` | int | 110 | -- | Min dB for display |
| `Oversampling` | int | 4 | -- | Oversampling factor |
| `ColourScheme` | int | -- | 0-4: blackWhite, rainbow, violetToOrange, hiseColours, preColours | Spectrogram colour palette |
| `GainFactor` | int | 1000 | -- | Gain in dB (1000 = 0.0 gain) |
| `ResamplingQuality` | string | "Low" | "Low", "Mid", "High" | Image resampling quality |
| `Gamma` | int | 60 | 0-150 | Gamma correction percentage |
| `Standardize` | bool | false | -- | Whether to standardize output |
| `FrequencyGamma` | int | 100 | 100-200 | Frequency axis gamma |
| `WindowType` | int | BlackmanHarris(4) | 0-6 | Window type for spectrum generation |

### Spectrum2D::Holder interface
```cpp
struct Holder
{
    virtual ~Holder();
    virtual Parameters::Ptr getParameters() const = 0;
    virtual float getXPosition(float input) const;
    virtual float getYPosition(float input) const;
};
```

ScriptFFT implements this with `getParameters()` returning `spectrumParameters`.

### LookupTable::ColourScheme enum
```cpp
enum class ColourScheme
{
    blackWhite,       // 0
    rainbow,          // 1
    violetToOrange,   // 2
    hiseColours,      // 3
    preColours,       // 4
    numColourSchemes  // 5
};
```

## Threading and Safety

### Lock usage
- `SimpleReadWriteLock lock` -- protects the FFT engine instance
- `setMagnitudeFunction()` and `setPhaseFunction()` acquire `ScopedWriteLock` before updating callbacks
- `process()` acquires `ScopedReadLock` for the main processing loop
- `prepare()` acquires `ScopedWriteLock` when creating the `juce::dsp::FFT` instance

### Realtime safety check (USE_BACKEND only)
Both `setMagnitudeFunction()` and `setPhaseFunction()` perform a `RealtimeSafetyInfo::check()` in backend builds:
```cpp
#if USE_BACKEND
if (auto co = dynamic_cast<WeakCallbackHolder::CallableObject*>(newMagnitudeFunction.getObject()))
{
    if (HiseJavascriptEngine::RootObject::RealtimeSafetyInfo::check(co, this, "FFT.setMagnitudeFunction"))
        reportScriptError("Callback is not safe for audio-thread execution");
}
#endif
```
This validates that the callback function is safe for audio-thread execution.

### WeakCallbackHolder callbacks
Both `magnitudeFunction` and `phaseFunction` are `WeakCallbackHolder` objects initialized with 2 args:
- arg[0]: buffer data (magnitude or phase buffer, or array of buffers for multi-channel)
- arg[1]: offset (int) -- current chunk offset in the input data

Called via `callSync()` which executes synchronously on the current thread.

## Processing Pipeline (process() method)

The `process()` method (line 8471) has two main code paths:

### Path 1: Spectrum2D mode (`enableSpectrum = true`)
If spectrum is enabled, creates a `Spectrum2D` object from the input data and generates a spectrum image. This happens BEFORE the callback processing.

### Path 2: Callback processing (`magnitudeFunction || phaseFunction`)
Processes data in overlapping chunks:
1. Calculates chunk step size: `numDelta = maxNumSamples * (1.0 - overlap)`
2. For each chunk offset:
   a. `copyToWorkBuffer()` -- copies input data chunk to work buffer
   b. `applyFFT()` -- applies window function, then `juce::dsp::FFT::performRealOnlyForwardTransform`, then extracts mag/phase
   c. Calls `magnitudeFunction` callback with buffer args and offset
   d. Calls `phaseFunction` callback with buffer args and offset
   e. `applyInverseFFT()` -- if enabled, reconstructs signal from (possibly modified) mag/phase
   f. `copyFromWorkBuffer()` -- if inverse enabled, adds reconstructed chunk to output (overlap-add)

### Return value
- If inverse FFT is enabled: returns a new Buffer (or array of Buffers for multi-channel) containing the reconstructed signal
- If only callbacks without inverse: returns `var()` (undefined)
- If only spectrum mode: returns `var()` (undefined)

### Error conditions in process()
- "You must call prepare before process" -- if scratchBuffers empty, fft null, or maxNumSamples 0
- "the process function is not defined" -- if no callbacks set AND spectrum not enabled

## prepare() Method Details

```cpp
void prepare(int powerOfTwoSize, int maxNumChannels)
```

- Stores specs in `lastSpecs` (for `reinitialise()`)
- Clamps channels to `jlimit(1, NUM_MAX_CHANNELS)` where `NUM_MAX_CHANNELS = 16`
- Validates `isPowerOfTwo(powerOfTwoSize)` -- reports script error if not
- Creates window buffer (2x FFT size) and applies window function
- Sets `spectrumParameters->order` and `spectrumParameters->Spectrum2DSize`
- Creates per-channel WorkBuffers:
  - `chunkInput`: always created (2x FFT size for complex data)
  - `chunkOutput`: only if `enableInverse` is true
  - `magBuffer`: only if `magnitudeFunction` exists or `enableInverse` is true
  - `phaseBuffer`: only if `phaseFunction` exists or `enableInverse` is true
- Creates `juce::dsp::FFT` with `useFallback` flag
- Uses hardcoded sample rate of 44100.0 in lastSpecs

## reinitialise() Pattern

```cpp
void reinitialise()
{
    if (lastSpecs)
        prepare(lastSpecs.blockSize, lastSpecs.numChannels);
}
```

Called after changes to: `setWindowType()`, `setMagnitudeFunction()`, `setPhaseFunction()`, `setEnableInverseFFT()`. This re-allocates all buffers with the new configuration.

## setOverlap() Details

```cpp
void setOverlap(double percentageOfOverlap)
{
    overlap = jlimit(0.0, 0.99, percentageOfOverlap);
    spectrumParameters->oversamplingFactor = nextPowerOfTwo(1.0 / (1.0 - overlap));
}
```

Clamps to [0.0, 0.99]. The oversampling factor is derived as the next power of two of `1/(1-overlap)`. E.g., overlap=0.5 -> factor=2, overlap=0.75 -> factor=4.

## dumpSpectrum() Method

```cpp
bool dumpSpectrum(var file, bool output, int numFreqPixels, int numTimePixels)
```

This method is registered with `ADD_API_METHOD_4(dumpSpectrum)` but has no doxygen comment in the header. It:
- Requires fallback engine (`useFallback` must be true)
- Gets rescaled/rotated spectrum image
- If `file` is a `ScriptFile`: writes PNG to that file
- If `file` is an int and `spectrumList` exists: stores image at that index in the list
- Returns false on failure

### getRescaledAndRotatedSpectrum()
- Requires fallback engine (error if not)
- Uses `gin::applyResize()` for resampling
- Rotates the image 90 degrees (transposes and flips)

## setUseFallbackEngine()

Inline method in the header:
```cpp
void setUseFallbackEngine(bool shouldUseFallback)
{
    useFallback = shouldUseFallback;
}
```

This flag is passed to `juce::dsp::FFT(order, useFallback)` during `prepare()`. The fallback engine is required for `dumpSpectrum()` operations.

## Integration with Graphics System

`GraphicsObject::drawFFTSpectrum(var fftObject, var area)` in `ScriptingGraphics.cpp:2179` can draw the FFT's spectrum image:
```cpp
if (auto obj = dynamic_cast<ScriptingObjects::ScriptFFT*>(fftObject.getObject()))
{
    auto b = ApiHelpers::getRectangleFromVar(area);
    drawActionHandler.addDrawAction(new ScriptedDrawActions::drawFFTSpectrum(
        obj->getSpectrum(false), b, obj->getParameters()->quality));
}
```

This draws the input spectrum (not the output spectrum) using the configured resampling quality.

## Multi-Channel Processing

The FFT supports multi-channel processing:
- Input can be a single `Buffer` or an `Array` of `Buffer` objects
- Each channel gets its own `WorkBuffer` (allocated in `prepare()`)
- Callbacks receive either a single buffer (mono) or array of buffers (multi-channel) as first arg
- Inverse FFT output follows the same pattern -- single buffer or array

### Channel handling in getBufferArgs()
```cpp
var getBufferArgs(bool useMagnitude, int numToUse)
{
    // Collects mag or phase buffers for each active channel
    // Returns single var for mono, array for multi-channel
}
```

## Preprocessor Guards

- `#if USE_BACKEND` -- used in `setMagnitudeFunction()` and `setPhaseFunction()` for realtime safety checks. These checks are only available in the HISE IDE, not in exported plugins.

No other preprocessor guards affect the FFT class.

## applyFFT() Implementation Details

```cpp
void applyFFT(int numChannelsThisTime, bool skipFirstWindowHalf)
```

For each channel:
1. Applies window function to input chunk (multiply with window buffer)
2. If `skipFirstWindowHalf` (first chunk): skips the first quarter of the window to avoid edge artifacts
3. Calls `fft->performRealOnlyForwardTransform()` (JUCE FFT)
4. If phase function or inverse enabled: extracts phase spectrum via `FFTHelpers::toPhaseSpectrum()`
5. If magnitude function or inverse enabled: extracts frequency spectrum via `FFTHelpers::toFreqSpectrum()`, then scales via `FFTHelpers::scaleFrequencyOutput(convertMagnitudesToDecibel)`

## applyInverseFFT() Implementation Details

```cpp
void applyInverseFFT(int numChannelsThisTime)
```

For each channel:
1. Unscales magnitude data (inverse of the scaling done in applyFFT)
2. Reconstructs complex array from mag+phase via `FFTHelpers::toComplexArray()`
3. Calls `fft->performRealOnlyInverseTransform()` on the output buffer

The output is then added (overlap-add) to the output data in `copyFromWorkBuffer()`.
