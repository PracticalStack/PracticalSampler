# SlotFX -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- prerequisite chain (Effect -> SlotFX)
- `enrichment/resources/survey/class_survey_data.json` -- SlotFX entry
- `enrichment/phase1/Effect/Readme.md` -- prerequisite class analysis
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` lines 2091-2158 -- ScriptingSlotFX declaration
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 3720-3957 -- ScriptingSlotFX implementation
- `HISE/hi_core/hi_modules/effects/fx/SlotFX.h` -- SlotFX C++ module class + HotswappableProcessor interface
- `HISE/hi_core/hi_modules/effects/fx/SlotFX.cpp` -- SlotFX C++ module implementation
- `HISE/hi_core/hi_modules/effects/fx/GainEffect.h` lines 39-86 -- EmptyFX (unity gain placeholder)
- `HISE/hi_core/hi_modules/hardcoded/HardcodedModuleBase.h` lines 341-454 -- HardcodedSwappableEffect
- `HISE/hi_core/hi_modules/hardcoded/HardcodedModules.h` lines 41-92 -- HardcodedMasterFX
- `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` lines 6227-6261 -- Synth::getSlotFX()
- `HISE/hi_scripting/scripting/scriptnode/api/DspNetwork.h` lines 196-255 -- DspNetwork::Holder
- `HISE/hi_scripting/scripting/ScriptProcessor.h` lines 360-367 -- JavascriptProcessor inherits DspNetwork::Holder

## Class Declaration

### ScriptingSlotFX (Scripting API Wrapper)

File: `hi_scripting/scripting/api/ScriptingApiObjects.h` line 2091

```cpp
class ScriptingSlotFX : public ConstScriptingObject
{
public:
    ScriptingSlotFX(ProcessorWithScriptingContent *p, Processor *fx);
    ~ScriptingSlotFX() {};

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("SlotFX"); }
    Identifier getObjectName() const override { return getClassName(); }
    bool objectDeleted() const override { return slotFX.get() == nullptr; }
    bool objectExists() const override { return slotFX != nullptr; }

    // Debug info
    String getDebugName() const override { return slotFX.get() != nullptr ? slotFX->getId() : "Invalid"; };
    String getDebugDataType() const override { return getObjectName().toString(); }
    String getDebugValue() const override { return String(); }

    // API Methods (9 declared, 8 registered -- see setBypassed note below)
    bool exists() { return checkValidObject(); };
    void setBypassed(bool shouldBeBypassed);  // DECLARED BUT NOT REGISTERED
    void clear();
    var setEffect(String effectName);
    var getCurrentEffect();
    bool swap(var otherSlot);
    var getModuleList();
    var getParameterProperties();
    String getCurrentEffectId();

