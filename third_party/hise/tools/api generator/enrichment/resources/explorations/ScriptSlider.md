# ScriptSlider -- Raw C++ Exploration

## Resource Files Consulted

- `HISE/tools/api generator/doc_builders/scripting-api-enrichment/phase1.md`
- `HISE/tools/api generator/enrichment/resources/survey/class_survey.md`
- `HISE/tools/api generator/enrichment/resources/survey/class_survey_data.json` (ScriptSlider entry)
- `HISE/tools/api generator/enrichment/phase1/Content/Readme.md` (prerequisite context)
- `HISE/tools/api generator/enrichment/resources/explorations/ScriptComponent_base.md` (base class context)
- `HISE/tools/api generator/enrichment/base/ScriptSlider.json`

## Source Files Examined

- Header (class declaration): `HISE/hi_scripting/scripting/api/ScriptingApiContent.h:963`
- Main implementation: `HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp` (ScriptSlider block around 2054-2708)
- Wrapper/UI bridge: `HISE/hi_scripting/scripting/api/ScriptComponentWrappers.cpp` (SliderWrapper)
- Matrix connection declaration: `HISE/hi_scripting/scripting/api/ScriptModulationMatrix.h`
- Matrix connection implementation: `HISE/hi_scripting/scripting/api/ScriptModulationMatrix.cpp`
- Mode/range provider: `HISE/hi_core/hi_core/MacroControlledComponents.h/.cpp` (HiSlider)
- Matrix helper infrastructure: `HISE/hi_core/hi_dsp/modules/ModulationMatrixTools.h/.cpp`

## Prerequisite Context Integration (Content)

Loaded prerequisite `Content` analysis first. ScriptSlider is consistent with that model:

- obtained via Content factory (`Content.addKnob(...)` -> `Content::addComponent<ScriptSlider>`)
- creation lifecycle is onInit-gated by `Content::allowGuiCreation`
- existing-name calls are idempotent and return existing components
- component property state is ValueTree-backed and rebuilt/restored through Content infrastructure

ScriptSlider-specific work here focuses on what this class adds over ScriptComponent + Content: slider mode system, range conversion, popup text function hook, modifier object bridge, and modulation matrix bridge.

## Survey Data (class_survey_data.json)

- `createdBy`: `Content`
- `creates`: none
- `seeAlso`:
  - ScriptButton (toggle vs continuous/stepped numeric control)
  - ScriptComboBox (named discrete choice list vs numeric range)
  - ScriptSliderPack (single value vs multiple values)

## Class Declaration and Inheritance

Declaration:

```cpp
struct ScriptSlider : public ScriptComponent,
                      public PluginParameterConnector
```

Direct implications:

- inherits full ScriptComponent property/event/callback model (covered in `ScriptComponent_base.md`)
- adds plugin parameter gesture bridge via `PluginParameterConnector` (host automation notification helper)

PluginParameterConnector behavior (in `ScriptingApiContent.cpp`):

- `setConnected(ScriptedControlAudioParameter*)` stores pointer and back-links controlled component
- `sendParameterChangeNotification(float)` wraps host gesture begin/set/end, with one-shot suppression via `deactivateNextUpdate()`

## ScriptSlider Properties Enum

`ScriptingApiContent.h` defines ScriptSlider-local properties starting at `ScriptComponent::Properties::numProperties`:

1. `Mode`
2. `Style`
3. `stepSize`
4. `middlePosition`
5. `suffix`
6. `filmstripImage`
7. `numStrips`
8. `isVertical`
9. `scaleFactor`
10. `mouseSensitivity`
11. `dragDirection`
12. `showValuePopup`
13. `showTextBox`
14. `scrollWheel`
15. `enableMidiLearn`
16. `sendValueOnDrag`
17. `matrixTargetId`

`priorityProperties` includes Mode, so mode handling is ordered early in property application.

## Nested Types and Internal Hierarchies

### ModifierObject (Script API object returned by createModifiers)

Nested `ScriptSlider::ModifierObject : ConstScriptingObject` exposes constants for `setModifiers()` calls.

String action constants:

- `TextInput`
- `FineTune`
- `ResetToDefault`
- `ContextMenu`
- `ScaleModulation`

Flag constants:

- `disabled` -> 0
- `noKeyModifier` -> `SliderWithShiftTextBox::ModifierObject::noKeyModifier` (1024)
- `shiftDown` -> `ModifierKeys::shiftModifier`
- `rightClick` -> `ModifierKeys::rightButtonModifier`
- `cmdDown` -> `ModifierKeys::commandModifier`
- `altDown` -> `ModifierKeys::altModifier`
- `ctrlDown` -> `ModifierKeys::ctrlModifier`
- `doubleClick` -> `SliderWithShiftTextBox::ModifierObject::doubleClickModifier` (512)

