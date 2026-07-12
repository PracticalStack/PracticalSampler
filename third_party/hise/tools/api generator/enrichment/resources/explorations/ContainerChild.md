# ContainerChild -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- prerequisite: DspNetwork (line 7), but ContainerChild is NOT listed as needing DspNetwork. The actual prerequisite relationship is ScriptDynamicContainer (line not explicit, but ContainerChild is a nested class of ScriptDynamicContainer).
- `enrichment/resources/survey/class_survey_data.json` -- ContainerChild entry (lines 440-469)
- `enrichment/phase1/ScriptDynamicContainer/Readme.md` -- prerequisite class distilled output
- `HISE/hi_scripting/scripting/api/ScriptingApiContent.h` lines 2396-2536 -- ChildReference class declaration
- `HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp` lines 6171-6832 -- full implementation
- `HISE/hi_scripting/scripting/api/DynamicComponentContainerIds.h` -- dyncomp::dcid namespace (property identifiers and helpers)
- `HISE/hi_scripting/scripting/api/DynamicComponentContainerIds.cpp` -- property defaults and validation
- `HISE/hi_scripting/scripting/api/DynamicComponentContainer.h` -- dyncomp::Data, dyncomp::Base infrastructure
- `HISE/hi_scripting/scripting/api/DynamicComponentContainer.cpp` -- Factory, DragContainerHandler

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiContent.h` lines 2396-2536

```cpp
struct ChildReference: public ConstScriptingObject,
                       public AssignableDotObject,
                       public ObjectWithJSONConverter,
                       public UserPresetStateManager