private:
    struct Wrapper;
    HotswappableProcessor* getSlotFX();
    DspNetwork::Holder* getDspNetworkHolder();
    WeakReference<Processor> slotFX;
};
```

### Inheritance
- Direct base: `ConstScriptingObject`
- No additional interfaces

### Key Internal Members
- `WeakReference<Processor> slotFX` -- stores the underlying processor (either a SlotFX module or a ScriptFX/scriptnode module)
- `getSlotFX()` -- casts slotFX to `HotswappableProcessor*`
- `getDspNetworkHolder()` -- casts slotFX to `DspNetwork::Holder*`

## Dual-Backend Architecture

ScriptingSlotFX operates in two distinct modes depending on what the underlying `Processor` implements:

### Mode 1: HotswappableProcessor (Classic SlotFX / HardcodedMasterFX)

When `slotFX` implements `HotswappableProcessor`, the API delegates to that interface:
- `setEffect()` -> `HotswappableProcessor::setEffect(name, synchronously)`
- `clear()` -> `HotswappableProcessor::clearEffect()`
- `swap()` -> `HotswappableProcessor::swap(other)`
- `getCurrentEffect()` -> `HotswappableProcessor::getCurrentEffect()`
- `getModuleList()` -> `HotswappableProcessor::getModuleList()`
- `getCurrentEffectId()` -> `HotswappableProcessor::getCurrentEffectId()`
- `getParameterProperties()` -> `HotswappableProcessor::getParameterProperties()`

Returns from `getCurrentEffect()` and `setEffect()` are wrapped in `ScriptingEffect` objects.

### Mode 2: DspNetwork::Holder (ScriptNode Networks)

When `slotFX` implements `DspNetwork::Holder`, the API delegates differently:
- `setEffect()` -> clears all networks, then `holder->getOrCreate(effectName)` -- returns a `DspNetwork` var
- `clear()` -> `holder->clearAllNetworks()`
- `getCurrentEffect()` -> `holder->getActiveNetwork()` -- returns a `DspNetwork` var
- `getModuleList()` -> (USE_BACKEND only) `BackendDllManager::getNetworkFiles()` listing .xml network files
- `getCurrentEffectId()` -> `holder->getActiveNetwork()->getId()`
- `getParameterProperties()` -> iterates root node parameters, building range/default JSON

`swap()` is NOT supported in DspNetwork::Holder mode -- it only works with `getSlotFX()`.

### Mode Detection

Both `getSlotFX()` and `getDspNetworkHolder()` use `dynamic_cast` on the stored `Processor`:
```cpp
HotswappableProcessor* getSlotFX() { return dynamic_cast<HotswappableProcessor*>(slotFX.get()); }
DspNetwork::Holder* getDspNetworkHolder() { return dynamic_cast<DspNetwork::Holder*>(slotFX.get()); }
```

Each API method tries `getSlotFX()` first, then falls back to `getDspNetworkHolder()`.

## HotswappableProcessor Interface

File: `hi_core/hi_modules/effects/fx/SlotFX.h` lines 17-36

```cpp
class HotswappableProcessor
{
public:
    virtual ~HotswappableProcessor() {};
    virtual bool setEffect(const String& name, bool synchronously) = 0;
    virtual bool swap(HotswappableProcessor* other) = 0;
    virtual void clearEffect() { setEffect("", false); }
    virtual StringArray getModuleList() const = 0;
    virtual Processor* getCurrentEffect() = 0;
    virtual const Processor* getCurrentEffect() const = 0;
    virtual String getCurrentEffectId() const = 0;
    virtual var getParameterProperties() const = 0;
};
```

### Implementors

1. **SlotFX** (`hi_core/hi_modules/effects/fx/SlotFX.h` line 47) -- Classic dynamic effect slot
   - `MasterEffectProcessor` + `HotswappableProcessor`
   - Wraps a single `MasterEffectProcessor` child, swappable at runtime

2. **HardcodedSwappableEffect** (`hi_core/hi_modules/hardcoded/HardcodedModuleBase.h` line 341)
   - Used by `HardcodedMasterFX`, `HardcodedPolyphonicFX`
   - Wraps compiled scriptnode DLL factories
   - Loads DSP networks from compiled project DLLs

## SlotFX C++ Module (the wrapped processor)

File: `hi_core/hi_modules/effects/fx/SlotFX.h` line 47

```cpp
class SlotFX : public MasterEffectProcessor,
               public HotswappableProcessor
