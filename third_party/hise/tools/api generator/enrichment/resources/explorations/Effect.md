# Effect (ScriptingEffect) -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` (Effect entry)
- `enrichment/phase1/Synth/Readme.md` (prerequisite -- module tree system)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` (class declaration, lines 1971-2088)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` (implementation, lines 3349-3716)
- `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` (Synth.getEffect, Synth.addEffect, lines 6036-6063, 6641-6658)
- `HISE/hi_core/hi_dsp/modules/EffectProcessor.h` (EffectProcessor base class hierarchy)
- `HISE/hi_core/hi_dsp/ProcessorInterfaces.h` (ProcessorWithCustomFilterStatistics)
- `HISE/hi_core/hi_modules/effects/fx/FilterHelpers.h` (FilterBank::FilterMode enum)
- `HISE/hi_core/hi_dsp/Processor.h` (DisplayValues struct)

---

## Class Declaration

```cpp
// ScriptingApiObjects.h:1971
class ScriptingEffect : public ConstScriptingObject
{
public:
    // Inner class
    class FilterModeObject : public ConstScriptingObject { ... };

    ScriptingEffect(ProcessorWithScriptingContent *p, EffectProcessor *fx);
    ~ScriptingEffect() {};

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("Effect"); }
    Identifier getObjectName() const override { return getClassName(); }
    bool objectDeleted() const override { return effect.get() == nullptr; }
    bool objectExists() const override { return effect != nullptr; }

    // Debug info
    String getDebugName() const override;
    String getDebugDataType() const override;
    String getDebugValue() const override;
    void doubleClickCallback(const MouseEvent &, Component*) override {};
    Component* createPopupComponent(const MouseEvent& e, Component *c) override;

    // API Methods (21 total)
    bool exists();
    String getId() const;
    void setAttribute(int parameterIndex, float newValue);
    float getAttribute(int index);
    String getAttributeId(int index);
    int getAttributeIndex(String id);
    int getNumAttributes() const;
    void setBypassed(bool shouldBeBypassed);
    bool isBypassed() const;
    bool isSuspended() const;
    String exportState();
    void restoreState(String base64State);
    String exportScriptControls();
    void restoreScriptControls(String base64Controls);
    float getCurrentLevel(bool leftChannel);
    var addModulator(var chainIndex, var typeName, var modName);
    var getModulatorChain(var chainIndex);
    var addGlobalModulator(var chainIndex, var globalMod, String modName);
    var addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName);
    void setDraggableFilterData(var filterData);
    var getDraggableFilterData();

    struct Wrapper;
    EffectProcessor* getEffect();

private:
    ApiHelpers::ModuleHandler moduleHandler;
    WeakReference<Processor> effect;
};
```

### Inheritance

- `ConstScriptingObject` -- base class for all read-only scripting API objects
- Stored reference: `WeakReference<Processor> effect` -- weak ref to the underlying `EffectProcessor`
- `objectDeleted()` / `objectExists()` -- standard weak ref validity checks
- `exists()` simply calls `checkValidObject()` from ConstScriptingObject

### Key Members

- `moduleHandler` (`ApiHelpers::ModuleHandler`) -- handles adding/removing modules in the processor tree. Takes `(Processor* parent, JavascriptProcessor* sp)`. Provides `addModule()`, `removeModule()`, and `addAndConnectToGlobalModulator()`.
- `effect` (`WeakReference<Processor>`) -- the underlying effect processor. Accessed via `getEffect()` which does a `dynamic_cast<EffectProcessor*>`.

---

## Constructor Analysis

```cpp
// ScriptingApiObjects.cpp:3373
ScriptingEffect(ProcessorWithScriptingContent *p, EffectProcessor *fx) :
    ConstScriptingObject(p, fx != nullptr ? fx->getNumParameters()+1 : 1),
    effect(fx),
    moduleHandler(fx, dynamic_cast<JavascriptProcessor*>(p))
```

The second argument to `ConstScriptingObject` is `numConstants`. It is set to `fx->getNumParameters() + 1` when valid (the +1 accounts for the base slot).

### Dynamic Constants (Parameter Names -> Indices)

