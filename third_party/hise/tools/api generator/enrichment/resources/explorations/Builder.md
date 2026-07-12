# Builder -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` (Builder entry)
- `enrichment/phase1/Synth/Readme.md` (prerequisite class -- module tree context)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:915-960` (class declaration)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp:10190-10582` (full implementation)
- `HISE/hi_scripting/scripting/api/ScriptingApi.h:1175-1176` (factory method)
- `HISE/hi_scripting/scripting/api/ScriptingApi.cpp:6348-6351` (factory implementation)
- `HISE/hi_core/hi_modules/raw/raw_builder.h` (underlying raw::Builder class)
- `HISE/hi_core/hi_modules/raw/raw_builder.cpp` (raw::Builder::create implementation)
- `HISE/hi_core/hi_modules/raw/raw_ids.h:75-98` (chain index constants)
- `HISE/hi_core/hi_dsp/modules/ModulatorSynth.h:96-103` (InternalChains enum)
- `HISE/hi_core/hi_core/MainController.h:1800-1836` (ScopedBadBabysitter)
- `HISE/hi_tools/hi_dispatch/hi_dispatch.h:261` (SUSPEND_GLOBAL_DISPATCH macro)

## Prerequisite Context (Synth)

Builder constructs what Synth.get*() retrieves. Both operate on the same module tree rooted at `MainController::getMainSynthChain()`. The Synth Readme documents:
- Module tree navigation with owner-rooted vs global-rooted search
- `ModulatorSynth::InternalChains` enum: MidiProcessor=0, GainModulation=1, PitchModulation=2, EffectChain=3
- Module handle types: ScriptingMidiProcessor, ScriptingModulator, ScriptingSynth, ScriptingEffect, Sampler, etc.

Builder is the write-side counterpart: it creates, configures, and destroys modules in that same tree. `Synth.get*()` methods then look up what Builder built.

---

## Class Declaration

```cpp
// ScriptingApiObjects.h:915-960
struct ScriptBuilder : public ConstScriptingObject
{
    ScriptBuilder(ProcessorWithScriptingContent* p);
    ~ScriptBuilder();

    // API methods (8 total)
    int create(var type, var id, int rootBuildIndex, int chainIndex);
    bool connectToScript(int buildIndex, String relativePath);
    int clearChildren(int buildIndex, int chainIndex);
    var get(int buildIndex, String interfaceType);
    int getExisting(String processorId);
    void setAttributes(int buildIndex, var attributeValues);
    void clear();
    void flush();

    Identifier getObjectName() const override { return "Builder"; }

private:
    bool flushed = true;
    struct Wrapper;
    Array<WeakReference<Processor>> createdModules;
    void createJSONConstants();
};
```

**Inheritance:** `ConstScriptingObject` (read-only scripting object base -- no dynamic properties).

**Key internal state:**
- `createdModules`: An `Array<WeakReference<Processor>>` that tracks all processors created or registered via this Builder instance. The array index IS the "build index" used by all methods. Index 0 is always the master container (MainSynthChain).
- `flushed`: Tracks whether `flush()` was called after modifications. The destructor warns if unflushed.

---

## Factory Method (obtainedVia)

```cpp
// ScriptingApi.h:1175-1176
/** Creates a Builder object that can be used to create the module tree. */
var createBuilder();

// ScriptingApi.cpp:6348-6351
juce::var ScriptingApi::Synth::createBuilder()
{
    return var(new ScriptingObjects::ScriptBuilder(getScriptProcessor()));
}
```

Created via `Synth.createBuilder()`. This is the only way to obtain a Builder instance.

---

## Constructor Analysis

```cpp
ScriptBuilder(ProcessorWithScriptingContent* p) :
    ConstScriptingObject(p, 6)  // 6 = number of constant slots
{
    createdModules.add(getScriptProcessor()->getMainController_()->getMainSynthChain());
    createJSONConstants();

    ADD_API_METHOD_0(clear);
    ADD_API_METHOD_4(create);
    ADD_API_METHOD_2(get);
    ADD_API_METHOD_1(getExisting);
    ADD_API_METHOD_2(setAttributes);
    ADD_API_METHOD_0(flush);
    ADD_API_METHOD_2(clearChildren);
    ADD_API_METHOD_2(connectToScript);
}
```

