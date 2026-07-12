# ErrorHandler -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` -- ErrorHandler entry
- `enrichment/base/ErrorHandler.json` -- 7 API methods
- No prerequisite classes required
- No base class exploration needed (not a component)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, line 496

```cpp
struct ScriptErrorHandler : public ConstScriptingObject,
                            public OverlayMessageBroadcaster::Listener
{
    ScriptErrorHandler(ProcessorWithScriptingContent* p);
    ~ScriptErrorHandler();
    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ErrorHandler"); }
    void overlayMessageSent(int state, const String& message) override;

    // API Methods
    void setErrorCallback(var errorCallback);
    void setCustomMessageToShow(int state, String messageToShow);
    void clearErrorLevel(int stateToClear);
    void clearAllErrors();
    String getErrorMessage() const;
    int getNumActiveErrors() const;
    int getCurrentErrorLevel() const;
    void simulateErrorEvent(int state);

private:
    StringArray customErrorMessages;
    void sendErrorForHighestState();
    BigInteger errorStates;
    struct Wrapper;
    WeakCallbackHolder callback;
    var args[2];
};
```

### Inheritance
- **ConstScriptingObject** -- standard HISE scripting API object base. Provides `addConstant()`, `ADD_API_METHOD_N` registration.
- **OverlayMessageBroadcaster::Listener** -- listener interface for system error events. Single method: `overlayMessageSent(int state, const String& message)`.

### Key Members
- `errorStates` (BigInteger) -- bit field tracking which error states are currently active
- `customErrorMessages` (StringArray) -- per-state custom message overrides, indexed by state enum value
- `callback` (WeakCallbackHolder) -- the user-registered error callback
- `args[2]` (var array) -- pre-allocated argument array for callback invocation (avoids allocation)

## Factory Method (obtainedVia)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 3598

```cpp
juce::var ScriptingApi::Engine::createErrorHandler()
{
    return new ScriptingObjects::ScriptErrorHandler(getScriptProcessor());
}
```

Created via `Engine.createErrorHandler()`. Registered as `ADD_API_METHOD_0(createErrorHandler)` in the Engine class (line 1439). No parameters, returns a new ErrorHandler instance each call.

## OverlayMessageBroadcaster -- The Upstream Event Source

**File:** `HISE/hi_core/hi_core/MainControllerHelpers.h`, line 660

This is the core infrastructure class that ErrorHandler listens to. It lives in `MainController` (via inheritance or composition) and acts as the central error event dispatcher for the entire HISE system.

### State Enum

```cpp
enum State
{
    AppDataDirectoryNotFound = 0,
#if HISE_INCLUDE_UNLOCKER_OVERLAY
    LicenseNotFound,
    ProductNotMatching,
    UserNameNotMatching,
    EmailNotMatching,
    MachineNumbersNotMatching,
    LicenseExpired,
    LicenseInvalid,
#endif
    CriticalCustomErrorMessage = 8,
    SamplesNotInstalled,
    SamplesNotFound,
    IllegalBufferSize,
    CustomErrorMessage,
    CustomInformation,
    numReasons
};
```

**Critical note on enum values:** When `HISE_INCLUDE_UNLOCKER_OVERLAY` is disabled (the common case for projects not using copy protection), the license-related constants (values 1-7) are NOT defined. However, `CriticalCustomErrorMessage` is hardcoded to `= 8`, so the remaining constants always have the same values regardless of the preprocessor state:

| Constant | Value | Always Available |
|----------|-------|-----------------|
| AppDataDirectoryNotFound | 0 | Yes |
| LicenseNotFound | 1 | Only with HISE_INCLUDE_UNLOCKER_OVERLAY |
| ProductNotMatching | 2 | Only with HISE_INCLUDE_UNLOCKER_OVERLAY |
| UserNameNotMatching | 3 | Only with HISE_INCLUDE_UNLOCKER_OVERLAY |
| EmailNotMatching | 4 | Only with HISE_INCLUDE_UNLOCKER_OVERLAY |
| MachineNumbersNotMatching | 5 | Only with HISE_INCLUDE_UNLOCKER_OVERLAY |
| LicenseExpired | 6 | Only with HISE_INCLUDE_UNLOCKER_OVERLAY |
| LicenseInvalid | 7 | Only with HISE_INCLUDE_UNLOCKER_OVERLAY |
| CriticalCustomErrorMessage | 8 | Yes |
| SamplesNotInstalled | 9 | Yes |
| SamplesNotFound | 10 | Yes |
| IllegalBufferSize | 11 | Yes |
| CustomErrorMessage | 12 | Yes |
| CustomInformation | 13 | Yes |