```cpp
if (fx != nullptr)
{
    setName(fx->getId());
    addScriptParameters(this, effect.get());

    for (int i = 0; i < fx->getNumParameters(); i++)
    {
        addConstant(fx->getIdentifierForParameterIndex(i).toString(), var(i));
    }
}
else
{
    setName("Invalid Effect");
}
```

**Key insight:** Constants are generated dynamically at construction time based on the wrapped effect processor's parameter list. Each parameter name is registered as a constant mapping to its integer index. This means every Effect instance has a different set of constants depending on which effect module it wraps (e.g., a SimpleGain effect would have `Gain` -> 0, while a PolyphonicFilter would have `Gain`, `Frequency`, `Q`, `Mode`, `Quality` -> 0,1,2,3,4).

There are NO hardcoded `addConstant()` calls. All constants are effect-type-specific.

### Method Registration

```cpp
ADD_API_METHOD_0(getId);
ADD_TYPED_API_METHOD_2(setAttribute, VarTypeChecker::Number, VarTypeChecker::Number);
ADD_TYPED_API_METHOD_1(setBypassed, VarTypeChecker::Number);
ADD_TYPED_API_METHOD_1(getAttribute, VarTypeChecker::Number);
ADD_TYPED_API_METHOD_1(getAttributeId, VarTypeChecker::Number);
ADD_TYPED_API_METHOD_1(getAttributeIndex, VarTypeChecker::String);
ADD_API_METHOD_0(isBypassed);
ADD_API_METHOD_0(isSuspended);
ADD_API_METHOD_1(getCurrentLevel);
ADD_API_METHOD_0(exportState);
ADD_API_METHOD_1(restoreState);
ADD_API_METHOD_1(restoreScriptControls);
ADD_API_METHOD_0(exportScriptControls);
ADD_API_METHOD_0(getNumAttributes);
ADD_API_METHOD_3(addModulator);
ADD_API_METHOD_1(getModulatorChain);
ADD_API_METHOD_3(addGlobalModulator);
ADD_API_METHOD_3(addStaticGlobalModulator);
ADD_API_METHOD_1(setDraggableFilterData);
ADD_API_METHOD_0(getDraggableFilterData);
```

### Typed Methods Summary

| Method | Param 1 | Param 2 |
|--------|---------|---------|
| `setAttribute` | Number | Number |
| `setBypassed` | Number | -- |
| `getAttribute` | Number | -- |
| `getAttributeId` | Number | -- |
| `getAttributeIndex` | String | -- |

All other methods use plain `ADD_API_METHOD_N` (untyped).

### Wrapper Struct

```cpp
struct ScriptingObjects::ScriptingEffect::Wrapper
{
    API_VOID_METHOD_WRAPPER_2(ScriptingEffect, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingEffect, getAttribute);
    API_METHOD_WRAPPER_1(ScriptingEffect, getAttributeId);
    API_METHOD_WRAPPER_1(ScriptingEffect, getAttributeIndex);
    API_METHOD_WRAPPER_0(ScriptingEffect, getNumAttributes);
    API_VOID_METHOD_WRAPPER_1(ScriptingEffect, setBypassed);
    API_METHOD_WRAPPER_0(ScriptingEffect, isBypassed);
    API_METHOD_WRAPPER_0(ScriptingEffect, isSuspended);
    API_METHOD_WRAPPER_0(ScriptingEffect, exportState);
    API_METHOD_WRAPPER_1(ScriptingEffect, getCurrentLevel);
    API_VOID_METHOD_WRAPPER_1(ScriptingEffect, restoreState);
    API_VOID_METHOD_WRAPPER_1(ScriptingEffect, restoreScriptControls);
    API_METHOD_WRAPPER_0(ScriptingEffect, exportScriptControls);
    API_METHOD_WRAPPER_3(ScriptingEffect, addModulator);
    API_METHOD_WRAPPER_3(ScriptingEffect, addGlobalModulator);
    API_METHOD_WRAPPER_1(ScriptingEffect, getModulatorChain);
    API_METHOD_WRAPPER_3(ScriptingEffect, addStaticGlobalModulator);
    API_METHOD_WRAPPER_0(ScriptingEffect, getId);
    API_VOID_METHOD_WRAPPER_1(ScriptingEffect, setDraggableFilterData);
    API_METHOD_WRAPPER_0(ScriptingEffect, getDraggableFilterData);
};
```

