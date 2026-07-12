# TransportHandler -- Raw Exploration

## Resources Consulted
- `resources/survey/class_survey.md` (line 125: TransportHandler role=event, base=ConstScriptingObject)
- `resources/survey/class_survey_data.json` (TransportHandler entry: seeAlso Timer, Broadcaster; createdBy Engine)
- No prerequisite class in the enrichment prerequisites table

## Source Files

| Role | File | Lines |
|------|------|-------|
| Header (class declaration) | `hi_scripting/scripting/api/ScriptingApi.h` | 1482-1639 |
| Implementation | `hi_scripting/scripting/api/ScriptingApi.cpp` | 8284-8745 |
| Callback struct impl | `hi_scripting/scripting/api/ScriptingApi.cpp` | 8284-8354 |
| Wrapper struct | `hi_scripting/scripting/api/ScriptingApi.cpp` | 8356-8377 |
| Constructor + methods | `hi_scripting/scripting/api/ScriptingApi.cpp` | 8379-8745 |
| Factory method (Engine) | `hi_scripting/scripting/api/ScriptingApi.cpp` | 3500-3503 |
| Factory wrapper macro | `hi_scripting/scripting/api/ScriptingApi.cpp` | 1215 |
| Factory registration | `hi_scripting/scripting/api/ScriptingApi.cpp` | 1438 |
| TempoListener base class | `hi_tools/hi_tools/MiscToolClasses.h` | 2335-2386 |
| MasterClock class + SyncModes | `hi_tools/hi_tools/MiscToolClasses.h` | 2225-2328 |
| TempoSyncer::Tempo enum | `hi_tools/hi_tools/MiscToolClasses.h` | 2142-2173 |
| ApiHelpers::isSynchronous | `hi_scripting/scripting/api/ScriptingApiObjects.cpp` | 6918-6921 |
| ApiHelpers::getDispatchType | `hi_scripting/scripting/api/ScriptingApiObjects.cpp` | 6902-6916 |

---

## Class Declaration

```cpp
// ScriptingApi.h:1482-1639
class TransportHandler : public ConstScriptingObject,
                         public TempoListener,
                         public ControlledObject,
                         public PooledUIUpdater::Listener
{
public:
    TransportHandler(ProcessorWithScriptingContent* sp);;
    ~TransportHandler();

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("TransportHandler"); }
    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("TransportHandler"); };

    // ... Callback struct, API methods, private members ...

    JUCE_DECLARE_WEAK_REFERENCEABLE(TransportHandler);
};
```

**Inheritance chain:**
- `ConstScriptingObject` -- HISE scripting object base (provides addConstant, ADD_API_METHOD macros, scripting integration)
- `TempoListener` -- virtual callbacks for tempo, transport, beat, grid, signature changes (called from audio thread)
- `ControlledObject` -- access to `getMainController()` for engine services
- `PooledUIUpdater::Listener` -- receives async dispatch messages via `handlePooledMessage()`

---

## Inner Callback Struct

```cpp
// ScriptingApi.h:1497-1519
struct Callback: public PooledUIUpdater::Broadcaster
{
    Callback(TransportHandler* p, const String& name, const var& f, bool sync, int numArgs);
    void call(var arg1, var arg2 = {}, var arg3 = {}, bool forceSynchronous = false);
    void callAsync();
    bool matches(const var& f) const;

private:
    void callSync();
    const int numArgs;
    var args[3];
    JavascriptProcessor* jp;
    WeakReference<TransportHandler> th;
    const bool synchronous = false;
    WeakCallbackHolder callback;
};
```

### Callback Constructor (ScriptingApi.cpp:8284-8322)

```cpp
ScriptingApi::TransportHandler::Callback::Callback(TransportHandler* p, const String& name, const var& f, bool sync, int numArgs_) :
    callback(p->getScriptProcessor(), p, f, numArgs_),
    jp(dynamic_cast<JavascriptProcessor*>(p->getScriptProcessor())),
    synchronous(sync),
    th(p),
    numArgs(numArgs_)
{
    callback.addAsSource(p, name);

    if (synchronous)
    {
        auto fObj = dynamic_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(f.getObject());

        if (fObj == nullptr)
            throw String("Must use inline functions for synchronous callback");

        if (fObj->parameterNames.size() != numArgs)
        {
            throw String("Parameter amount mismatch for callback. Expected " + String(numArgs));
        }

#if USE_BACKEND
        if (auto co = dynamic_cast<WeakCallbackHolder::CallableObject*>(f.getObject()))
        {
            if (HiseJavascriptEngine::RootObject::RealtimeSafetyInfo::check(co, p, "TransportHandler." + name))
                throw String("Callback contains unsafe API calls for audio-thread execution");
        }
#endif
    }

    setHandler(th->getMainController()->getGlobalUIUpdater());
    addPooledChangeListener(th);

    callback.incRefCount();

    if(!sync)
        callback.setHighPriority();
}
```

