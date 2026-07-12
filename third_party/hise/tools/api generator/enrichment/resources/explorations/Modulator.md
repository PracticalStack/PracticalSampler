# Modulator -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` (Modulator entry)
- `enrichment/phase1/Synth/Readme.md` (prerequisite: module tree system)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` (lines 1837-1967)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` (lines 2849-3346)
- `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` (Synth::getModulator, lines 5899-5922)
- `HISE/hi_core/hi_dsp/modules/Modulators.h` (Modulation + Modulator base classes)
- `HISE/hi_core/hi_modules/modulators/mods/GlobalModulators.h`
- `HISE/hi_core/hi_modules/modulators/mods/MatrixModulator.h`
- `HISE/hi_scripting/scripting/api/ScriptingBaseObjects.h` (AssignableObject, ConstScriptingObject)
- `HISE/hi_core/hi_dsp/modules/ModulatorChain.h`

---

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h:1837`

```cpp
class ScriptingModulator : public ConstScriptingObject,
                           public AssignableObject
```

### Inheritance

- **ConstScriptingObject** -- base for all scripting API objects. Provides `addConstant()`, `checkValidObject()`, `reportScriptError()`, `ADD_API_METHOD_N` macros, etc.
- **AssignableObject** -- enables bracket-operator access (`mod[paramIndex] = value`). Requires implementing:
  - `assign(int index, var newValue)` -- delegates to `setAttribute(index, (float)newValue)`
  - `getAssignedValue(int index)` -- returns `1.0` (TODO in source)
  - `getCachedIndex(const var& indexExpression)` -- looks up parameter name -> index via `mod->getIdentifierForParameterIndex()`

### Member Variables

```cpp
ApiHelpers::ModuleHandler moduleHandler;   // handles addModule/addAndConnectToGlobalModulator
WeakReference<Modulator> mod;              // the wrapped C++ Modulator
Modulation *m;                             // cast of mod to Modulation* (cached)
```

The `mod` field is a `WeakReference<Modulator>` -- if the modulator is deleted (e.g., removed from a chain), the reference becomes null. `objectDeleted()` and `objectExists()` check this.

The `m` field is a raw `Modulation*` pointer set at construction via `dynamic_cast<Modulation*>(m_)`. It is null when the wrapped modulator is null.

---

## Constructor Analysis

**File:** `ScriptingApiObjects.cpp:2881-2931`

```cpp
ScriptingModulator(ProcessorWithScriptingContent *p, Modulator *m_)
  : ConstScriptingObject(p, m_ != nullptr ? m_->getNumParameters() + 1 : 1)
  , mod(m_)
  , m(nullptr)
  , moduleHandler(m_, dynamic_cast<JavascriptProcessor*>(p))