**Key observations:**
1. The master synth chain is pre-added at index 0 in `createdModules`. This means `rootBuildIndex=0` always refers to the master container.
2. 6 constant slots are allocated (matching the 6 `addConstant()` calls in `createJSONConstants()`).
3. All methods use untyped `ADD_API_METHOD_N` -- NO `ADD_TYPED_API_METHOD_N` calls anywhere. All parameter types must be inferred.

---

## Destructor

```cpp
ScriptBuilder::~ScriptBuilder()
{
    if (!flushed && !createdModules.isEmpty())
    {
        debugError(dynamic_cast<Processor*>(getScriptProcessor()), "forgot to flush() a Builder!");
    }
}
```

The destructor issues a console error if the Builder was used but never flushed. This is a safety net -- forgetting `flush()` means the patch browser and UI won't update.

---

## Wrapper Registration (Method Binding)

```cpp
struct ScriptBuilder::Wrapper
{
    API_METHOD_WRAPPER_4(ScriptBuilder, create);
    API_METHOD_WRAPPER_2(ScriptBuilder, get);
    API_METHOD_WRAPPER_1(ScriptBuilder, getExisting);
    API_VOID_METHOD_WRAPPER_2(ScriptBuilder, clearChildren);
    API_VOID_METHOD_WRAPPER_2(ScriptBuilder, setAttributes);
    API_VOID_METHOD_WRAPPER_0(ScriptBuilder, clear);
    API_VOID_METHOD_WRAPPER_0(ScriptBuilder, flush);
    API_VOID_METHOD_WRAPPER_2(ScriptBuilder, connectToScript);
};
```

All 8 methods are untyped wrappers. No forced types.

---

## Dynamic Constants (createJSONConstants)

The `createJSONConstants()` method creates 6 JSON object constants from the module factory system. These are runtime-generated -- the available types depend on what modules are registered. Each constant is a `DynamicObject` where keys are cleaned type names and values are the original type strings.

### Constant 1: MidiProcessors

```cpp
MidiProcessorFactoryType f(root);
addConstant("MidiProcessors", createObjectForFactory(&f));
```
Creates a JSON object listing all registered MIDI processor types. The factory removes its constrainer to list ALL types, not just those valid for a specific context.

### Constant 2: Modulators

```cpp
ModulatorChainFactoryType f(NUM_POLYPHONIC_VOICES, Modulation::GainMode, root);
addConstant("Modulators", createObjectForFactory(&f));
```
Lists all registered modulator types. Uses `GainMode` for the factory, but the type list is the same regardless of mode.

### Constant 3: SoundGenerators

```cpp
ModulatorSynthChainFactoryType m(NUM_POLYPHONIC_VOICES, root);
addConstant("SoundGenerators", createObjectForFactory(&m));
```
Lists all registered sound generator (synth) types.

### Constant 4: Effects

```cpp
EffectProcessorChainFactoryType e(NUM_POLYPHONIC_VOICES, root);
addConstant("Effects", createObjectForFactory(&e));
```
Lists all registered effect processor types.

### Constant 5: InterfaceTypes

```cpp
var s(new DynamicObject());
addScriptProcessorInterfaceID<ScriptingMidiProcessor>(s);
addScriptProcessorInterfaceID<ScriptingModulator>(s);
addScriptProcessorInterfaceID<ScriptingSynth>(s);
addScriptProcessorInterfaceID<ScriptingEffect>(s);
addScriptProcessorInterfaceID<ScriptingAudioSampleProcessor>(s);
addScriptProcessorInterfaceID<ScriptSliderPackProcessor>(s);
addScriptProcessorInterfaceID<ScriptingTableProcessor>(s);
addScriptProcessorInterfaceID<ScriptingApi::Sampler>(s);
addScriptProcessorInterfaceID<ScriptedMidiPlayer>(s);
addScriptProcessorInterfaceID<ScriptRoutingMatrix>(s);
addScriptProcessorInterfaceID<ScriptingSlotFX>(s);
addConstant("InterfaceTypes", s);
```

The `addScriptProcessorInterfaceID` helper works as:
```cpp
template <typename T> void addScriptProcessorInterfaceID(var& ids)
{
    auto i = T::getClassName();
    ids.getDynamicObject()->setProperty(i, i.toString());
}
```

This creates a lookup object mapping interface type class names to themselves. The keys are used by the `get()` method to determine what wrapper type to create. The available interface types are:

| Script Wrapper Class | C++ Module Base |
|---|---|
| ScriptingMidiProcessor | MidiProcessor |
| ScriptingModulator | Modulator |
| ScriptingSynth | ModulatorSynth |
| ScriptingEffect | EffectProcessor |
| ScriptingAudioSampleProcessor | Processor |
| ScriptSliderPackProcessor | ExternalDataHolder |
| ScriptingTableProcessor | ExternalDataHolder |
| Sampler | ModulatorSampler |
| ScriptedMidiPlayer | MidiPlayer |
| ScriptRoutingMatrix | Processor |
| ScriptingSlotFX | EffectProcessor |

### Constant 6: ChainIndexes

```cpp
var chainIds(new DynamicObject());
chainIds.getDynamicObject()->setProperty("Direct", raw::IDs::Chains::Direct);  // -1
chainIds.getDynamicObject()->setProperty("Midi", raw::IDs::Chains::Midi);      // 0
chainIds.getDynamicObject()->setProperty("Gain", raw::IDs::Chains::Gain);      // 1
chainIds.getDynamicObject()->setProperty("Pitch", raw::IDs::Chains::Pitch);    // 2
chainIds.getDynamicObject()->setProperty("FX", raw::IDs::Chains::FX);          // 3
chainIds.getDynamicObject()->setProperty("GlobalMod", raw::IDs::Chains::GlobalModulatorSlot); // 1
addConstant("ChainIndexes", chainIds);
```

The chain index values come from `raw::IDs::Chains` which maps to:

| Name | Value | Source |
|---|---|---|
| Direct | -1 | Constant -- means "add as direct child" (sound generators to containers) |
| Midi | 0 | `ModulatorSynth::MidiProcessor` |
| Gain | 1 | `ModulatorSynth::GainModulation` |
| Pitch | 2 | `ModulatorSynth::PitchModulation` |
| FX | 3 | `ModulatorSynth::EffectChain` |
| GlobalMod | 1 | `GlobalModulatorContainer::GainModulation` (same numeric value as Gain) |

---

## Underlying raw::Builder Class

The `ScriptBuilder` delegates module creation/removal to `raw::Builder`, a low-level C++ helper in `hi_core/hi_modules/raw/`.

```cpp
class Builder
{
public:
    Builder(MainController* mc_);

    template <class T> T* create(Processor* parent, int chainIndex = IDs::Chains::Direct);
    Processor* create(Processor* parent, const Identifier& processorType, int chainIndex);
    template <class T> bool remove(Processor* p);
    // ... other methods
private:
    MainController* mc;
};
```

The `ScriptBuilder` uses the non-template `create(parent, processorType, chainIndex)` overload:

```cpp
Processor* Builder::create(Processor* parent, const Identifier& processorType, int chainIndex)
{
    Chain* c = nullptr;
    if (chainIndex == -1)
        c = dynamic_cast<Chain*>(parent);   // Direct: parent IS the chain
    else
        c = dynamic_cast<Chain*>(parent->getChildProcessor(chainIndex));  // Get specific chain

    auto f = c->getFactoryType();
    int index = f->getProcessorTypeIndex(processorType);

    if (index != -1)
    {
        auto p = f->createProcessor(index, processorType.toString());
        return addInternal<Processor>(p, c);
    }
    return nullptr;
}
```

The factory lookup mechanism:
1. Get the `Chain` from parent (either parent itself for Direct, or child processor at chainIndex)
2. Get the chain's `FactoryType`
3. Look up the processor type index by `Identifier`
4. Create the processor via the factory
5. Add it to the chain via `addInternal`

---

## Build Index System

The "build index" is the core addressing mechanism. It is simply an array index into `createdModules`:

- Index 0 is always the MainSynthChain (pre-populated in constructor)
- Each `create()` call appends the new processor and returns its index
- `getExisting()` finds or adds an existing processor and returns its index
- All other methods (`get`, `setAttributes`, `clearChildren`, `connectToScript`) take a build index to identify the target module

This is a session-local addressing system -- build indexes are meaningful only within a single Builder instance's lifetime. They are NOT persistent IDs.

---

## onInit-only Restriction

The `create()` method checks `interfaceCreationAllowed()`:

```cpp
if (!getScriptProcessor()->getScriptingContent()->interfaceCreationAllowed())
{
    reportScriptError("You can't use this method after the onInit callback!");
    RETURN_IF_NO_THROW(-1);
}
```

This restricts module creation to the `onInit` callback only. The `clear()`, `get()`, `getExisting()`, `setAttributes()`, `clearChildren()`, and `flush()` methods do NOT have this check, but they are only meaningful in conjunction with `create()`.

