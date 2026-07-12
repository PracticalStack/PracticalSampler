# ScriptDynamicContainer -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/explorations/ScriptComponent_base.md` -- base class infrastructure
- `enrichment/resources/base_methods/ScriptComponent.md` -- pre-distilled base methods
- `enrichment/phase1/Content/Readme.md` -- prerequisite: Content factory class
- `enrichment/resources/survey/class_survey_data.json` -- survey entry for ScriptDynamicContainer

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` (lines 2318-2517)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` (lines 6149-6940)
- **DynComp IDs:** `hi_scripting/scripting/api/DynamicComponentContainerIds.h`
- **DynComp ID defaults:** `hi_scripting/scripting/api/DynamicComponentContainerIds.cpp`
- **DynComp Data model:** `hi_scripting/scripting/api/DynamicComponentContainer.h`
- **DynComp Data impl:** `hi_scripting/scripting/api/DynamicComponentContainer.cpp`
- **DynComp component types:** `hi_scripting/scripting/api/DynamicComponentContainerTypes.cpp`
- **UI Wrapper:** `hi_scripting/scripting/api/ScriptComponentWrappers.h` (lines 1015-1050)
- **UI Wrapper impl:** `hi_scripting/scripting/api/ScriptComponentWrappers.cpp` (lines 3007-3016)

---

## Class Declaration

```cpp
struct ScriptDynamicContainer : public ScriptComponent,
                                public Dispatchable
```

Inherits from `ScriptComponent` (all 35 base methods) and `Dispatchable` (for async dispatching).

### Static Identity

```cpp
static Identifier getStaticObjectName() { RETURN_STATIC_IDENTIFIER("ScriptDynamicContainer"); }
```

### Own Properties Enum

```cpp
enum Properties
{
    numProperties  // no additional properties beyond ScriptComponent base
};
```

ScriptDynamicContainer adds NO additional component properties. It only has the base ScriptComponent properties, many of which are deactivated (see below).

---

## Constructor

```cpp
ScriptDynamicContainer(ProcessorWithScriptingContent* base, Content* parentContent,
                       Identifier panelName, int x, int y, int width, int height):
    ScriptComponent(base, panelName, 0),
    valueCallback(base, this, var(), 2)
{
    setDefaultValue(ScriptComponent::Properties::x, x);
    setDefaultValue(ScriptComponent::Properties::y, y);
    setDefaultValue(ScriptComponent::Properties::width, 200);
    setDefaultValue(ScriptComponent::Properties::height, 100);

    handleDefaultDeactivatedProperties();

    ADD_API_METHOD_1(setData);
    ADD_API_METHOD_1(setValueCallback);
}
```

### Key observations:
- Default size is 200x100 (not the usual component defaults)
- No constants registered via `addConstant()`
- No typed API methods on the container itself (both use plain `ADD_API_METHOD_1`)
- The `valueCallback` WeakCallbackHolder takes 2 arguments (id, value)

### Deactivated Properties

```cpp
void ScriptDynamicContainer::handleDefaultDeactivatedProperties()
{
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::macroControl));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::isPluginParameter));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::defaultValue));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::pluginParameterName));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::text));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::tooltip));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::processorId));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::parameterId));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::isMetaParameter));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::linkedTo));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::automationId));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::deferControlCallback));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::pluginParameterGroup));
    deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::saveInPreset));
}
```

**16 properties deactivated.** This is the most aggressive deactivation of any component type. The remaining active base properties are:
- `visible`, `enabled`, `locked`
- `x`, `y`, `width`, `height`
- `bgColour`, `itemColour`, `itemColour2`, `textColour`
- `useUndoManager`
- `parentComponent`

This makes sense: ScriptDynamicContainer is a pure container -- it does not participate in the preset/parameter/macro system. Values are managed internally via the dyncomp::Data value tree.

---

## Own API Methods (2 methods on the container)

### Wrapper Registration

```cpp
struct ScriptDynamicContainer::Wrapper
{
    API_METHOD_WRAPPER_1(ScriptDynamicContainer, setData);
    API_VOID_METHOD_WRAPPER_1(ScriptDynamicContainer, setValueCallback);
};
```