No typed wrappers in the Wrapper struct -- all use the standard `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` macros. The typed enforcement happens at registration time via `ADD_TYPED_API_METHOD_N`.

---

## Factory / ObtainedVia

The Effect object is created in two ways:

### 1. Synth.getEffect(name) -- retrieval

```cpp
// ScriptingApi.cpp:6036
ScriptingObjects::ScriptingEffect *ScriptingApi::Synth::getEffect(const String &name)
{
    WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

    if(getScriptProcessor()->objectsCanBeCreated())
    {
        Processor::Iterator<EffectProcessor> it(owner);
        EffectProcessor *fx;
        while((fx = it.getNextProcessor()) != nullptr)
        {
            if(fx->getId() == name)
            {
                return new ScriptEffect(getScriptProcessor(), fx);
            }
        }
        reportScriptError(name + " was not found. ");
        RETURN_IF_NO_THROW(new ScriptEffect(getScriptProcessor(), nullptr))
    }
    else
    {
        reportIllegalCall("getEffect()", "onInit");
        RETURN_IF_NO_THROW(new ScriptEffect(getScriptProcessor(), nullptr))
    }
}
```

- Uses owner-rooted `Processor::Iterator<EffectProcessor>` (subtree search, as described in Synth prerequisite)
- Restricted to `onInit` via `objectsCanBeCreated()`
- Returns invalid Effect (null EffectProcessor) on failure

### 2. Synth.addEffect(type, id, index) -- dynamic creation

```cpp
// ScriptingApi.cpp:6641
ScriptEffect* ScriptingApi::Synth::addEffect(const String &type, const String &id, int index)
{
    EffectProcessorChain* c = owner->effectChain;
    Processor* p = moduleHandler.addModule(c, type, id, index);
    return new ScriptingObjects::ScriptingEffect(getScriptProcessor(), dynamic_cast<EffectProcessor*>(p));
}
```

- Adds to the owner synth's effectChain
- Uses ModuleHandler.addModule for safe creation
- Returns new ScriptingEffect wrapping the created processor

### 3. Builder.create() -- also creates Effect handles (via survey data)

The survey data shows `createdBy: ["Builder", "SlotFX", "Synth"]`.

---

## EffectProcessor Base Class Hierarchy

```
Processor (hi_core/hi_dsp/Processor.h)
  + ProfiledProcessor
  |
  EffectProcessor (hi_core/hi_dsp/modules/EffectProcessor.h)
  |
  +-- MasterEffectProcessor (+ RoutableProcessor)
  |     Stereo signal processing. Has SoftBypass system.
  |     Used by: Reverbs, Delays, Dynamics, Convolution, etc.
  |
  +-- MonophonicEffectProcessor
  |     Monophonic modulation with stepped processing.
  |     Used by: Filters with modulation chains.
  |
  +-- VoiceEffectProcessor
        Polyphonic per-voice processing.
        Used by: Polyphonic filters, per-voice effects.
```

### EffectProcessor key infrastructure:

- `SuspensionState` struct: `{ numSilentBuffers, currentlySuspended, playing }` -- tracks silence detection
- `isSuspendedOnSilence()` -- override to enable automatic suspension on silence
- `isCurrentlySuspended()` -- returns current suspension state
- `hasTail()` -- pure virtual; whether effect produces output after input stops
- `modChains` (`ModulatorChain::Collection`) -- internal modulator chains
- `renderAllChains()` -- renders all internal modulator chains
- `numSilentCallbacksToWait = 86` -- number of silent callbacks before suspension triggers

### MasterEffectProcessor additions:
- `SoftBypassState` enum: `{ Inactive, Pending, Bypassed, numSoftBypassStates }` -- fade-out bypass
- `setSoftBypass()` -- smooth bypass with optional ramp
- `isFadeOutPending()` -- checks if soft bypass fade is in progress
- `killBuffer` -- pointer for kill buffer during bypass fade
- `RoutableProcessor` mixin -- provides routing matrix support

