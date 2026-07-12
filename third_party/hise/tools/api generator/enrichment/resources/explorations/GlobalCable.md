# GlobalCable Exploration (Phase 1 Step A1)

## 1. File Locations

### Primary Scripting API Files
- **Header declaration:** `hi_scripting/scripting/api/ScriptingApiObjects.h` (lines 2731-2801)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiObjects.cpp` (lines 8892-9507)

### Underlying Infrastructure
- **GlobalRoutingManager header:** `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingManager.h` (full file, 464 lines)
- **GlobalRoutingManager implementation:** `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingManager.cpp` (full file, 1968 lines)
- **GlobalCableNode header:** `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingNodes.h` (lines 114-170)

### Related Files
- **Factory method (getCable):** `hi_scripting/scripting/api/ScriptingApiObjects.cpp` (line 8713)
- **GlobalRoutingManagerReference:** `hi_scripting/scripting/api/ScriptingApiObjects.h` (lines 2647-2729)
- **Engine::getGlobalRoutingManager:** `hi_scripting/scripting/api/ScriptingApi.cpp` (line 2498)
- **ScriptComponent GlobalCableConnection:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` (lines 260-340)
- **GlobalModulatorContainer cable support:** `hi_core/hi_modules/synthesisers/synths/GlobalModulatorContainer.h` (lines 870-888) and `.cpp` (lines 35-79, 431-464)
- **getCableFromVar helper:** `hi_scripting/scripting/api/ScriptingApiObjects.cpp` (line 8666)
- **Runtime target enum:** `hi_tools/hi_tools/runtime_target.h` (line 47)

---

## 2. Class Declaration (Full Header)

```cpp
// ScriptingApiObjects.h lines 2731-2801

/** A wrapper around a global cable. */
struct GlobalCableReference : public ConstScriptingObject
{
    GlobalCableReference(ProcessorWithScriptingContent* ps, var c);

    ~GlobalCableReference();

    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("GlobalCable"); }

    // =============================================================================================

    /** Returns the value (converted to the input range). */
    double getValue() const;

    /** Returns the normalised value between 0...1 */
    double getValueNormalised() const;

    /** Sends the normalised value to all targets. */
    void setValueNormalised(double normalisedInput);

    /** Sends the value to all targets (after converting it from the input range. */
    void setValue(double inputWithinRange);

    /** Sends any type of data (JSON, string, buffers) to the target. */
    void sendData(var dataToSend);

    /** Set the input range using a min and max value (no steps / no skew factor). */
    void setRange(double min, double max);

    /** Set the input range using a min and max value and a mid point for skewing the range. */
    void setRangeWithSkew(double min, double max, double midPoint);

    /** Set the input range using a min and max value as well as a step size. */
    void setRangeWithStep(double min, double max, double stepSize);

    /** Registers a function that will be executed asynchronously when the data receives a JSON data chunk. */
    void registerDataCallback(var dataCallbackFunction);

    /** Registers a function that will be executed whenever a value is sent through the cable. */
    void registerCallback(var callbackFunction, var synchronous);

    /** Deregisteres a callback from the cable. */
    bool deregisterCallback(var callbackFunction);
    
    /** Connects the cable to a macro control. */
    void connectToMacroControl(int macroIndex, bool macroIsTarget, bool filterRepetitions);

    /** Connects the cable to a global LFO modulation output as source. */
    void connectToGlobalModulator(const String& lfoId, bool addToMod);
    
    /** Connects the cable to a module parameter using a JSON object for defining the range. */
    void connectToModuleParameter(const String& processorId, var parameterIndexOrId, var targetObject);
    
    // =============================================================================================

private:

    struct DummyTarget;
    struct Wrapper;
    struct Callback;
    struct DataCallback;

    var cable;

    ScopedPointer<DummyTarget> dummyTarget;
    OwnedArray<Callback> callbacks;
    OwnedArray<DataCallback> dataCallbacks;
    scriptnode::InvertableParameterRange inputRange;

    bool dataRecursion = false;
};
```

### Inheritance Chain
```
GlobalCableReference
  -> ConstScriptingObject
       -> ScriptingObject (provides getScriptProcessor(), reportScriptError(), etc.)
       -> DynamicObject (provides JUCE var-based object model)
       -> ApiClass (provides method registration via ADD_API_METHOD_N macros)
       -> DebugableObjectBase (provides getDebugName(), etc.)
```

### Object Name in HiseScript
The object presents itself as `"GlobalCable"` via `RETURN_STATIC_IDENTIFIER("GlobalCable")`.

---

## 3. Obtaining a GlobalCable Instance (obtainedVia)

### Step 1: Get a GlobalRoutingManager

In HiseScript:
```javascript
const var rm = Engine.getGlobalRoutingManager();
```

C++ implementation:
```cpp
// ScriptingApi.cpp line 2498
juce::var ScriptingApi::Engine::getGlobalRoutingManager()
{
    return var(new ScriptingObjects::GlobalRoutingManagerReference(getScriptProcessor()));
}
```