Note: `setData` uses `API_METHOD_WRAPPER_1` (returns a value), while `setValueCallback` uses `API_VOID_METHOD_WRAPPER_1`.

### setData

```cpp
var ScriptDynamicContainer::setData(const var& newData)
{
    // Invalidate all existing child references
    for(auto c: childReferences)
        c->setInvalid(nullptr);
    childReferences.clear();

    auto json = newData;
    bool getFirstChild = false;

    // Wrap single object in array
    if(!json.isArray())
    {
        json = var(Array<var>(newData));
        getFirstChild = true;
    }

    // Create Data model from JSON
    auto b = ApiHelpers::getIntRectangleFromVar(getLocalBounds(0));
    data = new dyncomp::Data(getScriptProcessor()->getMainController_(), json, b);

    auto dt = data->getValueTree(dyncomp::Data::TreeType::Data);

    // If single object was passed, return reference to that child (not root)
    if(getFirstChild)
        dt = dt.getChild(0);

    // Notify UI wrapper to rebuild
    dataBroadcaster.sendMessage(sendNotificationAsync, data);

    return getOrCreateChildReference(dt);
}
```

**Key behavior:**
- Accepts either a single JSON object or an array of JSON objects
- If a single object is passed, wraps it in an array internally but returns a reference to the first child (not the root)
- If an array is passed, returns a reference to the root data tree
- Returns a `ContainerChild` (ChildReference) object
- Invalidates all previous ChildReference objects
- Triggers async UI rebuild via `dataBroadcaster`

### setValueCallback

```cpp
void ScriptDynamicContainer::setValueCallback(const var& valueFunction)
{
    if(data != nullptr && HiseJavascriptEngine::isJavascriptFunction(valueFunction))
    {
        valueCallback = WeakCallbackHolder(getScriptProcessor(), this, valueFunction, 2);
        valueCallback.incRefCount();
        valueCallback.setThisObject(this);
        valueCallback.setHighPriority();

        valueListener.setCallback(data->getValueTree(dyncomp::Data::TreeType::Values),
            valuetree::AsyncMode::Synchronously, [this](const Identifier& id, const var& newValue)
        {
            if(valueCallback)
            {
                var args[2];
                args[0] = id.toString();
                args[1] = newValue;
                valueCallback.call(args, 2);
            }
        }, false);
    }
}
```

**Key behavior:**
- Requires `setData()` to have been called first (checks `data != nullptr`)
- Callback receives 2 arguments: (componentId: String, newValue: var)
- Listens to ANY property change on the Values tree (uses `AnyPropertyListener`)
- Synchronous callback mode
- High priority callback

---

## ChildReference Inner Class (ContainerChild)

This is a major inner class that acts as the scripting API handle to individual child components within the dynamic container. It is returned by `setData()` and by navigation methods on other ChildReferences.

### Class Declaration

```cpp
struct ChildReference: public ConstScriptingObject,
                       public AssignableDotObject,
                       public ObjectWithJSONConverter,
                       public UserPresetStateManager
```

- `ConstScriptingObject` -- standard HISE scripting object with API method registration
- `AssignableDotObject` -- enables `ref.property = value` dot-assignment syntax
- `ObjectWithJSONConverter` -- enables JSON serialization (writeAsJSON, writeToStream)
- `UserPresetStateManager` -- enables user preset state save/restore integration

Object name: `"ContainerChild"`

### ChildReference Constructor

```cpp
ChildReference(ScriptDynamicContainer* parent, dyncomp::Data::Ptr data_, const ValueTree& cd):
    ConstScriptingObject(parent->getScriptProcessor(), 0),
    parentContainer(parent),
    data(data_),
    componentData(cd),
    valueCallback(getScriptProcessor(), this, var(), 1),
    paintRoutine(getScriptProcessor(), this, var(), 1),
    childCallback(getScriptProcessor(), this, var(), 2),
    um(cd[dyncomp::dcid::useUndoManager] ? getScriptProcessor()->getMainController_()->getControlUndoManager() : nullptr)
```

