# ScriptSliderPack -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md`
- `enrichment/resources/survey/class_survey_data.json`
- `enrichment/resources/explorations/ScriptComponent_base.md`
- `enrichment/phase1/SliderPackData/Readme.md` -- unavailable (file missing)
- No `ComplexDataScriptComponent` base exploration file was found in `enrichment/resources/explorations/`

## Source Files

- Header declaration: `HISE/hi_scripting/scripting/api/ScriptingApiContent.h` (ScriptSliderPack at lines 1498-1591)
- API implementation: `HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp` (ScriptSliderPack block at lines 3498-3845)
- Wrapper declaration: `HISE/hi_scripting/scripting/api/ScriptComponentWrappers.h` (SliderPackWrapper at lines 925-949)
- Wrapper implementation: `HISE/hi_scripting/scripting/api/ScriptComponentWrappers.cpp` (SliderPackWrapper at lines 2499-2623)
- Core data model: `HISE/hi_tools/hi_standalone_components/SliderPack.h` and `HISE/hi_tools/hi_standalone_components/SliderPack.cpp`
- ExternalData infrastructure: `HISE/hi_core/hi_dsp/ProcessorInterfaces.cpp`
- Related scripting handle: `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h/.cpp` (ScriptSliderPackData)

---

## Survey Context (createdBy / creates / seeAlso)

From `class_survey_data.json`:

- Class: `ScriptSliderPack`
- Base class (survey): `ComplexDataScriptComponent`
- `createdBy`: `Content`
- `creates`: none
- `seeAlso`:
  - `ScriptSlider` -- single scalar control vs multi-value array editor
  - `ScriptTable` -- discrete bars vs interpolated curve graph
  - `SliderPackData` -- data handle counterpart for the UI component

From prerequisite table in `class_survey.md`:

- Prerequisite chain says `SliderPackData -> ScriptSliderPack`
- `enrichment/phase1/SliderPackData/Readme.md` was not present, so prerequisite context could not be loaded

---

## Inheritance and Type Layering

```text
ScriptComponent
  -> ComplexDataScriptComponent (also ExternalDataHolder, ComplexDataUIUpdaterBase::EventListener)
      -> ScriptSliderPack
```

`ScriptSliderPack` therefore combines:

1. ScriptComponent property/callback/value plumbing
2. ComplexDataScriptComponent data-source routing (internal object, connected processor data slot, or external referred holder)
3. SliderPack-specific scalar-array operations and wrapper behavior

### Class Declaration Highlights

`ScriptingApiContent.h` `struct ScriptSliderPack` contains:

- Enum `Properties`:
  - `SliderAmount`
  - `StepSize`
  - `FlashActive`
  - `ShowValueOverlay`
  - `SliderPackIndex`
  - `CallbackOnMouseUpOnly`
  - `StepSequencerMode`
- Overrides:
  - `setScriptObjectPropertyWithChangeMessage`
  - `setValue`
  - `getValue`
  - `changed`
  - `onComplexDataEvent`
- API methods:
  - `setSliderAtIndex`
  - `getSliderValueAt`
  - `setAllValues`
  - `setAllValuesWithUndo`
  - `getNumSliders`
  - `referToData`
  - `setWidthArray`
  - `registerAtParent`
  - `getDataAsBuffer`
  - `setAllValueChangeCausesCallback`
  - `setUsePreallocatedLength`
- Internal state:
  - `Array<var> widthArray`
  - `bool allValueChangeCausesCallback = true`

No nested helper classes inside ScriptSliderPack itself; wrapper and data model carry most runtime behavior.

---

## ComplexDataScriptComponent Infrastructure Used By ScriptSliderPack

`ScriptSliderPack` relies on inherited machinery in `ComplexDataScriptComponent`:

### Constructor Path

```cpp
ownedObject = snex::ExternalData::create(type);
ownedObject->setGlobalUIUpdater(base->getMainController_()->getGlobalUIUpdater());
ownedObject->setUndoManager(base->getMainController_()->getControlUndoManager());
```

For `ScriptSliderPack`, `type` is `snex::ExternalData::DataType::SliderPack`, so owned data is a `SliderPackData` object.

### Data Source Resolution

`getUsedData(requiredType)` resolves source in this order:

1. `otherHolder` (if set by `referToDataBase`)
2. `getConnectedProcessor()` cast to `ExternalDataHolder` using `processorId`
3. internal `ownedObject`

`cachedObjectReference` is updated and event-listener hookup is moved to the active source via `updateCachedObjectReference()`.

### Cross-object binding

`referToDataBase(var)` accepts:

- `ScriptComplexDataReferenceBase` (eg `ScriptSliderPackData`)
- another `ComplexDataScriptComponent` (eg another `ScriptSliderPack`)
- integer `-1` to return to internal owned object