---

## Inner Class: FilterModeObject

```cpp
// ScriptingApiObjects.h:1976
class FilterModeObject : public ConstScriptingObject
{
public:
    FilterModeObject(const ProcessorWithScriptingContent* p);
    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("FilterModes"); }
};
```

This is NOT an API class exposed as "Effect.FilterModes" -- it is a separate object created via `Engine.getFilterModeList()`:

```cpp
// ScriptingApi.cpp:1906
return var(new ScriptingObjects::ScriptingEffect::FilterModeObject(getScriptProcessor()));
```

### FilterModeObject Constants

```cpp
// ScriptingApiObjects.cpp:3688
FilterModeObject(const ProcessorWithScriptingContent* p) :
    ConstScriptingObject(const_cast<ProcessorWithScriptingContent*>(p),
                         (int)FilterBank::FilterMode::numFilterModes)
{
    #define ADD_FILTER_CONSTANT(x) addConstant(#x, (int)FilterBank::FilterMode::x)

    ADD_FILTER_CONSTANT(LowPass);        // 0
    ADD_FILTER_CONSTANT(HighPass);       // 1
    ADD_FILTER_CONSTANT(LowShelf);       // 2
    ADD_FILTER_CONSTANT(HighShelf);      // 3
    ADD_FILTER_CONSTANT(Peak);           // 4
    ADD_FILTER_CONSTANT(ResoLow);        // 5
    ADD_FILTER_CONSTANT(StateVariableLP);  // 6
    ADD_FILTER_CONSTANT(StateVariableHP);  // 7
    ADD_FILTER_CONSTANT(MoogLP);         // 8
    ADD_FILTER_CONSTANT(OnePoleLowPass);  // 9
    ADD_FILTER_CONSTANT(OnePoleHighPass); // 10
    ADD_FILTER_CONSTANT(StateVariablePeak);     // 11
    ADD_FILTER_CONSTANT(StateVariableNotch);    // 12
    ADD_FILTER_CONSTANT(StateVariableBandPass); // 13
    ADD_FILTER_CONSTANT(Allpass);        // 14
    ADD_FILTER_CONSTANT(LadderFourPoleLP); // 15
    ADD_FILTER_CONSTANT(LadderFourPoleHP); // 16
    ADD_FILTER_CONSTANT(RingMod);        // 17
}
```

These constants map to `FilterBank::FilterMode` enum (FilterHelpers.h:49-70). Note: `numFilterModes` = 18 (not exposed as constant, used as array size).

**Important:** The FilterModeObject is nested inside ScriptingEffect but is accessed via `Engine.getFilterModeList()`, not via the Effect object. It has object name "FilterModes", not "Effect". It is a constant container with no methods.

---

## ProcessorWithCustomFilterStatistics Interface

The `setDraggableFilterData` and `getDraggableFilterData` methods interact with the `ProcessorWithCustomFilterStatistics` interface:

```cpp
// ScriptingApiObjects.cpp:3672
void ScriptingEffect::setDraggableFilterData(var filterData)
{
    if(auto h = dynamic_cast<ProcessorWithCustomFilterStatistics*>(getEffect()))
        h->setFilterStatistics(filterData);
}

var ScriptingEffect::getDraggableFilterData()
{
    if(auto h = dynamic_cast<ProcessorWithCustomFilterStatistics*>(getEffect()))
        return h->getFilterStatisticsJSON();
    return var();
}
```

These methods silently return/do nothing if the effect does not implement `ProcessorWithCustomFilterStatistics`. This interface is implemented by:

- `JavascriptMasterEffect` (ScriptProcessorModules.h:573) -- Script FX
- `HardcodedMasterFX` (HardcodedModuleBase.h:343) -- hardcoded master effects
- `PolyFilterEffect` (Filters.h:47) -- the built-in Polyphonic Filter

### CustomFilterStats JSON Schema

The `CustomFilterStats::getDefaultProperties()` returns the default JSON structure:

```json
{
    "NumFilterBands": 1,
    "FilterDataSlot": 0,
    "FirstBandOffset": 0,
    "TypeList": ["Low Pass", "High Pass", "Low Shelf", "High Shelf", "Peak"],
    "ParameterOrder": ["Gain", "Freq", "Q", "Enabled", "Type"],
    "FFTDisplayBufferIndex": -1,
    "DragActions": {
        "DragX": "Freq",
        "DragY": "Gain",
        "ShiftDrag": "Q",
        "DoubleClick": "Enabled",
        "RightClick": ""
    }
}
```

The `DragActions` object maps mouse interactions to filter parameters:
- `DragX` -- horizontal drag maps to Freq
- `DragY` -- vertical drag maps to Gain
- `ShiftDrag` -- shift+drag maps to Q
- `DoubleClick` -- double-click toggles Enabled
- `RightClick` -- right-click (empty = no action)

The `ParameterOrder` defines which processor attributes correspond to each band parameter. `FirstBandOffset` is the attribute index where band parameters start. For multi-band filters, attribute index = `FirstBandOffset + filterIndex * ParameterOrder.length + parameterIndex`.

The `FilterDataSlot` references an `ExternalData::DataType::FilterCoefficients` slot for coefficient visualization.

---

## ModuleHandler

```cpp
// ScriptingApiObjects.h:46
class ModuleHandler
{
public:
    ModuleHandler(Processor* parent_, JavascriptProcessor* sp);
    ~ModuleHandler();

    bool removeModule(Processor* p);
    Processor* addModule(Chain* c, const String& type, const String& id, int index = -1);
    Modulator* addAndConnectToGlobalModulator(Chain* c, Modulator* globalModulator,
                                               const String& modName,
                                               bool connectAsStaticMod = false);
private:
    WeakReference<Processor> parent;
    WeakReference<JavascriptProcessor> scriptProcessor;
    Component::SafePointer<Component> mainEditor;
};
```

The ModuleHandler handles safe module tree manipulation (adding/removing processors). It is shared across Effect, Synth, and other handle classes. Key operations:

- `addModule(chain, type, id, index)` -- creates a new processor of the given type and adds it to the chain
- `addAndConnectToGlobalModulator(chain, globalMod, name, isStatic)` -- creates a GlobalModulatorContainer receiver and connects it to an existing global modulator. The `isStatic` flag determines whether to create a `GlobalStaticTimeVariantModulator` (true) or `GlobalTimeVariantModulator` (false).

---

## Threading and Lifecycle Constraints

### onInit restriction
`Synth.getEffect()` enforces `objectsCanBeCreated()` -- can only be called in `onInit`. This is the standard pattern for all module tree handle retrieval (see Synth prerequisite).

### restoreState threading
```cpp
void ScriptingEffect::restoreState(String base64State)
{
    if (checkValidObject())
    {
        auto vt = ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(base64State);
        if (!vt.isValid())
        {
            reportScriptError("Can't load module state");
            RETURN_VOID_IF_NO_THROW();
        }

        SuspendHelpers::ScopedTicket ticket(effect->getMainController());
        effect->getMainController()->getJavascriptThreadPool()
            .killVoicesAndExtendTimeOut(dynamic_cast<JavascriptProcessor*>(getScriptProcessor()));
        LockHelpers::freeToGo(effect->getMainController());
        ProcessorHelpers::restoreFromBase64String(effect, base64State);
    }
}
```

`restoreState` uses the full suspension protocol:
1. Acquires a `ScopedTicket` to request audio suspension
2. Kills voices and extends timeout via JavascriptThreadPool
3. Calls `LockHelpers::freeToGo` to wait for audio thread clearance
4. Then performs the actual restore

This is a heavy operation -- NOT safe for audio thread, requires full system suspension.

### setAttribute notification
```cpp
effect->setAttribute(parameterIndex, newValue, ProcessorHelpers::getAttributeNotificationType());
```

Uses `ProcessorHelpers::getAttributeNotificationType()` which returns the appropriate notification type based on the current thread context.

### setBypassed notification
```cpp
effect->setBypassed(shouldBeBypassed, sendNotification);
effect->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Bypassed,
                                dispatch::sendNotificationAsync);
```