Key: if `useUndoManager` is true in the component data, operations use the control undo manager.

### ChildReference API Method Registration

28 API methods registered:

```
ADD_API_METHOD_2(set);
ADD_API_METHOD_1(get);
ADD_API_METHOD_1(setBounds);
ADD_API_METHOD_1(getLocalBounds);
ADD_API_METHOD_0(isValid);
ADD_API_METHOD_0(getParent);
ADD_API_METHOD_1(getComponent);
ADD_API_METHOD_1(getAllComponents);
ADD_API_METHOD_1(addChildComponent);
ADD_API_METHOD_0(removeFromParent);
ADD_API_METHOD_0(removeAllChildren);
ADD_API_METHOD_1(setValue);
ADD_API_METHOD_1(setValueWithUndo);
ADD_API_METHOD_0(changed);
ADD_API_METHOD_0(getValue);
ADD_TYPED_API_METHOD_1(setControlCallback, VarTypeChecker::Function);  // FORCED TYPE
ADD_API_METHOD_1(sendRepaintMessage);
ADD_API_METHOD_1(updateValueFromProcessorConnection);
ADD_API_METHOD_1(loseFocus);
ADD_API_METHOD_1(resetValueToDefault);
ADD_API_METHOD_1(setPaintRoutine);
ADD_TYPED_API_METHOD_1(setChildCallback, VarTypeChecker::Function);    // FORCED TYPE
ADD_API_METHOD_1(isEqual);
ADD_API_METHOD_0(getNumChildComponents);
ADD_API_METHOD_1(getChildComponentIndex);
ADD_API_METHOD_1(toBase64);
ADD_API_METHOD_1(fromBase64);
ADD_API_METHOD_1(addStateToUserPreset);
```

**Two forced-type methods on ChildReference:**
- `setControlCallback` -- param 1: Function
- `setChildCallback` -- param 1: Function

**Two diagnostic callbacks registered:**
- `ADD_CALLBACK_DIAGNOSTIC(valueCallback, setControlCallback, 0)`
- `ADD_CALLBACK_DIAGNOSTIC(childCallback, setChildCallback, 0)`

### ChildReference Validity Pattern

ChildReferences can become invalid when:
1. The parent container calls `setData()` again (invalidates all previous refs)
2. The component is removed via `removeFromParent()`
3. The parent container is destroyed

```cpp
bool ChildReference::isValid() const
{
    if(invalid || parentContainer.get() == nullptr)
        return false;

    auto dt = data->getValueTree(dyncomp::Data::TreeType::Data);
    auto valid = valuetree::Helpers::isParent(componentData, dt);

    if(!valid)
    {
        data->refreshBroadcaster.removeListener(*const_cast<ChildReference*>(this));
        invalid = true;
    }

    return valid;
}
```

Most ChildReference methods call `isValidOrThrow()` which reports a script error if invalid.

### AssignableDotObject Pattern

ChildReference implements `assign` and `getDotProperty` for dot-access:

```cpp
bool assign(const Identifier& id, const var& newValue)
{
    if(dyncomp::dcid::Helpers::isValidProperty(id))
    {
        componentData.setProperty(id, newValue, um);
        return true;
    }
    return false;
}

var getDotProperty(const Identifier& id) const
{ return componentData[id]; }
```

This means you can write `ref.text = "Hello"` and `var t = ref.text` directly.

---

## dyncomp::Data -- The Data Model

The `dyncomp::Data` class is the core data model that ScriptDynamicContainer creates.

### Two Value Trees

```cpp
enum class TreeType
{
    Data,    // Component hierarchy and properties
    Values   // Component values (keyed by component ID)
};
```

- **Data tree:** Hierarchical ValueTree mirroring the component structure. Each node has properties from `dyncomp::dcid` (id, type, text, x, y, width, height, etc.)
- **Values tree:** Flat ValueTree where each property is `{componentId: value}`. This is what the value callback system listens to.

### Data Construction from JSON