Type mismatch is explicitly guarded (`Data Type mismatch`).

### Parent registration path

`registerComplexDataObjectAtParent(index)`:

- requires `ProcessorWithDynamicExternalData`
- registers `ownedObject` into dynamic external-data storage
- sets index property and updates cached reference
- for SliderPack type returns `new ScriptingObjects::ScriptSliderPackData(getScriptProcessor(), index)`

### ValueTree persistence pattern

ComplexDataScriptComponent `exportAsValueTree()` writes:

```cpp
v.setProperty("data", cachedObjectReference->toBase64String(), nullptr);
```

and `restoreFromValueTree()` reads with `fromBase64String(...)`.

This is the class-level JSON/value schema pattern for complex-data state persistence.

---

## ScriptSliderPack Constructor and API Registration

Location: `ScriptingApiContent.cpp` lines 3513-3574.

### Script properties registered

```cpp
ADD_NUMBER_PROPERTY(i00, "sliderAmount");
ADD_NUMBER_PROPERTY(i01, "stepSize");
ADD_SCRIPT_PROPERTY(i02, "flashActive");
ADD_SCRIPT_PROPERTY(i03, "showValueOverlay");
ADD_SCRIPT_PROPERTY(i05, "SliderPackIndex");
ADD_SCRIPT_PROPERTY(i06, "mouseUpCallback");
ADD_SCRIPT_PROPERTY(i07, "stepSequencerMode");
```

### Default values

- Geometry defaults: width 200, height 100
- Visual defaults:
  - `bgColour = 0x00000000`
  - `itemColour = 0x77FFFFFF`
  - `itemColour2 = 0x77FFFFFF`
  - `textColour = 0x33FFFFFF`
- Behavior defaults:
  - `CallbackOnMouseUpOnly = false`
  - `StepSequencerMode = false`
  - `FlashActive = true`
  - `ShowValueOverlay = true`
  - `SliderPackIndex = 0`
  - `SliderAmount = SliderPackData::NumDefaultSliders` (16)
  - `StepSize = 0.01`
  - inherited `defaultValue = 1.0f`

### Initialization sequence

- `handleDefaultDeactivatedProperties()` inherited from `ComplexDataScriptComponent`
- `initInternalPropertyFromValueTreeOrDefault(...)` for slider pack properties and inherited `min/max/processorId`
- `updateCachedObjectReference()` to attach to active `SliderPackData` source

### API registration

Registered with plain `ADD_API_METHOD_N` only:

- `setSliderAtIndex`
- `getSliderValueAt`
- `setAllValues`
- `setAllValuesWithUndo`
- `getNumSliders`
- `referToData`
- `setWidthArray`
- `registerAtParent`
- `getDataAsBuffer`
- `setAllValueChangeCausesCallback`
- `setUsePreallocatedLength`

No `ADD_TYPED_API_METHOD_N` calls are present in ScriptSliderPack constructor.

---

## Factory / obtainedVia

Creation is through `Content`:

- Declaration: `Content.addSliderPack(Identifier sliderPackName, int x, int y)` in `ScriptingApiContent.h`
- Registration: `setMethod("addSliderPack", Wrapper::addSliderPack);`
- Implementation: `return addComponent<ScriptSliderPack>(sliderPackName, x, y);`

So scripting obtainedVia is `Content.addSliderPack(name, x, y)`.

---

## Wrapper Layer (ScriptCreatedComponentWrappers::SliderPackWrapper)

The wrapper is where ScriptSliderPack properties are pushed into actual JUCE `SliderPack` component behavior.

### Construction behavior

- Creates `SliderPack` bound to `pack->getSliderPackData()`
- Applies stored `widthArray` immediately (`sp->setSliderWidths(pack->widthArray)`)
- Registers as source watcher listener (`pack->getSourceWatcher().addSourceListener(this)`)
- Initializes all properties
- Local/global LookAndFeel fallback path:
  - if local LAF implements `SliderPack::LookAndFeelMethods`, use it
  - else if global LAF implements that interface, use global one

### Property update routing

In `updateComponent(int propertyIndex, var newValue)`:

- Colour triplet updates call `updateColours(sp)`
- Tooltip mapped to `sp->setTooltip(...)`
- `FlashActive -> sp->setFlashActive(bool)`
- `ShowValueOverlay -> sp->setShowValueOverlay(bool)`
- `StepSize/min/max -> updateRange(...)` if not connected to external processor
- `CallbackOnMouseUpOnly -> sp->setCallbackOnMouseUp(bool)`
- `StepSequencerMode -> sp->setStepSequencerMode(bool)`