```

### Dynamic Constants (from wrapped modulator)

When `mod != nullptr`:
1. Casts `m_` to `Modulation*` and stores as `m`
2. Sets name from `mod->getId()`
3. Calls `addScriptParameters(this, mod.get())` -- adds a `ScriptParameters` constant (DynamicObject) mapping UI control names to indices, but only if the modulator is a `ProcessorWithScriptingContent` (i.e., a script modulator)
4. Iterates `mod->getNumParameters()` and adds each parameter name as a constant mapping to its index:
   ```cpp
   addConstant(mod->getIdentifierForParameterIndex(i).toString(), var(i));
   ```
   This means parameter constants are dynamic -- they depend on the specific modulator type. An LFO will have different constants than an envelope.

### API Method Registration

All methods use `ADD_API_METHOD_N` (untyped) except these which use `ADD_TYPED_API_METHOD_N`:

| Method | Typed Parameters |
|--------|-----------------|
| `setAttribute` | `VarTypeChecker::Number, VarTypeChecker::Number` |
| `setBypassed` | `VarTypeChecker::Number` |
| `setIntensity` | `VarTypeChecker::Number` |
| `setIsBipolar` | `VarTypeChecker::Number` |
| `getAttribute` | `VarTypeChecker::Number` |
| `getAttributeId` | `VarTypeChecker::Number` |
| `getAttributeIndex` | `VarTypeChecker::String` |

---

## ObtainedVia / Factory Pattern

ScriptingModulator instances are created by:

1. **`Synth.getModulator(name)`** -- owner-rooted search using `Processor::Iterator<Modulator>`. Restricted to `onInit` (checks `objectsCanBeCreated()`). Returns null wrapper on failure.
2. **`Synth.addModulator(chain, type, id)`** -- creates a new modulator via `moduleHandler.addModule()` and wraps it.
3. **`Synth.getAllModulators(regex)`** -- global search from MainSynthChain, returns array of ScriptingModulator objects.
4. **Self-referential creation** -- `addModulator()`, `addGlobalModulator()`, `addStaticGlobalModulator()`, `getModulatorChain()` on a Modulator instance all return new ScriptingModulator wrappers.
5. **`Effect.addModulator()` / `ChildSynth.addModulator()`** -- same pattern on other module handle types.
6. **`Builder.create()`** -- the Builder API can create modulators and returns ScriptingModulator.

All factory paths wrap the C++ `Modulator*` in the same `new ScriptingModulator(getScriptProcessor(), m)` pattern.

---

## Modulation Mode System

The `Modulation` base class (hi_core/hi_dsp/modules/Modulators.h:51) defines the core mode enum:

```cpp
enum Mode {
    GainMode = 0,    // 0.0..1.0, multiplied with destination
    PitchMode,       // -1.0..1.0, added to destination (converted via PitchConverters)
    PanMode,         // -1.0..1.0
    GlobalMode,      // -1.0..1.0, intensity ignored
    OffsetMode,      // -1.0..1.0, each modulator added using its intensity
    CombinedMode,    // 0..1, each modulator can be GainMode or OffsetMode
    numModes
};
```

### Mode Impact on setIntensity

`setIntensity()` (ScriptingApiObjects.cpp:3121-3149) branches on mode:

- **GainMode:** clamps to `[0.0, 1.0]`, calls `m->setIntensity(value)` directly
- **PitchMode:** clamps to `[-12.0, 12.0]`, converts to factor: `value / 12.0`, then calls `m->setIntensity(pitchFactor)`. The internal representation is `-1.0..1.0` (fraction of an octave).
- **else (Pan/Global/Offset/Combined):** clamps to `[-1.0, 1.0]`, calls `m->setIntensity(value)` directly

### Mode Impact on getIntensity

`getIntensity()` (ScriptingApiObjects.cpp:3153-3168):

- **PitchMode:** returns `m->getIntensity() * 12.0` (converts back to semitones)
- **else:** returns `m->getIntensity()` directly

### Mode Impact on getCurrentLevel

`getCurrentLevel()` (ScriptingApiObjects.cpp:3170-3183):

- Gets `m->getProcessor()->getDisplayValues().outL`
- **PitchMode:** applies `Modulation::PitchConverters::pitchFactorToOutputValue()` which converts `[0.5..2.0]` to `[0..1]` clipped
- **else:** returns raw value

---

## Bipolar System

`setIsBipolar()` delegates directly to `dynamic_cast<Modulation*>(mod.get())->setIsBipolar()`.
`isBipolar()` returns `dynamic_cast<Modulation*>(mod.get())->isBipolar()`.

Bipolar mode affects how the modulation value is interpreted:
- In GainMode, unipolar = `[0..1]`, bipolar = `[-1..1]`
- The `calcIntensityValue()` method in Modulation handles the conversion

---

## AssignableObject Pattern (Bracket Access)

The `AssignableObject` interface enables:
```javascript
mod["Frequency"] = 440.0;  // equivalent to mod.setAttribute(mod.Frequency, 440.0)
```

Implementation:
- `getCachedIndex(var)`: converts string to Identifier, iterates parameters to find match, returns index or -1
- `assign(int, var)`: calls `setAttribute(index, (float)newValue)`
- `getAssignedValue(int)`: returns `1.0` (hardcoded -- incomplete implementation)

---

## ModuleHandler Infrastructure

`ApiHelpers::ModuleHandler` (ScriptingApiObjects.h:46-70) provides:

- `addModule(Chain* c, String type, String id, int index)` -- adds a processor to a chain
- `addAndConnectToGlobalModulator(Chain* c, Modulator* globalMod, String name, bool connectAsStaticMod = false)` -- adds and connects a global modulator receiver
- `removeModule(Processor* p)` -- removes a processor

This handler is initialized with the parent processor and the JavascriptProcessor. It coordinates with the `KillStateHandler` for safe audio-thread operations.

---

## Global Modulator Connection System

### connectToGlobalModulator(containerId, modulatorId)

**ScriptingApiObjects.cpp:2993-3006**

- Dynamic casts `mod` to `GlobalModulator*` -- only works if the modulator is a global receiver type
- Calls `gm->connectToGlobalModulator(containerId + ":" + modulatorId)` -- concatenates with colon separator
- Reports error "connectToGlobalModulator() only works with global modulators!" if cast fails

### getGlobalModulatorId()

**ScriptingApiObjects.cpp:3008-3019**

- Checks if `mod->getType().toString().startsWith("Global")`
- If so, casts `m->getProcessor()` to `GlobalModulator*`
- Returns `gm->getItemEntryFor(gm->getConnectedContainer(), gm->getOriginalModulator())`
- Format is `"ContainerName:ModulatorName"`

### GlobalModulator Base Class

**GlobalModulators.h:46-119**

```cpp
class GlobalModulator: public LookupTableProcessor,
                       public Chain::Handler::Listener