**Key observations:**
- Sync callbacks require `inline function` -- a regular function will throw "Must use inline functions for synchronous callback"
- Sync callbacks are validated for parameter count at registration time
- Sync callbacks are checked for audio-thread safety in backend builds (USE_BACKEND)
- Async callbacks get `setHighPriority()` on the WeakCallbackHolder
- Both sync and async register with PooledUIUpdater (setHandler + addPooledChangeListener)

### Callback::call (ScriptingApi.cpp:8324-8334)

```cpp
void ScriptingApi::TransportHandler::Callback::call(var arg1, var arg2, var arg3, bool forceSync)
{
    args[0] = arg1;
    args[1] = arg2;
    args[2] = arg3;

    if (synchronous || forceSync)
        callSync();
    else
        sendPooledChangeMessage();
}
```

- Stores args in the 3-element array
- Sync path: calls `callSync()` immediately (executes on calling thread -- the audio thread)
- Async path: calls `sendPooledChangeMessage()` which queues via PooledUIUpdater

### Callback::callSync (ScriptingApi.cpp:8346-8354)

```cpp
void ScriptingApi::TransportHandler::Callback::callSync()
{
    auto ok = callback.callSync(args, numArgs);

#if USE_BACKEND
    if(!ok.wasOk())
        debugError(dynamic_cast<Processor*>(jp), ok.getErrorMessage());
#endif
}
```

### Callback::callAsync (ScriptingApi.cpp:8336-8339)

```cpp
void ScriptingApi::TransportHandler::Callback::callAsync()
{
    callback.call(args, numArgs);
}
```

### handlePooledMessage (ScriptingApi.h:8632-8636)

```cpp
void handlePooledMessage(PooledUIUpdater::Broadcaster* b) override
{
    if (auto asC = dynamic_cast<Callback*>(b))
        asC->callAsync();
}
```

The async dispatch chain: `call()` -> `sendPooledChangeMessage()` -> (PooledUIUpdater timer tick) -> `handlePooledMessage()` -> `callAsync()` -> `callback.call(args, numArgs)`

---

## Factory Method

```cpp
// ScriptingApi.cpp:3500-3503
var ScriptingApi::Engine::createTransportHandler()
{
    return new TransportHandler(getScriptProcessor());
}
```

- Declared at ScriptingApi.h:643: `/** Creates an object that can listen to transport events. */`
- Registered at ScriptingApi.cpp:1438: `ADD_API_METHOD_0(createTransportHandler);`
- Wrapper at ScriptingApi.cpp:1215: `API_METHOD_WRAPPER_0(Engine, createTransportHandler)`
- Each call creates a NEW instance -- no singleton caching
- Multiple TransportHandler instances can coexist (each with its own callback slots and grid multiplier)

---

## Constructor (ScriptingApi.cpp:8379-8417)

```cpp
ScriptingApi::TransportHandler::TransportHandler(ProcessorWithScriptingContent* sp) :
    ConstScriptingObject(sp, (int)MasterClock::SyncModes::numSyncModes),
    ControlledObject(sp->getMainController_())
{
    addConstant("Inactive", (int)MasterClock::SyncModes::Inactive);         // 0
    addConstant("ExternalOnly", (int)MasterClock::SyncModes::ExternalOnly); // 1
    addConstant("InternalOnly", (int)MasterClock::SyncModes::InternalOnly); // 2
    addConstant("PreferInternal", (int)MasterClock::SyncModes::PreferInternal); // 3
    addConstant("PreferExternal", (int)MasterClock::SyncModes::PreferExternal); // 4
    addConstant("SyncInternal", (int)MasterClock::SyncModes::SyncInternal); // 5

    getMainController()->addTempoListener(this);

    ADD_TYPED_API_METHOD_2(setOnTempoChange, VarTypeChecker::Number, VarTypeChecker::Function);
    addDiagnostic("setOnTempoChange", WeakCallbackHolder::checkCallbackNumArgs<1, 1>);
    ADD_TYPED_API_METHOD_2(setOnBeatChange, VarTypeChecker::Number, VarTypeChecker::Function);
    addDiagnostic("setOnBeatChange", WeakCallbackHolder::checkCallbackNumArgs<2, 1>);
    ADD_TYPED_API_METHOD_2(setOnGridChange, VarTypeChecker::Number, VarTypeChecker::Function);
    addDiagnostic("setOnGridChange", WeakCallbackHolder::checkCallbackNumArgs<3, 1>);
    ADD_TYPED_API_METHOD_2(setOnSignatureChange, VarTypeChecker::Number, VarTypeChecker::Function);
    addDiagnostic("setOnSignatureChange", WeakCallbackHolder::checkCallbackNumArgs<2, 1>);
    ADD_TYPED_API_METHOD_2(setOnTransportChange, VarTypeChecker::Number, VarTypeChecker::Function);
    addDiagnostic("setOnTransportChange", WeakCallbackHolder::checkCallbackNumArgs<1, 1>);
    ADD_TYPED_API_METHOD_1(setOnBypass, VarTypeChecker::Function);
    addDiagnostic("setOnBypass", WeakCallbackHolder::checkCallbackNumArgs<1>);
    ADD_API_METHOD_1(setSyncMode);
    ADD_API_METHOD_1(startInternalClock);
    ADD_API_METHOD_1(stopInternalClock);
    ADD_API_METHOD_2(setEnableGrid);
    ADD_API_METHOD_0(sendGridSyncOnNextCallback);
    ADD_API_METHOD_1(stopInternalClockOnExternalStop);
    ADD_API_METHOD_1(setLinkBpmToSyncMode);
    ADD_API_METHOD_0(isNonRealtime);
    ADD_API_METHOD_1(setLocalGridMultiplier);
    ADD_API_METHOD_0(getGridLengthInSamples);
    ADD_API_METHOD_1(setLocalGridBypassed);
    ADD_API_METHOD_1(getGridPosition);
    ADD_API_METHOD_0(isPlaying);
}
```

