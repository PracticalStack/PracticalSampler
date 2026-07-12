# Content -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` -- Content entry (createdBy: [], creates: 18 classes, seeAlso: ScriptPanel/Engine)
- `enrichment/base/Content.json` -- 32 methods (note: base JSON lists 32, but 2 listed methods `setUpdateExistingPosition` and `storeAllControlsAsPreset` do not exist in C++ source as public script API. See Missing Methods section.)
- No prerequisite class -- Content is the root UI factory

## Class Declaration

File: `HISE/hi_scripting/scripting/api/ScriptingApiContent.h`, line 149

```cpp
class ScriptingApi::Content : public ScriptingObject,
    public DynamicObject,
    public RestorableObject,
    public ValueTreeUpdateWatcher::Listener
```

### Inheritance Chain
- **ScriptingObject** -- base class providing `getScriptProcessor()`, `getProcessor()`, `reportScriptError()` etc.
- **DynamicObject** -- JUCE's dynamic property object. Content IS a DynamicObject, meaning its methods are registered via `setMethod()` (NOT the ADD_API_METHOD_N pattern used by ConstScriptingObject classes). This is a critical architectural distinction.
- **RestorableObject** -- provides `exportAsValueTree()` / `restoreFromValueTree()` for preset persistence
- **ValueTreeUpdateWatcher::Listener** -- receives `valueTreeNeedsUpdate()` when the underlying `contentPropertyData` ValueTree changes

### Key Architectural Distinction: DynamicObject-based Registration
Unlike most HISE API classes (which inherit `ConstScriptingObject` and use `ADD_API_METHOD_N`/`ADD_TYPED_API_METHOD_N` macros), Content inherits `DynamicObject` and registers methods via `setMethod("name", Wrapper::name)`. This means:
- There are NO `addConstant()` calls (no constants on this class)
- There are NO `ADD_TYPED_API_METHOD_N` calls (no forced parameter types)
- All methods use NativeFunctionArgs wrappers that manually extract and cast arguments

## Constructor

File: `ScriptingApiContent.cpp`, line 7752

```cpp
ScriptingApi::Content::Content(ProcessorWithScriptingContent *p) :
    ScriptingObject(p),
    asyncRebuildBroadcaster(*this),
    updateDispatcher(p->getMainController_()),
    height(50),
    width(600),
    name(String()),
    allowGuiCreation(true),
    dragCallback(p, nullptr, var(), 1),
    suspendCallback(p, nullptr, var(), 1),
    colour(Colour(0xff777777))
```

### Method Registrations (all via setMethod)
The constructor registers ALL methods. These are the complete registrations:

**Component factory methods (14):**
- addButton, addKnob, addLabel, addComboBox, addTable, addImage, addViewport
- addPanel, addAudioWaveform, addSliderPack, addFloatingTile, addMultipageDialog
- addWebView, addDynamicContainer

**Interface configuration (6):**
- setContentTooltip, setToolbarProperties, setHeight, setWidth, setName
- makeFrontInterface, makeFullScreenInterface

**Component retrieval (3):**
- getComponent, getAllComponents, componentExists

**Object factories (5):**
- createPath, createShader, createMarkdownRenderer, createSVG, createLocalLookAndFeel

**State management (3):**
- storeAllControlsAsPreset, restoreAllControlsFromPreset, setValuePopupData

**Component property helpers (2):**
- setPropertiesFromJSON, clear

**Rendering/display (3):**
- setUseHighResolutionForPanels, createScreenshot, addVisualGuide

**Input/interaction (6):**
- isCtrlDown, isMouseDown, getComponentUnderMouse, getComponentUnderDrag
- refreshDragImage, callAfterDelay

**Callbacks (3):**
- setSuspendTimerCallback, setKeyPressCallback, showModalTextInput

**Info getters (3):**
- getInterfaceSize, getScreenBounds, getCurrentTooltip

### No Constants
Content has NO `addConstant()` calls. No constants are registered on this class.

### Initial State
- Default width: 600, height: 50
- Default colour: 0xff777777
- `allowGuiCreation` starts true, set false by `endInitialization()`
- `allowAsyncFunctions` starts false, set true by `endInitialization()`

## Initialization Lifecycle

