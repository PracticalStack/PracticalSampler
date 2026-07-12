# ChildSynth -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- prerequisite: Synth
- `enrichment/resources/survey/class_survey_data.json` -- ChildSynth entry
- `enrichment/phase1/Synth/Readme.md` -- prerequisite class analysis
- No base class exploration needed (ChildSynth is not a component class)

## Prerequisite Context: Synth
ChildSynth operates within the module tree system described by the Synth class. Key context:
- Module tree navigation uses `Processor::Iterator<T>` for subtree search
- Modulator chain indices use `ModulatorSynth::InternalChains` enum (0=MidiProcessor, 1=GainModulation, 2=PitchModulation, 3=EffectChain)
- `objectsCanBeCreated()` restricts certain operations to onInit
- Attribute system: index-based parameter access on the underlying processor
- ModuleHandler provides `addModule()` and `addAndConnectToGlobalModulator()`

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:2231`

```cpp
class ScriptingSynth : public ConstScriptingObject
{
public:
    ScriptingSynth(ProcessorWithScriptingContent *p, ModulatorSynth *synth_);
    ~ScriptingSynth() {};

    static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("ChildSynth"); }
    Identifier getObjectName() const override { return getClassName(); };
    bool objectDeleted() const override { return synth.get() == nullptr; };
    bool objectExists() const override { return synth != nullptr; };

    // Debug helpers
    String getDebugName() const override;  // returns synth->getId() or "Invalid"
    String getDebugDataType() const override;  // returns "ChildSynth"
    String getDebugValue() const override;  // returns "N voices"
    void doubleClickCallback(const MouseEvent &, Component*) override {};
    Component* createPopupComponent(const MouseEvent& e, Component *c) override;
    // ... API methods ...