### Diagnostic Registration

The `addDiagnostic` calls register parameter-count validators:
- `checkCallbackNumArgs<N, P>` where N = expected callback args, P = parameter position (1-indexed)
- These fire at parse time in the IDE, not at runtime

---

## Destructor (ScriptingApi.cpp:8419-8424)

```cpp
ScriptingApi::TransportHandler::~TransportHandler()
{
    getMainController()->getPluginBypassHandler().listeners.removeListener(*this);
    getMainController()->removeTempoListener(this);
    getMainController()->removeMusicalUpdateListener(this);
}
```

Removes from all three listener registrations: bypass, tempo, and musical update.

---

## Constants: MasterClock::SyncModes

```cpp
// MiscToolClasses.h:2225-2233
enum class SyncModes
{
    Inactive,       // 0 -- No syncing going on
    ExternalOnly,   // 1 -- Only reacts on external clock events
    InternalOnly,   // 2 -- Only reacts on internal clock events
    PreferInternal, // 3 -- Override the clock value with the internal clock if it plays
    PreferExternal, // 4 -- Override the clock value with the external clock if it plays
    SyncInternal,   // 5 -- Sync the internal clock when external playback starts
    numSyncModes
};
```

These are exposed as constants on each TransportHandler instance: `th.Inactive`, `th.ExternalOnly`, etc.
Used as arguments to `setSyncMode()`.

---

## TempoListener Base Class (MiscToolClasses.h:2335-2386)

```cpp
class TempoListener
{
public:
    enum CallbackTypes
    {
        TempoChange,
        TranportChange,        // (note: typo in original source)
        MusicalPositionChange,
        SignatureChange,
        numCallbackTypes
    };

    virtual ~TempoListener() {};

    /** Called synchronously in the audio callback before processing.
     *  Called once per block. */
    virtual void tempoChanged(double newTempo) {};

    /** Called when the transport state changes (user presses play on the DAW). */
    virtual void onTransportChange(bool isPlaying, double ppqPosition) {};

    /** Called whenever the transport position needs to be resynced. */
    virtual void onResync(double ppqPosition) {};

    /** Called for each musical pulse. Takes time signature denominator into account. 
     *  Disabled by default -- requires addPulseListener(). */
    virtual void onBeatChange(int beatIndex, bool isNewBar) {};

    /** Called on every grid change. For sample-accurate sequencers.
     *  Disabled by default -- requires addPulseListener(). */
    virtual void onGridChange(int gridIndex, uint16 timestamp, bool firstGridEventInPlayback) {};

    /** Called whenever a time signature change occurs. */
    virtual void onSignatureChange(int newNominator, int numDenominator) {};
};
```

**Threading model:** All TempoListener callbacks are called from the audio thread, synchronously, once per audio block, before processing. This is critical context for understanding the sync vs async callback dispatch in TransportHandler.

---

## TempoSyncer::Tempo Enum (MiscToolClasses.h:2142-2173)

Used by `setEnableGrid(bool, int tempoFactor)` -- the `tempoFactor` parameter is an index into this enum.