The `GlobalRoutingManagerReference` constructor calls `GlobalRoutingManager::Helpers::getOrCreate(mc)`:
```cpp
// ScriptingApiObjects.cpp line 8637
ScriptingObjects::GlobalRoutingManagerReference::GlobalRoutingManagerReference(ProcessorWithScriptingContent* sp) :
    ConstScriptingObject(sp, 0),
    ControlledObject(sp->getMainController_()),
    errorCallback(sp, this, var(), 1)
{
    auto ptr = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(getMainController());
    manager = ptr.get();

    ADD_API_METHOD_1(getCable);
    ADD_API_METHOD_2(connectToOSC);
    ADD_API_METHOD_2(sendOSCMessage);
    ADD_API_METHOD_2(addOSCCallback);
    ADD_API_METHOD_1(removeOSCCallback);
    ADD_API_METHOD_3(setEventData);
    ADD_API_METHOD_2(getEventData);
}
```

### Step 2: Get a Cable from the Manager

In HiseScript:
```javascript
const var cable = rm.getCable("MyCableName");
```

C++ implementation:
```cpp
// ScriptingApiObjects.cpp line 8713
juce::var ScriptingObjects::GlobalRoutingManagerReference::getCable(String cableId)
{
    if (auto m = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(manager.getObject()))
    {
        auto c = m->getSlotBase(cableId, scriptnode::routing::GlobalRoutingManager::SlotBase::SlotType::Cable);

        return new GlobalCableReference(getScriptProcessor(), var(c.get()));
    }

    return var();
}
```

Key: `getSlotBase()` creates the cable if it doesn't exist yet:
```cpp
// GlobalRoutingManager.cpp line 654
juce::ReferenceCountedObjectPtr<scriptnode::routing::GlobalRoutingManager::SlotBase> 
GlobalRoutingManager::getSlotBase(const String& id, SlotBase::SlotType t)
{
    auto isCable = t == SlotBase::SlotType::Cable;
    auto& lToUse = isCable ? cables : signals;

    for (auto c : lToUse)
    {
        if (c->id == id)
            return c;
    }

    SlotBase::Ptr newSlot;

    if (isCable)
    {
        newSlot = new Cable(id);
        addOSCTarget(newSlot);
    }
    else
        newSlot = new Signal(id);

    lToUse.add(newSlot);
    listUpdater.sendMessage(sendNotificationSync, t, getIdList(t));

    return newSlot;
}
```

---

## 4. Constructor -- Method Registrations

```cpp
// ScriptingApiObjects.cpp line 8958
ScriptingObjects::GlobalCableReference::GlobalCableReference(ProcessorWithScriptingContent* ps, var c) :
    ConstScriptingObject(ps, 0),  // 0 constants
    cable(c),
    dummyTarget(new DummyTarget(*this)),
    inputRange(0.0, 1.0)
{
    ADD_API_METHOD_0(getValue);
    ADD_API_METHOD_0(getValueNormalised);
    ADD_API_METHOD_1(setValue);
    ADD_API_METHOD_1(sendData);
    ADD_API_METHOD_1(setValueNormalised);
    ADD_API_METHOD_2(setRange);
    ADD_API_METHOD_3(setRangeWithSkew);
    ADD_API_METHOD_3(setRangeWithStep);
    ADD_API_METHOD_2(registerCallback);
    ADD_API_METHOD_1(registerDataCallback);
    ADD_API_METHOD_1(deregisterCallback);
    ADD_API_METHOD_3(connectToMacroControl);
    ADD_API_METHOD_2(connectToGlobalModulator);
    ADD_API_METHOD_3(connectToModuleParameter);

    inputRange.checkIfIdentity();
}
```

**Constants:** None (`ConstScriptingObject(ps, 0)`, no `addConstant()` calls).

**All method registrations (14 total):**

| # | Macro | Method | Args |
|---|-------|--------|------|
| 1 | `ADD_API_METHOD_0` | `getValue` | 0 |
| 2 | `ADD_API_METHOD_0` | `getValueNormalised` | 0 |
| 3 | `ADD_API_METHOD_1` | `setValue` | 1 |
| 4 | `ADD_API_METHOD_1` | `sendData` | 1 |
| 5 | `ADD_API_METHOD_1` | `setValueNormalised` | 1 |
| 6 | `ADD_API_METHOD_2` | `setRange` | 2 |
| 7 | `ADD_API_METHOD_3` | `setRangeWithSkew` | 3 |
| 8 | `ADD_API_METHOD_3` | `setRangeWithStep` | 3 |
| 9 | `ADD_API_METHOD_2` | `registerCallback` | 2 |
| 10 | `ADD_API_METHOD_1` | `registerDataCallback` | 1 |
| 11 | `ADD_API_METHOD_1` | `deregisterCallback` | 1 |
| 12 | `ADD_API_METHOD_3` | `connectToMacroControl` | 3 |
| 13 | `ADD_API_METHOD_2` | `connectToGlobalModulator` | 2 |
| 14 | `ADD_API_METHOD_3` | `connectToModuleParameter` | 3 |