The Content instance has a strict lifecycle managed by the script compilation process:

1. **`beginInitialization()`** -- Called before onInit runs. Sets `allowGuiCreation = true`, clears updateWatcher, clears guides and registered key presses, resets profiling.

2. **onInit callback executes** -- During this phase, `addXXX()` calls are allowed. Components are created via `addComponent<T>(name, x, y)`.

3. **`endInitialization()`** -- Called after onInit completes. Sets `allowGuiCreation = false` (blocking further addXXX calls), sets `allowAsyncFunctions = true`, creates the `ValueTreeUpdateWatcher`.

This means:
- Component creation (addButton, addKnob, etc.) is ONLY allowed during onInit
- Calling addXXX after onInit produces: `"Tried to add a component after onInit()"`
- Certain async methods only work after initialization is complete

## Component Factory Pattern (addComponent<T>)

File: `ScriptingApiContent.h`, line 3510

```cpp
template<class Subtype> Subtype* addComponent(Identifier name, int x, int y)
{
    if (!allowGuiCreation)
    {
        reportScriptError("Tried to add a component after onInit()");
        return nullptr;
    }

    if (auto sc = getComponentWithName(name))
    {
        // If component already exists, optionally update position and return existing
        if (x != -1 && y != -1)
        {
            sc->handleScriptPropertyChange("x");
            sc->handleScriptPropertyChange("y");
            sc->setScriptObjectProperty(ScriptComponent::Properties::x, x);
            sc->setScriptObjectProperty(ScriptComponent::Properties::y, y);
        }
        return dynamic_cast<Subtype*>(sc);
    }

    // Create new ValueTree child and ScriptComponent
    ValueTree newChild("Component");
    newChild.setProperty("type", Subtype::getStaticObjectName().toString(), nullptr);
    newChild.setProperty("id", name.toString(), nullptr);
    newChild.setProperty("x", jmax(0, x), nullptr);
    newChild.setProperty("y", jmax(0, y), nullptr);
    contentPropertyData.addChild(newChild, -1, nullptr);

    Subtype *t = new Subtype(getScriptProcessor(), this, name, x, y, 0, 0);
    components.add(t);
    updateParameterSlots();
    restoreSavedValue(name);
    return t;
}
```

Key behaviors:
- If a component with the same name already exists, returns the existing one (optionally updating position)
- This is why calling addButton("MyButton", x, y) is idempotent during onInit
- x,y of -1 means "don't set position" (used when calling with just name)
- After creation, saved values are restored automatically

### Wrapper Argument Patterns for addXXX

All addXXX wrappers accept either 1 or 3 arguments:
```cpp
// 1 arg: just name (position = -1, -1 meaning "don't set")
Content.addButton("MyButton");
// 3 args: name, x, y
Content.addButton("MyButton", 10, 20);
```

## Inner Classes and Structures

### RebuildListener (line 160)
Observer pattern for content rebuild events. Provides:
- `contentWasRebuilt()` -- called when component list changes
- `contentRebuildStateChanged(bool)` -- called during rebuild
- `onDragAction(DragAction, ScriptComponent*, var&)` -- for drag operations
- `suspendStateChanged(bool)` -- when timers are suspended

DragAction enum: `Start, End, Repaint, Query, Drag`

### PluginParameterConnector (line 193)
Links ScriptComponents to DAW plugin parameters (`ScriptedControlAudioParameter`).

### ScriptComponent (line 211)
The massive base class for all UI components. This is NOT explored here -- see `ScriptComponent_base.md` for that exploration. Content contains ScriptComponent as an inner class.

### VisualGuide (line 2897)
```cpp
struct VisualGuide
{
    enum class Type { HorizontalLine, VerticalLine, Rectangle };
    Rectangle<float> area;
    Colour c;
    Type t;
};
```
Used by `addVisualGuide()`. Guides are stored in `Array<VisualGuide> guides`.

### TextInputDataBase (line 2911)
Abstract base for the modal text input system:
```cpp
struct TextInputDataBase: public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<TextInputDataBase>;
    TextInputDataBase(const String& parentId_);
    virtual void show(Component* parentComponent) = 0;
    bool done = false;
    String parentId;
};
```