### Preprocessor: HISE_INCLUDE_UNLOCKER_OVERLAY

**File:** `HISE/hi_core/hi_core.h`

```cpp
#ifndef HISE_INCLUDE_UNLOCKER_OVERLAY
#define HISE_INCLUDE_UNLOCKER_OVERLAY (USE_COPY_PROTECTION && !USE_SCRIPT_COPY_PROTECTION)
#endif
```

This is enabled only when using the built-in (non-script) copy protection system. When disabled, the license-related constants are simply not added to the ErrorHandler object.

### Preprocessor: HISE_DEACTIVATE_OVERLAY

```cpp
#ifndef HISE_DEACTIVATE_OVERLAY
#define HISE_DEACTIVATE_OVERLAY 0
#endif
```

Controls the default value of `useDefaultOverlay`. When 0 (default), the default overlay is active. The ErrorHandler constructor explicitly calls `setUseDefaultOverlay(false)` to disable the default overlay, so creating an ErrorHandler always replaces the built-in UI.

### Listener Interface

```cpp
class Listener
{
public:
    virtual void overlayMessageSent(int state, const String& message) = 0;
    virtual ~Listener();
private:
    friend class WeakReference<Listener>;
    WeakReference<Listener>::Master masterReference;
};
```

Weak-referenced listener pattern. The broadcaster holds `Array<WeakReference<Listener>, CriticalSection>`, so listeners can be garbage collected without explicit removal (though the destructor does remove).

### Event Dispatch Mechanism

```cpp
void OverlayMessageBroadcaster::sendOverlayMessage(int newState, const String& newCustomMessage)
{
    if (currentState == DeactiveOverlay::State::CriticalCustomErrorMessage)
        return;  // Once critical, no more state changes

#if USE_BACKEND
    Logger::getCurrentLogger()->writeToLog("!" + newCustomMessage);
#endif

    currentState = newState;
    customMessage = newCustomMessage;
    internalUpdater.triggerAsyncUpdate();
}
```

Key behaviors:
1. **CriticalCustomErrorMessage is sticky** -- once the system enters this state, no further state changes are accepted. This is a terminal error.
2. **Backend logs only** -- in the HISE IDE, errors are logged to console rather than showing overlays.
3. **Async dispatch** -- uses `AsyncUpdater` (JUCE message thread callback) to notify listeners. This means callbacks arrive on the message thread.

### Default Error Messages

`getOverlayTextMessage(State s)` provides built-in messages for each state:

| State | Default Message |
|-------|----------------|
| AppDataDirectoryNotFound | "The application directory is not found. (The installation seems to be broken. Please reinstall this software.)" |
| IllegalBufferSize | "The audio buffer size should be a multiple of [HISE_EVENT_RASTER]. Please adjust your audio settings" |
| SamplesNotFound | "The sample directory could not be located. Click below to choose the sample folder." |
| SamplesNotInstalled | Varies based on HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON / HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON |
| LicenseNotFound | "This computer is not registered..." (if USE_COPY_PROTECTION) |
| ProductNotMatching | "The license key is invalid (wrong plugin name / version)..." |
| MachineNumbersNotMatching | "The machine ID is invalid / not matching..." |
| UserNameNotMatching | "The user name is invalid..." |
| EmailNotMatching | "The email name is invalid..." |
| LicenseExpired | "The license key is expired..." |
| CustomErrorMessage / CriticalCustomErrorMessage / CustomInformation | No default -- uses custom message |

## Constructor Analysis

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp`, line 10651

```cpp
ScriptErrorHandler(ProcessorWithScriptingContent* p) :
    ConstScriptingObject(p, OverlayMessageBroadcaster::State::numReasons),
    callback(p, this, var(), 2)