All use **untyped** `ADD_API_METHOD_N` (not `ADD_TYPED_API_METHOD_N`).

---

## 5. API Wrapper Struct

```cpp
// ScriptingApiObjects.cpp line 8892
struct ScriptingObjects::GlobalCableReference::Wrapper
{
    API_METHOD_WRAPPER_0(GlobalCableReference, getValue);
    API_METHOD_WRAPPER_0(GlobalCableReference, getValueNormalised);
    API_VOID_METHOD_WRAPPER_1(GlobalCableReference, setValue);
    API_VOID_METHOD_WRAPPER_1(GlobalCableReference, sendData);
    API_VOID_METHOD_WRAPPER_1(GlobalCableReference, setValueNormalised);
    API_VOID_METHOD_WRAPPER_2(GlobalCableReference, setRange);
    API_VOID_METHOD_WRAPPER_3(GlobalCableReference, setRangeWithSkew);
    API_VOID_METHOD_WRAPPER_3(GlobalCableReference, setRangeWithStep);
    API_VOID_METHOD_WRAPPER_2(GlobalCableReference, registerCallback);
    API_VOID_METHOD_WRAPPER_1(GlobalCableReference, registerDataCallback);
    API_METHOD_WRAPPER_1(GlobalCableReference, deregisterCallback);
    API_VOID_METHOD_WRAPPER_3(GlobalCableReference, connectToMacroControl);
    API_VOID_METHOD_WRAPPER_2(GlobalCableReference, connectToGlobalModulator);
    API_VOID_METHOD_WRAPPER_3(GlobalCableReference, connectToModuleParameter);
};
```

---

## 6. All Method Implementations (Full Bodies)

### 6.1 getValue()

```cpp
// ScriptingApiObjects.cpp line 8987
double ScriptingObjects::GlobalCableReference::getValue() const
{
    auto v = getValueNormalised();
    return inputRange.convertFrom0to1(v, true);
}
```

Returns the cable's normalised value converted back through the local `inputRange`. If no range set, `inputRange` is identity (0...1), so this returns the raw normalised value.

### 6.2 getValueNormalised()

```cpp
// ScriptingApiObjects.cpp line 8993
double ScriptingObjects::GlobalCableReference::getValueNormalised() const
{
    if (auto c = getCableFromVar(cable))
        return c->lastValue;
    
    return 0.0;
}
```

Reads `Cable::lastValue` directly. No lock taken -- the double read is atomic on most platforms but not formally atomic.

### 6.3 setValueNormalised()

```cpp
// ScriptingApiObjects.cpp line 9001
void ScriptingObjects::GlobalCableReference::setValueNormalised(double normalisedInput)
{
    if (auto c = getCableFromVar(cable))
        c->sendValue(nullptr, normalisedInput);
}
```

Sends the normalised value to all cable targets. `nullptr` source means no target is skipped.

### 6.4 setValue()

```cpp
// ScriptingApiObjects.cpp line 9007
void ScriptingObjects::GlobalCableReference::setValue(double inputWithinRange)
{
    auto v = inputRange.convertTo0to1(inputWithinRange, true);
    setValueNormalised(v);
}
```

Converts using the local `inputRange` first, then delegates.

### 6.5 sendData()

```cpp
// ScriptingApiObjects.cpp line 9013
void ScriptingObjects::GlobalCableReference::sendData(var dataToSend)
{
    if(auto c = getCableFromVar(cable))
    {
        MemoryOutputStream mos;
        dataToSend.writeToStream(mos);
        mos.flush();

        ScopedValueSetter<bool> svs(dataRecursion, true);
        c->sendData(nullptr, const_cast<void*>(mos.getData()), mos.getDataSize());
    }
}
```

Serializes any `var` (JSON, string, buffers, etc.) to a binary stream, sets a `dataRecursion` guard, then pushes the raw bytes through the cable. The recursion guard prevents this same reference's `DataCallback` from re-firing.

### 6.6 setRange()

```cpp
// ScriptingApiObjects.cpp line 9027
void ScriptingObjects::GlobalCableReference::setRange(double min, double max)
{
    inputRange = scriptnode::InvertableParameterRange(min, max);
    inputRange.checkIfIdentity();
}
```

### 6.7 setRangeWithSkew()

```cpp
// ScriptingApiObjects.cpp line 9033
void ScriptingObjects::GlobalCableReference::setRangeWithSkew(double min, double max, double midPoint)
{
    inputRange = scriptnode::InvertableParameterRange(min, max);
    inputRange.setSkewForCentre(midPoint);
    inputRange.checkIfIdentity();
}
```

### 6.8 setRangeWithStep()