`Data::fromJSON()` converts JSON to ValueTree:
1. Accepts either `{ ContentProperties: [...], FloatingTileData: {...} }` or a direct array
2. Converts `ValueTreeConverters::convertDynamicObjectToContentProperties()` 
3. Sets position from the container's bounds
4. Converts legacy type names: `ScriptButton` -> `Button`, `ScriptSlider` -> `Slider`, `ScriptComboBox` -> `ComboBox`, `ScriptLabel` -> `Label`, `ScriptPanel` -> `Panel`, `ScriptViewport` -> `Viewport`
5. Sets default width=128, height=50 if missing
6. Sets `parentComponent` property from parent's `id`

### Data Sub-handlers

Data creates `SubDataHandler` objects for special component types:

- **DragContainerHandler** -- Manages FX chain drag-and-drop reordering. On value change, switches FX processing order. On child add/remove, calls `HotswappableProcessor::setEffect()`.
- **ComplexDataHandler** -- Connects to SliderPack, Table, or AudioFile external data. Synchronizes base64 state between the data model and the complex data objects.

### RefreshType Enum

```cpp
enum class RefreshType
{
    repaint,
    changed,
    updateValueFromProcessorConnection,
    loseFocus,
    resetValueToDefault,
    numRefreshTypes
};
```

These are used by ChildReference's `sendMessage()` to broadcast refresh commands to the UI component tree. ChildReference methods like `sendRepaintMessage(bool recursive)`, `loseFocus(bool recursive)`, etc. all delegate to this broadcast mechanism.

---

## dyncomp Component Type Factory

The `dyncomp::Factory` registers these component types:

| Type ID | C++ Class | Description |
|---------|-----------|-------------|
| `"Button"` | `dyncomp::Button` | Toggle/momentary button |
| `"Slider"` | `dyncomp::Slider` | Knob/slider with mode support |
| `"ComboBox"` | `dyncomp::ComboBox` | Dropdown selector |
| `"Panel"` | `dyncomp::Panel` | Custom drawable panel |
| `"Label"` | `dyncomp::Label` | Text input/display |
| `"FloatingTile"` | `dyncomp::FloatingTile` | Embedded floating tile |
| `"DragContainer"` | `dyncomp::DragContainer` | Drag-reorderable container |
| `"Viewport"` | `dyncomp::FlexBox` | Flexbox layout container |
| `"TextBox"` | `dyncomp::TextBox` | Multi-line text display |
| `"TableEditor"` | `ComplexDataEditor<TableEditor>` | Table curve editor |
| `"SliderPack"` | `ComplexDataEditor<SliderPack>` | Multi-slider array |
| `"AudioFile"` | `ComplexDataEditor<MultiChannelAudioBufferDisplay>` | Audio waveform display |

Legacy type names (`ScriptButton`, `ScriptSlider`, etc.) are converted automatically by `fromJSON()`.

---

## dyncomp Property System (dcid namespace)

### All Valid Properties

From `DynamicComponentContainerIds.cpp::getProperties()`:

| Property | Default | Description |
|----------|---------|-------------|
| `id` | `""` | Component identifier |
| `index` | `0` | Component index |
| `type` | `""` | Component type (Button, Slider, etc.) |
| `text` | `""` | Display text |
| `enabled` | `true` | Enabled state |
| `visible` | `true` | Visibility |
| `tooltip` | `""` | Tooltip text |
| `class` | `""` | CSS class selector |
| `defaultValue` | `0.0` | Default value |
| `useUndoManager` | `false` | Enable undo support |
| `elementStyle` | `""` | Inline CSS style |
| `parentComponent` | `""` | Parent component ID (auto-set) |
| `x` | `0` | X position |
| `y` | `0` | Y position |
| `width` | `128` | Width |
| `height` | `50` | Height |
| `isMomentary` | `false` | Button: momentary mode |
| `radioGroupId` | `0` | Button: radio group |
| `setValueOnClick` | `false` | Button: set value on click |
| `useCustomPopup` | `false` | ComboBox: custom popup |
| `items` | `""` | ComboBox: item list |
| `editable` | `true` | Label: editable |
| `multiline` | `false` | Label: multiline |
| `updateEachKey` | `false` | Label: update on each keystroke |
| `popupMenuAlign` | `false` | ComboBox: popup alignment |
| `min` | `0.0` | Slider: minimum value |
| `max` | `1.0` | Slider: maximum value |
| `middlePosition` | `-10` | Slider: middle position (skew) |
| `stepSize` | `0.01` | Slider: step size |
| `mode` | `""` | Slider: mode (Linear, Frequency, etc.) |
| `suffix` | `""` | Slider: value suffix |
| `style` | `"Knob"` | Slider: visual style |
| `showValuePopup` | `false` | Slider: show value popup |
| `processorId` | `""` | Processor connection ID |
| `parameterId` | `""` | Parameter connection ID |
| `filmstripImage` | `""` | Filmstrip image reference |
| `numStrips` | `64` | Filmstrip: number of frames |
| `isVertical` | `true` | Filmstrip/layout: vertical orientation |
| `scaleFactor` | `1.0` | Filmstrip: scale factor |
| `animationSpeed` | `(void)` | Animation speed |
| `dragMargin` | `(void)` | DragContainer: drag margin |
| `bgColour` | `0x80000000` | Background colour |
| `itemColour` | `0x33FFFFFF` | Item colour |
| `itemColour2` | `0x33FFFFFF` | Item colour 2 |
| `textColour` | `0xCCFFFFFF` | Text colour |

### CSS Support Properties

CSS-relevant properties (used for stylesheet integration): `id`, `type`, `text`, `bgColour`, `itemColour`, `itemColour2`, `textColour`, `x`, `y`, `width`, `height`.

### Basic Properties

Properties updated via `updateBasicProperties`: `enabled`, `visible`, `class`, `elementStyle`, `bgColour`, `itemColour`, `itemColour2`, `textColour`.

---

## ChildReference Key Method Implementations

### addChildComponent

```cpp
var ChildReference::addChildComponent(const var& childData)
{
    // Accepts JSON with either bounds:[x,y,w,h] or separate x,y,width,height
    // Converts via Data::fromJSON
    // Adds as child of this component's ValueTree
    // Returns a new ChildReference for the added child
}
```

### removeFromParent

```cpp
void ChildReference::removeFromParent()
{
    // Uses PARENT's undo manager (not self)
    // Clears value callback
    // Removes all value properties for this component and children
    // Async removal of the ValueTree child via SafeAsyncCall
}
```

Note: Removal is async (deferred to message thread via `SafeAsyncCall`).

### setValue / getValue

```cpp
void ChildReference::setValue(var newValue)
{
    // Sets on Values tree, excluding listener (no self-notification)
    // Forces um=nullptr (use setValueWithUndo for undo)
    auto vt = data->getValueTree(dyncomp::Data::TreeType::Values);
    vt.setPropertyExcludingListener(&valueListener, id, newValue, nullptr);
}

var ChildReference::getValue() const
{
    // Returns from Values tree if present, else defaultValue from component data
    auto vt = data->getValueTree(dyncomp::Data::TreeType::Values);
    if(vt.hasProperty(id))
        return vt[id];
    return componentData[dyncomp::dcid::defaultValue];
}
```

### setControlCallback

The ChildReference's `setControlCallback` is distinct from `ScriptComponent.setControlCallback`:
- Takes a function with 1 parameter (just the value, not component+value)
- Sets `this` as the callback's `this` object
- Listens specifically to this component's ID in the Values tree
- Synchronous mode, high priority

### setPaintRoutine

```cpp
void ChildReference::setPaintRoutine(var newPaintRoutine)
{
    // Creates a GraphicsObject for this specific component
    // Executes paint on JavascriptThreadPool (LowPriorityCallbackExecution)
    // Calls graphics->getDrawHandler().flush(0, 0) after painting
}
```

### setChildCallback

```cpp
void ChildReference::setChildCallback(const var& newChildCallback)
{
    // Callback signature: function(childId, wasAdded)
    // Listens to ValueTree child add/remove events
    // Synchronous mode
}
```