```cpp
enum Tempo
{
#if HISE_USE_EXTENDED_TEMPO_VALUES
    EightBar = 0, SixBar, FourBar, ThreeBar, TwoBars, Whole,
#else
    Whole = 0,
#endif
    HalfDuet,           // 1/2D
    Half,               // 1/2
    HalfTriplet,        // 1/2T
    QuarterDuet,        // 1/4D
    Quarter,            // 1/4
    QuarterTriplet,     // 1/4T
    EighthDuet,         // 1/8D
    Eighth,             // 1/8
    EighthTriplet,      // 1/8T
    SixteenthDuet,      // 1/16D
    Sixteenth,          // 1/16
    SixteenthTriplet,   // 1/16T
    ThirtyTwoDuet,      // 1/32D
    ThirtyTwo,          // 1/32
    ThirtyTwoTriplet,   // 1/32T
    SixtyForthDuet,     // 1/64D
    SixtyForth,         // 1/64
    SixtyForthTriplet,  // 1/64T
    numTempos           // 19 (without extended) or 24 (with extended)
};
```

Without `HISE_USE_EXTENDED_TEMPO_VALUES` (the default), valid indices are 0-18 where:
- 0 = Whole (1/1)
- 1 = HalfDuet (1/2D) ... through 18 = SixtyForthTriplet (1/64T)

The validation in `setEnableGrid` uses `isPositiveAndBelow(tempoFactor, (int)TempoSyncer::numTempos)` which means 0 <= tempoFactor < numTempos.

---

## Sync Parameter Semantics

### ApiHelpers::isSynchronous (ScriptingApiObjects.cpp:6918-6921)

```cpp
bool ApiHelpers::isSynchronous(const var& syncValue)
{
    return getDispatchType(syncValue, false) == dispatch::DispatchType::sendNotificationSync;
}
```

### ApiHelpers::getDispatchType (ScriptingApiObjects.cpp:6902-6916)

```cpp
dispatch::DispatchType ApiHelpers::getDispatchType(const var& syncValue, bool getDontForFalse)
{
    using Type = dispatch::DispatchType;

    if ((int)syncValue == SyncMagicNumber)        // 911
        return Type::sendNotificationSync;

    if ((int)syncValue == AsyncMagicNumber)       // 912
        return Type::sendNotificationAsync;

    if ((int)syncValue == AsyncHiPriorityMagicNumber) // 913
        return Type::sendNotificationAsyncHiPriority;

    return (bool)syncValue ? Type::sendNotificationSync 
                           : (getDontForFalse ? Type::dontSendNotification 
                                              : Type::sendNotificationAsync);
}
```

The `sync` parameter accepts:
- `SyncNotification` (magic number 911) -- synchronous, fires on audio thread
- `AsyncNotification` (magic number 912) -- asynchronous, fires on UI thread via PooledUIUpdater
- `AsyncHiPriorityNotification` (magic number 913) -- asynchronous, fires on a separate faster thread
- `true` -- treated as sync (same as SyncNotification)
- `false` -- treated as async (same as AsyncNotification)

For TransportHandler, the `isSynchronous` helper is used, which only returns true for SyncNotification/true. Both async and async-hi-priority map to the async callback slot.

---

## Private Member Variables (ScriptingApi.h:1590-1619)

```cpp
double bpm = 120.0;
bool play = false;
int nom = 4;
int denom = 4;
int beat = 0;
bool newBar = true;
int gridIndex = 0;
int gridTimestamp = 0;
bool firstGridInPlayback = false;
int localGridMultiplier = 1;
int localBitShift = 0;
bool nextLocalIsFirst = false;
bool localBypassed = false;
int lastGridIndex = -1;

struct Wrapper;

ScopedPointer<Callback> tempoChangeCallback;
ScopedPointer<Callback> transportChangeCallback;
ScopedPointer<Callback> timeSignatureCallback;
ScopedPointer<Callback> beatCallback;
ScopedPointer<Callback> gridCallback;

ScopedPointer<Callback> bypassCallback;

ScopedPointer<Callback> tempoChangeCallbackAsync;
ScopedPointer<Callback> transportChangeCallbackAsync;
ScopedPointer<Callback> timeSignatureCallbackAsync;
ScopedPointer<Callback> beatCallbackAsync;
ScopedPointer<Callback> gridCallbackAsync;
```

**Dual callback pattern:** Each event type (tempo, transport, signature, beat, grid) has two `ScopedPointer<Callback>` slots:
- A sync slot (e.g. `tempoChangeCallback`) for audio-thread callbacks
- An async slot (e.g. `tempoChangeCallbackAsync`) for UI-thread callbacks

The `bypassCallback` only has one slot (always async -- `setOnBypass` hardcodes `false` for sync).