```cpp
// ScriptingApiObjects.cpp line 9040
void ScriptingObjects::GlobalCableReference::setRangeWithStep(double min, double max, double stepSize)
{
    inputRange = scriptnode::InvertableParameterRange(min, max, stepSize);
    inputRange.checkIfIdentity();
}
```

### 6.9 registerDataCallback()

```cpp
// ScriptingApiObjects.cpp line 9135
void ScriptingObjects::GlobalCableReference::registerDataCallback(var dataCallbackFunction)
{
    if (HiseJavascriptEngine::isJavascriptFunction(dataCallbackFunction))
    {
        auto nc = new DataCallback(*this, dataCallbackFunction);
        dataCallbacks.add(nc);
    }
}
```

Creates a `DataCallback` inner struct and adds it to the `dataCallbacks` OwnedArray. The callback is always async/high-priority.

### 6.10 registerCallback()

```cpp
// ScriptingApiObjects.cpp line 9261
void ScriptingObjects::GlobalCableReference::registerCallback(var callbackFunction, var synchronous)
{
    if (HiseJavascriptEngine::isJavascriptFunction(callbackFunction))
    {
        bool isSync = ApiHelpers::isSynchronous(synchronous);
        
        auto nc = new Callback(*this, callbackFunction, isSync);
        callbacks.add(nc);
    }
}
```

The `synchronous` parameter is resolved via:
```cpp
// ScriptingApiObjects.cpp line 6893
bool ApiHelpers::isSynchronous(const var& syncValue)
{
    return getDispatchType(syncValue, false) == dispatch::DispatchType::sendNotificationSync;
}
```

Synchronous callbacks run in `callSync` (audio-thread safe with realtime-safe function). Asynchronous callbacks use `PooledUIUpdater::SimpleTimer` polling with `ModValue` changed-value detection.

### 6.11 deregisterCallback()

```cpp
// ScriptingApiObjects.cpp line 9272
bool ScriptingObjects::GlobalCableReference::deregisterCallback(var callbackFunction)
{
    for(auto c: dataCallbacks)
    {
        if(c->callback.matches(callbackFunction))
        {
            dataCallbacks.removeObject(c);
            return true;
        }
    }

    for(auto c: callbacks)
    {
        if(c->callback.matches(callbackFunction))
        {
            callbacks.removeObject(c);
            return true;
        }
    }

    return false;
}
```

Searches both `dataCallbacks` and `callbacks` arrays. Uses `WeakCallbackHolder::matches()` to identify which callback to remove. Returns `true` if found and removed, `false` otherwise.

### 6.12 connectToMacroControl()

```cpp
// ScriptingApiObjects.cpp line 9345
void ScriptingObjects::GlobalCableReference::connectToMacroControl(int macroIndex, bool macroIsTarget, bool filterRepetitions)
{
    if (auto c = getCableFromVar(cable))
    {
        if (macroIsTarget)
        {
            using CableType = scriptnode::routing::GlobalRoutingManager::CableTargetBase;

            for (int i = 0; i < c->getTargetList().size(); i++)
            {
                if (auto m = dynamic_cast<MacroCableTarget*>(c->getTargetList()[i].get()))
                {
                    if (macroIndex == -1 || m->macroIndex == macroIndex)
                    {
                        c->removeTarget(m);
                        i--;
                        continue;
                    }
                }
            }

            if (macroIndex != -1)
                c->addTarget(new MacroCableTarget(getScriptProcessor()->getMainController_(), macroIndex, filterRepetitions));
        }
        else
        {
            // not implemented
            jassertfalse;
        }
    }
}
```

When `macroIndex == -1`, removes all existing `MacroCableTarget` entries. When `macroIndex >= 0`, removes any existing one for the same index first, then creates and adds a new `MacroCableTarget`. The `macroIsTarget=false` path (macro as source) is not implemented (`jassertfalse`).

### 6.13 connectToGlobalModulator()

```cpp
// ScriptingApiObjects.cpp line 9378
void ScriptingObjects::GlobalCableReference::connectToGlobalModulator(const String& lfoId, bool addToMod)
{
    auto mc = getScriptProcessor()->getMainController_()->getMainSynthChain();
    
    if(auto p = ProcessorHelpers::getFirstProcessorWithName(mc, lfoId))
    {
        if(auto gc = dynamic_cast<GlobalModulatorContainer*>(p->getParentProcessor(true)))
        {
            gc->connectToGlobalCable(dynamic_cast<Modulator*>(p), cable, addToMod);
        }
    }
}
```

Looks up a processor by name, verifies its parent is a `GlobalModulatorContainer`, then calls `connectToGlobalCable` passing the raw `cable` var (which holds the `ReferenceCountedObject*` to the Cable).

### 6.14 connectToModuleParameter()