---

## Threading and Lifecycle Infrastructure

### ScopedBadBabysitter

Used in both `create()` and `clear()`:

```cpp
MainController::ScopedBadBabysitter sb(mc);
```

This RAII guard sets `mc->flakyThreadingAllowed = true` for the duration of the scope. It tells HISE's threading checks to ignore thread-safety violations during module tree manipulation. The comment in MainController.h explains: "the main controller will ignore all threading issues and just does what it wants until the bad babysitter leaves the scope."

In exported plugins (`!USE_BACKEND`), `ScopedBadBabysitter` is a no-op dummy class.

### SUSPEND_GLOBAL_DISPATCH

Used in `clear()`:

```cpp
SUSPEND_GLOBAL_DISPATCH(mc, "clear from builder");
```

Expands to:
```cpp
dispatch::RootObject::ScopedGlobalSuspender sps(mc->getRootDispatcher(), dispatch::Paused, dispatch::CharPtr("clear from builder"));
```

Pauses the global dispatch system while the module tree is being demolished. This prevents notification storms as processors are removed.

### Sample Loading Thread Skip

`clear()` has a special guard:
```cpp
if (getScriptProcessor()->getMainController_()->getKillStateHandler().getCurrentThread() ==
    MainController::KillStateHandler::TargetThread::SampleLoadingThread)
{
    debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "skipping Builder.clear() on project load");
    return;
}
```

When loading a project, the script is compiled on the sample loading thread. The `clear()` method skips execution in this case to avoid destroying modules during project initialization.

### Backend-only UI Coordination (clear)

```cpp
#if USE_BACKEND
if (!CompileExporter::shouldSkipAudioDriverInitialisation())
{
    sb = new MainController::ScopedBadBabysitter(mc);
    mc->getProcessorChangeHandler().sendProcessorChangeMessage(
        mc->getMainSynthChain(),
        MainController::ProcessorChangeHandler::EventType::ClearBeforeRebuild,
        false
    );
    auto sp = getScriptProcessor();
    MessageManager::callAsync([sp]() {
        sp->getScriptingContent()->setIsRebuilding(true);
    });
    Thread::getCurrentThread()->wait(500);
    dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout(500);
}
#endif
```

In the backend IDE:
1. Sends a `ClearBeforeRebuild` message to update the patch browser
2. Sets `isRebuilding(true)` on the message thread to lock the UI
3. Waits 500ms for UI to settle
4. Extends the script timeout by 500ms to compensate for the wait

This entire block is skipped in exported plugins. It is also skipped when `CompileExporter::shouldSkipAudioDriverInitialisation()` is true (during export/batch processing).

---

## clear() -- Full Module Tree Demolition

The `clear()` method removes everything from the MainSynthChain except the calling script processor itself:

1. Iterates the MainSynthChain's child processors
2. For internal chains (indices < `numInternalChains` = 4): removes all children in each chain, EXCEPT the calling script processor (`thisAsP`)
3. For direct children (sound generators): removes them from the container
4. Each removal sends a `sendDeleteMessage()` on the message thread (with `MessageManagerLock`)
5. Uses `raw::Builder::remove<Processor>()` for the actual removal
6. After demolition, removes unconnected global routing cables via `GlobalRoutingManager::removeUnconnectedSlots`

The self-preservation check is critical:
```cpp
if (cToRemove == thisAsP)
    continue;
```

---

## create() -- Module Construction

```cpp
int create(var type, var id, int rootBuildIndex, int chainIndex)
```

1. Checks `interfaceCreationAllowed()` (onInit only)
2. Looks up the parent module from `createdModules[rootBuildIndex]`
3. First checks if a processor with the given `id` already exists under that parent -- if so, just adds it to `createdModules` and returns its index (idempotent behavior)
4. Creates `MainController::ScopedBadBabysitter` for threading safety
5. Delegates to `raw::Builder::create(parent, type, chainIndex)`
6. Sets the processor's ID
7. Appends to `createdModules`, sets `flushed = false`
8. Returns the new build index

The idempotent check (`ProcessorHelpers::getFirstProcessorWithName`) means calling `create()` twice with the same ID under the same parent won't create duplicates -- it will just return the existing module's index.

---

## clearChildren() -- Chain-Level Cleanup

```cpp
int clearChildren(int buildIndex, int chainIndex)
```

