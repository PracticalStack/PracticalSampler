# Timer -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- Timer entry (domain: ui, role: utility)
- `enrichment/phase1/Engine/Readme.md` -- prerequisite class context (Timer created by `Engine.createTimerObject()`)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:2834-2912` -- class declaration
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp:5374-5480` -- full implementation
- `HISE/hi_scripting/scripting/api/ScriptingApi.h:634` -- factory method declaration
- `HISE/hi_scripting/scripting/api/ScriptingApi.cpp:3488` -- factory method implementation
- `HISE/hi_scripting/scripting/api/ScriptingBaseObjects.h:253-320` -- WeakCallbackHolder definition
- `HISE/hi_scripting/scripting/api/ScriptMacroDefinitions.h:28-43` -- ADD_CALLBACK_DIAGNOSTIC macro
- `HISE/hi_scripting/scripting/components/PopupEditors.cpp:329` -- IDE snippet template
- `HISE/hi_core/hi_core/UtilityClasses.h:267-306` -- ControlledObject base class

---

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h`, line 2834

```cpp
class TimerObject : public ConstScriptingObject,
                    public ControlledObject
{
public:
    TimerObject(ProcessorWithScriptingContent *p);
    ~TimerObject();

    void mainControllerIsDeleted() { stopTimer(); };

    Identifier getObjectName() const override { return "Timer"; }
    bool objectDeleted() const override { return false; }
    bool objectExists() const override { return false; }

    void timerCallback();

    int getNumChildElements() const override { return 2; }
    DebugInformationBase* getChildElement(int index) override;

    // API Methods
    void startTimer(int intervalInMilliSeconds);
    void stopTimer();
    void setTimerCallback(var callbackFunction);
    bool isTimerRunning() const;
    var getMilliSecondsSinceCounterReset();
    void resetCounter();

private:
    uint32 milliSecondCounter;
    struct Wrapper;

    struct InternalTimer : public Timer
    {
        InternalTimer(TimerObject* parent_): parent(parent_) {}
        void timerCallback() { parent->timerCallback(); }
    private:
        TimerObject* parent;
    };

    InternalTimer it;
    WeakCallbackHolder tc;
};
```

### Inheritance Chain

1. **ConstScriptingObject** -- standard HISE scripting API base, provides `addApiMethod`, `ADD_API_METHOD_N` registration, debug information, and the `getScriptProcessor()` accessor.
2. **ControlledObject** -- provides access to the `MainController` via `getMainController()`. Constructed with `notifyOnShutdown=true`, meaning `mainControllerIsDeleted()` is called during shutdown, which stops the timer cleanly.

### Key Observations

- `objectDeleted()` returns `false` and `objectExists()` returns `false` -- this is unusual. Most API objects return `true` for `objectExists()`. This means the timer object technically reports "does not exist" in the debug system, but still functions. This is likely because the Timer has no underlying processor reference to validate -- it is a standalone utility object.
- The class is nested inside `ScriptingObjects` namespace (as `ScriptingObjects::TimerObject`).
- `getObjectName()` returns `"Timer"` -- this is the scripting API class name.

---

## Constructor Analysis

**File:** `ScriptingApiObjects.cpp`, line 5384

```cpp
ScriptingObjects::TimerObject::TimerObject(ProcessorWithScriptingContent *p) :
    ConstScriptingObject(p, 0),           // 0 = no constants
    ControlledObject(p->getMainController_(), true),  // true = notify on shutdown
    it(this),                              // InternalTimer with parent back-pointer
    tc(p, this, {}, 0)                     // WeakCallbackHolder: empty initial callback, 0 args
{
    ADD_API_METHOD_0(isTimerRunning);
    ADD_API_METHOD_1(startTimer);
    ADD_API_METHOD_0(stopTimer);
    ADD_TYPED_API_METHOD_1(setTimerCallback, VarTypeChecker::Function);
    ADD_CALLBACK_DIAGNOSTIC(tc, setTimerCallback, 0);
    ADD_API_METHOD_0(resetCounter);
    ADD_API_METHOD_0(getMilliSecondsSinceCounterReset);
}
```

### Key Details

- **Zero constants:** `ConstScriptingObject(p, 0)` -- no `addConstant()` calls. The Timer class has no constants.
- **One typed method:** `setTimerCallback` uses `ADD_TYPED_API_METHOD_1` with `VarTypeChecker::Function`, enforcing that the argument must be a function.
- **Callback diagnostic:** `ADD_CALLBACK_DIAGNOSTIC(tc, setTimerCallback, 0)` registers a diagnostic for the `setTimerCallback` method in backend builds. The macro expands to `tc.addCallbackDiagnostic(this, "setTimerCallback", 0)` which enables realtime safety analysis of the callback function.
- **All other methods** use plain `ADD_API_METHOD_N` (untyped).

---

## Wrapper Struct (Method Registration)

```cpp
struct ScriptingObjects::TimerObject::Wrapper
{
    API_METHOD_WRAPPER_0(TimerObject, isTimerRunning);
    API_VOID_METHOD_WRAPPER_1(TimerObject, startTimer);
    API_VOID_METHOD_WRAPPER_0(TimerObject, stopTimer);
    API_VOID_METHOD_WRAPPER_1(TimerObject, setTimerCallback);
    API_METHOD_WRAPPER_0(TimerObject, getMilliSecondsSinceCounterReset);
    API_VOID_METHOD_WRAPPER_0(TimerObject, resetCounter);
};
```

These are the standard HISE API wrapper macros that bridge the JavaScript engine to C++ method calls. `API_METHOD_WRAPPER_0` is for methods with return values and 0 params; `API_VOID_METHOD_WRAPPER_N` is for void methods.

---

## Internal Timer Mechanism

The TimerObject uses a **composition pattern** with JUCE's `Timer` class rather than inheriting from it directly:

```cpp
struct InternalTimer : public Timer
{
    InternalTimer(TimerObject* parent_): parent(parent_) {}
    void timerCallback() { parent->timerCallback(); }
private:
    TimerObject* parent;
};
```

- `InternalTimer` inherits from `juce::Timer`, which fires callbacks on the **message thread** (JUCE's UI/event thread).
- The `InternalTimer::timerCallback()` delegates to `TimerObject::timerCallback()`.
- This composition avoids multiple inheritance conflicts (both `ConstScriptingObject` and `juce::Timer` could clash).

### Threading Implication

**All timer callbacks execute on the message thread.** This is a fundamental constraint inherited from JUCE's Timer implementation. The timer callback is NOT on the audio thread. This makes it safe for UI operations but not suitable for sample-accurate timing.

---

## Method Implementations

### startTimer

```cpp
void ScriptingObjects::TimerObject::startTimer(int intervalInMilliSeconds)
{
    if (intervalInMilliSeconds > 10)
    {
        it.startTimer(intervalInMilliSeconds);
        resetCounter();
    }
    else
        throw String("Go easy on the timer");
}
```

**Key behavior:**
- **Minimum interval: 10ms.** Any value <= 10 throws an exception with the message "Go easy on the timer". This prevents excessive CPU usage from too-frequent timer callbacks.
- Calling `startTimer` automatically calls `resetCounter()`, resetting the millisecond counter.
- If the timer is already running, calling `startTimer` again changes the interval (JUCE `Timer::startTimer` behavior) and resets the counter.

### stopTimer

```cpp
void ScriptingObjects::TimerObject::stopTimer()
{
    it.stopTimer();
}
```

Simple delegation to the internal JUCE timer's `stopTimer()`.

### setTimerCallback

```cpp
void ScriptingObjects::TimerObject::setTimerCallback(var callbackFunction)
{
    tc = WeakCallbackHolder(getScriptProcessor(), this, callbackFunction, 0);
    tc.incRefCount();
    tc.setThisObject(this);
    tc.addAsSource(this, "timerCallback");
}
```

**Key behavior:**
- Creates a new `WeakCallbackHolder` wrapping the callback function.
- `incRefCount()` prevents premature garbage collection of the callback.
- `setThisObject(this)` sets the `this` context for the callback to the TimerObject itself.
- `addAsSource(this, "timerCallback")` registers this as a debug source for the callback (used in the HISE IDE's watch system).
- The callback takes **0 arguments** (the `0` in the `WeakCallbackHolder` constructor).
- Calling `setTimerCallback` replaces any previously set callback.

### timerCallback (internal)

```cpp
void ScriptingObjects::TimerObject::timerCallback()
{
    if (tc)
        tc.call(nullptr, 0);
    else
        it.stopTimer();
}
```

**Key behavior:**
- If the `WeakCallbackHolder` is valid (callback still exists, script processor not deleted), it calls the callback with 0 arguments.
- If the callback holder is invalid (e.g., script was recompiled, callback was garbage collected), the timer **automatically stops itself**. This is a safety mechanism.

### isTimerRunning

```cpp
bool ScriptingObjects::TimerObject::isTimerRunning() const
{
    return it.isTimerRunning();
}
```

Direct delegation to JUCE `Timer::isTimerRunning()`.

### getMilliSecondsSinceCounterReset

```cpp
var ScriptingObjects::TimerObject::getMilliSecondsSinceCounterReset()
{
    auto now = Time::getMillisecondCounter();
    return now - milliSecondCounter;
}
```

**Key behavior:**
- Uses `juce::Time::getMillisecondCounter()` which returns a `uint32` value that wraps around approximately every 49.7 days.
- Returns the difference as a `var` (will be a number in JavaScript).
- The counter is set by `resetCounter()` and also automatically reset by `startTimer()`.

### resetCounter

```cpp
void ScriptingObjects::TimerObject::resetCounter()
{
    milliSecondCounter = Time::getMillisecondCounter();
}
```

Stores the current millisecond counter value. Subsequent calls to `getMilliSecondsSinceCounterReset()` return the elapsed time since this snapshot.

---

## Destructor

```cpp
ScriptingObjects::TimerObject::~TimerObject()
{
    it.stopTimer();
}
```

Stops the timer on destruction. Combined with the `mainControllerIsDeleted()` override (which also calls `stopTimer()`), this ensures clean shutdown in two scenarios:
1. Normal object destruction (destructor)
2. MainController shutdown (via ControlledObject notification)

---

## Debug Information

```cpp
hise::DebugInformationBase* ScriptingObjects::TimerObject::getChildElement(int index)
{
    if (index == 0)
    {
        WeakReference<TimerObject> safeThis(this);
        auto vf = [safeThis]() {
            if (safeThis != nullptr)
                return var(safeThis->getMilliSecondsSinceCounterReset());
            return var(0);
        };
        Identifier id("%PARENT%.durationSinceReset");
        return new LambdaValueInformation(vf, id, {}, 
            (DebugInformation::Type)getTypeNumber(), getLocation());
    }
    if (index == 1)
    {
        return tc.createDebugObject("timerCallback");
    }
    return nullptr;
}
```

Exposes two child elements in the HISE IDE debugger:
1. **durationSinceReset** -- a live-updating display of `getMilliSecondsSinceCounterReset()`
2. **timerCallback** -- the callback function object for inspection

---

## Factory Method (obtainedVia)

**File:** `ScriptingApi.h:634`, `ScriptingApi.cpp:3488`

```cpp
// Declaration
ScriptingObjects::TimerObject* createTimerObject();