```cpp
// ScriptingApiObjects.cpp line 9440
void ScriptingObjects::GlobalCableReference::connectToModuleParameter(const String& processorId, var parameterIndex, var targetRange)
{
    auto mc = getScriptProcessor()->getMainController_()->getMainSynthChain();
    
    if(processorId.isEmpty() && (int)parameterIndex == -1)
    {
        if (auto c = getCableFromVar(cable))
        {
            // Clear all module connections for this cable
            for (int i = 0; i < c->getTargetList().size(); i++)
            {
                if (auto ppt = dynamic_cast<ProcessorParameterTarget*>(c->getTargetList()[i].get()))
                {
                    c->removeTarget(ppt);
                    i--;
                    continue;
                }
            }
        }
    }
    
    if(auto p = ProcessorHelpers::getFirstProcessorWithName(mc, processorId))
    {
        auto indexToUse = -1;
        
        if(parameterIndex.isString())
        {
            Identifier pId(parameterIndex.toString());
            indexToUse = p->parameterNames.indexOf(pId);
            
            if(indexToUse == -1)
                reportScriptError("Can't find parameter ID " + pId.toString());
        }
        else
        {
            indexToUse = (int)parameterIndex;
        }
        
        if (auto c = getCableFromVar(cable))
        {
            for (int i = 0; i < c->getTargetList().size(); i++)
            {
                if (auto ppt = dynamic_cast<ProcessorParameterTarget*>(c->getTargetList()[i].get()))
                {
                    if (p == ppt->processor &&
                        (indexToUse == -1 || ppt->parameterIndex == indexToUse))
                    {
                        c->removeTarget(ppt);
                        i--;
                        continue;
                    }
                }
            }
            
            auto range = scriptnode::RangeHelpers::getDoubleRange(targetRange);
            
            auto smoothing = (double)targetRange.getProperty("SmoothingTime", 0.0);

            if (indexToUse != -1)
                c->addTarget(new ProcessorParameterTarget(p, indexToUse, range, smoothing));
        }
    }
    else
    {
        reportScriptError("Can't find module with ID " + processorId);
    }
}
```

**Parameter consumption patterns:**
- `parameterIndex` -- tested with `.isString()` for name-based lookup, otherwise cast to `(int)` for index-based.
- `targetRange` -- passed to `RangeHelpers::getDoubleRange(const var&)` which reads `DynamicObject` properties via `obj.hasProperty(r)` / `obj[r]` for scriptnode range IDs (MinValue, MaxValue, StepSize, SkewFactor, etc.). Also reads `.getProperty("SmoothingTime", 0.0)` directly.
- Empty `processorId` with `parameterIndex == -1` is the clear-all-connections sentinel.

---

## 7. Inner Classes / Nested Structs

### 7.1 DummyTarget

Created in the constructor; registers as a `CableTargetBase` on the cable to make the scripting reference visible in the debug UI. Does NOT process values -- `sendValue` is empty.

```cpp
// ScriptingApiObjects.cpp line 8910
struct ScriptingObjects::GlobalCableReference::DummyTarget : public scriptnode::routing::GlobalRoutingManager::CableTargetBase
{
    DummyTarget(GlobalCableReference& p) :
        parent(p)
    {
        if (auto c = getCableFromVar(parent.cable))
            c->addTarget(this);
    }

    String getTargetId() const override 
    { 
        String s;
        s << dynamic_cast<Processor*>(parent.getScriptProcessor())->getId();
        s << ".";
        s << parent.getDebugName();
        s << " (Script Reference)";
        return s;
    }

    void sendValue(double v) final override {};

    ~DummyTarget()
    {
        if (auto c = getCableFromVar(parent.cable))
        {
            c->removeTarget(this);
        }
    }

    GlobalCableReference& parent;
};
```

### 7.2 Callback (value callback)

```cpp
// ScriptingApiObjects.cpp line 9145
struct ScriptingObjects::GlobalCableReference::Callback: public scriptnode::routing::GlobalRoutingManager::CableTargetBase,
                                                         public PooledUIUpdater::SimpleTimer
{
    Callback(GlobalCableReference& p, const var& f, bool synchronous) :
        SimpleTimer(p.getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
        parent(p),
        sync(synchronous),
        callback(p.getScriptProcessor(), &p, f, 1)
    {
        id << dynamic_cast<Processor*>(p.getScriptProcessor())->getId();
        id << ".";

        auto ilf = dynamic_cast<WeakCallbackHolder::CallableObject*>(f.getObject());

        if (ilf != nullptr && (!synchronous || ilf->isRealtimeSafe()))
        {
            if (auto dobj = dynamic_cast<DebugableObjectBase*>(ilf))
            {
                id << dobj->getDebugName();
                funcLocation = dobj->getLocation();
            }

            callback.incRefCount();
            callback.setHighPriority();

            if (auto c = getCableFromVar(parent.cable))
            {
                c->addTarget(this);
            }

            if (!synchronous)
                start();
            else
                stop();
        }
        else
        {
            stop();
        }
    }

    void timerCallback() override
    {
        if (sync)
            return;

        double nv;

        if (value.getChangedValue(nv))
            callback.call1(nv);
    }

    void sendValue(double v) override
    {
        v = parent.inputRange.convertFrom0to1(v, false);

        if (sync)
        {
            var a(v);
            callback.callSync(&a, 1);
        }
        else
            value.setModValueIfChanged(v);
    }

    ~Callback()
    {
        if (auto c = getCableFromVar(parent.cable))
        {
            c->removeTarget(this);
        }
    }

    GlobalCableReference& parent;
    WeakCallbackHolder callback;
    const bool sync = false;
    ModValue value;
    String id;
    DebugableObject::Location funcLocation;
};
```

