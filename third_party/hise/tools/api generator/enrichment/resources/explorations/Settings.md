# Settings -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- no prerequisites for Settings
- `enrichment/resources/survey/class_survey_data.json` -- Settings entry (lines 2302-2323)
- No base class explorations needed (Settings inherits ApiClass + ScriptingObject directly)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.h`, line 748

```cpp
class Settings : public ApiClass,
                 public ScriptingObject
{
public:
    Settings(ProcessorWithScriptingContent* s);

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Settings"); }

    // ... 33 API methods ...

private:
    GlobalSettingManager* gm;
    AudioProcessorDriver* driver;
    MainController* mc;

    struct Wrapper;
};
```

**Category:** namespace (registered as ApiClass, accessed as `Settings.methodName()`)

**Inheritance:**
- `ApiClass` -- provides the method registration infrastructure (`ADD_API_METHOD_N`)
- `ScriptingObject` -- provides `getScriptProcessor()`, `reportScriptError()`

**No ConstScriptingObject:** Unlike most scripting API objects, Settings does NOT inherit from `ConstScriptingObject`. It uses `ApiClass` directly, making it a namespace-style global object (like `Engine`, `Math`, `Console`).

## Constructor

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 2726

```cpp
ScriptingApi::Settings::Settings(ProcessorWithScriptingContent* s) :
    ScriptingObject(s),
    ApiClass(0)  // <-- 0 constants
{
    mc = dynamic_cast<MainController*>(getScriptProcessor()->getMainController_());
    gm = dynamic_cast<GlobalSettingManager*>(mc);
    driver = dynamic_cast<AudioProcessorDriver*>(mc);

    ADD_API_METHOD_0(getZoomLevel);
    ADD_API_METHOD_1(setZoomLevel);
    // ... 31 more ADD_API_METHOD_N registrations ...
}
```

**Key observations:**
- `ApiClass(0)` -- zero constants registered. No `addConstant()` calls.
- All methods use `ADD_API_METHOD_N` (untyped). No `ADD_TYPED_API_METHOD_N` calls.
- Three infrastructure pointers are obtained via `dynamic_cast` from MainController:
  - `mc` (MainController*) -- core engine access
  - `gm` (GlobalSettingManager*) -- zoom, disk mode, voice multiplier, OpenGL
  - `driver` (AudioProcessorDriver*) -- audio device management, MIDI inputs

## Wrapper Struct

All wrappers use `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` (standard untyped wrappers). No typed wrappers.

## How Settings is Obtained

Settings is NOT obtained via a factory method. It is a globally registered namespace:

```cpp
// ScriptProcessorModules.cpp, lines 309, 604, 976, 1846, 1987
scriptEngine->registerApiClass(new ScriptingApi::Settings(this));
```

It is registered in all script processor types (JavascriptMidiProcessor, JavascriptModuleProcessor, etc.), so `Settings` is available as a global namespace in all script contexts.

**obtainedVia:** Global namespace -- always available as `Settings`.

## Infrastructure Classes

### GlobalSettingManager

**File:** `HISE/hi_core/hi_core/StandaloneProcessor.h`, line 21

```cpp
class GlobalSettingManager
{
public:
    class ScaleFactorListener { ... };

    void setGlobalScaleFactor(double scaleFactor, NotificationType sendNotification=dontSendNotification);
    float getGlobalScaleFactor() const noexcept;

    void addScaleFactorListener(ScaleFactorListener* newListener);
    void removeScaleFactorListener(ScaleFactorListener* listenerToRemove);

    HiseSettings::Data& getSettingsObject();

    int diskMode = 0;
    bool allSamplesFound = false;
    double globalBPM = -1.0;
    int voiceAmountMultiplier = 2;
    int channelData = 1;

#if HISE_USE_OPENGL_FOR_PLUGIN
    bool useOpenGL = (bool)HISE_DEFAULT_OPENGL_VALUE;
#else
    bool useOpenGL = false;
#endif
};
```

Settings methods that delegate to GlobalSettingManager:
- `getZoomLevel()` -> `gm->getGlobalScaleFactor()`
- `setZoomLevel()` -> `gm->setGlobalScaleFactor()` with `jlimit(0.25, 2.0, newLevel)`
- `getDiskMode()` -> `driver->diskMode` (member variable, inherited from GlobalSettingManager)
- `setDiskMode()` -> `driver->diskMode = mode; mc->getSampleManager().setDiskMode(...)`
- `getCurrentVoiceMultiplier()` -> `driver->voiceAmountMultiplier`
- `setVoiceMultiplier()` -> `driver->voiceAmountMultiplier = newVoiceAmount`
- `isOpenGLEnabled()` -> `driver->useOpenGL`
- `setEnableOpenGL()` -> `driver->useOpenGL = shouldBeEnabled`

### AudioProcessorDriver

**File:** `HISE/hi_core/hi_core/StandaloneProcessor.h`, line 108

```cpp
class AudioProcessorDriver: public GlobalSettingManager
{
public:
    AudioProcessorDriver(AudioDeviceManager* manager, AudioProcessorPlayer* callback_);