### toBase64 / fromBase64

Serializes/deserializes the component tree and optionally values using zstd compression:
- `toBase64(includeValue)` -- compresses component data + optional values
- `fromBase64(b64)` -- restores via async SafeAsyncCall (removes all children, copies properties)

### addStateToUserPreset

Registers/unregisters this ChildReference with the `UserPresetHandler` as a state manager. When registered:
- `exportAsValueTree()` serializes the component tree + values via `toBase64(true)`
- `restoreFromValueTree()` restores from the base64 state
- `resetUserPresetState()` removes all children and resets to defaultValue

---

## UI Integration

### DynamicComponentWrapper

The wrapper creates a `WrapperComponent` that listens to `dataBroadcaster`:

```cpp
DynamicComponentWrapper(ScriptContentComponent* content, ScriptDynamicContainer* container, int index):
    ScriptCreatedComponentWrapper(content, index)
{
    auto wc = new WrapperComponent();
    container->dataBroadcaster.addListener(*wc, WrapperComponent::onChange);
    component = wc;
    initAllProperties();
}
```

When `setData()` fires, `WrapperComponent::onChange` creates a `dyncomp::Root` component from the data and adds it as a visible child. The Root then recursively builds the component tree using `Data::create()` and the Factory.

### sendRepaintMessage Override

ScriptDynamicContainer is listed as overriding `sendRepaintMessage` in the base class virtual table. This override sends refresh messages through the dyncomp data model's refresh broadcaster rather than the standard repaint mechanism.

---

## getOrCreateChildReference Pattern

```cpp
var ScriptDynamicContainer::getOrCreateChildReference(const ValueTree& v)
{
    // Clean up invalid references
    for(int i = 0; i < childReferences.size(); i++)
        if(!childReferences[i]->isValid())
            childReferences.remove(i--);

    // Return existing reference if found
    for(auto ref: childReferences)
        if(ref->matchesValueTree(v))
            return var(ref);

    // Create new reference
    auto newRef = new ChildReference(this, data, v);
    childReferences.add(newRef);
    return var(newRef);
}
```

This ensures identity stability -- calling `getComponent("id")` twice returns the same ChildReference object.

---

## Threading Model

- `setData()` -- Can be called from any script thread. UI rebuild is async via `dataBroadcaster`.
- `setValueCallback()` -- Synchronous callback on the value tree listener thread.
- ChildReference `setValue()` -- Directly modifies ValueTree (not thread-safe for audio thread).
- ChildReference `removeFromParent()` / `removeAllChildren()` -- Async via `SafeAsyncCall`.
- ChildReference `setPaintRoutine()` -- Paint execution runs on `JavascriptThreadPool` (LowPriorityCallbackExecution).
- ChildReference `fromBase64()` -- Async restoration via `SafeAsyncCall`.

---

## Popup Menu Integration

```cpp
var getPopupMenuTarget(const MouseEvent& e) override
{
    if(auto b = dyncomp::Base::findBaseParent(e.eventComponent))
        return getOrCreateChildReference(b->getDataTree());
    return var(this);
}
```

This allows the dynamic container to provide context-aware popup menus -- right-clicking a child component returns a ChildReference for that specific child.

---

## Key Architectural Summary

1. **ScriptDynamicContainer** is a thin wrapper that creates a `dyncomp::Data` model from JSON and exposes it via `ContainerChild` references.
2. **dyncomp::Data** owns two ValueTrees (Data for hierarchy, Values for component values) and a Factory for creating UI components.
3. **ChildReference (ContainerChild)** is the primary scripting handle -- it provides 28 methods for property access, hierarchy traversal, value management, paint routines, serialization, and user preset integration.
4. The container itself has only 2 own methods (`setData`, `setValueCallback`) plus the inherited ScriptComponent base methods.
5. The property system is completely separate from ScriptComponent's property system -- it uses `dyncomp::dcid` identifiers validated by `Helpers::isValidProperty()`.
6. CSS support is integrated at the dyncomp level -- `class`, `elementStyle`, and colour properties feed into the simple_css renderer.