**Key details:**
- Synchronous callbacks: `sendValue` calls `callback.callSync()` directly -- this runs on whatever thread `Cable::sendValue` was called from (potentially audio thread). Requires the function to be realtime-safe (`ilf->isRealtimeSafe()`).
- Asynchronous callbacks: `sendValue` sets `ModValue` atomically; the `SimpleTimer` (driven by `PooledUIUpdater`) polls for changes and calls `callback.call1()` on the message thread.
- The value passed to the callback is converted from 0..1 through the `inputRange`.

### 7.3 DataCallback

```cpp
// ScriptingApiObjects.cpp line 9047
struct ScriptingObjects::GlobalCableReference::DataCallback: public scriptnode::routing::GlobalRoutingManager::CableTargetBase
{
    DataCallback(GlobalCableReference& p, const var& f):
      parent(p),
      callback(p.getScriptProcessor(), &p, f, 1)
    {
        id << dynamic_cast<Processor*>(p.getScriptProcessor())->getId() << ".dataCallback";

        callback.incRefCount();
        callback.setHighPriority();

        auto ilf = dynamic_cast<WeakCallbackHolder::CallableObject*>(f.getObject());

        if (ilf != nullptr)
        {
            if (auto dobj = dynamic_cast<DebugableObjectBase*>(ilf))
            {
                id << dobj->getDebugName();
                funcLocation = dobj->getLocation();
            }
        }

        if (auto c = getCableFromVar(parent.cable))
        {
            c->addTarget(this);
        }
    };

    ~DataCallback()
    {
        if (auto c = getCableFromVar(parent.cable))
        {
            c->removeTarget(this);
        }
    }

    void sendValue(double d) override {};

    void sendData(const void* data, size_t numBytes) override
    {
        if(!parent.dataRecursion)
        {
            MemoryInputStream mis(data, numBytes, false);
            auto x = var::readFromStream(mis);
            callback.call1(x);
        }
    }

    String id;
    DebugableObjectBase::Location funcLocation;
    GlobalCableReference& parent;
    WeakCallbackHolder callback;
};
```

**Key details:**
- `sendValue` is a no-op -- DataCallback only handles binary data.
- `sendData` deserializes the binary stream back to a `var` via `var::readFromStream()`.
- Protected by `parent.dataRecursion` flag to prevent re-entrant callbacks when this same reference sent the data.

### 7.4 MacroCableTarget (file-scope helper struct)

```cpp
// ScriptingApiObjects.cpp line 9295
struct MacroCableTarget : public scriptnode::routing::GlobalRoutingManager::CableTargetBase,
                          public ControlledObject
{
    MacroCableTarget(MainController* mc, int index, bool filterReps) :
        ControlledObject(mc),
        macroIndex(index),
        filterRepetitions(filterReps)
    {
        macroData = mc->getMainSynthChain()->getMacroControlData(macroIndex);
    };

    String getTargetId() const override
    {
        return "Macro " + String(macroIndex + 1);
    }

    void sendValue(double v) override
    {
        if (macroData == nullptr)
            macroData = getMainController()->getMainSynthChain()->getMacroControlData(macroIndex);

        auto newValue = 127.0f * jlimit(0.0f, 1.0f, (float)v);
        
        if ((!filterRepetitions || lastValue != newValue) && macroData != nullptr)
        {
            lastValue = newValue;
            macroData->setValue(newValue);
        }
    }

    const bool filterRepetitions;
    float lastValue = -1.0f;
    const int macroIndex;
    WeakReference<MacroControlBroadcaster::MacroControlData> macroData;
};
```

Macro values are scaled 0..1 -> 0..127 before being sent to `macroData->setValue()`.

### 7.5 ProcessorParameterTarget (file-scope helper struct)

```cpp
// ScriptingApiObjects.cpp line 9391
struct ProcessorParameterTarget : public scriptnode::routing::GlobalRoutingManager::CableTargetBase,
                                  public ControlledObject
{
    ProcessorParameterTarget(Processor* p, int index, const scriptnode::InvertableParameterRange& range, double smoothingTimeMs) :
        ControlledObject(p->getMainController()),
        targetRange(range),
        parameterIndex(index),
        processor(p)
    {
        lastValue.prepare(p->getSampleRate() / (double)p->getLargestBlockSize(), smoothingTimeMs);

        id << processor->getId();
        id << "::";
        id << processor->parameterNames[index].toString();
    };

    String getTargetId() const override
    {
        return id;
    }

    void sendValue(double v) override
    {
        lastValue.set(v);
        auto newValue = jlimit(0.0f, 1.0f, (float)lastValue.advance());
        auto cv = targetRange.convertFrom0to1(newValue, true);
        processor->setAttribute(parameterIndex, cv, sendNotification);
    }

    const int parameterIndex;
    const scriptnode::InvertableParameterRange targetRange;
    WeakReference<Processor> processor;
    String id;
    sdouble lastValue;
};
```