Sends both the standard bypass notification and an async dispatch message for UI update.

---

## exportScriptControls / restoreScriptControls Constraint

```cpp
String ScriptingEffect::exportScriptControls()
{
    if (dynamic_cast<ProcessorWithScriptingContent*>(effect.get()) == nullptr)
    {
        reportScriptError("exportScriptControls can only be used on Script Processors");
    }
    // ...
}

void ScriptingEffect::restoreScriptControls(String base64Controls)
{
    if (dynamic_cast<ProcessorWithScriptingContent*>(effect.get()) == nullptr)
    {
        reportScriptError("restoreScriptControls can only be used on Script Processors");
    }
    // ...
}
```

These two methods check if the wrapped effect is a `ProcessorWithScriptingContent` (i.e., a Script FX module). They report a script error if used on built-in (non-scripted) effects. The `exportScriptControls` call passes `true` for the `exportScriptOnly` parameter of `getBase64String`, while `exportState` passes `false`.

---

## isSuspended Logic

```cpp
bool ScriptingEffect::isSuspended() const
{
    if(checkValidObject())
    {
        auto fx = const_cast<ScriptingEffect*>(this)->getEffect();
        return fx->isSuspendedOnSilence() && fx->isCurrentlySuspended();
    }
    return false;
}
```

Returns true only when BOTH conditions are met:
1. The effect has silence suspension enabled (`isSuspendedOnSilence()`)
2. The effect is currently in suspended state (`isCurrentlySuspended()`)

This means an effect that does not opt into silence suspension will always return false, even if no audio is flowing.

---

## getCurrentLevel

```cpp
float ScriptingEffect::getCurrentLevel(bool leftChannel)
{
    if (checkValidObject())
    {
        return leftChannel ? effect->getDisplayValues().outL : effect->getDisplayValues().outR;
    }
    return 0.0f;
}
```

Uses `Processor::DisplayValues` struct:
```cpp
struct DisplayValues {
    float inL;
    float outL;
    float inR;
    float outR;
};
```

Returns the OUTPUT level (outL/outR), not the input level. The `leftChannel` parameter selects between L and R.

---

## Relationship to Other Handle Classes

The Effect class shares a near-identical API pattern with other module handle classes:

- **Modulator** (ScriptingModulator) -- same attribute/bypass/state methods, but for modulators
- **ChildSynth** (ScriptingSynth) -- same pattern for child synth modules
- **MidiProcessor** (ScriptingMidiProcessor) -- same pattern for MIDI processors

All four handle classes share:
- `exists()`, `getId()`, `setAttribute()`, `getAttribute()`, `getAttributeId()`, `getAttributeIndex()`, `getNumAttributes()`
- `setBypassed()`, `isBypassed()`
- `exportState()`, `restoreState()`
- `addModulator()`, `addGlobalModulator()`, `addStaticGlobalModulator()`, `getModulatorChain()`

Effect-specific methods:
- `isSuspended()` -- unique to Effect (silence suspension is an EffectProcessor feature)
- `getCurrentLevel()` -- unique to Effect
- `exportScriptControls()` / `restoreScriptControls()` -- shared with some other handles
- `setDraggableFilterData()` / `getDraggableFilterData()` -- unique to Effect

---

## Preprocessor Guards

No preprocessor guards (`#if USE_BACKEND`, etc.) are used in the ScriptingEffect class implementation. The `createPopupComponent` method calls `DebugableObject::Helpers::showProcessorEditorPopup` which is likely backend-only, but the guard is inside that helper, not in Effect itself.

---

## Debug Interface

```cpp
String getDebugName() const override {
    return effect.get() != nullptr ? effect->getId() : "Invalid";
}
String getDebugDataType() const override { return getObjectName().toString(); } // "Effect"
String getDebugValue() const override { return String(); } // empty
Component* createPopupComponent(const MouseEvent& e, Component* t) override {
    return DebugableObject::Helpers::showProcessorEditorPopup(t, effect.get());
}
```

Double-clicking an Effect reference in the HISE script debugger opens the processor editor popup for the wrapped effect.