`updateValue(var)` reapplies `widthArray` via `sp->setSliderWidths(ssp->widthArray)`.

### Color mapping

- `itemColour -> Slider::thumbColourId`
- `itemColour2 -> Slider::textBoxOutlineColourId`
- `bgColour -> Slider::backgroundColourId`
- `textColour -> Slider::trackColourId`

---

## Upstream Data Providers (Provider -> Dependency -> API class chain)

### Chain A: UI component internal data

`Content.addSliderPack(...)`
-> constructs `ScriptSliderPack`
-> `ComplexDataScriptComponent` calls `ExternalData::create(SliderPack)`
-> internal `SliderPackData` owned object
-> wrapped by `SliderPackWrapper` into JUCE `SliderPack`

### Chain B: External processor data source

`processorId` points ScriptComponent to connected processor
-> `ComplexDataScriptComponent::getExternalHolder()` casts processor to `ExternalDataHolder`
-> `getComplexBaseType(SliderPack, SliderPackIndex)` resolves upstream `SliderPackData`
-> ScriptSliderPack methods operate on that external data

### Chain C: Explicit referred object

`referToData(...)` with ScriptSliderPackData or another complex-data component
-> `otherHolder` set to referenced holder
-> cached source re-routed
-> ScriptSliderPack now edits shared data object

### Chain D: Engine data-handle side

Engine-side factories provide non-UI handles that can be bound into ScriptSliderPack:

- `Engine.createAndRegisterSliderPackData(index)` returns `ScriptSliderPackData`
- `Engine.getComplexDataReference("SliderPack", moduleId, index)` can return `ScriptSliderPackData`

`ScriptSliderPack.referToData(...)` accepts these handles through shared `ScriptComplexDataReferenceBase` path.

### Build-target nuance in providers

In `ProcessorWithExternalData::createAndInit` (`ProcessorInterfaces.cpp`):

```cpp
#if USE_BACKEND
if(auto af = dynamic_cast<SliderPackData*>(d))
    af->setUsePreallocatedLength(128);
#endif
```

Backend builds preallocate 128 slots for SliderPackData to avoid DLL heap reallocations; this changes resizing memory behavior in IDE/backend context vs non-backend targets.

---

## Enum / Constant Behavioral Tracing (mode selectors and toggles)

No constructor `addConstant()` constants exist in ScriptSliderPack. Behavioral selectors are implemented as properties / booleans. Their runtime consequences:

### 1) `CallbackOnMouseUpOnly` (property `mouseUpCallback`)

Consumption points:

- `SliderPackWrapper::updateComponent` maps property to `SliderPack::setCallbackOnMouseUp`
- `SliderPack::sliderValueChanged`:
  - if true: live slider movements use `dontSendNotification` and no undo
  - if false: live movements send sync notification and can use undo path
- `SliderPack::mouseDown` and `mouseDrag` use same notification switch
- `SliderPack::mouseUp` when true commits whole pack via `setFromFloatArray(..., sendNotificationAsync, true)`

Behavioral consequence:

- `false`: callback/content updates occur during drag
- `true`: callback/content updates are deferred until mouse release, with a consolidated undoable update

### 2) `StepSequencerMode` (property `stepSequencerMode`)

Consumption points:

- Wrapper maps property to `SliderPack::setStepSequencerMode`
- `setStepSequencerMode` sets `toggleMaxMode`
- In `mouseDown` / `mouseDrag` / `sliderValueChanged`:
  - normal mode: value follows vertical drag proportion
  - toggle mode: click toggles among range start/end and optional midpoint "ghost note" (modifier-dependent)
  - `mouseDoubleClick` returns early when toggle mode is active

Behavioral consequence:

- `false`: continuous bar editing behavior
- `true`: step-sequencer style state toggling per step, including modifier-mediated midpoint state

### 3) `FlashActive`

Consumption points:

- Property -> `SliderPackData::setFlashActive`
- `SliderPack::paintOverChildren` draws flash overlays only if timer running and `isFlashActive()`
- `SliderPack::timerCallback` exits early when flash inactive

Behavioral consequence:

- `true`: transient highlight flashes for displayed index changes
- `false`: no flash animation repaint cycle

### 4) `ShowValueOverlay`

Consumption points:

- Property -> `SliderPackData::setShowValueOverlay`
- `SliderPack::paintOverChildren` text popup shown only when dragging/shift-preview and `isValueOverlayShown()`

Behavioral consequence:

- `true`: overlay text with `#index: value` appears during interaction
- `false`: overlay suppressed

### 5) Script method selector `setAllValueChangeCausesCallback(bool)`

Consumption points inside ScriptSliderPack methods:

- `setSliderAtIndex`: decides whether `setValue` sends notification and whether display-change message is emitted
- `setAllValues`: decides between content-change callback path and display-only refresh path
- `setAllValuesWithUndo`: current implementation always uses notify path (`true || ...` hardcoded)

Behavioral consequence:

- `true`: bulk/single scripted writes produce control callback flow
- `false`: scripted writes can update visual data without control callback (except undo variant currently forced to notify)

### 6) Data-model selector `setUsePreallocatedLength(int)`

Consumption points in `SliderPackData`:

- allocates `preallocatedData` buffer and redirects `dataBuffer` storage
- `setNumSliders` reuses preallocated storage when active
- `swapBuffer` copies into preallocated buffer instead of swapping heap object when active

Behavioral consequence:

- preallocation > 0: resizing and data swapping keep a fixed backing store up to cap and preserve values when shrinking below cap
- preallocation == 0: standard heap buffer replacement path

### 7) Width distribution selector `setWidthArray(var)`

Consumption points:

- ScriptSliderPack stores `widthArray` and triggers change message
- Wrapper reapplies `widthArray` on value updates
- `SliderPack::resized` chooses layout mode:
  - if width array empty or size mismatch: equal width distribution
  - else: normalized cumulative breakpoints define per-slider widths
- `getSliderIndexForMouseEvent` uses same width map for hit testing

Behavioral consequence:

- width map changes both rendering geometry and mouse-to-index mapping

---

## Threading / Lifecycle Constraints Observed

- ScriptSliderPack overrides `changed()` and directly calls `getScriptProcessor()->controlCallback(this, value);`
  - this bypasses ScriptComponent base `changed()` onInit guard behavior
  - callback triggering is driven by complex-data events (`onComplexDataEvent` for ContentChange)
- ComplexData source changes are event-listener based (`ComplexDataUIUpdaterBase` async/sync notifications)
- SliderPackData uses `SimpleReadWriteLock` around buffer access in core data methods
- Wrapper updates and painting are GUI-thread/component-layer operations

No class-local explicit `interfaceCreationAllowed()` check exists in ScriptSliderPack constructor/method block.

---

## Preprocessor Guards Relevant To Behavior

- `USE_BACKEND` in `ProcessorInterfaces.cpp`:
  - backend-only automatic `setUsePreallocatedLength(128)` for SliderPackData during external-data object init
- `USE_BACKEND` in `SliderPack::rebuildSliders()`:
  - backend metadata hookup (`DocumentWindowWithEmbeddedPopupMenu::setSubComponentTargetId`)
- `HISE_NO_GUI_TOOLS` guards in `SliderPack` rebuild/hover pseudo-class logic:
  - CSS class selector and pseudo-state propagation are conditionally compiled

No ScriptSliderPack-specific methods are compiled out with class-local `#if` in `ScriptingApiContent.cpp` block.

---

## Interface Compatibility / Listener Patterns

- `ScriptSliderPack` is `JUCE_DECLARE_WEAK_REFERENCEABLE`
- Data binding uses `WeakReference` to holders and cached data object
- Event bridge pattern:
  - `ComplexDataUIUpdaterBase::EventListener` implemented by component and data listeners
  - wrapper also attaches through source watcher for redirection updates
- Wrapper acts as `SliderPackData::Listener` for UI sync pathways

This matches the same compatibility model used by other complex-data components (`ScriptTable`, `ScriptAudioWaveform`).

---

## Property-to-Data Wiring Summary

In `setScriptObjectPropertyWithChangeMessage` of ScriptSliderPack:

- `sliderAmount` -> `SliderPackData::setNumSliders`
- `defaultValue` -> `SliderPackData::setDefaultValue`
- `min/max/stepSize` -> `SliderPackData::setRange(...)`
- `flashActive` -> `SliderPackData::setFlashActive`
- `showValueOverlay` -> `SliderPackData::setShowValueOverlay`
- `parameterId` explicitly ignored (no processor parameter connection semantics)

Then delegates to `ComplexDataScriptComponent::setScriptObjectPropertyWithChangeMessage` for shared behavior.

---

## Public API Surface Count (class-local)

ScriptSliderPack registers 11 class-local API methods in its constructor block.

With inherited ScriptComponent methods included, base JSON currently lists 42 total methods for this class.

---

## Distillation-oriented Notes

- Constructor has no `addConstant()` calls -> class constants table should be empty
- Constructor has no `ADD_TYPED_API_METHOD_N` calls -> forced type map comes from inherited ScriptComponent typed registrations (`setControlCallback`, `setKeyPressCallback`)
- Class is architecturally non-trivial (ComplexData source routing + wrapper behavior + slider-pack data model), so Readme `Details` section is warranted