Constructor uses `ConstScriptingObject(sp, 13)` and registers exactly 13 constants.

### MatrixConnectionBase abstraction

Private abstract base:

- owns `parent` (weak ScriptSlider), `gc` (weak GlobalModulatorContainer), `matrixData` ValueTree, `targetId`
- virtuals:
  - `getDisplayBuffer(int)`
  - `createIntensityConverter(int)`

Two concrete implementations:

1. `MultiMatrixModulatorConnection` (for matrix targets that are matrix modulators)
2. `MatrixCableConnection` (for cable/parameter style matrix targets)

### MatrixCableConnection hierarchy

`MatrixCableConnection` (declared in `ScriptModulationMatrix.h`) contains:

- `QueryFunction : ModulationDisplayValue::QueryFunction`
- `Target : GlobalRoutingManager::CableTargetBase + ReferenceCountedObject + SimpleTimer`
- `Target::AuxTarget : GlobalRoutingManager::CableTargetBase + ReferenceCountedObject`

State buckets:

- `allTargets`
- `scaleTargets` (TargetMode::Gain)
- `addTargets` (TargetMode::Unipolar/Bipolar)

Important lock:

- `SimpleReadWriteLock listLock` used during target list rebuild/read.

## Constructor Wiring and API Registration

Source: `ScriptingApiContent.cpp` ScriptSlider ctor.

### Property registration macros

- registers all ScriptSlider properties with selector hints (`ChoiceSelector`, `ToggleSelector`, `FileSelector`) where relevant
- notable selectors:
  - Mode: choice
  - Style: choice
  - filmstripImage: file
  - dragDirection: choice
  - showValuePopup: choice

### Default values

- geometry: width 128, height 48
- mode/style defaults: `Linear`, `Knob`
- range defaults: min 0.0, max 1.0, step 0.01, middlePosition "disabled"
- popup/interaction defaults:
  - showValuePopup `No`
  - showTextBox false
  - scrollWheel true
  - mouseSensitivity 1.0
  - dragDirection `Diagonal`
  - sendValueOnDrag true
- matrix default: `matrixTargetId` empty

### Initialization-from-ValueTree

`initInternalPropertyFromValueTreeOrDefault(...)` applied for key properties including Mode, Style, middlePosition, stepSize, min, max, suffix, filmstripImage, linkedTo.

Mode has special handling:

- `dontUpdateMode` when mode property missing in tree to avoid unwanted mutation behavior on first init.

### API method registration in ScriptSlider constructor

Registered methods and typed constraints:

- `setValuePopupFunction` -> typed param1 Function + callback diagnostic expects 1 arg
- `setMidPoint` -> typed param1 Colour (Number | String) to allow midpoint numbers and the sentinel string `"disabled"`
- `setRange` -> untyped (3 args)
- `setMode` -> typed String
- `setStyle` -> typed String
- `setMinValue` -> typed Number
- `setMaxValue` -> typed Number
- `getMinValue` -> untyped (0)
- `getMaxValue` -> untyped (0)
- `contains` -> untyped (1)
- `createModifiers` -> untyped (0)
- `connectToModulatedParameter` -> untyped (2)
- `setModifiers` -> typed (String, IndexOrArray)

### addConstant calls in constructor

- none in ScriptSlider constructor itself
- commented-out old constants for modes exist but inactive

## Factory / obtainedVia Chain

Primary creation path:

1. script calls `Content.addKnob(id, x, y)`
2. `Content::addKnob` calls `addComponent<ScriptSlider>`
3. `addComponent` checks `allowGuiCreation` (onInit gate)
4. if existing id found, returns existing component (and may update x/y)
5. otherwise adds component ValueTree child and constructs `new ScriptSlider(...)`

Runtime wrapper creation path:

- `ScriptSlider::createComponentWrapper(...)` -> `new ScriptCreatedComponentWrappers::SliderWrapper(...)`
- wrapper instantiates actual JUCE `HiSlider`

## Property-to-Behavior Bridges

## setScriptObjectPropertyWithChangeMessage dispatch

ScriptSlider overrides property setter dispatch and intercepts key property IDs:

- `mode` -> calls `setMode(...)`
- `style` -> calls `setStyle(...)`
- `middlePosition` -> calls `setMidPoint(...)`
- `defaultValue` -> clamps to [min,max], sanitizes float, stores clamped value
- `matrixTargetId` -> creates/destroys matrix connection object and popup modulation data
- `filmstripImage` -> loads pooled image or clears to default skin
- `parameterId` special case with connected `MatrixModulator` and parameter Value reroutes into `connectToModulatedParameter(...)`