Uses `sdouble` (a smoothed double from `snex_Types.h`, inherits `pimpl::_ramp<double>`) for value smoothing. The `SmoothingTime` property from the target range JSON controls this.

---

## 8. getCableFromVar Helper

```cpp
// ScriptingApiObjects.cpp line 8666
scriptnode::routing::GlobalRoutingManager::Cable* getCableFromVar(const var& v)
{
    if (auto c = v.getObject())
    {
        return static_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(c);
    }

    return nullptr;
}
```

File-scope free function. The `cable` member is a `var` holding a `ReferenceCountedObject*` to a `GlobalRoutingManager::Cable`.

---

## 9. Destructor

```cpp
// ScriptingApiObjects.cpp line 8982
ScriptingObjects::GlobalCableReference::~GlobalCableReference()
{
    callbacks.clear();
}
```

The `DummyTarget`, `Callback`, and `DataCallback` instances are owned (`ScopedPointer` / `OwnedArray`), so they self-deregister from the cable in their own destructors. `dataCallbacks` is also an `OwnedArray` cleared by default destruction.

---

## 10. Underlying Cable Infrastructure

### 10.1 GlobalRoutingManager::SlotBase

```cpp
// GlobalRoutingManager.h line 142
struct SlotBase: public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<SlotBase>;
    using List = ReferenceCountedArray<SlotBase>;

    enum class SlotType
    {
        Cable,
        Signal,
        numTypes
    };

    SlotBase(const String& id_, SlotType t) :
        id(id_),
        type(t)
    {};

    virtual ~SlotBase() {};

    virtual bool cleanup() = 0;
    virtual bool isConnected() const = 0;
    virtual SelectableTargetBase::List getTargetList() const = 0;
    
    const String id;
    const SlotType type;

    SimpleReadWriteLock lock;
};
```

### 10.2 GlobalRoutingManager::CableTargetBase

```cpp
// GlobalRoutingManager.h line 92
struct CableTargetBase: public SelectableTargetBase
{
    using List = Array<WeakReference<CableTargetBase>>;

    virtual ~CableTargetBase() {};

    virtual void sendValue(double v) = 0;
    virtual void sendData(const void* data, size_t numBytes) {};
    virtual Path getTargetIcon() const = 0;

    JUCE_DECLARE_WEAK_REFERENCEABLE(CableTargetBase);
};
```

### 10.3 Cable::sendValue and Cable::sendData (Implementation)

```cpp
// GlobalRoutingManager.cpp line 1738
void GlobalRoutingManager::Cable::sendValue(CableTargetBase* source, double v)
{
    lastValue = jlimit(0.0, 1.0, v);

    for (auto t : targets)
    {
        if (t == source)
            continue;

        t->sendValue(lastValue);
    }
}
```

```cpp
// GlobalRoutingManager.cpp line 1725
void GlobalRoutingManager::Cable::sendData(CableTargetBase* source, void* data, size_t numBytes)
{
    lastData.replaceAll(data, numBytes);

    for (auto t : targets)
    {
        if (t == source)
            continue;

        t->sendData(data, numBytes);
    }
}
```

### 10.4 Cable::addTarget and Cable::removeTarget

```cpp
// GlobalRoutingManager.cpp line 1709
void GlobalRoutingManager::Cable::addTarget(CableTargetBase* n)
{
    SimpleReadWriteLock::ScopedWriteLock sl(lock);
    targets.addIfNotAlreadyThere(n);
    n->sendValue(lastValue);

    if(lastData.getSize() > 0)
        n->sendData(lastData.getData(), lastData.getSize());
}

void GlobalRoutingManager::Cable::removeTarget(CableTargetBase* n)
{
    SimpleReadWriteLock::ScopedWriteLock sl(lock);
    targets.removeAllInstancesOf(n);
}
```

On `addTarget`, the new target immediately receives the current `lastValue` and any stored `lastData`.

---

## 11. InvertableParameterRange

The `inputRange` member on `GlobalCableReference` starts as `(0.0, 1.0)`, meaning identity by default. All cable values are stored as normalised 0..1.

`setRange`, `setRangeWithSkew`, and `setRangeWithStep` configure this local input range, which maps user-facing values to/from the internal 0..1 normalised space.

---

## 12. RangeHelpers::getDoubleRange (for connectToModuleParameter)

```cpp
// ParameterData.cpp line 355
scriptnode::InvertableParameterRange RangeHelpers::getDoubleRange(const var& obj, IdSet set)
{
    ValueTree v(PropertyIds::ID);

    for (auto r : getRangeIds(false, set))
    {
        if (obj.hasProperty(r))
            v.setProperty(r, obj[r], nullptr);
    }

    return getDoubleRange(v, set);
}
```