```

The second argument to ConstScriptingObject (`numReasons`) sets the number of constants. The WeakCallbackHolder is initialized with 2 argument slots.

### Constants Registered

All use `ADD_API_METHOD_N` (plain, not typed):

| Constant | Source Enum Value | Preprocessor Guard |
|----------|-------------------|--------------------|
| AppDataDirectoryNotFound | 0 | None |
| LicenseNotFound | 1 | HISE_INCLUDE_UNLOCKER_OVERLAY |
| ProductNotMatching | 2 | HISE_INCLUDE_UNLOCKER_OVERLAY |
| UserNameNotMatching | 3 | HISE_INCLUDE_UNLOCKER_OVERLAY |
| EmailNotMatching | 4 | HISE_INCLUDE_UNLOCKER_OVERLAY |
| MachineNumbersNotMatching | 5 | HISE_INCLUDE_UNLOCKER_OVERLAY |
| LicenseExpired | 6 | HISE_INCLUDE_UNLOCKER_OVERLAY |
| LicenseInvalid | 7 | HISE_INCLUDE_UNLOCKER_OVERLAY |
| CriticalCustomErrorMessage | 8 | None |
| SamplesNotInstalled | 9 | None |
| SamplesNotFound | 10 | None |
| IllegalBufferSize | 11 | None |
| CustomErrorMessage | 12 | None |
| CustomInformation | 13 | None |

### Side Effects

```cpp
p->getMainController_()->addOverlayListener(this);
p->getMainController_()->setUseDefaultOverlay(false);
```

**Creating an ErrorHandler disables the default overlay.** This is a significant architectural side effect -- the built-in `DeactiveOverlay` component (the default error UI) is deactivated, and the script takes full responsibility for showing error information.

## Destructor

```cpp
ScriptingObjects::ScriptErrorHandler::~ScriptErrorHandler()
{
    getScriptProcessor()->getMainController_()->removeOverlayListener(this);
}
```

Unregisters from the broadcaster. Does NOT re-enable the default overlay.

## DeactiveOverlay -- The Default Error UI

**File:** `HISE/hi_core/hi_components/plugin_components/FrontendBar.h`, line 49

```cpp
class DeactiveOverlay : public Component,
    public ButtonListener,
    public ControlledObject,
    public Timer,
    public AsyncUpdater,
    public OverlayMessageBroadcaster::Listener