```

ContainerChild is the scripting-facing name (`getObjectName()` returns `"ContainerChild"`), but the C++ class is `ScriptingApi::Content::ScriptDynamicContainer::ChildReference`. It is a nested struct inside ScriptDynamicContainer.

### Inheritance Chain

1. **ConstScriptingObject** -- base for all scripting API objects. Provides `ADD_API_METHOD_N` registration, `reportScriptError()`, `getScriptProcessor()`
2. **AssignableDotObject** -- enables dot-assignment syntax: `ref.text = "Hello"` maps to `assign()`, `ref.text` maps to `getDotProperty()`
3. **ObjectWithJSONConverter** -- enables `JSON.stringify(ref)` via `writeAsJSON()` / `writeToStream()`
4. **UserPresetStateManager** -- enables participation in the user preset system via `exportAsValueTree()` / `restoreFromValueTree()` / `resetUserPresetState()`

### Key Members

```cpp
WeakReference<ScriptDynamicContainer> parentContainer;  // back-pointer to owning container
UndoManager* um = nullptr;                              // set if useUndoManager=true in data
mutable bool invalid = false;                           // invalidation flag
var lastValue;                                          // dedup for value callbacks
WeakCallbackHolder valueCallback;                       // user-set control callback (1 arg)
WeakCallbackHolder paintRoutine;                        // user-set paint callback (1 arg: Graphics)
ReferenceCountedObjectPtr<ScriptingObjects::GraphicsObject> graphics; // for paint routine
valuetree::PropertyListener valueListener;              // listens to Values tree
ValueTree componentData;                                // this component's node in the Data tree
dyncomp::Data::Ptr data;                                // shared data model (ref-counted)
WeakCallbackHolder childCallback;                       // user-set child add/remove callback (2 args)
valuetree::ChildListener childListener;                 // listens to child add/remove on componentData
```

## Constructor Analysis

**File:** `ScriptingApiContent.cpp` lines 6204-6248

```cpp
ChildReference(ScriptDynamicContainer* parent, dyncomp::Data::Ptr data_, const ValueTree& cd)
```

**No constants registered** -- no `addConstant()` calls. The constructor passes `0` as numConstants to `ConstScriptingObject`.

### Method Registration

All methods use `ADD_API_METHOD_N` (untyped) except two:

```cpp
ADD_TYPED_API_METHOD_1(setControlCallback, VarTypeChecker::Function);
ADD_TYPED_API_METHOD_1(setChildCallback, VarTypeChecker::Function);
```

Both force their parameter to be `Function` type.

**Diagnostic registrations:**
```cpp
ADD_CALLBACK_DIAGNOSTIC(valueCallback, setControlCallback, 0);
ADD_CALLBACK_DIAGNOSTIC(childCallback, setChildCallback, 0);
```
These register compile-time diagnostics for the callback parameters.

### UndoManager Setup

```cpp
um(cd[dyncomp::dcid::useUndoManager] ? getScriptProcessor()->getMainController_()->getControlUndoManager() : nullptr)
```

If the component data has `useUndoManager=true`, the ChildReference gets an UndoManager for property changes, child add/remove, and value operations (except `setValue()` which forces `nullptr`; `setValueWithUndo()` always uses the undo manager).

### Refresh Broadcaster

```cpp
data->refreshBroadcaster.addListener(*this, onRefresh, false);
```

The ChildReference listens to the shared `refreshBroadcaster` on the Data model. This is how `sendRepaintMessage`, `changed`, `loseFocus`, `resetValueToDefault`, and `updateValueFromProcessorConnection` propagate to the visual component layer.

## Factory / obtainedVia

ContainerChild instances are NOT created directly by script code. They are created internally by `ScriptDynamicContainer::getOrCreateChildReference()`:

```cpp
var ScriptDynamicContainer::getOrCreateChildReference(const ValueTree& v)
{
    // cleanup invalid refs
    for(int i = 0; i < childReferences.size(); i++)
        if(!childReferences[i]->isValid())
            childReferences.remove(i--);

    // return existing if found
    for(auto ref: childReferences)
        if(ref->matchesValueTree(v))
            return var(ref);

    // create new
    auto newRef = new ChildReference(this, data, v);
    childReferences.add(newRef);
    return var(newRef);
}
```

**How scripts obtain a ContainerChild:**
1. `ScriptDynamicContainer.setData(json)` -- returns the root ChildReference
2. `ChildReference.getComponent(id)` -- returns a child by ID (recursive search)
3. `ChildReference.getAllComponents(regex)` -- returns array of matching children
4. `ChildReference.getParent()` -- returns parent reference
5. `ChildReference.addChildComponent(json)` -- creates and returns new child

All these go through `getOrCreateChildReference()`, which deduplicates -- the same ValueTree node always returns the same ChildReference object.

## Data Model Integration (dyncomp::Data)

ContainerChild operates on the two-ValueTree model from `dyncomp::Data`:

### Data Tree (TreeType::Data)
Hierarchical component structure. Each node is a `Component` ValueTree with properties from `dyncomp::dcid`. The ChildReference's `componentData` member points to one node in this tree.

### Values Tree (TreeType::Values)
Flat key-value store. Each component's runtime value is stored by its `id` property. `getValue()` reads from here; `setValue()` writes here.

### Value Flow

```
setValue(x)
  -> data->getValueTree(Values).setPropertyExcludingListener(&valueListener, id, x, nullptr)
  -> (no callback fires -- valueListener is excluded)

changed()
  -> valueListener.sendMessageForAllProperties()
  -> sendMessage(RefreshType::changed)
  -> (triggers onValue callback AND visual refresh)

setControlCallback(fn)
  -> creates valueCallback WeakCallbackHolder
  -> sets up valueListener on Values tree for this component's id
  -> listener calls onValue() which calls valueCallback