    double getCurrentSampleRate();
    int getCurrentBlockSize();
    void setCurrentSampleRate(double newSampleRate);
    void setCurrentBlockSize(int newBlockSize);
    void setAudioDeviceType(const String deviceName);
    void setAudioDevice(const String &deviceName);
    void toggleMidiInput(const String &midiInputName, bool enableInput);

    AudioDeviceManager *deviceManager;
    AudioProcessorPlayer *callback;
};
```

Settings methods that delegate to AudioProcessorDriver:
- Audio device type: `setAudioDeviceType()`, `getAvailableDeviceTypes()`
- Audio device: `setAudioDevice()`, `getCurrentAudioDevice()`, `getAvailableDeviceNames()`
- Buffer size: `setBufferSize()` -> `driver->setCurrentBlockSize()`, `getCurrentBufferSize()` -> `driver->getCurrentBlockSize()`
- Sample rate: `setSampleRate()` -> `driver->setCurrentSampleRate()`, `getCurrentSampleRate()`
- Output channels: `setOutputChannel()` -> `CustomSettingsWindow::flipEnablement(driver->deviceManager, index)`
- MIDI inputs: `toggleMidiInput()` -> `driver->toggleMidiInput()`, `isMidiInputEnabled()` -> `driver->deviceManager->isMidiInputEnabled()`

### MainController (direct access)

Settings methods using `mc` directly:
- `clearMidiLearn()` -> `mc->getMacroManager().getMidiControlAutomationHandler()->clear(sendNotification)`
- `setEnableDebugMode()` -> `mc->getDebugLogger().startLogging()` / `stopLogging()`
- `toggleMidiChannel()` / `isMidiChannelEnabled()` -> `mc->getMainSynthChain()->getActiveChannelData()`

## Method Implementation Details

### Audio Device Methods -- Standalone Pattern

Most audio device methods follow this pattern:
1. Get `currentDevice` from `driver->deviceManager->getCurrentAudioDevice()`
2. Return empty/default if null
3. Use `HiseSettings::ConversionHelpers` for conversion

Example (getAvailableBufferSizes):
```cpp
AudioIODevice* currentDevice = driver->deviceManager->getCurrentAudioDevice();
Array<var> result;
if (currentDevice != nullptr)
{
    Array<int> bufferSizes = HiseSettings::ConversionHelpers::getBufferSizesForDevice(currentDevice);
    for (auto x : bufferSizes)
        result.add(x);
}
return result;
```

**Null device pattern:** When no audio device is selected, getter methods return empty arrays or empty strings. `getCurrentSampleRate()` returns -1. `getCurrentOutputChannel()` returns 0. `getCurrentBufferSize()` returns `false` (effectively 0 -- this looks like a minor bug where `return false` is used instead of `return 0`).

### Zoom Level Clamping

```cpp
void setZoomLevel(double newLevel) {
    newLevel = jlimit(0.25, 2.0, newLevel);
    gm->setGlobalScaleFactor(newLevel, sendNotificationAsync);
}
```

Zoom is clamped to [0.25, 2.0] range (25% to 200%).

### MIDI Channel Indexing

```cpp
void toggleMidiChannel(int index, bool value) {
    HiseEvent::ChannelFilterData *newData = mc->getMainSynthChain()->getActiveChannelData();
    if (index == 0)
        newData->setEnableAllChannels(value);
    else
        newData->setEnableMidiChannel(index - 1, value);
}
```

Index 0 = all channels toggle. Index 1-16 = individual MIDI channels (converted to 0-based internally).

### DiskMode Enum

```cpp
// MainController.h, line 177
enum class DiskMode
{
    SSD = 0,
    HDD,
    numDiskModes
};
```

Two modes: 0 = SSD (fast, larger preload buffer), 1 = HDD (slow, stream-optimized).

### getUserDesktopSize

```cpp
var getUserDesktopSize() {
    auto area = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
    Array<var> desktopSize;
    desktopSize.add(area.getWidth());
    desktopSize.add(area.getHeight());
    return desktopSize;
}
```

Returns `[width, height]` array of the main display's user area (excludes taskbar).

### setSampleFolder -- Backend/Frontend Split

```cpp
void setSampleFolder(var sampleFolder) {
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(sampleFolder.getObject()))
    {
        auto newLocation = sf->f;
        if(newLocation.isDirectory())
        {
#if USE_BACKEND
            getScriptProcessor()->getMainController_()->getCurrentFileHandler().createLinkFile(FileHandlerBase::Samples, newLocation);
#else
            FrontendHandler::setSampleLocation(newLocation);
#endif
        }
    }
}
```

Takes a `File` object (not a string). Creates a link file in backend, sets location directly in frontend.

### isIppEnabled -- Platform-Specific

```cpp
bool isIppEnabled(bool returnTrueIfMacOS) {
#if JUCE_WINDOWS
#if USE_IPP
    return true;
#else
    return false;
#endif
#else
    return returnTrueIfMacOS;
#endif
}
```

On Windows: returns compile-time IPP state. On macOS: returns whatever `returnTrueIfMacOS` is. This allows scripts to check "is fast FFT available?" since macOS has vDSP (always fast) while Windows needs IPP.

### crashAndBurn -- Debug Tool

```cpp
void crashAndBurn() {
#if USE_BACKEND
    auto includeSymbols = GET_HISE_SETTING(..., HiseSettings::Project::CompileWithDebugSymbols);
    if(!includeSymbols)
        reportScriptError("You need to enable CompileWithDebugSymbols...");
#endif
    volatile int* x = nullptr;
    *x = 90;      // null pointer dereference
    abort();       // fallback
}
```

Deliberately crashes the process for testing crash reporting. In backend, warns if debug symbols are not enabled.

### Perfetto Tracing -- Conditional Compilation

```cpp
void startPerfettoTracing() {
#if PERFETTO
    auto& mp = MelatoninPerfetto::get();
    mp.beginSession();
#else
    reportScriptError("Perfetto is not enabled...");
#endif
}