```

This is the built-in error overlay that ErrorHandler replaces. It is a JUCE Component that:
- Listens for the same `OverlayMessageBroadcaster` events
- Shows a modal overlay with error text and action buttons
- Handles license checking, sample location, etc.

The `DeactiveOverlay` uses `OverlayMessageBroadcaster::State` via a type alias: `using State = OverlayMessageBroadcaster::State;`

**Relationship:** ErrorHandler and DeactiveOverlay are alternative consumers of the same event stream. Creating an ErrorHandler deactivates DeactiveOverlay via `setUseDefaultOverlay(false)`. The `isUsingDefaultOverlay()` method on the broadcaster controls which system handles errors.

## Wrapper Struct (Method Registration)

All methods use plain `API_VOID_METHOD_WRAPPER_N` / `API_METHOD_WRAPPER_N` -- no typed wrappers (`ADD_TYPED_API_METHOD_N` is not used).

```cpp
struct Wrapper
{
    API_VOID_METHOD_WRAPPER_1(ScriptErrorHandler, setErrorCallback);
    API_VOID_METHOD_WRAPPER_2(ScriptErrorHandler, setCustomMessageToShow);
    API_VOID_METHOD_WRAPPER_1(ScriptErrorHandler, clearErrorLevel);
    API_VOID_METHOD_WRAPPER_0(ScriptErrorHandler, clearAllErrors);
    API_METHOD_WRAPPER_0(ScriptErrorHandler, getErrorMessage);
    API_METHOD_WRAPPER_0(ScriptErrorHandler, getNumActiveErrors);
    API_METHOD_WRAPPER_0(ScriptErrorHandler, getCurrentErrorLevel);
    API_VOID_METHOD_WRAPPER_1(ScriptErrorHandler, simulateErrorEvent);
};
```

## Error Event Sources (Who Sends Overlay Messages)

The `sendOverlayMessage` calls found across the codebase reveal what triggers errors:

| Source File | State | Trigger |
|-------------|-------|---------|
| ModulatorSampler.cpp | CustomErrorMessage | Sample loading errors |
| SampleImporter.cpp | CustomErrorMessage | Sample import failures |
| ModulatorSamplerSound.cpp | CustomErrorMessage | Sound loading issues |
| ModulatorSamplerData.cpp | CustomErrorMessage, SamplesNotFound | Sample resolution failures |
| MainController.cpp | CustomErrorMessage | Buffer size validation, compilation errors |
| MainControllerHelpers.cpp | IllegalBufferSize | Audio buffer size not a multiple of HISE_EVENT_RASTER |
| ExternalFilePool.cpp | CriticalCustomErrorMessage | Missing external files |
| DebugLogger.cpp | CustomInformation | Crash log recovery, log export |
| PresetBrowser.cpp | CriticalCustomErrorMessage | Preset browser errors |
| PresetHandler.cpp | SamplesNotFound | Missing samples during loading |

## Callback Mechanism

### setErrorCallback

```cpp
void setErrorCallback(var errorCallback)
{
    if (HiseJavascriptEngine::isJavascriptFunction(errorCallback))
    {
        callback = WeakCallbackHolder(getScriptProcessor(), this, errorCallback, 2);
        callback.incRefCount();
        callback.addAsSource(this, "onErrorCallback");
        callback.setThisObject(this);
        callback.setHighPriority();
    }
}
```

Key details:
- Validates that the argument is a JavaScript function
- Creates a new `WeakCallbackHolder` with 2 argument slots
- Sets high priority (processed before regular callbacks)
- `setThisObject(this)` makes the ErrorHandler instance available as `this` in the callback
- Source name is "onErrorCallback" for debugging

### overlayMessageSent (Event Handler)

```cpp
void overlayMessageSent(int state, const String& message) override
{
    errorStates.setBit(state, true);

    if (state == OverlayMessageBroadcaster::CustomErrorMessage ||
        state == OverlayMessageBroadcaster::CustomInformation ||
        state == OverlayMessageBroadcaster::CriticalCustomErrorMessage)
    {
        customErrorMessages.set(state, message);
    }

    sendErrorForHighestState();
}
```

Behavior:
1. Sets the bit for the received state in `errorStates`
2. For custom message states (CustomErrorMessage, CustomInformation, CriticalCustomErrorMessage), stores the message text
3. Calls `sendErrorForHighestState()` which invokes the callback with the lowest active error state

### sendErrorForHighestState (Internal)

```cpp
void sendErrorForHighestState()
{
    if (callback)
    {
        args[0] = getCurrentErrorLevel();
        args[1] = getErrorMessage();
        callback.call(args, 2);
    }
}
```

The naming is slightly misleading -- `getCurrentErrorLevel()` returns the LOWEST set bit (highest priority), not the highest numbered state.

### Error Priority Model

`getCurrentErrorLevel()` iterates from bit 0 upward and returns the first set bit. This means **lower-numbered states have higher priority**. The state enum is ordered by severity:
- 0: AppDataDirectoryNotFound (most critical)
- 1-7: License issues
- 8: CriticalCustomErrorMessage
- 9-10: Sample issues
- 11: IllegalBufferSize
- 12-13: Custom messages (least critical)

When multiple errors are active simultaneously, the callback always reports the lowest-numbered (highest priority) one.

### clearErrorLevel Behavior

```cpp
void clearErrorLevel(int stateToClear)
{
    errorStates.clearBit(stateToClear);
    if (!errorStates.isZero())
    {
        sendErrorForHighestState();
    }
}
```

After clearing a state, if other errors remain, the callback fires again with the next highest-priority error. This allows the UI to update to show the next error in priority order.

### getErrorMessage Logic

```cpp
String getErrorMessage() const
{
    auto el = getCurrentErrorLevel();
    if (el == -1)
        return {};
    auto m = customErrorMessages[el];
    if (m.isNotEmpty())
        return m;
    return getScriptProcessor()->getMainController_()->getOverlayTextMessage(
        (OverlayMessageBroadcaster::State)el);
}
```

Message resolution order:
1. If a custom message was set via `setCustomMessageToShow()`, use it
2. If a custom message was received via `overlayMessageSent()` for custom states, use it
3. Fall back to the built-in default message from `getOverlayTextMessage()`

## Threading Considerations

- **Callback arrives on message thread** -- `OverlayMessageBroadcaster` uses `AsyncUpdater` for dispatch
- **No onInit-only restrictions** -- the object can be created and configured at any time, though typically done in onInit
- **WeakCallbackHolder with high priority** -- callback is processed before regular script callbacks
- **No audio thread interaction** -- this is purely a UI/service-layer mechanism

## Related Preprocessors

| Preprocessor | Effect on ErrorHandler |
|--------------|----------------------|
| HISE_INCLUDE_UNLOCKER_OVERLAY | Controls whether license-related constants are added |
| HISE_DEACTIVATE_OVERLAY | Controls default overlay state (ErrorHandler always disables it) |
| USE_BACKEND | In backend, overlay messages are logged to console instead |
| USE_COPY_PROTECTION | Influences HISE_INCLUDE_UNLOCKER_OVERLAY |
| USE_SCRIPT_COPY_PROTECTION | Influences HISE_INCLUDE_UNLOCKER_OVERLAY |
| HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON | Affects SamplesNotInstalled default message |
| HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON | Affects SamplesNotInstalled default message |