private:
    ApiHelpers::ModuleHandler moduleHandler;
    WeakReference<Processor> synth;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingSynth);
};
```

### Key observations:
- Inherits from `ConstScriptingObject` -- standard pattern for script handle objects
- Wraps a `WeakReference<Processor>` (not `ModulatorSynth*` directly -- the weak ref stores as Processor)
- Has `ApiHelpers::ModuleHandler` for adding/removing modules and connecting global modulators
- Debug value shows active voice count via `dynamic_cast<ModulatorSynth*>(synth.get())->getNumActiveVoices()`
- `createPopupComponent` shows a processor editor popup in the IDE

## Constructor

**File:** `ScriptingApiObjects.cpp:4218`

```cpp
ScriptingObjects::ScriptingSynth::ScriptingSynth(ProcessorWithScriptingContent *p, ModulatorSynth *synth_) :
    ConstScriptingObject(p, synth_ != nullptr ? synth_->getNumParameters() + 1 : 1),
    synth(synth_),
    moduleHandler(synth_, dynamic_cast<JavascriptProcessor*>(p))
{
    if (synth != nullptr)
    {
        setName(synth->getId());
        addScriptParameters(this, synth.get());

        for (int i = 0; i < synth->getNumParameters(); i++)
        {
            addConstant(synth->getIdentifierForParameterIndex(i).toString(), var(i));
        }
    }
    else
    {
        setName("Invalid Effect");  // Note: says "Effect" not "Synth" -- copy-paste artifact
    }

    // API method registration -- all use ADD_API_METHOD_N (no typed variants)
    ADD_API_METHOD_0(getId);
    ADD_API_METHOD_2(setAttribute);
    ADD_API_METHOD_1(getAttribute);
    ADD_API_METHOD_1(getAttributeId);
    ADD_API_METHOD_1(getAttributeIndex);
    ADD_API_METHOD_1(setBypassed);
    ADD_API_METHOD_0(isBypassed);
    ADD_API_METHOD_1(getChildSynthByIndex);
    ADD_API_METHOD_1(getCurrentLevel);
    ADD_API_METHOD_0(exportState);
    ADD_API_METHOD_1(restoreState);
    ADD_API_METHOD_0(getNumAttributes);
    ADD_API_METHOD_3(addModulator);
    ADD_API_METHOD_1(getModulatorChain);
    ADD_API_METHOD_3(addGlobalModulator);
    ADD_API_METHOD_3(addStaticGlobalModulator);
    ADD_API_METHOD_0(asSampler);
    ADD_API_METHOD_0(getRoutingMatrix);
    ADD_API_METHOD_2(setModulationInitialValue);
    ADD_API_METHOD_3(setEffectChainOrder);
}
```

### Constants (dynamic, instance-specific)
The constructor registers **two types of dynamic constants**:

1. **Processor parameter indices** -- via the for-loop calling `addConstant()`. Each parameter of the underlying `ModulatorSynth` is registered by its identifier string, mapped to its integer index. For base `ModulatorSynth`, these are:
   - `Gain` -> 0
   - `Balance` -> 1
   - `VoiceLimit` -> 2
   - `KillFadeTime` -> 3
   
   Subclasses (e.g., ModulatorSampler, SineSynth) add more parameters.

2. **ScriptParameters** -- via `addScriptParameters()`, which creates a `DynamicObject` containing `{componentName: index}` mappings for all UI components if the target is a `ProcessorWithScriptingContent`. Stored as the constant `ScriptParameters`.

### No static constants
There are zero `addConstant()` calls with literal values. All constants are derived from the wrapped processor at construction time.

### No typed API methods
All 20 methods use `ADD_API_METHOD_N` (plain), not `ADD_TYPED_API_METHOD_N`. This means no forced parameter types in the Wrapper struct.

## Wrapper Struct

**File:** `ScriptingApiObjects.cpp:4194`

All wrappers use `API_METHOD_WRAPPER_N` or `API_VOID_METHOD_WRAPPER_N` -- standard untyped patterns.

## Factory Methods / obtainedVia

ChildSynth objects are created by:

1. **`Synth.getChildSynth(name)`** -- searches the owner's subtree using `Processor::Iterator<ModulatorSynth>`, restricted to onInit. Returns `new ScriptingSynth(getScriptProcessor(), m)`.

2. **`Synth.getChildSynthByIndex(index)`** -- casts the owner to `Chain*`, gets processor at index from `c->getHandler()->getProcessor(index)`. Restricted to onInit.

3. **`ChildSynth.getChildSynthByIndex(index)`** -- same pattern as Synth's, but scoped to the wrapped synth's children. Restricted to onInit.

4. **`Builder.create()`** -- can create synth modules that return as ChildSynth references.

## ModuleHandler Infrastructure

**File:** `ScriptingApiObjects.h:46`

```cpp
class ModuleHandler
{
public:
    ModuleHandler(Processor* parent_, JavascriptProcessor* sp);
    ~ModuleHandler();
    bool removeModule(Processor* p);
    Processor* addModule(Chain* c, const String& type, const String& id, int index = -1);
    Modulator* addAndConnectToGlobalModulator(Chain* c, Modulator* globalModulator,
                                               const String& modName, bool connectAsStaticMod = false);
private:
    WeakReference<Processor> parent;
    WeakReference<JavascriptProcessor> scriptProcessor;
    Component::SafePointer<Component> mainEditor;
};
```

Used by:
- `addModulator()` calls `moduleHandler.addModule(c, typeName, modName, -1)`
- `addGlobalModulator()` calls `moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName)`
- `addStaticGlobalModulator()` calls `moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName, true)` (note `true` for static)

## addScriptParameters Helper

**File:** `ScriptingApiObjects.cpp:175`

```cpp
void addScriptParameters(ConstScriptingObject* this_, Processor* p)
{
    DynamicObject::Ptr scriptedParameters = new DynamicObject();
    if (ProcessorWithScriptingContent* pwsc = dynamic_cast<ProcessorWithScriptingContent*>(p))
    {
        for (int i = 0; i < pwsc->getScriptingContent()->getNumComponents(); i++)
        {
            scriptedParameters->setProperty(
                pwsc->getScriptingContent()->getComponent(i)->getName(), var(i));
        }
    }
    this_->addConstant("ScriptParameters", var(scriptedParameters.get()));
}
```

This is only meaningful if the target synth is a `ProcessorWithScriptingContent` (i.e., has a script interface). For most child synths, this will be an empty object. For a Script Synthesiser used as a child, it would contain component name-to-index mappings.

## ModulatorSynth::InternalChains Enum

**File:** `HISE/hi_core/hi_dsp/modules/ModulatorSynth.h:96`

```cpp
enum InternalChains
{
    MidiProcessor = 0,
    GainModulation,    // = 1
    PitchModulation,   // = 2
    EffectChain,       // = 3
    numInternalChains
};
```

These indices are used by:
- `addModulator(chainIndex, ...)` -- expects 1 (Gain) or 2 (Pitch) for modulator chains
- `getModulatorChain(chainIndex)` -- same
- `addGlobalModulator(chainIndex, ...)` -- same
- `addStaticGlobalModulator(chainIndex, ...)` -- same
- `setModulationInitialValue(chainIndex, ...)` -- same
- `setEffectChainOrder(...)` -- hardcodes `ModulatorSynth::EffectChain` (3) internally

## ModulatorSynth::Parameters Enum

**File:** `ModulatorSynth.h:85`

```cpp
enum Parameters
{
    Gain = 0,          // volume as gain factor 0...1
    Balance,           // stereo balance -100 to 100
    VoiceLimit,        // amount of voices
    KillFadeTime,      // fade time when voices are killed
    numModulatorSynthParameters
};
```

These are the base attribute indices. Subclasses extend with additional parameters.

## Method-Level Infrastructure Notes

### onInit Restriction
`getChildSynthByIndex()` enforces `objectsCanBeCreated()` -- restricted to onInit. Reports error via `reportIllegalCall("getChildSynth()", "onInit")`.

### setModulationInitialValue
Casts `synth->getChildProcessor(chainIndex)` to `ModulatorChain*`. If the cast fails (invalid chain index), reports script error. Calls `mc->setInitialValue(initialValue)` on the modulator chain.

### setEffectChainOrder -- Bug/Limitation
```cpp
void ScriptingObjects::ScriptingSynth::setEffectChainOrder(bool doPoly, var slotRange, var chainOrder)
{
    if(checkValidObject())
    {
        auto r = Result::ok();
        auto p = ApiHelpers::getPointFromVar(slotRange, &r).toInt();
        if(!r.wasOk())
            reportScriptError(r.getErrorMessage());

        if(auto fx = dynamic_cast<EffectProcessorChain*>(
            synth->getChildProcessor(ModulatorSynth::EffectChain)))
        {
            fx->setFXOrder(false, { p.x, p.y }, chainOrder);  // NOTE: hardcodes false, ignores doPoly!
        }
    }
}
```

**Important:** The `doPoly` parameter is accepted but **hardcoded to `false`** when calling `setFXOrder`. This means only master effect order can be changed, regardless of what the caller passes. This appears to be a bug or intentional limitation.

The `slotRange` is parsed as a Point (x,y) -> converted to `Range<int>` for the dynamic range of effects that can be reordered. `chainOrder` is an array of indices within that range.

### EffectProcessorChain::setFXOrder Details

**File:** `EffectProcessorChain.h:185`

```cpp
void setFXOrder(bool changePolyOrder, Range<int> dynamicRange, var newOrder)
```

When `changePolyOrder=true`: reorders `VoiceEffectProcessor` (polyphonic effects).
When `changePolyOrder=false`: reorders `MasterEffectProcessor` (master/mono effects).

The method:
1. Takes effects before the dynamic range start, appends them unchanged
2. Reorders effects within the dynamic range per the `newOrder` array
3. Appends effects after the dynamic range end
4. Acquires AudioLock to swap the reordered array
5. Bypasses any effects not in the new order
6. In USE_BACKEND: sends RebuildModuleList change message

### asSampler
Does a `dynamic_cast<ModulatorSampler*>` on the wrapped synth. Returns `var()` (undefined) silently if not a sampler -- intentionally does not report an error ("don't complain here, handle it on scripting level").

### getRoutingMatrix
Creates a new `ScriptRoutingMatrix` wrapping `synth.get()`. Does NOT check `checkValidObject()` first -- will create a matrix wrapping nullptr if synth is invalid.

### addModulator / addGlobalModulator / addStaticGlobalModulator
These cast `synth->getChildProcessor(chainIndex)` to `ModulatorChain*`. If null, report script error. Then delegate to `moduleHandler.addModule()` or `moduleHandler.addAndConnectToGlobalModulator()`. Return a new `ScriptingModulator` wrapping the created modulator.

The difference between `addGlobalModulator` and `addStaticGlobalModulator`: the latter passes `true` as the `connectAsStaticMod` parameter to `addAndConnectToGlobalModulator`. Static global modulators are time-variant modulators that provide a single value per block rather than per-voice modulation.

### getModulatorChain
Casts `synth->getChildProcessor(chainIndex)` to `Modulator*` (not `ModulatorChain*` -- the chain itself IS a Modulator). Returns it wrapped in a `ScriptingModulator`. This gives script access to the chain as a modulator handle.

### getCurrentLevel
Returns `synth->getDisplayValues().outL` or `.outR` depending on `leftChannel` parameter. These are the peak display values, not real-time sample values.

## Comparison with Synth Class Methods

ChildSynth shares many method signatures with the Synth namespace but operates on an arbitrary child sound generator rather than the parent synth. Key differences:

| Capability | Synth | ChildSynth |
|-----------|-------|------------|
| MIDI generation (playNote, addNoteOn, etc.) | Yes | No |
| Timer system | Yes | No |
| Voice management (noteOff, killAll, etc.) | Yes | No |
| Module tree search (getModulator, getEffect, etc.) | Yes | No |
| setAttribute/getAttribute | On parent synth | On wrapped child synth |
| addModulator | On parent synth chains | On child synth chains |
| getChildSynth/ByIndex | On parent's children | On child's children (recursive) |
| asSampler | Via getSampler(name) | Via asSampler() cast |
| getRoutingMatrix | Yes | Yes |
| setEffectChainOrder | No | Yes |
| setModulationInitialValue | No | Yes |
| exportState/restoreState | No (use preset system) | Yes (base64 state) |
| getCurrentLevel | No (use peak methods) | Yes |

ChildSynth is a focused handle for parameter control, bypass, state, and modulator chain manipulation on a specific child sound generator. It does NOT provide MIDI generation or voice management -- those remain on the parent Synth.

## Threading / Lifecycle Constraints

- `getChildSynthByIndex()` -- onInit only (objectsCanBeCreated check)
- `setAttribute()` -- uses `ProcessorHelpers::getAttributeNotificationType()` for thread-safe notification
- `setBypassed()` -- sends `sendNotification` + `ProcessorChangeEvent::Bypassed` dispatch
- `setEffectChainOrder()` -- acquires AudioLock internally via `LockHelpers::SafeLock`
- `restoreState()` -- validates base64 string before restoring
- No explicit threading guards on most getter methods (getAttribute, getId, etc.)

## Preprocessor Guards
None specific to ChildSynth. The `setFXOrder` implementation uses `#if USE_BACKEND` for the rebuild notification, but this is in EffectProcessorChain, not in ChildSynth itself.