**State caching:** The current state (bpm, play, nom, denom, beat, newBar, gridIndex, gridTimestamp, firstGridInPlayback) is cached in member variables. This allows:
1. Registration methods to immediately fire the callback with current state
2. Grid filtering logic to work with cached values

---

## clearIf Helper (ScriptingApi.h:1584-1588)

```cpp
void clearIf(ScopedPointer<Callback>& cb, const var& f)
{
    if (cb != nullptr && cb->matches(f))
        cb = nullptr;
}
```

When registering a sync callback, the corresponding async slot is cleared IF it holds the same function reference (and vice versa). This prevents the same function from being called both synchronously and asynchronously for the same event type.

---

## Method Implementations

### setOnTempoChange (ScriptingApi.cpp:8426-8443)

```cpp
void ScriptingApi::TransportHandler::setOnTempoChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
    if (isSync)
    {
        clearIf(tempoChangeCallbackAsync, f);
        tempoChangeCallback = new Callback(this, "onTempoChange", f, isSync, 1);
        tempoChangeCallback->call(bpm, {}, {}, true);
    }
    else
    {
        clearIf(tempoChangeCallback, f);
        tempoChangeCallbackAsync = new Callback(this, "onTempoChange", f, isSync, 1);
        tempoChangeCallbackAsync->call(bpm, {}, {}, true);
    }
}
```

- Creates a new Callback with 1 arg
- Immediately fires with current `bpm` value (forceSync=true, so always synchronous on first call)
- Clears the opposite-mode slot if it held the same function

### setOnTransportChange (ScriptingApi.cpp:8446-8465)

```cpp
void ScriptingApi::TransportHandler::setOnTransportChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
    if (isSync)
    {
        clearIf(tempoChangeCallbackAsync, f);  // BUG: should be transportChangeCallbackAsync

        transportChangeCallback = new Callback(this, "onTransportChange", f, isSync, 1);
        transportChangeCallback->call(play, {}, {}, true);
    }
    else
    {
        clearIf(transportChangeCallback, f);

        transportChangeCallbackAsync = new Callback(this, "onTransportChange", f, isSync, 1);
        transportChangeCallbackAsync->call(play, {}, {}, true);
    }
}
```

**BUG at line 8452:** The sync branch calls `clearIf(tempoChangeCallbackAsync, f)` -- this clears the TEMPO async callback instead of the TRANSPORT async callback. This is a copy-paste error from `setOnTempoChange`. The async branch correctly clears `transportChangeCallback`. The bug means that registering a sync transport callback will NOT clear the async transport callback for the same function, and may inadvertently clear an unrelated tempo callback.

### setOnSignatureChange (ScriptingApi.cpp:8472-8490)

```cpp
void ScriptingApi::TransportHandler::setOnSignatureChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
    if (isSync)
    {
        clearIf(timeSignatureCallbackAsync, f);
        timeSignatureCallback = new Callback(this, "onTimeSignatureChange", f, isSync, 2);
        timeSignatureCallback->call(nom, denom, {}, true);
    }
    else
    {
        clearIf(timeSignatureCallback, f);
        timeSignatureCallbackAsync = new Callback(this, "onTimeSignatureChange", f, isSync, 2);
        timeSignatureCallbackAsync->call(nom, denom, {}, true);
    }
}
```

- 2 args: nominator, denominator
- Immediately fires with current nom/denom

### setOnBeatChange (ScriptingApi.cpp:8573-8593)

```cpp
void ScriptingApi::TransportHandler::setOnBeatChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
    if (f.isUndefined())
        getMainController()->removeMusicalUpdateListener(this);
    else
    {
        getMainController()->addMusicalUpdateListener(this);

        if (isSync)
        {
            clearIf(beatCallbackAsync, f);
            beatCallback = new Callback(this, "onBeatChange", f, isSync, 2);
        }
        else
        {
            clearIf(beatCallback, f);
            beatCallbackAsync = new Callback(this, "onBeatChange", f, isSync, 2);
        }
    }
}
```

- 2 args: beatIndex, isNewBar
- **Musical update listener lifecycle:** Calls `addMusicalUpdateListener(this)` when a function is provided, `removeMusicalUpdateListener(this)` when `undefined` is passed. This enables beat tracking.
- Does NOT immediately fire with current state (unlike tempo/transport/signature)

### setOnGridChange (ScriptingApi.cpp:8596-8617)

```cpp
void ScriptingApi::TransportHandler::setOnGridChange(var sync, var f)
{
    auto isSync = ApiHelpers::isSynchronous(sync);
    
    if (f.isUndefined())
        getMainController()->removeMusicalUpdateListener(this);
    else
    {
        getMainController()->addMusicalUpdateListener(this);

        if (isSync)
        {
            clearIf(gridCallbackAsync, f);
            gridCallback = new Callback(this, "onGridChange", f, isSync, 3);
        }
        else
        {
            clearIf(gridCallback, f);
            gridCallbackAsync = new Callback(this, "onGridChange", f, isSync, 3);
        }
    }
}
```