// Implementation
ScriptingObjects::TimerObject* ScriptingApi::Engine::createTimerObject()
{
    return new ScriptingObjects::TimerObject(getScriptProcessor());
}
```

Created via `Engine.createTimerObject()`. Simple factory -- creates a new `TimerObject` with the current script processor as owner. No singleton pattern, no caching -- each call creates a new independent timer.

---

## IDE Snippet Template

**File:** `PopupEditors.cpp:329`

```cpp
ADD_HS_SNIPPET("timer (...)", 
    "const var $TIMER_VAR$ = Engine.createTimerObject();\n\n"
    "$TIMER_VAR$.setTimerCallback(function()\n"
    "{\n\t$// timer callback$\n});\n\n"
    "$TIMER_VAR$.startTimer($30$);\n",
```

This shows the canonical usage pattern: create, set callback, start with interval.

---

## WeakCallbackHolder Context

The `WeakCallbackHolder` (`tc` member) is a core HISE infrastructure for safely calling JavaScript functions from C++. Key properties:

- **Weak reference semantics:** If the owning script processor is recompiled or deleted, the callback holder becomes invalid (operator bool returns false).
- **Thread safety:** The `call()` method can be invoked from any thread -- internally it dispatches to the correct thread context based on the callback's requirements.
- **Zero arguments:** Timer callbacks receive no arguments (`WeakCallbackHolder(p, this, {}, 0)` -- the `0` is the argument count).
- **CallableObject interface:** The callback function must implement the `CallableObject` interface, which includes realtime safety analysis (`getRealtimeSafetyReport`) in backend builds.

---

## Relationship to ScriptPanel Timer

ScriptPanel also has a `setTimerCallback` method (`ScriptingApiContent.h:1868`). The key distinction:

- **ScriptPanel timer** is integrated into a UI component -- the timer callback typically triggers `repaint()` for animation.
- **TimerObject** is a standalone utility object -- created via `Engine.createTimerObject()`, not tied to any visual component.
- Both ultimately use JUCE's `Timer` class and fire on the message thread.
- TimerObject additionally provides `getMilliSecondsSinceCounterReset()` and `resetCounter()` for elapsed-time measurement, which ScriptPanel's timer does not have.

---

## Preprocessor Guards

None. The Timer class has no preprocessor guards (`USE_BACKEND`, etc.). It is available in all build targets (backend, frontend, standalone).

The only backend-specific aspect is the `ADD_CALLBACK_DIAGNOSTIC` macro, which expands to nothing in non-backend builds (see `ScriptMacroDefinitions.h:42-43`).

---

## Threading and Lifecycle Summary

- **Timer fires on the message thread** (JUCE Timer guarantee).
- **Minimum interval: 10ms** (enforced by startTimer throwing an exception).
- **Auto-stop on invalid callback:** If the WeakCallbackHolder becomes invalid, the timer stops itself.
- **Clean shutdown:** Timer stops on both object destruction and MainController shutdown.
- **No audio-thread interaction:** Timer is purely a message-thread utility. Not suitable for sample-accurate timing.
- **No onInit restriction:** Timer can be created and started at any time (unlike some Engine methods that are init-only).