void stopPerfettoTracing(var traceFileToUse) {
#if PERFETTO
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(traceFileToUse.getObject()))
    {
        auto& mp = MelatoninPerfetto::get();
        mp.customFileLocation = sf->f;
        mp.endSession(true);
        // ...validates .pftrace extension
    }
#else
    reportScriptError("Perfetto is not enabled...");
#endif
}
```

Requires `PERFETTO=1` compile flag. `stopPerfettoTracing` requires a File object with `.pftrace` extension.

### setEnableOpenGL -- Deferred Application

```cpp
void setEnableOpenGL(bool shouldBeEnabled) {
    driver->useOpenGL = shouldBeEnabled;
}
```

Just sets a flag. The OpenGL context is not created/destroyed immediately -- it takes effect on the next interface rebuild. The `isOpenGLEnabled()` return value may be out of sync with the actual rendering state until then.

### Output Channel Selection

```cpp
void setOutputChannel(int index) {
    CustomSettingsWindow::flipEnablement(driver->deviceManager, index);
}
```

Delegates to `CustomSettingsWindow::flipEnablement`, a static method on the UI component. The index selects a channel pair (stereo pair index, not individual channel).

## Preprocessor Guards

| Guard | Methods Affected | Purpose |
|-------|-----------------|---------|
| `PERFETTO` | `startPerfettoTracing`, `stopPerfettoTracing` | Perfetto profiling library |
| `USE_BACKEND` | `setSampleFolder`, `crashAndBurn` | Backend-specific behavior |
| `USE_IPP` | `isIppEnabled` | Intel Performance Primitives |
| `JUCE_WINDOWS` | `isIppEnabled` | Platform detection |
| `HISE_USE_OPENGL_FOR_PLUGIN` | (indirect via `useOpenGL` default) | OpenGL support compile flag |

## Standalone vs Plugin Context

Many Settings methods are primarily useful in **standalone** builds where the application manages its own audio device. In plugin builds (VST/AU/AAX), the host manages the audio device, so methods like `setAudioDevice`, `setBufferSize`, `setSampleRate`, `getAvailableDeviceNames`, etc. operate on the host-provided device manager which may have limited or no effect.

The zoom, disk mode, MIDI channel, OpenGL, debug, and sample folder methods are useful in both contexts.

## HiseSettings::ConversionHelpers

Used by several methods to convert JUCE audio device data into script-friendly formats:
- `getChannelPairs(AudioIODevice*)` -- pairs output channels into stereo pairs
- `getBufferSizesForDevice(AudioIODevice*)` -- gets valid buffer sizes
- `getSampleRates(AudioIODevice*)` -- gets supported sample rates

## No Constants

The constructor passes `ApiClass(0)`, registering zero constants. There are no `addConstant()` calls. This is consistent with the class being a pure method namespace.

## No Typed Methods

All 33 methods use `ADD_API_METHOD_N` (untyped). None use `ADD_TYPED_API_METHOD_N`. Parameter types must be inferred from C++ signatures.
