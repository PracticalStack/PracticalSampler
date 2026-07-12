# ScriptPanel -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/explorations/ScriptComponent_base.md` -- base class exploration
- `enrichment/resources/base_methods/ScriptComponent.md` -- pre-distilled base methods
- `enrichment/resources/survey/class_survey_data.json` -- ScriptPanel entry
- `enrichment/base/ScriptPanel.json` -- authoritative method list
- `hi_core/hi_core/MiscComponents.cpp` -- MouseCallbackComponent (callback levels, event object)
- `hi_scripting/scripting/api/ScriptingApi.cpp` -- ApiHelpers::getMouseCursorNames()

## Class Declaration

**Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` (lines 1734-2066)
**Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` (lines 4220-5498)
**Full path:** `ScriptingApi::Content::ScriptPanel`

### Inheritance Chain

```
ScriptPanel : public ScriptComponent,
              public SuspendableTimer,
              public GlobalSettingManager::ScaleFactorListener,
              public HiseJavascriptEngine::CyclicReferenceCheckBase,
              public MainController::SampleManager::PreloadListener
```

Key bases:
- `ScriptComponent` -- all 35 base component methods (see ScriptComponent_base.md)
- `SuspendableTimer` -- provides `startTimer(int ms)` and `stopTimer()` methods (from JUCE Timer, with suspension)
- `ScaleFactorListener` -- notified when global UI scale factor changes (currently no-op override)
- `CyclicReferenceCheckBase` -- prevents circular reference memory leaks in HiseScript
- `PreloadListener` -- listens for sample preload state changes

## Properties (enum)

ScriptPanel adds 13 properties beyond the base ScriptComponent properties:

| Enum Name | Property ID | Default | Type/Selector | Description |
|-----------|-------------|---------|---------------|-------------|
| borderSize | "borderSize" | 2.0 | Slider (0-20, step 1) | Border width in pixels |
| borderRadius | "borderRadius" | 6.0 | Slider (0-20, step 1) | Corner radius in pixels |
| opaque | "opaque" | false | Toggle | If true, panel is opaque (no alpha) |
| allowDragging | "allowDragging" | 0 | Toggle | Enables drag behavior for the panel |
| allowCallbacks | "allowCallbacks" | "No Callbacks" | Choice | Mouse callback level (see callback levels) |
| PopupMenuItems | "popupMenuItems" | "" | Multiline | Newline-separated popup menu items |
| PopupOnRightClick | "popupOnRightClick" | true | Toggle | Show popup on right click vs left click |
| popupMenuAlign | "popupMenuAlign" | false | Toggle | Align popup to bottom of component |
| selectedPopupIndex | "selectedPopupIndex" | -1 | Number | Currently selected popup item index |
| stepSize | "stepSize" | 0.0 | Number | Value step size for the panel |
| enableMidiLearn | "enableMidiLearn" | false | Toggle | Enable MIDI learn on right-click |
| holdIsRightClick | "holdIsRightClick" | true | Toggle | Touch hold acts as right-click |
| isPopupPanel | "isPopupPanel" | false | Toggle | Panel is a popup (hidden until showAsPopup) |
| bufferToImage | "bufferToImage" | false | Toggle | Buffer paint output to image |

Default overrides from base ScriptComponent:
- `saveInPreset` = false (base default is true)
- `isPluginParameter` = false
- `textColour` = 0x23FFFFFF (different from base 0xFFFFFFFF)
- `itemColour` = 0x30000000 (different from base 0x66333333)
- `itemColour2` = 0x30000000 (different from base 0xFB111111)
- `width` = 100, `height` = 50

Deactivated properties: `linkedTo` (added in handleDefaultDeactivatedProperties)

## Constructor and Constants

The constructor calls `addConstant("data", new DynamicObject())`, making `data` a persistent DynamicObject constant accessible as `panel.data`. This is the primary mechanism for storing arbitrary per-panel state.

## API Method Registration

### Methods from SuspendableTimer (NOT in base JSON but registered)

Two methods are registered via `ADD_API_METHOD` but come from the `SuspendableTimer` base class and are NOT listed in the base JSON:

```cpp
ADD_API_METHOD_1(startTimer);    // SuspendableTimer::startTimer(int ms)
ADD_API_METHOD_0(stopTimer);     // SuspendableTimer::stopTimer()
```

These are directly exposed as scripting API methods. `startTimer` takes a millisecond interval and starts periodic timer callbacks. `stopTimer` stops the timer.

### ScriptPanel-specific methods (30 total registered)

```cpp
ADD_API_METHOD_0(repaint);
ADD_API_METHOD_0(repaintImmediately);
ADD_API_METHOD_1(setPaintRoutine);
ADD_API_METHOD_3(setImage);
ADD_TYPED_API_METHOD_1(setMouseCallback, VarTypeChecker::Function);
ADD_TYPED_API_METHOD_1(setLoadingCallback, VarTypeChecker::Function);
ADD_TYPED_API_METHOD_1(setTimerCallback, VarTypeChecker::Function);
ADD_TYPED_API_METHOD_3(setFileDropCallback, VarTypeChecker::String, VarTypeChecker::String, VarTypeChecker::Function);
ADD_API_METHOD_1(startTimer);
ADD_API_METHOD_0(stopTimer);
ADD_API_METHOD_2(loadImage);
ADD_API_METHOD_0(unloadAllImages);
ADD_API_METHOD_1(isImageLoaded);
ADD_API_METHOD_1(setDraggingBounds);
ADD_API_METHOD_2(setPopupData);
ADD_API_METHOD_3(setPanelValueWithUndo);
ADD_API_METHOD_1(showAsPopup);
ADD_API_METHOD_0(closeAsPopup);
ADD_API_METHOD_1(setIsModalPopup);
ADD_API_METHOD_0(isVisibleAsPopup);
ADD_API_METHOD_0(addChildPanel);
ADD_API_METHOD_0(removeFromParent);
ADD_API_METHOD_0(getChildPanelList);
ADD_API_METHOD_0(getParentPanel);
ADD_API_METHOD_3(setMouseCursor);
ADD_API_METHOD_0(getAnimationData);
ADD_API_METHOD_1(setAnimation);
ADD_API_METHOD_1(setAnimationFrame);
ADD_API_METHOD_3(startExternalFileDrag);
ADD_API_METHOD_1(startInternalDrag);
```

Plus 35 inherited from ScriptComponent = 65 total methods.

### Forced-type methods (ADD_TYPED_API_METHOD)

| Method | Param 1 | Param 2 | Param 3 |
|--------|---------|---------|---------|
| setMouseCallback | Function | -- | -- |
| setLoadingCallback | Function | -- | -- |
| setTimerCallback | Function | -- | -- |
| setFileDropCallback | String | String | Function |

### Callback Diagnostics

```cpp
ADD_CALLBACK_DIAGNOSTIC(mouseRoutine, setMouseCallback, 0);
ADD_CALLBACK_DIAGNOSTIC(loadRoutine, setLoadingCallback, 0);
ADD_CALLBACK_DIAGNOSTIC(timerRoutine, setTimerCallback, 0);
ADD_CALLBACK_DIAGNOSTIC(fileDropRoutine, setFileDropCallback, 2);
```

## Virtual Method Overrides

ScriptPanel overrides two virtual methods from ScriptComponent:

### changed()

```cpp
void ScriptingApi::Content::ScriptPanel::changed()
{
    if(pluginParameterInfo.p != nullptr)
    {
        auto v = (float)getValue();
        FloatSanitizers::sanitizeFloatNumber(v);
        auto idx = getScriptProcessor()->getScriptingContent()->getComponentIndex(getName());
        pluginParameterInfo.p->setAttribute(idx, v, dispatch::DispatchType::sendNotificationAsyncHiPriority);
    }
    else
    {
        ScriptComponent::changed();
    }
}
```

When `isPluginParameter` is enabled and `HISE_SEND_PANEL_CHANGED_TO_PLUGIN_PARAMETER` preprocessor is set, `changed()` directly sends the value to the DAW plugin parameter system instead of going through the normal control callback path. Otherwise, falls through to base `ScriptComponent::changed()`.

### sendRepaintMessage()

```cpp
void ScriptingApi::Content::ScriptPanel::sendRepaintMessage()
{
    ScriptComponent::sendRepaintMessage();
    repaint();
}
```