The concrete implementation `TextInputData` (line 8824 in cpp) handles:
- TextEditor creation with configurable bounds, colours, fonts
- Alignment support via `prop["alignment"]`
- Callback with `(bool ok, String text)` arguments
- TextEditor listener pattern for return/escape/focus-lost

### LafRegistry (line 3357)
Backend-only registry for Look and Feel objects. Tracks:
- Which LAF objects exist and their variable names
- Which components each LAF is assigned to
- Render state tracking (whether script-based LAF components have rendered)
- Used by the REST API to wait for component rendering

### AsyncRebuildMessageBroadcaster (line 3472)
Handles async rebuild notifications. Calls `sendRebuildMessage()` on the message thread.

## Broadcasters

Content exposes two public `LambdaBroadcaster` members:

1. **`textInputBroadcaster`** -- `LambdaBroadcaster<TextInputDataBase::Ptr>` (line 2929)
   - Fired by `showModalTextInput()`
   - Listeners create and display the TextEditor

2. **`interfaceSizeBroadcaster`** -- `LambdaBroadcaster<int, int>` (line 2931)
   - Fired by `setHeight()`, `setWidth()`, `makeFrontInterface()`
   - Carries (width, height) pair

## ValueTree Data Model

The `contentPropertyData` ValueTree stores the persistent component hierarchy:
- Root type: "ContentProperties"
- Each child: type "Component" with properties: type, id, x, y, width, height, etc.
- Nested children represent parent-child component relationships
- The ValueTreeUpdateWatcher monitors changes and triggers rebuild

## Key Method Implementation Details

### getComponent(var name) -- line 8009
- Linear search through components array by name
- USE_BACKEND: special "throw at definition" mechanism for IDE navigation
- Returns `var()` (undefined) if not found, logs error with `logErrorAndContinue`

### getAllComponents(String regex) -- line 8034
- Optimized path for ".*" (get all) -- skips regex matching
- Uses `RegexFunctions::matchesWildcard()` for pattern matching
- Returns Array of component references

### createPath() -- line 8460
- Creates a new `ScriptingObjects::PathObject`
- Takes NO arguments in C++ (base JSON shows `data` param, but wrapper calls CHECK_ARGUMENTS(0))
- Note: The PathObject's loadFromData would be called separately

### createLocalLookAndFeel() -- line 8467
- Creates `ScriptingObjects::ScriptedLookAndFeel(getScriptProcessor(), false)` -- the `false` means "local" (not global)
- Registers debug info listener for LAF registry tracking (backend only)

### createShader(fileName) -- line 8747
- Creates `ScriptingObjects::ScriptShader`
- Registers as screenshot listener
- HISE_SUPPORT_GLSL_LINE_NUMBERS preprocessor: optional line number support
- If fileName is not empty, calls `setFragmentShader(fileName)`

### createScreenshot(area, directory, name) -- line 8764
- `area` can be either a ScriptComponent reference or a [x,y,w,h] array
- `directory` must be a ScriptFile object pointing to a directory
- Creates PNG file in the target directory
- Coordinates with screenshot listeners (e.g., shaders need to prepare first)
- Blocking operation: waits for all listeners to be ready

### addVisualGuide(guideData, colour) -- line 8973
- Array of 4 elements = Rectangle guide
- Array of 2 elements = Line guide:
  - [0, y] = horizontal line at y
  - [x, 0] = vertical line at x
- Passing non-array clears ALL guides
- Notifies screenshot listeners

### setToolbarProperties() -- line 8150
DEPRECATED -- always throws: `reportScriptError("2017...")`

### isCtrlDown() -- line 8160
Returns true if EITHER Ctrl OR Command is down (cross-platform handling).

### isMouseDown() -- line 9036
Returns 0 (not down), 1 (left button), 2 (right button).

### getComponentUnderMouse() -- line 9048
Returns the JUCE componentID of whatever JUCE Component is under the mouse. This is the JUCE component ID, not necessarily the HiseScript component name.

### callAfterDelay(ms, function, thisObject) -- line 9058
- Creates a WeakCallbackHolder
- Optional `thisObject` (3rd arg is optional in wrapper, defaults to var())
- Uses `Timer::callAfterDelay()` -- JUCE timer, not accurate for DSP
- Callback runs on message thread