Fallback for other properties delegates to ScriptComponent implementation.

## Wrapper-side realization (SliderWrapper)

Script object state is translated into live `HiSlider` state in wrapper methods:

- `updateSliderRange`
  - TempoSync mode clamps min/max to tempo index limits and forces interval 1
  - invalid min/max or step disables slider and forces linear 0..1 range
  - applies skew from `middlePosition` only when `ApiHelpers::shouldApplyMidPoint(min, max, middlePosition)` returns true
  - applies `suffix` only when ScriptSlider mode is Linear
  - sets double-click return from `defaultValue` if inside range
- `updateSliderStyle`
  - `dragDirection` only affects rotary style
  - Range style (`TwoValueHorizontal`) forces no textbox
  - writes CSS helper class `.linear-slider` for linear styles
  - showTextBox toggles textbox visibility and shift-text-input behavior
- `updateSensitivity` maps `mouseSensitivity` scaler to JUCE drag sensitivity
- `updateFilmstrip` applies `FilmstripLookAndFeel` when image is valid
- `showValuePopup` + `getValuePopupPosition` implement popup location enum behavior
- `getTextForValuePopup` executes custom popup function if provided and valid JS function

## Enum/Constant Behavioral Tracing

### Mode values (string -> HiSlider::Mode)

Defined by options list in ScriptSlider:

- `Frequency`
- `Decibel`
- `Time`
- `TempoSync`
- `Linear`
- `Discrete`
- `Pan`
- `NormalizedPercentage`

Behavioral consequences across chain:

1. `ScriptSlider::setMode` maps string to enum index and stores `m`
2. if mode string invalid: falls back to internal `HiSlider::Mode::Linear` and returns early (no property update)
3. default-range migration logic:
   - checks whether current range/step/skew still matches old mode defaults
   - if yes, switching mode also rewrites min/max/step/suffix/middlePosition from new mode defaults
   - if no, custom range is preserved
4. wrapper `updateSliderRange` applies mode to live HiSlider
5. HiSlider provides mode-specific ranges/suffixes via `HiSlider::getRangeForMode` and `HiSlider::getSuffixForMode`

### Midpoint parsing and sentinel behavior

- midpoint parsing delegates to `RangeHelpers::parseMidPointValue(var)`:
  - numeric value or numeric string -> `{ true, parsedDouble }`
  - string `"disabled"` -> `{ false, DBL_MAX }`
  - invalid non-numeric string -> `{ false, 0.0 }`
- `ApiHelpers::getMidPointValue` / `shouldApplyMidPoint` use this parser for all ScriptSlider skew call sites.
- practical result: midpoint skew is applied only for numeric values strictly inside `(min, max)`.
- legacy `-1` is no longer a dedicated disable token; it behaves as a normal numeric midpoint and only disables skew when out of range.

Mode default ranges from `HiSlider::getRangeForMode`:

- Frequency: 20..20000, interval 1, skew center 1500
- Decibel: -100..0, interval 0.1, skew center -18
- Time: 0..20000, interval 1, skew center 1000
- TempoSync: 0..(TempoSyncer::numTempos-1), interval 1
- Pan: -100..100, interval 1
- NormalizedPercentage: 0..1, interval 0.01
- Linear: 0..1, interval 0.01
- Discrete: interval 1 with default NormalisableRange base

Text suffix behavior from `HiSlider::getSuffixForMode`:

- Frequency -> ` Hz`
- Decibel -> ` dB`
- Time -> ` ms`
- Pan -> `L` or `R` depending on sign
- NormalizedPercentage -> `%`
- TempoSync / Linear / Discrete -> empty

### Style values

Values from options list:

- `Knob`
- `Horizontal`
- `Vertical`
- `Range`

Behavior:

- `setStyle` maps to JUCE SliderStyle
- only `Range` maps to `TwoValueHorizontal`
- range-only API guard:
  - `setMinValue`, `setMaxValue`, `getMinValue`, `getMaxValue`, `contains`
  - these only function when style is Range; otherwise log errors and return fallback
- ValueTree export includes `rangeMin`/`rangeMax` only when style is Range

### dragDirection values

`Diagonal`, `Vertical`, `Horizontal`

Behavior:

- only relevant when style is rotary
- maps to RotaryHorizontalVerticalDrag, RotaryVerticalDrag, RotaryHorizontalDrag

### showValuePopup values

`No`, `Above`, `Below`, `Left`, `Right`

Behavior:

- `No` prevents popup creation on drag start
- others create popup and calculate relative popup location
- linear styles adjust popup Y offset for below placement

### setModifiers action constants and flag values

Action keys consumed by slider core:

- TextInput -> opens inline text editor
- ResetToDefault -> sets slider to double-click return value
- ContextMenu -> triggers MIDI learn popup or custom popup
- ScaleModulation -> delegates drag delta to scale function
- FineTune -> used for slider velocity-mode fine drag modifier mapping

No-key and double-click pseudo flags are interpreted by `SliderWithShiftTextBox::ModifierObject::getActionForModifier`.

## Upstream Data Providers and External-State Dependencies

### 1) Modulation matrix chain (parameter targets)

Provider -> dependency -> ScriptSlider chain:

1. `GlobalModulatorContainer` owns matrix ValueTree (`getMatrixModulatorData()`)
2. `MatrixIds::Helpers` resolves source list / target type / connection rows
3. `ScriptSlider::setScriptObjectPropertyWithChangeMessage(matrixTargetId)` decides connection class:
   - target type Modulators -> `MultiMatrixModulatorConnection`
   - target type Parameters -> `MatrixCableConnection`
4. connection object subscribes to matrix ValueTree changes and source cables
5. calculated modulated value is pushed back through script processor control callback

### 2) Source value providers for MatrixCableConnection

`MatrixCableConnection::Target` source values come from routing cables:

- source IDs from `MatrixIds::Helpers::fillModSourceList` (gain modulation child processors in GlobalModulatorContainer)
- routing lookup via `routing::GlobalRoutingManager::getOrCreate(...).getSlotBase(sourceId, Cable)`
- target subscribes as cable target and receives `sendValue(double)` updates
- optional aux modulation source via `AuxTarget` multiplies intensity contribution

### 3) Target classification provider

`MatrixIds::Helpers::getTargetType` determines modulator-target vs parameter-target:

- builds modulator target list from `MatrixModulator` processors in synth chain
- if targetId in that list -> Modulators, else Parameters

This directly controls which matrix connection implementation ScriptSlider creates.

### 4) Build-target and runtime environment influence

- `USE_BACKEND` guards only diagnostic logging for illegal range in normalized conversion methods
- core mode/range behavior itself is not backend-only
- Content frontend/backend behavior (from prerequisite) still governs component lifecycle and update dispatcher state, so ScriptSlider creation and rebuild timing follows that host class behavior

## JSON / DynamicObject Patterns

ScriptSlider-specific JSON object patterns:

- `setModifiers(String, var)`:
  - ensures `modObject` dynamic object exists
  - stores action key -> modifier data as dynamic properties
- `createModifiers()`:
  - returns `var(new ModifierObject(...))` (script object with constants)
- value popup text callback invocation in wrapper uses `var::NativeFunctionArgs` with one numeric argument

Related matrix JSON/ValueTree patterns used by ScriptSlider dependencies:

- Matrix connection rows are ValueTree children with properties:
  - `TargetId`, `SourceIndex`, `Mode`, `Intensity`, `AuxIndex`, `AuxIntensity`, `Inverted`
- matrix helper `Properties::RangeData` uses DynamicObject keys `InputRange`, `OutputRange`, `mode`, `UseMidPositionAsZero`

## Threading / Lifecycle / Safety Notes

- creation lifecycle: constrained by Content (`allowGuiCreation` -> onInit only)
- matrix recalculation path can execute on audio thread context (`calculateNewModValue` checks current thread via KillStateHandler)
- target list access in matrix connection guarded with `SimpleReadWriteLock`
- some operations use async timer / UI updater (`SimpleTimer`, popup updates, ringbuffer writes)
- normalized conversion error logs are backend-only (`#if USE_BACKEND`) but method still returns deterministic fallback when invalid

## Preprocessor Guards Observed

- `#if USE_BACKEND` in ScriptSlider normalized conversion methods for error logging
- `#if USE_FRONTEND` in Content constructor affects update dispatcher setup (indirect lifecycle impact)

No ScriptSlider class-level feature guards like `HISE_INCLUDE_*` were found in the explored code paths.

## Method Surface (class-specific additions over ScriptComponent)

Wrapper + registrations indicate ScriptSlider-specific public API additions:

- `setValuePopupFunction`
- `setMidPoint`
- `setRange`
- `setMode`
- `setStyle`
- `setMinValue`
- `setMaxValue`
- `getMinValue`
- `getMaxValue`
- `contains`
- `createModifiers`
- `setModifiers`
- `connectToModulatedParameter`

Base ScriptComponent methods still appear in ScriptSlider base JSON and remain inherited/available.

## Constants Inventory for Distillation

ScriptSlider constructor:

- no active `addConstant()` registrations

Dynamic constant object from `createModifiers()`:

- object type: `Modifiers`
- 13 constants registered (5 action-name strings + 8 flag constants)