```

Processor name: `"SlotFX"` / display: `"Effect Slot"`

### Constructor
```cpp
SlotFX::SlotFX(MainController *mc, const String &uid) :
    MasterEffectProcessor(mc, uid)
{
    finaliseModChains();
    createList();     // populates effectList from factory
    clearEffect();    // loads EmptyFX placeholder
}
```

### Effect List Creation (Constrainer)

`createList()` builds the available effect list using `EffectProcessorChainFactoryType` with a `Constrainer`:

```cpp
void SlotFX::createList()
{
    ScopedPointer<FactoryType> f = new EffectProcessorChainFactoryType(128, this);
    f->setConstrainer(new Constrainer());
    auto l = f->getAllowedTypes();
    for (int i = 0; i < l.size(); i++)
        effectList.add(l[i].type.toString());
}
```

The Constrainer disallows these effect types inside a SlotFX:
- `PolyFilterEffect` -- polyphonic filter
- `PolyshapeFX` -- polyphonic waveshaper
- `HarmonicFilter` -- harmonic filter
- `HarmonicMonophonicFilter` -- harmonic monophonic filter
- `StereoEffect` -- stereo routing
- `RouteEffect` -- routing
- `SlotFX` -- no nested SlotFX

All remaining `MasterEffectProcessor` types are allowed.

### clearEffect()

Loads an `EmptyFX` placeholder:

```cpp
void SlotFX::clearEffect()
{
    // Release old wrapped effect async
    ScopedPointer<MasterEffectProcessor> newEmptyFX;
    if (wrappedEffect != nullptr)
    {
        LOCK_PROCESSING_CHAIN(this);
        newEmptyFX.swapWith(wrappedEffect);
    }
    if (newEmptyFX != nullptr)
        getMainController()->getGlobalAsyncModuleHandler().removeAsync(newEmptyFX.release(), ProcessorFunction());

    // Create new EmptyFX
    newEmptyFX = new EmptyFX(getMainController(), "Empty");
    if (getSampleRate() > 0)
        newEmptyFX->prepareToPlay(getSampleRate(), getLargestBlockSize());
    newEmptyFX->setParentProcessor(this);
    auto newId = getId() + "_" + newEmptyFX->getId();
    newEmptyFX->setId(newId);
    {
        LOCK_PROCESSING_CHAIN(this);
        newEmptyFX.swapWith(wrappedEffect);
    }
}
```

### setEffect() -- The Core Swap Logic

```cpp
bool SlotFX::setEffect(const String& typeName, bool /*synchronously*/)
{
    LockHelpers::freeToGo(getMainController());
    int index = effectList.indexOf(typeName);
    if (currentIndex == index) return true;  // same effect already loaded

    if (index != -1)
    {
        ScopedPointer<FactoryType> f = new EffectProcessorChainFactoryType(128, this);
        f->setConstrainer(new Constrainer());
        currentIndex = index;

        if (auto p = f->createProcessor(f->getProcessorTypeIndex(typeName), typeName))
        {
            // Prepare new processor
            if (getSampleRate() > 0)
                p->prepareToPlay(getSampleRate(), getLargestBlockSize());
            p->setParentProcessor(this);
            auto newId = getId() + "_" + p->getId();
            p->setId(newId);

            // Swap out old processor (async delete)
            ScopedPointer<MasterEffectProcessor> pendingDeleteProcessor;
            if (wrappedEffect != nullptr)
            {
                LOCK_PROCESSING_CHAIN(this);
                wrappedEffect->setIsOnAir(false);
                wrappedEffect.swapWith(pendingDeleteProcessor);
            }
            if (pendingDeleteProcessor != nullptr)
                getMainController()->getGlobalAsyncModuleHandler().removeAsync(
                    pendingDeleteProcessor.release(), ProcessorFunction());

            // Install new processor
            {
                LOCK_PROCESSING_CHAIN(this);
                wrappedEffect = dynamic_cast<MasterEffectProcessor*>(p);
                wrappedEffect->setIsOnAir(isOnAir());
                wrappedEffect->setKillBuffer(*(this->killBuffer));
                isClear = wrappedEffect == nullptr ||
                          dynamic_cast<EmptyFX*>(wrappedEffect.get()) != nullptr;
            }

            // Auto-compile Script FX
            if (auto sp = dynamic_cast<JavascriptProcessor*>(wrappedEffect.get()))
            {
                hasScriptFX = true;
                sp->compileScript();
            }
            return true;
        }
        else
        {
            clearEffect();
            return true;
        }
    }
    else
    {
        clearEffect();
        return false;
    }
}
```

Key observations:
- Uses `LOCK_PROCESSING_CHAIN` macro for audio-thread-safe swapping
- Old processors are deleted asynchronously via `GlobalAsyncModuleHandler`
- If the loaded effect is a JavascriptProcessor (Script FX), it auto-compiles
- The `isClear` flag tracks whether the current effect is EmptyFX (for fast-path skipping)
- Same-index check prevents redundant reloading

### swap()

```cpp
bool SlotFX::swap(HotswappableProcessor* otherSwap)
{
    if (auto otherSlot = dynamic_cast<SlotFX*>(otherSwap))
    {
        auto te = wrappedEffect.release();
        auto oe = otherSlot->wrappedEffect.release();
        int tempIndex = currentIndex;
        currentIndex = otherSlot->currentIndex;
        otherSlot->currentIndex = tempIndex;
        {
            ScopedLock sl(getMainController()->getLock());
            bool tempClear = isClear;
            isClear = otherSlot->isClear;
            otherSlot->isClear = tempClear;
            wrappedEffect = oe;
            otherSlot->wrappedEffect = te;
        }
        wrappedEffect.get()->sendRebuildMessage(true);
        otherSlot->wrappedEffect.get()->sendRebuildMessage(true);
        sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Any);
        otherSlot->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Any);
        return true;
    }
    return false;
}
```

Swap is atomic (under MainController lock), exchanges both the wrapped effects AND their indices/clear flags. Only works between two `SlotFX` instances (not HardcodedSwappableEffect).

### Rendering

```cpp
void SlotFX::renderWholeBuffer(AudioSampleBuffer &buffer)
{
    if (isClear) return;  // fast-path: skip if EmptyFX
    if (auto w = wrappedEffect.get())
    {
        if (!w->isSoftBypassed())
        {
            wrappedEffect->renderAllChains(0, buffer.getNumSamples());
            // Multi-channel routing support
            if (buffer.getNumChannels() > 2)
            {
                auto l = getLeftSourceChannel();
                auto r = getRightSourceChannel();
                if (l + r != 1) // non-standard channel pair
                {
                    float* ptr[2] = { buffer.getWritePointer(l), buffer.getWritePointer(r) };
                    AudioSampleBuffer mBuffer(ptr, 2, buffer.getNumSamples());
                    wrappedEffect->renderWholeBuffer(mBuffer);
                    return;
                }
            }
            wrappedEffect->renderWholeBuffer(buffer);
        }
    }
}
```

The `isClear` flag provides an early-exit optimization -- when EmptyFX is loaded, no processing occurs.

### Soft Bypass Delegation

```cpp
void SlotFX::setSoftBypass(bool shouldBeSoftBypassed, bool useRamp) override
{
    if (wrappedEffect != nullptr && !ProcessorHelpers::is<EmptyFX>(getCurrentEffect()))
        wrappedEffect->setSoftBypass(shouldBeSoftBypassed, useRamp);
}
```

Soft bypass is delegated to the wrapped effect, but only if it's not EmptyFX.

### getCurrentEffectId()

```cpp
String getCurrentEffectId() const override
{
    return isPositiveAndBelow(currentIndex, effectList.size())
        ? effectList[currentIndex]
        : "No Effect";
}
```

Returns the type name string (not the instance ID), or "No Effect" if index is out of range.

## EmptyFX (Unity Gain Placeholder)

File: `hi_core/hi_modules/effects/fx/GainEffect.h` line 39

```cpp
class EmptyFX : public MasterEffectProcessor
```

A no-op `MasterEffectProcessor` that passes audio through unchanged. Used as the default/cleared state of a SlotFX. Key characteristics:
- No parameters, no child processors, no modulation chains
- `hasTail()` returns false
- `applyEffect()` is empty
- `setSoftBypass()` is overridden to do nothing (no-op)

## ScriptingSlotFX Constructor and Registration

```cpp
ScriptingObjects::ScriptingSlotFX::ScriptingSlotFX(ProcessorWithScriptingContent *p, Processor* fx) :
    ConstScriptingObject(p, fx != nullptr ? fx->getNumParameters()+1 : 1),
    slotFX(fx)
{
    if (fx != nullptr)
    {
        setName(fx->getId());
        addScriptParameters(this, slotFX.get());  // adds ScriptParameters constant
        for (int i = 0; i < fx->getNumParameters(); i++)
            addConstant(fx->getIdentifierForParameterIndex(i).toString(), var(i));
    }
    else
        setName("Invalid Effect");

    ADD_API_METHOD_1(setEffect);
    ADD_API_METHOD_0(getCurrentEffect);
    ADD_API_METHOD_0(clear);
    ADD_API_METHOD_1(swap);
    ADD_API_METHOD_0(getModuleList);
    ADD_API_METHOD_0(getParameterProperties);
    ADD_API_METHOD_0(getCurrentEffectId);
};
```

### Registered Methods (Wrapper struct)

All use plain `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` (no typed variants):

```cpp
struct Wrapper
{
    API_METHOD_WRAPPER_1(ScriptingSlotFX, setEffect);
    API_VOID_METHOD_WRAPPER_0(ScriptingSlotFX, clear);
    API_METHOD_WRAPPER_1(ScriptingSlotFX, swap);
    API_METHOD_WRAPPER_0(ScriptingSlotFX, getCurrentEffect);
    API_METHOD_WRAPPER_0(ScriptingSlotFX, getModuleList);
    API_METHOD_WRAPPER_0(ScriptingSlotFX, getParameterProperties);
    API_METHOD_WRAPPER_0(ScriptingSlotFX, getCurrentEffectId);
};
```

**No typed API methods** -- all use plain ADD_API_METHOD_N.

### setBypassed -- Dead Method

`setBypassed(bool)` is declared in the header (line 2120) with a Doxygen comment, but:
- Has NO implementation in any .cpp file
- Is NOT registered in the Wrapper struct
- Is NOT added via ADD_API_METHOD

It appears in the base JSON because the Doxygen parser picked up the declaration, but it is NOT actually callable from HiseScript. This is a dead/unfinished method.

### Dynamic Constants

The constructor registers parameter names from the underlying processor:
```cpp
for (int i = 0; i < fx->getNumParameters(); i++)
    addConstant(fx->getIdentifierForParameterIndex(i).toString(), var(i));