- 3 args: gridIndex, timestamp, firstGridInPlayback
- Same musical update listener lifecycle as setOnBeatChange
- Does NOT immediately fire with current state

### setOnBypass (ScriptingApi.cpp:8619-8624)

```cpp
void ScriptingApi::TransportHandler::setOnBypass(var f)
{
    bypassCallback = new Callback(this, "onGridChange", f, false, 1);

    getMainController()->getPluginBypassHandler().listeners.addListener(*this, TransportHandler::onBypassUpdate, true);
}
```

- Always async (hardcoded `false` for sync parameter)
- 1 arg: isBypassed (bool)
- Registers with the plugin bypass handler
- Note: the internal name passed to Callback is "onGridChange" -- this is cosmetic (used for debug/diagnostics) and appears to be a copy-paste leftover but does not affect functionality
- The `true` parameter to `addListener` triggers an immediate callback with current state

### onBypassUpdate static callback (ScriptingApi.cpp:8741-8745)

```cpp
void ScriptingApi::TransportHandler::onBypassUpdate(TransportHandler& handler, bool state)
{
    if(handler.bypassCallback != nullptr)
        handler.bypassCallback->call(state, {}, {}, true);
}
```

Called by the bypass handler. Always calls with `forceSync=true` to immediately deliver.

---

## TempoListener Virtual Overrides

### tempoChanged (ScriptingApi.cpp:8492-8501)

```cpp
void ScriptingApi::TransportHandler::tempoChanged(double newTempo)
{
    bpm = newTempo;

    if (tempoChangeCallback != nullptr)
        tempoChangeCallback->call(newTempo);

    if (tempoChangeCallbackAsync != nullptr)
        tempoChangeCallbackAsync->call(newTempo);
}
```

### onTransportChange (ScriptingApi.cpp:8505-8514)

```cpp
void ScriptingApi::TransportHandler::onTransportChange(bool isPlaying, double /*ppqPosition*/)
{
    play = isPlaying;

    if (transportChangeCallback != nullptr)
        transportChangeCallback->call(isPlaying);

    if (transportChangeCallbackAsync != nullptr)
        transportChangeCallbackAsync->call(isPlaying);
}
```

Note: ppqPosition is ignored.

### onBeatChange (ScriptingApi.cpp:8516-8526)

```cpp
void ScriptingApi::TransportHandler::onBeatChange(int newBeat, bool isNewBar)
{
    beat = newBeat;
    newBar = isNewBar;

    if (beatCallback != nullptr)
        beatCallback->call(newBeat, newBar);

    if (beatCallbackAsync != nullptr)
        beatCallbackAsync->call(newBeat, newBar);
}
```

### onSignatureChange (ScriptingApi.cpp:8528-8538)

```cpp
void ScriptingApi::TransportHandler::onSignatureChange(int newNominator, int numDenominator)
{
    nom = newNominator;
    denom = numDenominator;

    if (timeSignatureCallback != nullptr)
        timeSignatureCallback->call(newNominator, numDenominator);

    if (timeSignatureCallbackAsync != nullptr)
        timeSignatureCallbackAsync->call(newNominator, numDenominator);
}
```

### onGridChange (ScriptingApi.cpp:8540-8571)

```cpp
void ScriptingApi::TransportHandler::onGridChange(int gridIndex_, uint16 timestamp, bool firstGridInPlayback_)
{
    gridIndex = gridIndex_;
    gridTimestamp = timestamp;
    firstGridInPlayback = firstGridInPlayback_;

    if (firstGridInPlayback)
        nextLocalIsFirst = true;

    auto unsignedIndex = (uint32)gridIndex;
    auto mask = (uint32)(localGridMultiplier - 1);
    auto filtered = unsignedIndex & mask;

    if((mask && filtered) || localBypassed)
    {
        return;
    }

    auto thisGridIndex = gridIndex >> localBitShift;

    if(thisGridIndex != (lastGridIndex+1))
        nextLocalIsFirst = true;

    if (gridCallback != nullptr)
        gridCallback->call(thisGridIndex, gridTimestamp, nextLocalIsFirst);

    if (gridCallbackAsync != nullptr)
        gridCallbackAsync->call(thisGridIndex, gridTimestamp, nextLocalIsFirst);

    lastGridIndex = thisGridIndex;
    nextLocalIsFirst = false;
}
```