The JSON object for `targetRange` uses scriptnode range property IDs: `MinValue`, `MaxValue`, `StepSize`, `SkewFactor`, plus a custom `SmoothingTime` property read separately.

---

## 13. Threading / Lifecycle Constraints

### Value Dispatch
- `Cable::sendValue()` iterates through `targets` without taking a lock. Targets are `WeakReference<CableTargetBase>`, so they can safely become null.
- `Cable::addTarget()` / `Cable::removeTarget()` take `SimpleReadWriteLock::ScopedWriteLock` on `SlotBase::lock`.
- `Cable::sendValue()` does NOT take a read lock, meaning it can run concurrently with structural changes. This is a deliberate lock-free design.

### Synchronous Callbacks
- When `synchronous=true`, the `Callback::sendValue()` calls `callback.callSync()` directly on the calling thread, which may be the audio thread. The constructor checks `ilf->isRealtimeSafe()` before registering.
- When `synchronous=false`, `ModValue` is used for thread-safe value transfer. The `PooledUIUpdater::SimpleTimer` polls at the UI update rate.

### Data Callbacks
- `DataCallback::sendData()` calls `callback.call1()` which is a high-priority async call via `WeakCallbackHolder`.
- Uses `dataRecursion` flag to prevent re-entrant callbacks.

### Not Audio-Thread Safe in General
- `sendData()` performs heap allocation (`MemoryOutputStream`). Must not be called from the audio thread.
- `setValue()` / `setValueNormalised()` call through to `Cable::sendValue()` which simply writes a double and iterates targets. If targets include synchronous script callbacks that do allocations, this propagates unsafety.
- `registerCallback()`, `registerDataCallback()`, `deregisterCallback()` all perform heap allocation and are not audio-thread safe.
- `connectToMacroControl()`, `connectToGlobalModulator()`, `connectToModuleParameter()` perform processor lookups and allocations.

### Lifecycle
- No explicit `onInit`-only restriction. The object can be created and used at any time.
- The `DummyTarget` is created in the constructor and destroyed in the destructor, maintaining cable target list consistency.

---

## 14. Preprocessor Guards

### In DummyTarget, Callback, DataCallback selectCallback methods:
```cpp
#if USE_BACKEND
    // editor navigation code (goto-workspace, etc.)
#endif
```

### The GlobalCableReference class itself has NO preprocessor guards
It compiles in all configurations (backend, frontend, project DLL).

---

## 15. GlobalRoutingManager Creation / Singleton Pattern

One `GlobalRoutingManager` per `MainController`. It is lazily created on first access via `GlobalRoutingManager::Helpers::getOrCreate(mc)`. Stored as a `ReferenceCountedObject` via `MainController::setGlobalRoutingManager()`.

---

## 16. Key Patterns and Observations

1. **All cable values are normalised (0..1)** internally. The `inputRange` on `GlobalCableReference` is a local transformation layer per-reference.

2. **Source-skipping pattern**: `Cable::sendValue(CableTargetBase* source, double v)` skips the sender to avoid feedback loops.

3. **Weak reference target list**: Targets are stored as `Array<WeakReference<CableTargetBase>>`, allowing targets to be destroyed without notification.

4. **Data channel**: Cables support both `double` value dispatch and arbitrary binary data dispatch (serialized `var`). These are independent channels -- `DataCallback` ignores values, `Callback` ignores data.

5. **Recursion guard**: `dataRecursion` flag in `GlobalCableReference` prevents a reference's own `DataCallback` from firing when it sends data via `sendData()`.

6. **Multiple callbacks per cable**: You can register multiple callbacks (both value and data) per `GlobalCableReference`. They're stored in `OwnedArray<Callback>` and `OwnedArray<DataCallback>`.

7. **No constants**: The class registers zero constants (`ConstScriptingObject(ps, 0)`).

8. **The `synchronous` parameter** in `registerCallback` accepts various forms through `ApiHelpers::isSynchronous()` / `ApiHelpers::getDispatchType()`. Truthy values are synchronous, falsy values are asynchronous.

9. **connectToModuleParameter clearing**: Passing empty `processorId` and `parameterIndex == -1` clears all `ProcessorParameterTarget` connections. Passing a valid processor with `parameterIndex == -1` removes all connections for that processor.

10. **connectToMacroControl clearing**: Passing `macroIndex == -1` removes all `MacroCableTarget` connections from the cable.

11. **ScriptComponent GlobalCable connection**: Script components can set `processorId` to `"GlobalCable"` to connect their value control to the global cable system (one-way: component -> cable).

12. **GlobalModulatorContainer cable support**: The `connectToGlobalModulator` method connects a modulator inside a `GlobalModulatorContainer` to the cable. Different modulator types (TimeVariant, VoiceStart, Envelope) are sorted into separate arrays.