```

For a SlotFX module, `getNumParameters()` is typically 0 (SlotFX has no parameters of its own -- `getAttribute` returns -1, `setInternalAttribute` is empty). For a HardcodedMasterFX, parameters come from the loaded DSP network.

Additionally, `addScriptParameters()` adds a `ScriptParameters` constant containing scripted UI component names (for Script FX children).

## Factory / obtainedVia

### Synth.getSlotFX(name)

File: `hi_scripting/scripting/api/ScriptingApi.cpp` line 6227

```cpp
ScriptingApi::Synth::ScriptSlotFX* ScriptingApi::Synth::getSlotFX(const String& name)
{
    WARN_IF_AUDIO_THREAD(true, ScriptGuard::ObjectCreation);

    if (getScriptProcessor()->objectsCanBeCreated())
    {
        // First: search HotswappableProcessor instances
        Processor::Iterator<HotswappableProcessor> it(owner);
        while (auto p = dynamic_cast<Processor*>(it.getNextProcessor()))
        {
            if (p->getId() == name)
                return new ScriptSlotFX(getScriptProcessor(), p);
        }

        // Second: search DspNetwork::Holder instances
        Processor::Iterator<DspNetwork::Holder> it2(owner);
        while (auto p = dynamic_cast<Processor*>(it2.getNextProcessor()))
        {
            if (p->getId() == name)
                return new ScriptSlotFX(getScriptProcessor(), p);
        }

        reportScriptError(name + " was not found. ");
    }
    else
    {
        reportIllegalCall("getSlotFX()", "onInit");
    }
}
```

**onInit-only restriction** -- `objectsCanBeCreated()` check with `reportIllegalCall`.

The search iterates the module tree under the parent synth, first looking for `HotswappableProcessor` instances (SlotFX, HardcodedMasterFX), then `DspNetwork::Holder` instances (Script FX modules).

### Builder.create()

From the survey data: SlotFX is also createdBy `Builder`. The Builder creates a SlotFX module in the module tree, and the result can be obtained via `Synth.getSlotFX()`.

### Type matching for ScriptingSlotFX

From `ScriptingApiObjects.cpp` line 10419:
```cpp
RETURN_IF_MATCH(ScriptingSlotFX, hise::EffectProcessor);
```

This means the ScriptingSlotFX handle matches `EffectProcessor` in the type system.

## ScriptingSlotFX Method Implementation Details

### setEffect(effectName)

The scripting wrapper adds threading safety around the C++ setEffect:

```cpp
var ScriptingSlotFX::setEffect(String effectName)
{
    if (effectName == "undefined")
    {
        reportScriptError("Invalid effectName");
        // ...
    }

    if(auto slot = getSlotFX())
    {
        auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());
        {
            SuspendHelpers::ScopedTicket ticket(slotFX->getMainController());
            slotFX->getMainController()->getJavascriptThreadPool().killVoicesAndExtendTimeOut(jp);
            LockHelpers::freeToGo(slotFX->getMainController());
            slot->setEffect(effectName, false);
        }
        return new ScriptingEffect(getScriptProcessor(),
            dynamic_cast<EffectProcessor*>(slot->getCurrentEffect()));
    }
    else if (auto holder = getDspNetworkHolder())
    {
        // For DspNetwork mode: if same network already active, return it
        if(auto an = holder->getActiveNetwork())
        {
            if(an->getId() == effectName)
                return var(an);
        }
        holder->clearAllNetworks();
        auto dn = holder->getOrCreate(effectName);
        return var(dn);
    }
}
```

Key points:
- Validates `"undefined"` string input
- Uses `ScopedTicket` + `killVoicesAndExtendTimeOut` for thread-safe swap
- In HotswappableProcessor mode: returns an `Effect` handle
- In DspNetwork mode: returns a `DspNetwork` object
- In DspNetwork mode: skips reload if same network already active

### getModuleList() -- USE_BACKEND guard

```cpp
var ScriptingSlotFX::getModuleList()
{
    Array<var> list;
    if (auto slot = getSlotFX())
    {
        auto sa = slot->getModuleList();
        for (const auto& s : sa)
            list.add(var(s));
    }
    else if (auto h = getDspNetworkHolder())
    {
#if USE_BACKEND
        auto files = BackendDllManager::getNetworkFiles(
            getScriptProcessor()->getMainController_());
        for(auto n: files)
            list.add(var(n.getFileNameWithoutExtension()));
#else
        jassertfalse;  // Not available in frontend
#endif
    }
    return var(list);
}
```

**Important**: In DspNetwork::Holder mode, `getModuleList()` only works in `USE_BACKEND` builds. In exported plugins (frontend), it hits a `jassertfalse` and returns empty.

### getParameterProperties() -- DspNetwork mode details

For DspNetwork mode, builds JSON array with per-parameter properties:
```cpp
for (int i = 0; i < rn->getNumParameters(); i++)
{
    auto pdata = rn->getParameterFromIndex(i)->data;
    auto rng = scriptnode::RangeHelpers::getDoubleRange(pdata);
    auto prop = new DynamicObject();
    var obj(prop);
    scriptnode::RangeHelpers::storeDoubleRange(obj, rng,
        RangeHelpers::IdSet::ScriptComponents);
    prop->setProperty("text", pdata[PropertyIds::ID]);
    prop->setProperty("defaultValue", pdata[PropertyIds::DefaultValue]);
    list.add(var(prop));
}
```

Returns array of objects with: `text` (parameter name), `defaultValue`, and range properties (min, max, skew, etc. in ScriptComponent format).

For HotswappableProcessor mode (plain SlotFX): `getParameterProperties()` returns `var()` (empty/undefined) -- the SlotFX C++ class returns empty. The HardcodedSwappableEffect has a real implementation.

### swap(otherSlot)

```cpp
bool ScriptingSlotFX::swap(var otherSlot)
{
    if (auto t = getSlotFX())
    {
        if (auto sl = dynamic_cast<ScriptingSlotFX*>(otherSlot.getObject()))
        {
            if (auto other = sl->getSlotFX())
                return t->swap(other);
            else
                reportScriptError("Target Slot is invalid");
        }
        else
            reportScriptError("Target Slot does not exist");
    }
    else
        reportScriptError("Source Slot is invalid");
}
```

Only works between two `HotswappableProcessor` instances. The parameter must be another `SlotFX` scripting object.

## Threading and Lifecycle

### onInit-only creation
`Synth.getSlotFX()` enforces `objectsCanBeCreated()` -- only callable in `onInit`.

### setEffect threading
The scripting wrapper uses:
1. `SuspendHelpers::ScopedTicket` -- suspends audio processing
2. `killVoicesAndExtendTimeOut` -- kills voices and extends script timeout
3. `LockHelpers::freeToGo` -- waits for all locks to be released

The C++ SlotFX::setEffect uses `LOCK_PROCESSING_CHAIN` (audio lock) for the actual swap, and async deletion via `GlobalAsyncModuleHandler`.

### swap threading
Uses `ScopedLock` on `MainController::getLock()` for the exchange.

## Preprocessor Guards

- `USE_BACKEND` -- affects `getModuleList()` in DspNetwork::Holder mode (network file listing only available in IDE)
- `USE_BACKEND` -- affects `SlotFX::createEditor()` (returns `SlotFXEditor` in backend, null in frontend)

## State Serialization

SlotFX module handles its own save/restore:

```cpp
ValueTree SlotFX::exportAsValueTree() const override
{
    ValueTree v = MasterEffectProcessor::exportAsValueTree();
    return v;
}

void SlotFX::restoreFromValueTree(const ValueTree& v) override
{
    LockHelpers::noMessageThreadBeyondInitialisation(getMainController());
    MasterEffectProcessor::restoreFromValueTree(v);
    auto d = v.getChildWithName("ChildProcessors").getChild(0);
    setEffect(d.getProperty("Type"), true);
    wrappedEffect->restoreFromValueTree(d);
}
```

On restore, it reads the child processor's Type property, loads that effect type, then restores the child's full state. The wrapped effect's state (parameters, etc.) is preserved.

## DynamicComponentContainer Integration

From `DynamicComponentContainer.h` line 147 comment:
> on child add/remove it will call HotswappableProcessor::setEffect() with the `text` property of...

This indicates SlotFX/HotswappableProcessor integrates with ScriptDynamicContainer for dynamic UI-driven effect switching.