**Grid multiplier filtering logic:**
1. `mask = localGridMultiplier - 1` (e.g. multiplier=4 -> mask=3 -> binary 0b11)
2. `filtered = gridIndex & mask` -- if non-zero, this grid tick is filtered out
3. The result is that only every Nth grid tick passes through (where N = localGridMultiplier)
4. `thisGridIndex = gridIndex >> localBitShift` -- the local grid index is the global index divided by the multiplier
5. If `thisGridIndex != lastGridIndex + 1`, it means the grid was discontinuous (playback jumped), so `nextLocalIsFirst` is set to true
6. If `localBypassed` is true, all grid ticks are filtered out

---

## Clock / Grid Control Methods

### setEnableGrid (ScriptingApi.cpp:8626-8637)

```cpp
void ScriptingApi::TransportHandler::setEnableGrid(bool shouldBeEnabled, int tempoFactor)
{
    if (isPositiveAndBelow(tempoFactor, (int)TempoSyncer::numTempos))
    {
        auto t = (TempoSyncer::Tempo)tempoFactor;
        getMainController()->getMasterClock().setClockGrid(shouldBeEnabled, t);
    }
    else
    {
        reportScriptError("Illegal tempo value. Use 1-18");
    }
}
```

- `tempoFactor` is a TempoSyncer::Tempo index (0-18 without extended values)
- Reports script error if out of range (the error message says "1-18" but the actual valid range is 0 to numTempos-1, which is 0-18)
- Delegates to `MasterClock::setClockGrid()`
- This is a GLOBAL setting -- affects all TransportHandler instances

### setLocalGridMultiplier (ScriptingApi.cpp:8639-8656)

```cpp
void ScriptingApi::TransportHandler::setLocalGridMultiplier(int factor)
{
    if (factor != 1 && !isPowerOfTwo(factor))
        reportScriptError("factor must be power of two (or 1).");

    factor = jlimit(1, 64, factor);

    if(factor == 1)
    {
        localGridMultiplier = 1;
        localBitShift = 0;
    }
    else
    {
        localGridMultiplier = factor;
        localBitShift = log2(factor);
    }
}
```

- Must be 1 or a power of two (2, 4, 8, 16, 32, 64)
- Reports script error if not power of two
- Clamped to range [1, 64]
- This is LOCAL to this TransportHandler instance -- other instances are not affected
- Slows down grid callbacks by only passing every Nth grid tick

### setLocalGridBypassed (ScriptingApi.cpp:8658-8667)

```cpp
void ScriptingApi::TransportHandler::setLocalGridBypassed(bool shouldBeBypassed)
{
    if(shouldBeBypassed != localBypassed)
    {
        localBypassed = shouldBeBypassed;

        if(!localBypassed)
            nextLocalIsFirst = true;
    }
}
```

- When unbypassing, sets `nextLocalIsFirst = true` so the next grid callback gets `firstGridInPlayback = true`
- LOCAL to this instance

### setSyncMode (ScriptingApi.cpp:8699-8702)

```cpp
void ScriptingApi::TransportHandler::setSyncMode(int syncMode)
{
    getMainController()->getMasterClock().setSyncMode((MasterClock::SyncModes)syncMode);
}
```

- Directly sets the MasterClock sync mode
- No validation -- the enum cast is unchecked
- GLOBAL setting

### startInternalClock (ScriptingApi.cpp:8669-8682)

```cpp
void ScriptingApi::TransportHandler::startInternalClock(int timestamp)
{
    auto& clock = getMainController()->getMasterClock();

    if(clock.changeState(timestamp, true, true))
    {
        if(getMainController()->isInsideAudioRendering())
        {
            auto gi = clock.processAndCheckGrid(getMainController()->getBufferSizeForCurrentBlock(), {});
            auto ph = clock.createInternalPlayHead();
            getMainController()->handleTransportCallbacks(ph, gi);
        }
    }
}
```

- `changeState(timestamp, internalClock=true, startPlayback=true)` -- starts the internal clock
- If inside audio rendering, immediately processes grid and transport callbacks
- The `timestamp` parameter positions the start event within the current audio block (sample offset)
- GLOBAL -- affects the MasterClock, which all TransportHandler instances share

### stopInternalClock (ScriptingApi.cpp:8684-8697)

```cpp
void ScriptingApi::TransportHandler::stopInternalClock(int timestamp)
{
    auto& clock = getMainController()->getMasterClock();

    if(clock.changeState(timestamp, true, false))
    {
        if(getMainController()->isInsideAudioRendering())
        {
            auto gi = clock.processAndCheckGrid(getMainController()->getBufferSizeForCurrentBlock(), {});
            auto ph = clock.createInternalPlayHead();
            getMainController()->handleTransportCallbacks(ph, gi);
        }
    }
}
```