```

Parameters: `UseTable = 0`, `Inverted`

Types: `VoiceStart`, `TimeVariant`, `StaticTimeVariant`, `Envelope`

Key methods: `connectToGlobalModulator(String itemEntry)`, `getOriginalModulator()`, `getConnectedContainer()`, `disconnect()`

### addGlobalModulator vs addStaticGlobalModulator

Both use `moduleHandler.addAndConnectToGlobalModulator()` but:
- `addGlobalModulator` passes `connectAsStaticMod = false` (default) -- creates a time-variant receiver
- `addStaticGlobalModulator` passes `connectAsStaticMod = true` -- creates a static time-variant receiver that samples at voice start

Both expect the `globalMod` parameter to be a ScriptingModulator wrapping the source modulator inside a GlobalModulatorContainer.

---

## MatrixModulator System

**MatrixModulator.h:39-118**

```cpp
class MatrixModulator: public EnvelopeModulator,
                       public ExternalDataHolder,
                       public ModulatorSynthChain::Handler::Listener
```

`setMatrixProperties()` (ScriptingApiObjects.cpp:3021-3032) works specifically with MatrixModulator:
- Casts `mod` to `MatrixModulator*`
- Finds the `GlobalModulatorContainer` from the main synth chain
- Converts `matrixData` to `RangeData` via `MatrixIds::Helpers::Properties::RangeData::fromJSON()`
- Stores the range data keyed by `mm->getMatrixTargetId()`
- Sends a property update notification

This method is only functional when the modulator is a MatrixModulator instance.

---

## Child Chain Access

### addModulator(chainIndex, typeName, modName)

**ScriptingApiObjects.cpp:3239-3259**

- Gets child processor at `chainIndex` and casts to `ModulatorChain*`
- Uses `moduleHandler.addModule(c, typeName, modName, -1)` to create
- Wraps result in new ScriptingModulator
- Chain indices correspond to the modulator's `getChildProcessor()` ordering

### getModulatorChain(chainIndex)

**ScriptingApiObjects.cpp:3261-3278**

- Gets child processor at `chainIndex` and casts to `Modulator*`
- Wraps in new ScriptingModulator
- This allows accessing sub-chains as modulators themselves (ModulatorChain extends Modulator via EnvelopeModulator)

The chain indices depend on the specific modulator type. For example, a TimeVariantModulator typically has no child chains, while a modulator with an internal intensity chain would have one.

---

## State Export/Restore

### exportState / restoreState

- `exportState()`: calls `ProcessorHelpers::getBase64String(mod, false)` -- serializes full processor state
- `restoreState()`: validates base64 string via `ValueTreeHelpers::getValueTreeFromBase64String()`, then calls `ProcessorHelpers::restoreFromBase64String()`

### exportScriptControls / restoreScriptControls

- Only works on Script Processors (`ProcessorWithScriptingContent`)
- `exportScriptControls()`: calls `getBase64String(mod, false, true)` -- the third parameter indicates script-controls-only export
- `restoreScriptControls()`: calls `restoreFromBase64String(mod, base64Controls, true)` -- restores UI control values without recompiling the script

---

## asTableProcessor

**ScriptingApiObjects.cpp:3330-3345**

- Casts `mod` to `LookupTableProcessor*`
- If successful, creates and returns a `ScriptingTableProcessor` wrapper
- If cast fails (not a table modulator), returns `var()` (undefined) -- deliberately does not report error
- Allows conversion to `TableProcessor` scripting API for table manipulation

---

## setAttribute / getAttribute System

- `setAttribute(index, value)`: delegates to `mod->setAttribute(index, value, ProcessorHelpers::getAttributeNotificationType())`
- `getAttribute(index)`: delegates to `mod->getAttribute(parameterIndex)`
- `getAttributeId(index)`: returns `mod->getIdentifierForParameterIndex(parameterIndex).toString()`
- `getAttributeIndex(id)`: returns `mod->getParameterIndexForIdentifier(parameterId)`
- `getNumAttributes()`: returns `mod->getNumParameters()`

Parameter indices are specific to each modulator type. The constructor registers parameter names as constants, so scripts can use `mod.Frequency` instead of raw indices.

---

## setBypassed / isBypassed

- `setBypassed(bool)`: calls `mod->setBypassed(shouldBeBypassed, sendNotification)` and then `mod->sendOtherChangeMessage(ProcessorChangeEvent::Bypassed)`
- `isBypassed()`: returns `mod->isBypassed()`

The `sendOtherChangeMessage` call ensures the UI is notified of the bypass state change.

---

## Exists Pattern

`exists()` calls `checkValidObject()` which is inherited from `ConstScriptingObject`. It checks `objectExists()` (which checks `mod != nullptr`). If the modulator has been removed or was never found, this returns false and logs a console error.

---

## Shared Patterns with Effect, ChildSynth, MidiProcessor

ScriptingModulator shares a nearly identical API surface with `ScriptingEffect`, `ScriptingSynth`, and `ScriptingMidiProcessor`:
- All have: `getId`, `getType`, `exists`, `setAttribute`, `getAttribute`, `getAttributeId`, `getAttributeIndex`, `getNumAttributes`, `setBypassed`, `isBypassed`, `exportState`, `restoreState`, `exportScriptControls`, `restoreScriptControls`, `addModulator`, `addGlobalModulator`, `addStaticGlobalModulator`, `getModulatorChain`
- The implementations are nearly copy-paste across all four classes

Modulator-specific methods not shared: `setIntensity`, `getIntensity`, `setIsBipolar`, `isBipolar`, `getCurrentLevel`, `asTableProcessor`, `connectToGlobalModulator`, `getGlobalModulatorId`, `setMatrixProperties`

---

## Threading Considerations

- `setAttribute` uses `ProcessorHelpers::getAttributeNotificationType()` which returns the appropriate notification type based on the current thread
- `setBypassed` uses `sendNotification` directly
- `setIntensity` calls `m->setIntensity()` (which writes to a `LinearSmoothedValue<float>`) and then `sendOtherChangeMessage(Intensity, sendNotificationAsync)`
- Module creation/removal via `ModuleHandler` coordinates with `KillStateHandler`
- `getCurrentLevel()` reads display values which are written by the audio thread -- this is a read-only display query

---

## Preprocessor Guards

No preprocessor guards affect the ScriptingModulator class itself. The class is available in all build targets (backend, frontend, DLL).

---

## Modulator Type Hierarchy (C++)

The C++ `Modulator` class (Modulators.h:251) extends `Processor`:
```cpp
class Modulator: public Processor
```

Key subclass hierarchy:
- `VoiceStartModulator` -- processes at voice start only
- `TimeVariantModulator` -- processes per audio block
- `EnvelopeModulator` -- processes per voice per block
- `ModulatorChain` -- container that chains multiple modulators (extends EnvelopeModulator via Chain)

The `Modulation` class is a separate mixin (not inheriting from Modulator) that provides intensity/bipolar/mode logic. Both `VoiceStartModulator` and `TimeVariantModulator` inherit from both `Modulator` and `Modulation`.

`ModulatorChain` inherits from both `Chain` and `EnvelopeModulator`, making it both a container and a modulator itself -- this is why `getModulatorChain()` can return a ScriptingModulator wrapper.