1. Finds the module at `createdModules[buildIndex]`
2. If `chainIndex == -1`: treats the module itself as the chain (Direct)
3. Otherwise: gets the child processor at `chainIndex` and casts to `Chain*`
4. Iterates the chain's handler, removing all processors with `sendDeleteMessage()` + `h->remove()`
5. Returns the number of processors that were removed

Note: Unlike `clear()`, this does NOT have the self-preservation check. If the calling script processor is in the targeted chain, it would be removed.

---

## get() -- Typed Reference Retrieval

```cpp
var get(int buildIndex, String interfaceType)
```

Returns a typed scripting wrapper for the module at the given build index. Uses a macro-based dispatch:

```cpp
#define RETURN_IF_MATCH(Type, PType) \
    if(id == Type::getClassName() && dynamic_cast<PType*>(p.get()) != nullptr) \
        return var(new Type(getScriptProcessor(), dynamic_cast<PType*>(p.get())));
```

The `interfaceType` string must match one of the class names from the `InterfaceTypes` constant. The module must also be dynamically castable to the corresponding C++ type. If neither condition is met, returns `var()` (undefined).

This is the Builder's equivalent of `Synth.getEffect()`, `Synth.getModulator()`, etc. -- but addressed by build index instead of name, and with explicit type selection.

---

## getExisting() -- Register Pre-existing Modules

```cpp
int getExisting(String processorId)
```

1. First checks if the processor is already in `createdModules` -- returns its index if found
2. Otherwise searches globally from MainSynthChain using `ProcessorHelpers::getFirstProcessorWithName`
3. Adds the found processor to `createdModules` and returns the new index
4. Reports error if not found

This allows mixing Builder-created modules with pre-existing ones. You can register a module that was created by a previous compilation or exists in the preset, then reference it by build index.

---

## setAttributes() -- Batch Attribute Setting

```cpp
void setAttributes(int buildIndex, var attributeValues)
```

1. Finds the module at `createdModules[buildIndex]`
2. Builds an `attributeIds` array from the module's parameter identifiers
3. Iterates the JSON object's properties
4. For each property, looks up the attribute index by `Identifier` match
5. Casts value to `float`, sanitizes it, calls `setAttribute(idx, v, dontSendNotification)`
6. After all attributes are set, sends a single batch notification via `sendOtherChangeMessage`

Key detail: Attributes are looked up by Identifier (name), not by numeric index. The JSON object maps attribute names to values. The `dontSendNotification` flag is used per-attribute, with one async notification at the end.

---

## connectToScript() -- External Script Linking

```cpp
bool connectToScript(int buildIndex, String relativePath)
```

Casts the module to `JavascriptProcessor*` and calls `setConnectedFile(relativePath, true)`. Only works if the target module is a script processor (JavascriptMidiProcessor, etc.). Returns false for non-script modules.

---

## flush() -- Rebuild Notification

```cpp
void flush()
```

Only acts if `!flushed`:
1. Sets `flushed = true`
2. Schedules async message thread work:
   - Sets `isRebuilding(false)` on the scripting content
   - Sends `sendRebuildMessage(true)` to the synth chain
   - Sends `RebuildModuleList` event to the processor change handler

This must be called after all `create()` / `clear()` operations to update the patch browser and other UI elements.

---

## Preprocessor Guards

| Guard | Location | Effect |
|---|---|---|
| `USE_BACKEND` | `clear()` | Backend IDE gets UI coordination (ClearBeforeRebuild, isRebuilding, 500ms wait). Exported plugins skip this entirely. |
| `USE_BACKEND` | `MainController::ScopedBadBabysitter` | In backend: real threading bypass. In frontend: no-op dummy. |

The `CompileExporter::shouldSkipAudioDriverInitialisation()` check within the `USE_BACKEND` block further conditions the UI coordination -- skipped during export/batch.

---

## Builder Workflow Pattern

The intended usage pattern (from the Doxygen description and the `flushed` flag behavior):

1. **Comment-out by default** -- Builder code is typically commented out
2. **Activate** -- Uncomment the Builder code
3. **Run once** -- Execute the onInit to build the module tree
4. **Deactivate** -- Re-comment the Builder code

This matches domain correction #5 from AGENTS.md: "Builder API workflow: commented out by default; activate-modify-run once-deactivate."

The `flushed` destructor warning enforces the pattern: you must call `flush()` to finalize your changes.