- Mirror of startInternalClock with `startPlayback=false`
- Same immediate processing pattern

### stopInternalClockOnExternalStop (ScriptingApi.cpp:8467-8470)

```cpp
void ScriptingApi::TransportHandler::stopInternalClockOnExternalStop(bool shouldStop)
{
    getMainController()->getMasterClock().setStopInternalClockOnExternalStop(shouldStop);
}
```

- Simple delegation to MasterClock
- GLOBAL setting

### sendGridSyncOnNextCallback (ScriptingApi.cpp:8704-8707)

```cpp
void ScriptingApi::TransportHandler::sendGridSyncOnNextCallback()
{
    getMainController()->getMasterClock().setNextGridIsFirst();
}
```

- Forces the next grid callback to have `firstGridInPlayback = true`
- GLOBAL -- affects MasterClock

### setLinkBpmToSyncMode (ScriptingApi.cpp:8709-8712)

```cpp
void ScriptingApi::TransportHandler::setLinkBpmToSyncMode(bool shouldPrefer)
{
    getMainController()->getMasterClock().setLinkBpmToSyncMode(shouldPrefer);
}
```

- GLOBAL -- affects MasterClock BPM source selection based on sync mode

---

## Query Methods

### isNonRealtime (ScriptingApi.cpp:8714-8717)

```cpp
bool ScriptingApi::TransportHandler::isNonRealtime() const
{
    return getScriptProcessor()->getMainController_()->getSampleManager().isNonRealtime();
}
```

- Queries the SampleManager for non-realtime (bounce/export) mode
- No allocation, lock-free read

### getGridLengthInSamples (ScriptingApi.cpp:8719-8727)

```cpp
double ScriptingApi::TransportHandler::getGridLengthInSamples() const
{
    auto bpm = getMainController()->getBpm();
    auto gridSpeed = getMainController()->getMasterClock().getCurrentClockGrid();
    auto tf = TempoSyncer::getTempoFactor(gridSpeed);
    auto sr = getMainController()->getMainSynthChain()->getSampleRate();
    tf *= (float)localGridMultiplier;
    return TempoSyncer::getTempoInSamples(bpm, sr, tf);
}
```

- Computes grid duration in samples based on: current BPM, clock grid speed, local multiplier, and sample rate
- Accounts for the local grid multiplier (multiplied into the tempo factor)
- Pure computation, no allocations

### isPlaying (ScriptingApi.cpp:8729-8732)

```cpp
bool ScriptingApi::TransportHandler::isPlaying() const
{
    return getMainController()->getMasterClock().isPlaying();
}
```

- Reads the MasterClock play state
- Lock-free read

### getGridPosition (ScriptingApi.cpp:8734-8739)

```cpp
int ScriptingApi::TransportHandler::getGridPosition(int timestamp) const
{
    auto ppq = getMainController()->getMasterClock().getPPQPos(timestamp);
    return ppq;
}
```

- Gets the PPQ position for the given timestamp offset
- The return type `int` truncates the PPQ to an integer grid position
- Lock-free read

---

## Preprocessor Guards

None. TransportHandler has no `#if USE_BACKEND`, `#if USE_FRONTEND`, or any other preprocessor guards around the class declaration or implementation. All methods are available in all build configurations.

The only backend-only code is inside the `Callback::Callback` constructor (the realtime safety check at line 8306-8311), which is a diagnostic/validation check that does not affect runtime behavior.

---

## Threading Summary

| Context | Thread | What runs here |
|---------|--------|----------------|
| Callback registration (`setOn*`) | Any thread (typically onInit/message thread) | Allocates Callback objects, stores in ScopedPointer |
| TempoListener virtual overrides | Audio thread | Cache state, dispatch to sync/async callbacks |
| Sync callback dispatch | Audio thread (via `callSync()`) | Executes user's inline function |
| Async callback dispatch | UI thread (via PooledUIUpdater -> `handlePooledMessage` -> `callAsync`) | Executes user's function |
| Query methods (`isPlaying`, etc.) | Any thread | Lock-free reads |
| Clock control (`startInternalClock`, etc.) | Should be called from audio thread (for timestamp accuracy) but not enforced | Delegates to MasterClock |

---

## Bug: setOnTransportChange clearIf Target

**Location:** ScriptingApi.cpp:8452
**Issue:** The sync branch of `setOnTransportChange` calls `clearIf(tempoChangeCallbackAsync, f)` instead of `clearIf(transportChangeCallbackAsync, f)`.
**Impact:** When registering a synchronous transport callback, the async transport callback is NOT cleared (it should be). Additionally, if the tempo async callback happens to hold the same function reference, it would be incorrectly cleared.
**Root cause:** Copy-paste from `setOnTempoChange` without updating the variable name.