```

The `setPropertyExcludingListener` pattern means `setValue()` does NOT trigger the control callback. Calling `changed()` afterward does.

## Property System (dyncomp::dcid)

### Valid Properties

From `DynamicComponentContainerIds.cpp` Helpers::getProperties():

| Property | Default | Notes |
|----------|---------|-------|
| id | "" | Component identifier |
| index | 0 | Used by DragContainer |
| type | "" | Component type string |
| text | "" | Display text |
| enabled | true | |
| visible | true | |
| tooltip | "" | |
| class | "" | CSS class |
| defaultValue | 0.0 | |
| useUndoManager | false | Enables undo for property/value changes |
| elementStyle | "" | Inline CSS |
| parentComponent | "" | Reparenting target |
| x | 0 | Position |
| y | 0 | Position |
| width | 128 | Size |
| height | 50 | Size |
| isMomentary | false | Button: momentary mode |
| radioGroupId | 0 | Button: radio group |
| setValueOnClick | false | Button: set value on click |
| useCustomPopup | false | ComboBox: custom popup |
| items | "" | ComboBox: item list |
| editable | true | Label: editable |
| multiline | false | Label: multiline |
| updateEachKey | false | Label: update per keystroke |
| popupMenuAlign | false | ComboBox: popup alignment |
| min | 0.0 | Slider: minimum |
| max | 1.0 | Slider: maximum |
| middlePosition | -10 | Slider: mid-point (-10 = off) |
| stepSize | 0.01 | Slider: step size |
| mode | "" | Slider: mode string |
| suffix | "" | Slider: value suffix |
| style | "Knob" | Slider: display style |
| showValuePopup | false | Slider: show popup on drag |
| processorId | "" | Processor connection |
| parameterId | "" | Parameter connection |
| filmstripImage | "" | Image reference |
| numStrips | 64 | Filmstrip frame count |
| isVertical | true | Filmstrip/DragContainer orientation |
| scaleFactor | 1.0 | Filmstrip scale |
| animationSpeed | var() | Animation timing |
| dragMargin | var() | Drag margin |
| bgColour | 0x80000000 | Background colour |
| itemColour | 0x33FFFFFF | Item colour |
| itemColour2 | 0x33FFFFFF | Item colour 2 |
| textColour | 0xCCFFFFFF | Text colour |

### Property Validation

`set()` and `get()` both validate via `dyncomp::dcid::Helpers::isValidProperty()`, which checks against the static property list. Invalid properties throw a script error.

### Dot Assignment (AssignableDotObject)

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

Dot assignment respects the UndoManager. Dot read returns raw ValueTree properties (no default fallback, unlike `get()` which falls back to defaults).

## Validity System

### Invalidation Sources

1. **Parent setData() call** -- `ScriptDynamicContainer::setData()` calls `setInvalid(nullptr)` on ALL existing ChildReferences before clearing them
2. **removeFromParent()** -- calls `setInvalid()` on self (indirectly via SafeAsyncCall)
3. **Parent container destroyed** -- WeakReference to parentContainer becomes null
4. **Data tree divergence** -- `isValid()` checks `valuetree::Helpers::isParent(componentData, dataTree)`, which fails if the node was removed from the tree

### isValid() Implementation

```cpp
bool isValid() const
{
    if(invalid || parentContainer.get() == nullptr)
        return false;

    auto dt = data->getValueTree(Data::TreeType::Data);
    auto valid = valuetree::Helpers::isParent(componentData, dt);

    if(!valid)
    {
        data->refreshBroadcaster.removeListener(*const_cast<ChildReference*>(this));
        invalid = true;
    }

    return valid;
}
```

Once invalid, the reference auto-disconnects from the refresh broadcaster and sets the flag permanently. All mutating methods call `isValidOrThrow()` which throws a script error if invalid.

### setInvalid()

```cpp
void setInvalid(UndoManager* umToUse)
{
    invalid = true;
    valueCallback.clear();
    childCallback.clear();
    paintRoutine.clear();
    valueListener.shutdown();
    childListener.shutdown();
    // also removes value from Values tree
    Identifier idToRemove(componentData[dyncomp::dcid::id].toString());
    auto vt = data->getValueTree(Data::TreeType::Values);
    vt.removeProperty(idToRemove, umToUse);
    lastValue = var();
}
```

## Callback System

### Value Callback (setControlCallback)

- Registered via `setControlCallback(fn)` where fn takes 1 argument (the new value)
- Uses `WeakCallbackHolder` with `setThisObject(this)` -- `this` inside callback is the ChildReference
- `setHighPriority()` is called -- bypasses normal callback queue
- Listens synchronously to the component's specific property in the Values tree
- `onValue()` deduplicates: only fires if `newValue != lastValue` and is not void

### Paint Routine (setPaintRoutine)

- Registered via `setPaintRoutine(fn)` where fn takes 1 argument (Graphics object)
- Creates a `GraphicsObject` via `data->createGraphicsObject(componentData, this)`
- Paint is executed via `onRefresh()` when `RefreshType::repaint` is received
- Runs on the JavaScript thread pool as a `LowPriorityCallbackExecution` task
- After paint, calls `graphics->getDrawHandler().flush(0, 0)` to commit draw actions

### Child Callback (setChildCallback)

- Registered via `setChildCallback(fn)` where fn takes 2 arguments (id, wasAdded)
- Listens synchronously to child add/remove on the componentData ValueTree
- `onChildChange(v, wasAdded)` passes the child's `id` property and the boolean flag

## Hierarchy Traversal

### getComponent(childId) -- Recursive Search

Uses `valuetree::Helpers::forEach` to recursively search all descendants. Returns the first match. Returns `var()` if not found.

### getAllComponents(regex) -- Wildcard Match

Uses `RegexFunctions::matchesWildcard(regex, id)` for matching. Returns an array of all matching ChildReferences across the entire subtree.

### getParent()

Returns `parentContainer->getOrCreateChildReference(componentData.getParent())`. Since the root data tree is also a ValueTree, this can return a reference to the root node.

### getChildComponentIndex(childIdOrComponent)

Accepts either a String (id lookup) or a ChildReference object (ValueTree identity match). Searches only direct children of this component. Returns -1 if not found.

## Dynamic Component Manipulation

### addChildComponent(childData)

```cpp
var addChildComponent(const var& childData)
{
    // Supports both {bounds: [x,y,w,h]} and {x:, y:, width:, height:}
    Rectangle<int> b;
    if(childData.hasProperty(dcid::bounds))
        b = ApiHelpers::getRectangleFromVar(childData[dcid::bounds]).toNearestInt();
    else
        b = Rectangle<int>(x, y, width_or_128, height_or_50);

    auto v = dyncomp::Data::fromJSON(childData, b);
    componentData.addChild(v, -1, um);

    return parentContainer->getOrCreateChildReference(v);
}
```

Converts JSON to ValueTree, adds as last child, returns new ChildReference. Respects undo manager.

### removeFromParent()

Uses `SafeAsyncCall` to defer the actual ValueTree removal. Before removing:
1. Clears the value callback
2. Recursively removes all value properties from the Values tree
3. Uses the PARENT's undo manager (not this component's), determined by checking parent's `useUndoManager`

### removeAllChildren()

Also deferred via `SafeAsyncCall`. Removes all children from componentData using this component's undo manager.

## Serialization (Base64)

### toBase64(includeValues)

1. Creates a new ValueTree named after the component's id
2. Adds a deep copy of componentData as child
3. If includeValues=true, recursively collects all descendant values from the Values tree into a "Values" child
4. Compresses with zstd (`ZDefaultCompressor`)
5. Returns base64 string

### fromBase64(b64)

1. Decompresses from base64/zstd
2. Extracts "Component" child -> copies all properties and children into componentData
3. Extracts "Values" child -> copies all properties into the Values tree
4. Deferred via `SafeAsyncCall`
5. Uses undo manager for all operations

## User Preset Integration

### addStateToUserPreset(shouldAdd)

Registers/unregisters this ChildReference with `UserPresetHandler` as a `UserPresetStateManager`.

### exportAsValueTree()

Exports using `toBase64(true)` -- includes values. Wraps in a ValueTree with the component's id and a "state" property.

### restoreFromValueTree(v)

Calls `fromBase64()` with the "state" property.

### resetUserPresetState()

Removes all children, then if defaultValue is set, calls `setValue(defaultValue)` and `changed()`.

## JSON Serialization (ObjectWithJSONConverter)

`writeAsJSON()` and `writeToStream()` both convert componentData to a DynamicObject via `ValueTreeConverters::convertContentPropertiesToDynamicObject()`. This means `JSON.stringify(childRef)` produces the JSON representation of the component's data properties.

## RefreshType Enumeration

From `DynamicComponentContainer.h`:

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

These map directly to the `sendRepaintMessage()`, `changed()`, `updateValueFromProcessorConnection()`, `loseFocus()`, and `resetValueToDefault()` methods. The `sendMessage()` helper broadcasts to the refresh broadcaster with the appropriate type and recursive flag.

## Threading Model

- **Value callbacks** are synchronous (`valuetree::AsyncMode::Synchronously`)
- **Child callbacks** are synchronous
- **Paint routines** are dispatched to the JavaScript thread pool as `LowPriorityCallbackExecution`
- **removeFromParent()** and **removeAllChildren()** use `SafeAsyncCall` to defer execution
- **fromBase64()** also uses `SafeAsyncCall` for deferred restoration
- No audio-thread interaction -- this is purely a UI/scripting layer

## Preprocessor Guards

None. ContainerChild has no conditional compilation guards.