### showModalTextInput(properties, callback) -- line 8966
Creates TextInputData and broadcasts via `textInputBroadcaster`.

Properties JSON schema:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| x | int | -- | X position of text editor |
| y | int | -- | Y position of text editor |
| width | int | -- | Width of text editor |
| height | int | -- | Height of text editor |
| text | String | "" | Initial text content |
| alignment | String | "centred" | Text alignment |
| bgColour | int/hex | 0x88000000 | Background colour |
| itemColour | int/hex | 0 | Outline colour |
| textColour | int/hex | 0xAAFFFFFF | Text colour |
| fontName | String | "" | Font name (empty = GLOBAL_BOLD_FONT) |
| fontStyle | String | "plain" | Font style |
| fontSize | float | 13.0 | Font size |
| parentComponent | String | "" | ID of parent component to attach to |

Callback signature: `function(bool ok, String text)`
- `ok` = true if Return pressed, false if Escape or focus lost

### setValuePopupData(jsonData) -- line 8692
Simply stores the JSON. Consumed by `ScriptCreatedComponentWrapper::ValuePopup::Properties`.

Value Popup JSON schema:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| fontName | String | "Default" | Font name |
| fontSize | float | 14.0 | Font size |
| borderSize | float | 2.0 | Border line thickness |
| borderRadius | float | 2.0 | Corner radius |
| margin | float | 3.0 | Content margin |
| bgColour | int/hex | 0xFFFFFFFF | Background colour |
| itemColour | int/hex | 0xaa222222 | Item colour |
| itemColour2 | int/hex | 0xaa222222 | Item colour 2 |
| textColour | int/hex | 0xFFFFFFFF | Text colour |

### setSuspendTimerCallback(suspendFunction) -- line 9127
- Only accepts actual JavaScript functions (validated via `HiseJavascriptEngine::isJavascriptFunction`)
- Stored as WeakCallbackHolder with 1 argument
- Called by `suspendPanelTimers(bool)` -- passes boolean `shouldBeSuspended`
- The suspend system also iterates all ScriptPanels and calls `suspendTimer()`

### setKeyPressCallback(keyPress, callback) -- line 9135
- `keyPress` is parsed via `ApiHelpers::getKeyPress()`:
  - Can be a string description (e.g., "ctrl + a") via `KeyPress::createFromDescription()`
  - Can be a JSON object with `keyCode`, `character`, `shift`, `cmd`/`ctrl`, `alt`
- If callback is a valid function, adds/replaces in `registeredKeyPresses` array
- If callback is NOT a function, removes the matching key press registration
- Key presses are handled in `Content::handleKeyPress()` which creates a keyboard callback object

### Keyboard Callback Object Schema
Created by `Content::createKeyboardCallbackObject(KeyPress)`:
| Property | Type | Description |
|----------|------|-------------|
| isFocusChange | bool | Always false for key presses |
| character | String | Printable character (empty if special key) |
| specialKey | bool | True if not a printable character |
| isWhitespace | bool | Character is whitespace |
| isLetter | bool | Character is a letter |
| isDigit | bool | Character is a digit |
| keyCode | int | Raw key code |
| description | String | Human-readable key description |
| shift | bool | Shift modifier active |
| cmd | bool | Command/Ctrl modifier active |
| alt | bool | Alt modifier active |

### makeFrontInterface(width, height) -- line 8131
- Sets width and height
- Broadcasts size change
- Calls `JavascriptMidiProcessor::addToFront(true)` -- registers this script as the front interface

### makeFullScreenInterface() -- line 8142
- Gets resolution from HiseDeviceSimulator
- Calls addToFront(true)
- Note: description says "mobile devices" but implementation uses device simulator resolution

### setHeight/setWidth -- lines 8099/8110
- Only broadcasts if the value actually changed AND the other dimension is non-zero
- This prevents spurious resize events during initial setup

### getScreenBounds(getTotalArea) -- line 7987
- Uses `MessageManagerLock` for thread safety
- `getTotalArea=true`: returns total display area
- `getTotalArea=false`: returns user area (excluding taskbar etc.)
- Returns [x, y, width, height] array