Calls base implementation (async repaint broadcaster) AND also triggers the panel's own paint routine.

## Mouse Callback Levels (allowCallbacks property)

The `allowCallbacks` property uses `MouseCallbackComponent::getCallbackLevels()`:

| Index | Value | Description |
|-------|-------|-------------|
| 0 | "No Callbacks" | No mouse callbacks fired |
| 1 | "Context Menu" | Only popup menu interactions |
| 2 | "Clicks Only" | Mouse down/up, double click |
| 3 | "Clicks & Hover" | Clicks + mouse enter/exit |
| 4 | "Clicks, Hover & Dragging" | Clicks + hover + drag events |
| 5 | "All Callbacks" | All above + mouse move |

### Mouse Event Object Properties

The mouse callback receives a JSON object with these properties (from `MouseCallbackComponent::getCallbackPropertyNames()`):

| Property | Type | Description |
|----------|------|-------------|
| mouseDownX | int | X position of initial mouse down |
| mouseDownY | int | Y position of initial mouse down |
| x | int | Current x position |
| y | int | Current y position |
| clicked | bool | True on mouse down |
| doubleClick | bool | True on double click |
| rightClick | bool | True if right button |
| mouseUp | bool | True on mouse up |
| drag | bool | True during drag |
| isDragOnly | bool | True if was only a drag (no click callback) |
| dragX | int | Horizontal drag distance |
| dragY | int | Vertical drag distance |
| insideDrag | bool | True if drag is inside component bounds |
| hover | bool | True on hover (requires "Clicks & Hover" or above) |
| result | int | Popup menu result (selected item index) |
| itemText | String | Popup menu selected item text |
| shiftDown | bool | True if Shift held |
| cmdDown | bool | True if Cmd/Ctrl held |
| altDown | bool | True if Alt held |
| ctrlDown | bool | True if Ctrl held |

### File Drop Callback Levels

The `setFileDropCallback` uses a separate set of callback levels:

| Index | Value | Description |
|-------|-------|-------------|
| 0 | "No Callbacks" | No file drop callbacks |
| 1 | "Drop Only" | Only fired when files are dropped |
| 2 | "Drop & Hover" | Drop + hover while dragging over |
| 3 | "All Callbacks" | All file drag events |

## Paint Routine Infrastructure

### repaint() -- thread-aware dispatching

The `repaint()` method checks the current thread:
- If on **SampleLoadingThread**, **ScriptingThread**, or **MessageThread**: calls `internalRepaint(false)` directly
- If on any other thread (e.g., audio thread): defers via `getJavascriptThreadPool().addDeferredPaintJob(this)`

### internalRepaint() and internalRepaintIdle()

`internalRepaint()` schedules a `LowPriorityCallbackExecution` job that calls `internalRepaintIdle()`.

`internalRepaintIdle()` is the core paint execution:
1. Checks if parent has moved on or async functions are disallowed
2. Computes canvas dimensions from `getBoundsForImage()` (accounting for scale factor)
3. Skips if not showing or zero dimensions (unless forced)
4. Calls `engine->callExternalFunction(paintRoutine, args, &r)` with a `GraphicsObject` argument
5. After callback, calls `graphics->getDrawHandler().flush()` to commit draw actions

### repaintImmediately()

Currently just calls `repaint()` -- same behavior. (The internal timerCallbackInternal has an `#if 0` block showing old synchronous paint code that was removed.)

### setBoundsForImage / getScaleFactorForCanvas

Canvas resolution accounts for:
- `Content::usesDoubleResolution()` (set via `Content.setUseHighResolutionForPanels`)
- Global scale factor from `GlobalSettingManager`
- Capped at 2.0x maximum

### setImage() -- fixed image mode

Calling `setImage()` sets `usesClippedFixedImage = true` and clears the paint routine. It clips a region from a loaded image and renders it as a single draw action. Either x or y offset must be 0 (the other dimension is calculated from the aspect ratio).

## Timer Infrastructure

ScriptPanel inherits from `SuspendableTimer`. The timer callback mechanism:

1. `setTimerCallback(var)` stores a `WeakCallbackHolder` with 0 parameters
2. `startTimer(int ms)` starts the JUCE timer at the given interval (inherited, not ScriptPanel-specific)
3. `stopTimer()` stops it (inherited)
4. `timerCallback()` override calls `timerRoutine.call(nullptr, 0)` via WeakCallbackHolder
5. On `preRecompileCallback()`, timer is stopped and callback cleared

The timer fires on the message thread. There is no parameter to the timer callback function.

## Loading Callback

`setLoadingCallback(var)` registers a callback that fires when sample preloading starts/finishes:
- Registers this panel as a `PreloadListener` on the SampleManager
- `preloadStateChanged(bool isPreloading)` calls `loadRoutine.call1(isPreloading)`
- Callback receives one boolean parameter: true when loading starts, false when done
- Passing a non-function value removes the listener

## File Drop Callback

`setFileDropCallback(String callbackLevel, String wildcard, var dropFunction)`:
- `callbackLevel` uses file callback levels: "No Callbacks", "Drop Only", "Drop & Hover", "All Callbacks"
- `wildcard` filters accepted file extensions (e.g., "*.wav;*.aif")
- `dropFunction` receives one argument with file information

## Child Panel Hierarchy

ScriptPanel supports a parent-child panel system:

- `addChildPanel()` creates a new ScriptPanel with `this` as parent, adds to `childPanels` array
- `removeFromParent()` removes from parent's childPanels, returns true/false
- `getChildPanelList()` returns Array of child ScriptPanel references
- `getParentPanel()` returns parent ScriptPanel or undefined
- Child panels are created with the second constructor `ScriptPanel(ScriptPanel* parent)` which sets `isChildPanel = false` (note: actually the `isChildPanel` flag in the first constructor is set true, and the child constructor does not set it -- it stays default false, but the addChildPanel method sets `childPanels.getLast()->isChildPanel = true`)

Child panels have their own paint routines, mouse callbacks, timers, etc. They are full ScriptPanel instances.

## Popup System

### Standard Popup (via popupMenuItems property)

The `PopupMenuItems` property contains newline-separated items. When `PopupOnRightClick` is true (default), right-clicking opens the popup menu. `popupMenuAlign` controls alignment. The result is delivered through the mouse callback's `result` and `itemText` properties.

### Panel as Popup (isPopupPanel + showAsPopup)

- `setPopupData(jsonData, position)` -- sets FloatingTile JSON config and bounds `[x, y, w, h]`
- `showAsPopup(closeOtherPopups)` -- shows the panel as a popup overlay, closing others if requested
- `closeAsPopup()` -- hides the popup
- `isVisibleAsPopup()` -- returns current popup visibility
- `setIsModalPopup(shouldBeModal)` -- makes popup modal with dark background

The popup system uses `parent->addPanelPopup(this, closeOther)` which manages popup panels in the Content's `popupPanels` array.

### isShowing() override

The `isShowing()` method checks: if `isPopupPanel` is true, only returns true if `shownAsPopup` is also true. This prevents the paint routine from executing for hidden popup panels.

## Undo System (setPanelValueWithUndo)

`setPanelValueWithUndo(oldValue, newValue, actionName)`:
- For simple values (numbers): creates `BorderPanel::UndoableControlEvent`
- For complex values (arrays/objects): creates `PanelComplexDataUndoEvent`
- Both call `setValue()` + `changed()` on perform/undo
- Uses `MainController::getControlUndoManager()`

## Drag and Drop

### External File Drag

`startExternalFileDrag(fileToDrag, moveOriginal, finishCallback)`:
- Accepts a string path, File object, or array of either
- Calls JUCE's `DragAndDropContainer::performExternalDragDropOfFiles`
- On Windows, called synchronously; on other platforms, deferred via `MessageManager::callAsync`
- Optional finish callback is called synchronously when drag completes

### Internal Drag

`startInternalDrag(dragData)`:
- Sends drag action to Content's RebuildListener system
- Triggers `DragAction::Start` with this panel and the drag data

### Dragging Bounds

`setDraggingBounds(area)` stores a `[x, y, w, h]` array that constrains the panel's drag area when `allowDragging` is enabled.

## Animation (Lottie)

Guarded by `#if HISE_INCLUDE_RLOTTIE`:

- `setAnimation(base64LottieAnimation)` -- loads Lottie JSON animation (base64 encoded), sets scale factor 2.0, sizes to panel
- `setAnimationFrame(int numFrame)` -- renders specific frame, updates animation data, flushes draw handler
- `getAnimationData()` -- returns JSON object:
  - `active` (bool) -- whether animation is loaded and valid
  - `currentFrame` (int) -- current frame number
  - `numFrames` (int) -- total frames
  - `frameRate` (int) -- animation frame rate

AnimationListener interface notifies UI wrapper when animation or paint routine changes.

## Mouse Cursor

`setMouseCursor(pathIcon, colour, hitPoint)`:
- If `pathIcon` is a Path object: sets custom cursor from path with color and hit point `[x, y]` (normalized 0-1)
- If `pathIcon` is a string: uses standard JUCE cursor type name
- Updates via `LambdaBroadcaster` (lock-free update on global UI updater)

### Valid standard cursor names (from ApiHelpers::getMouseCursorNames):

"ParentCursor", "NoCursor", "NormalCursor", "WaitCursor", "IBeamCursor", "CrosshairCursor", "CopyingCursor", "PointingHandCursor", "DraggingHandCursor", "LeftRightResizeCursor", "UpDownResizeCursor", "UpDownLeftRightResizeCursor", "TopEdgeResizeCursor", "BottomEdgeResizeCursor", "LeftEdgeResizeCursor", "RightEdgeResizeCursor", "TopLeftCornerResizeCursor", "TopRightCornerResizeCursor", "BottomLeftCornerResizeCursor", "BottomRightCornerResizeCursor"

## Image Loading

- `loadImage(imageName, prettyName)` -- loads from Images pool via expansion handler, stores in `loadedImages` array with pretty name alias
- `unloadAllImages()` -- clears all loaded images
- `isImageLoaded(prettyName)` -- checks if a prettyName is in the loaded images list
- Images are referenced from the project's Images folder or from expansion packs

## Plugin Parameter Integration

The `PluginParameterInfo` struct tracks whether this panel forwards value changes to the DAW:
- Updated when `isPluginParameter` property changes
- Requires `HISE_SEND_PANEL_CHANGED_TO_PLUGIN_PARAMETER` preprocessor to be enabled
- When active, `changed()` sends values directly to `Processor::setAttribute` instead of through the control callback

## Profiling

Two profiling IDs are registered:
- `pRepaint` -- tracks repaint() calls
- `pPaintRoutine` -- tracks paint routine execution time

Both are registered with `contentProfile` for the Content profiling system.

## preRecompileCallback

Clears:
- `cachedList` (debug watch list)
- `timerRoutine`, `loadRoutine`, `mouseRoutine` (WeakCallbackHolders)
- Stops the timer
- Calls base `ScriptComponent::preRecompileCallback()`

## Debug Watch System

ScriptPanel provides rich debug information via `DebugWatchIndex`:

| Index | Label | Content |
|-------|-------|---------|
| Data | data | The `data` DynamicObject constant |
| ChildPanels | childPanels | Array of child panel references |
| PaintRoutine | paintRoutine | The paint function |
| TimerCallback | timerCallback | The timer function |
| MouseCallback | mouseCallback | The mouse function |
| PreloadCallback | loadingCallback | The loading function |
| FileCallback | fileCallback | The file drop function |

## Key Threading Notes

- `repaint()` is safe to call from any thread -- automatically dispatches appropriately
- Paint routine executes on the scripting thread via `JavascriptThreadPool::LowPriorityCallbackExecution`
- Timer callback fires on the message thread
- Mouse callback fires on the message thread (via `mouseRoutine.call1`)
- Loading callback fires when preload state changes (sample loading thread -> deferred)
- File drop callback fires on the message thread

## Preprocessor Guards

- `HISE_INCLUDE_RLOTTIE` -- guards all Lottie animation code (setAnimation, setAnimationFrame, getAnimationData)
- `HISE_SEND_PANEL_CHANGED_TO_PLUGIN_PARAMETER` -- enables direct plugin parameter forwarding in changed()
- `PERFETTO` -- enables performance tracing flow management