### restoreAllControlsFromPreset(fileName) -- line 8244
- Different implementation for USE_FRONTEND vs backend
- Frontend: reads from pre-embedded ValueTree via ProjectHandler
- Backend: reads from XML file on disk
- Supports absolute paths or UserPresets-relative paths

### storeAllControlsAsPreset(fileName, automationData)
- This is registered as a setMethod but takes TWO arguments including automation data
- Exports current ValueTree, merges with existing file if present
- Internal API only -- not directly callable from script in the standard base JSON form

## Missing Methods from Base JSON

Two methods in the base JSON are NOT found in C++ source:

1. **`setUpdateExistingPosition`** -- No implementation found anywhere in the codebase. The behavior it describes (updating position at addXXX calls) is already handled internally by the `addComponent<T>` template when a component already exists.

2. Note: `storeAllControlsAsPreset` exists in C++ but is registered with the wrapper pattern. It takes `(String, ValueTree)` but the base JSON may not include it as a public API method.

## Preprocessor Guards

- **`USE_FRONTEND`**: In the constructor, `updateDispatcher.suspendUpdates(true)` is called. In `restoreAllControlsFromPreset`, uses embedded ValueTree instead of file I/O.
- **`USE_BACKEND`**: `getComponent()` has "throw at definition" debugging. `valueTreeNeedsUpdate()` triggers rebuild. LafRegistry only created in backend.
- **`HISE_SUPPORT_GLSL_LINE_NUMBERS`**: Optional shader debug feature for createShader.
- **`HISE_MACROS_ARE_PLUGIN_PARAMETERS`**: Affects ScriptComponent's plugin parameter reporting.

## Threading Model

- Content operates primarily on the **scripting thread** and **message thread**
- `callAfterDelay` uses JUCE Timer -- runs on message thread, explicitly NOT accurate for DSP
- `getScreenBounds` acquires `MessageManagerLock` -- blocks if not on message thread
- `isCtrlDown`, `isMouseDown`, `getComponentUnderMouse` query JUCE Desktop -- safe from any thread
- Component creation (`addXXX`) is onInit-only -- runs on scripting thread during compilation
- `showModalTextInput` broadcasts async to message thread
- `interfaceSizeBroadcaster` uses `sendNotificationAsync`

## Factory Output Types

Content creates 18 different types as documented in the survey:
- **UI Components** (14): ScriptButton, ScriptSlider, ScriptComboBox, ScriptLabel, ScriptTable, ScriptSliderPack, ScriptImage, ScriptAudioWaveform, ScriptPanel, ScriptFloatingTile, ScriptWebView, ScriptedViewport, ScriptMultipageDialog, ScriptDynamicContainer
- **Utility Objects** (4): ScriptLookAndFeel (via createLocalLookAndFeel), Path (via createPath), ScriptShader (via createShader), MarkdownRenderer (via createMarkdownRenderer)

Additionally creates SVG objects via `createSVG()` but SVGObject is not a separate API class.

## obtainedVia

Content is NOT created by any factory method. It is accessed as a built-in namespace `Content` within script processors. Every `JavascriptProcessor`/`ProcessorWithScriptingContent` has exactly one Content instance.

## Component Storage

Components are stored in `ReferenceCountedArray<ScriptComponent> components` (line 3568). They are reference-counted to allow anonymous controls. The parallel `contentPropertyData` ValueTree maintains the persistent/serializable state.

## Value Popup System

`setValuePopupData()` stores JSON that configures how value popups appear when interacting with knobs/sliders. The data is consumed by `ScriptCreatedComponentWrapper::ValuePopup::Properties` which extends `ObjectWithDefaultProperties`. Properties are applied globally to all components in this Content.

## Drag Operation System

Content provides three methods for the drag system:
- `getComponentUnderDrag()` -- queries RebuildListeners via `DragAction::Query`
- `refreshDragImage()` -- triggers repaint via `DragAction::Repaint`
- These work with the RebuildListener's `onDragAction()` callback

## Clean Up / Recompile Cycle

`cleanJavascriptObjects()` (line 8487):
- Sets `allowAsyncFunctions = false`
- For each component: cancels callbacks, clears control callback, clears local LAF
- For ScriptPanels specifically: clears constant data, cancels pending functions, clears paint/timer/mouse/loading callbacks
