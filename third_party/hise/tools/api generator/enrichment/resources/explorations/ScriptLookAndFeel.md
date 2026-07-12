# ScriptLookAndFeel (ScriptedLookAndFeel) -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey.md` -- prerequisite check (row 15: soft prerequisite for ScriptComponent subclasses)
- `enrichment/resources/survey/class_survey_data.json` -- ScriptLookAndFeel entry
- `enrichment/resources/explorations/ScriptComponent_base.md` -- base component context
- `enrichment/base/ScriptLookAndFeel.json` -- authoritative method list (8 methods)

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingGraphics.h` (lines 793-1515)
- **Implementation:** `hi_scripting/scripting/api/ScriptingGraphics.cpp` (lines 2502-6185)
- **C++ class:** `ScriptingObjects::ScriptedLookAndFeel`
- **Scripting name:** `ScriptLookAndFeel` (returned by `getObjectName()`, line 4116-4117)
- **Inheritance:** `ConstScriptingObject`, `ControlledObject`

## Factory Methods (obtainedVia)

Two factory methods produce ScriptedLookAndFeel instances:

1. **`Engine.createGlobalScriptLookAndFeel()`** (ScriptingApi.cpp:2070-2081)
   - Returns existing global LAF if one is already set, otherwise creates a new one with `isGlobal=true`
   - The global LAF is stored on `MainController` via `setCurrentScriptLookAndFeel(this)`
   - Affects ALL components that don't have a local LAF

2. **`Content.createLocalLookAndFeel()`** (ScriptingApiContent.cpp:8473-8475)
   - Always creates a new instance with `isGlobal=false`
   - Must be assigned to components via `component.setLocalLookAndFeel(laf)`
   - Propagates to child components when set (see ScriptComponent_base.md, method 28)

## Constructor (lines 2515-2535)

```cpp
ScriptedLookAndFeel(ProcessorWithScriptingContent* sp, bool isGlobal) :
    ConstScriptingObject(sp, 0),  // 0 = no constants
    ControlledObject(sp->getMainController_()),
    functions(new DynamicObject()),
    wasGlobal(isGlobal),
    lastResult(Result::ok())
{
    ADD_API_METHOD_2(registerFunction);
    ADD_API_METHOD_2(setGlobalFont);
    ADD_API_METHOD_2(loadImage);
    ADD_API_METHOD_0(unloadAllImages);
    ADD_API_METHOD_1(isImageLoaded);
    ADD_API_METHOD_1(setInlineStyleSheet);
    ADD_API_METHOD_1(setStyleSheet);
    ADD_API_METHOD_3(setStyleSheetProperty);

    additionalProperties = ValueTree("additionalProperties");

    if(isGlobal)
        getScriptProcessor()->getMainController_()->setCurrentScriptLookAndFeel(this);
}
```

Key observations:
- **Zero constants** -- `ConstScriptingObject(sp, 0)` means no `addConstant()` calls
- All 8 methods use plain `ADD_API_METHOD_N` (no typed variants)
- `functions` is a `DynamicObject` that stores registered paint callback functions
- `additionalProperties` ValueTree stores CSS variable properties set via `setStyleSheetProperty`
- `wasGlobal` flag is `const bool` -- cannot change after construction

## Wrapper Struct (lines 2502-2512)

```cpp
struct Wrapper {
    API_VOID_METHOD_WRAPPER_2(ScriptedLookAndFeel, registerFunction);
    API_VOID_METHOD_WRAPPER_2(ScriptedLookAndFeel, setGlobalFont);
    API_VOID_METHOD_WRAPPER_2(ScriptedLookAndFeel, loadImage);
    API_VOID_METHOD_WRAPPER_0(ScriptedLookAndFeel, unloadAllImages);
    API_METHOD_WRAPPER_1(ScriptedLookAndFeel, isImageLoaded);
    API_VOID_METHOD_WRAPPER_1(ScriptedLookAndFeel, setInlineStyleSheet);
    API_VOID_METHOD_WRAPPER_1(ScriptedLookAndFeel, setStyleSheet);
    API_VOID_METHOD_WRAPPER_3(ScriptedLookAndFeel, setStyleSheetProperty);
};
```

All use plain `API_*_METHOD_WRAPPER_N` -- no typed variants.

## Destructor (lines 2537-2541)

```cpp
~ScriptedLookAndFeel()
{
    SimpleReadWriteLock::ScopedWriteLock sl(
        getMainController()->getJavascriptThreadPool().getLookAndFeelRenderLock());
    clearScriptContext();
}
```

Acquires the LookAndFeelRenderLock before clearing. This is a write lock on the same lock that paint routines acquire a read lock on (see callWithGraphics).

## Dual Rendering Mode Architecture

ScriptLookAndFeel supports two mutually exclusive rendering approaches:

### Mode 1: Script Paint Functions (registerFunction)

- Functions stored in `functions` DynamicObject
- `hasScriptFunctions` flag set to `true` when any function is registered
- Each paint function receives `(Graphics g, var obj)` where `obj` is a DynamicObject with component-specific properties
- Functions are called via `callWithGraphics()` (for draw functions) or `callDefinedFunction()` (for data-returning functions)

### Mode 2: CSS Stylesheets (setInlineStyleSheet / setStyleSheet)

- CSS parsed by `simple_css::Parser` and stored in `css` (StyleSheet::Collection)
- `currentStyleSheet` stores the raw CSS string
- `isUsingCSS()` returns true when `currentStyleSheet` is non-empty

### Mode 3: Combined (CombinedLaf)

When both script functions AND CSS are active on a local LAF, a `CombinedLaf` is created. It uses the `CALL_LAF` macro pattern:
```cpp
#define CALL_LAF(functionId, ...) \
    if(functionDefined(#functionId)) Laf::functionId(__VA_ARGS__); \
    else css.functionId(__VA_ARGS__)
```
This checks if a script function is registered for each specific draw operation. If yes, the script path runs; otherwise CSS handles it. This allows mixing: some components styled via CSS, others via script functions.

## Inner Class Hierarchy

### LafBase (line 799)
- Inherits: `ProfiledLookAndFeel`
- Abstract base with `virtual ScriptedLookAndFeel* get() = 0`
- Provides `getStyleSheetLookAndFeel()` (returns nullptr by default)
- Profiling support via `HISE_INCLUDE_PROFILING_TOOLKIT`

### Laf (line 819)
- Inherits: `GlobalHiseLookAndFeel`, `LafBase`, and ~18 LookAndFeelMethods interfaces
- This is the main JUCE LookAndFeel subclass used for script paint function rendering
- `get()` returns `MainController::getCurrentScriptLookAndFeel()` (the global LAF)
- Contains `functionDefined(String)` helper that checks if a script function is registered
- Contains static helpers: `writeId()`, `setColourOrBlack()`, `addParentFloatingTile()`
- `useRectangleClass` flag controls whether `ApiHelpers::getVarRectangle` returns array or Rectangle object

### LocalLaf (line 1095)
- Inherits: `Laf`
- Overrides `get()` to return a specific `ScriptedLookAndFeel*` via weak reference (instead of global)
- Used when a LAF is assigned locally to a component

### CSSLaf (line 987)
- Inherits: `StyleSheetLookAndFeel`, multiple LookAndFeelMethods, `LafBase`, `StyleSheet::Collection::DataProvider`
- Pure CSS rendering path
- Created per-component with component's property tree and style sheet properties
- Manages CSS selectors, animations, property listeners
- `loadFont()` implementation loads fonts from MainController

### CombinedLaf (line 1106)
- Inherits: `LocalLaf`
- Contains a `CSSLaf css` member
- Delegates to script functions when defined, falls back to CSS
- Created when component has a local LAF with both CSS and script functions active

### LAF Selection Logic (ScriptingApiContent.cpp:1854-1898)

When `ScriptComponent::createLocalLookAndFeel()` is called:
1. If LAF uses CSS AND has script functions -> `CombinedLaf`
2. If LAF uses CSS only -> `CSSLaf`
3. If LAF uses script functions only -> `LocalLaf`

## Registered Paint Function Names (getAllFunctionNames, lines 2664-2736)

Complete list of 62 valid function names that can be passed to `registerFunction`:

```
drawAlertWindow
getAlertWindowMarkdownStyleData
drawAlertWindowIcon
drawPopupMenuBackground
drawPopupMenuItem
drawToggleButton
drawRotarySlider
drawLinearSlider
drawDialogButton
drawComboBox
drawNumberTag
createPresetBrowserIcons
drawPresetBrowserBackground
drawPresetBrowserDialog
drawPresetBrowserColumnBackground
drawPresetBrowserListItem
drawPresetBrowserSearchBar
drawPresetBrowserTag
drawWavetableBackground
drawWavetablePath
drawTableBackground
drawTablePath
drawTablePoint
drawTableMidPoint
drawTableRuler
drawScrollbar
drawMidiDropper
drawThumbnailBackground
drawThumbnailText
drawThumbnailPath
drawThumbnailRange
drawThumbnailRuler
getThumbnailRenderOptions
drawAhdsrBackground
drawAhdsrBall
drawAhdsrPath
drawKeyboardBackground
drawWhiteNote
drawBlackNote
drawSliderPackBackground
drawSliderPackFlashOverlay
drawSliderPackRightClickLine
drawSliderPackTextPopup
getIdealPopupMenuItemSize
drawTableRowBackground
drawTableCell
drawTableHeaderBackground
drawTableHeaderColumn
drawFilterDragHandle
drawFilterBackground
drawFilterPath
drawFilterGridLines
drawAnalyserBackground
drawAnalyserPath
drawAnalyserGrid
drawMatrixPeakMeter
getModulatorDragData
drawModulationDragBackground
drawModulationDragger
drawFlexAhdsrBackground
drawFlexAhdsrCurvePoint
drawFlexAhdsrFullPath
drawFlexAhdsrPosition
drawFlexAhdsrSegment
drawFlexAhdsrText
```

### Function Categories

| Category | Functions | Callback Signature |
|----------|----------|-------------------|
| Standard components | drawToggleButton, drawRotarySlider, drawLinearSlider, drawComboBox, drawDialogButton | `function(g, obj)` |
| Popup menu | drawPopupMenuBackground, drawPopupMenuItem, getIdealPopupMenuItemSize | `function(g, obj)` / returns value |
| Alert window | drawAlertWindow, getAlertWindowMarkdownStyleData, drawAlertWindowIcon | `function(g, obj)` / returns obj |
| Preset browser | drawPresetBrowser{Background,Dialog,ColumnBackground,ListItem,SearchBar,Tag}, createPresetBrowserIcons | `function(g, obj)` / returns Path |
| Table editor (curve) | drawTable{Background,Path,Point,MidPoint,Ruler} | `function(g, obj)` |
| Viewport table | drawTable{RowBackground,Cell,HeaderBackground,HeaderColumn} | `function(g, obj)` |
| Slider pack | drawSliderPack{Background,FlashOverlay,RightClickLine,TextPopup} | `function(g, obj)` |
| Keyboard | drawKeyboardBackground, drawWhiteNote, drawBlackNote | `function(g, obj)` |
| Audio thumbnail | drawThumbnail{Background,Path,Text,Range,Ruler}, getThumbnailRenderOptions | `function(g, obj)` / returns obj |
| AHDSR envelope | drawAhdsr{Background,Path,Ball} | `function(g, obj)` |
| Flex AHDSR | drawFlexAhdsr{Background,CurvePoint,FullPath,Position,Segment,Text} | `function(g, obj)` |
| Filter | drawFilter{Background,Path,GridLines,DragHandle} | `function(g, obj)` |
| Analyser | drawAnalyser{Background,Path,Grid} | `function(g, obj)` |
| Misc | drawScrollbar, drawMidiDropper, drawNumberTag, drawMatrixPeakMeter | `function(g, obj)` |
| Modulation | getModulatorDragData, drawModulationDragBackground, drawModulationDragger | `function(g, obj)` / returns obj |
| Wavetable | drawWavetable{Background,Path} | `function(g, obj)` |

## callWithGraphics -- The Paint Function Dispatch (lines 2757-2901)

This is the core mechanism for script-based rendering:

1. Checks `hasScriptFunctions` flag
2. Marks the component as rendered in the LAF registry
3. Checks `lastResult` -- if a previous call errored, all subsequent calls are skipped
4. Looks up the function by name in the `functions` DynamicObject
5. Reuses or creates a `GraphicsObject` per (component, functionName) pair (pooled in `graphics` array)
6. Injects `parentName` property from the component's parent
7. Copies non-hidden component JUCE properties to the obj (line 2836-2857) -- this is how custom properties from floating tiles and other JUCE components become visible
8. Calls the script function with args: `[GraphicsObject, argsObject]`
9. On success, flushes draw actions; on error, outputs to console and sets `lastResult` to failed (halting further calls)
10. Renders the accumulated draw actions to the actual Graphics context

### Threading Safety

- Acquires `ScopedTryReadLock` on `LookAndFeelRenderLock` (line 2823) -- non-blocking, skips rendering if lock unavailable
- The destructor acquires a write lock on the same lock
- This prevents rendering during script recompilation

## callDefinedFunction -- Non-Graphics Callbacks (lines 2903-2937)

For functions that return data rather than draw (e.g., `getIdealPopupMenuItemSize`, `getThumbnailRenderOptions`, `createPresetBrowserIcons`, `getAlertWindowMarkdownStyleData`, `getModulatorDragData`):

- Takes `var* args, int numArgs` directly
- Uses `callExternalFunctionRaw` (returns var, no graphics context)
- Catches exceptions and outputs to console
- Same read lock pattern as callWithGraphics

## obj Construction Pattern

Every draw function constructs a `DynamicObject*` with properties specific to that component. Common patterns:

### Standard Properties
- `id` -- component ID (via `writeId()` which checks componentID, then parent FloatingTile)
- `area` -- component bounds as `[x, y, w, h]` array or Rectangle object (controlled by `useRectangleClass`)
- `enabled` -- component enabled state

### Colour Properties
Via `setColourOrBlack(obj, propName, component, colourId)`:
- `bgColour` -- background colour (ARGB int, 0 if not specified)
- `itemColour1` / `itemColour` -- first item colour
- `itemColour2` -- second item colour
- `textColour` -- text colour
- `itemColour3` -- third item colour (some components)

### Interaction State
- `hover` / `over` -- mouse hover state
- `down` / `clicked` -- mouse button down
- `value` -- current component value

### Additional Context
- `parentType` -- identifier of parent floating tile (via `addParentFloatingTile`)
- `parentName` -- name of parent component (injected in callWithGraphics)
- `text` -- display text

## ModulationDisplayValue Integration (Slider Draw Functions)

For `drawRotarySlider` and `drawLinearSlider`, a `ModulationDisplayValue` is computed and stored to the obj JSON. Properties added by `storeToJSON()`:

| Property | Type | Description |
|----------|------|-------------|
| `valueNormalized` | double | Normalized slider value (0-1) |
| `scaledValue` | double | Modulation scale factor |
| `addValue` | double | Additive modulation offset |
| `modulationActive` | bool | Whether modulation is connected |
| `modMinValue` | float | Modulation range minimum |
| `modMaxValue` | float | Modulation range maximum |
| `lastModValue` | double | Last modulation output value |

These are in addition to the standard `value`, `min`, `max`, `skew` properties on the slider obj.

## Image Management

### loadImage (lines 6126-6153)
- Creates a `PoolReference` from the image name (using `ProjectHandler::SubDirectories::Images`)
- If prettyName already exists, updates the image reference if it changed
- Uses `ExpansionHandler::loadImageReference()` -- supports expansion pack images
- Uses `TimeoutExtender` to prevent script timeout during image loading
- Outputs console warning if image not found

### getLoadedImage (lines 4157-4168)
- Internal method (not exposed to script) used by paint functions to retrieve loaded images
- Linear search by prettyName through `loadedImages` array

### isImageLoaded (lines 6160-6169)
- Returns true if prettyName exists in `loadedImages` array

### unloadAllImages (lines 6155-6158)
- Clears the entire `loadedImages` array

### NamedImage struct (lines 1496-1500)
```cpp
struct NamedImage {
    PooledImage image;
    String prettyName;
};
```

## CSS Stylesheet Infrastructure

### setInlineStyleSheet (lines 2558-2565)
- Sets `useInlineStyleSheet = true` if code is non-empty
- Generates unique file identifier from hash: `"inline_" + hash`
- Delegates to `setStyleSheetInternal()`

### setStyleSheet (lines 2646-2653)
- Stores filename in `currentStyleSheetFile`
- Loads file content via `loadStyleSheetFile()`
- Delegates to `setStyleSheetInternal()`

### loadStyleSheetFile (lines 2579-2627)
- **Must have `.css` extension** -- reports script error otherwise
- **USE_BACKEND path:**
  - Looks in Scripts subdirectory
  - Tries `getExternalScriptFile()` first, then file directly
  - If file doesn't exist, creates a default CSS file with `* { color: white; }`
  - Adds file watcher for live editing
  - Adds as shader file for the script engine
- **Frontend path:**
  - Loads from embedded script collection via `getExternalScriptFromCollection()`

### setStyleSheetInternal (lines 2629-2644)
- Stores raw CSS string
- Parses via `simple_css::Parser`
- Reports script error if parse fails
- Acquires write lock on `LookAndFeelRenderLock`
- Clears graphics pool and replaces CSS collection

### setStyleSheetProperty (lines 2655-2661)
- Converts value using `ApiHelpers::convertStyleSheetProperty(value, type)` (see ScriptComponent_base.md method 31 for type vocabulary)
- Stores in `additionalProperties` ValueTree

### CSS Type Conversion (from ScriptComponent_base.md)

| Type | Conversion |
|------|------------|
| `"path"` | PathObject to base64 string |
| `"color"` | int colour to `"#AARRGGBB"` string |
| `"%"` | number * 100 + "%" |
| `"px"` | number + "px" |
| `"em"` | number + "em" |
| `"vh"` | number + "vh" |
| `"deg"` | number + "deg" |
| `""` (empty) | no conversion |

## Global Font

### setGlobalFont (lines 2553-2556)
```cpp
void setGlobalFont(const String& fontName, float fontSize)
{
    f = getScriptProcessor()->getMainController_()->getFontFromString(fontName, fontSize);
}
```
- Stores as member `Font f = GLOBAL_BOLD_FONT()` (default)
- All `Laf` font getter methods (`getAlertWindowMessageFont`, `getTextButtonFont`, `getComboBoxFont`, `getPopupMenuFont`, `getAlertWindowFont`, `getAlertWindowTitleFont`) return this font
- Affects only fonts used by JUCE LookAndFeel methods, not script paint function fonts (those are controlled by `g.setFont()` in the callback)

## clearScriptContext (lines 2941-2946)
```cpp
void clearScriptContext()
{
    functions = var();
    graphics.clear();
    loadedImages.clear();
}
```
Called from destructor (with write lock held). Clears all registered functions, graphics objects, and loaded images.

## Debug/Profiling Support

- `getNumChildElements()` / `getChildElement()` -- exposes registered functions as debug children
- `getLocation()` -- returns the source location of the first registered function
- `setEnableProfiling()` -- guarded by `HISE_INCLUDE_PROFILING_TOOLKIT`, creates `ProfileDataSource` for paint routine profiling

## Preprocessor Guards

- `HISE_INCLUDE_PROFILING_TOOLKIT` -- profiling support in LafBase and callWithGraphics
- `USE_BACKEND` -- file loading path in loadStyleSheetFile (file watcher, external script files)
- `HISE_USE_SCRIPT_RECTANGLE_OBJECT` -- controls whether area properties are arrays or Rectangle objects (checked via `HISE_GET_PREPROCESSOR` in Laf constructor)
- `HISE_USE_CUSTOM_ALERTWINDOW_LOOKANDFEEL` -- guards the default alert window LAF creation (line 6112)
- `PERFETTO` -- performance tracing in callWithGraphics

## Key Behavioral Patterns

1. **Error halting:** Once a paint function throws an error, `lastResult` is set to failed and ALL subsequent paint calls are skipped (line 2785-2786). This prevents cascading errors.

2. **Graphics object pooling:** GraphicsObject instances are cached per (component, functionName) pair and reused across paint calls. This avoids allocation during rendering.

3. **Non-blocking paint:** The `ScopedTryReadLock` means paint calls are silently skipped if the script engine is being modified (compilation, destruction). This prevents deadlocks.

4. **Fallback pattern:** Every draw function checks `functionDefined()` first. If the script function is not registered, the default JUCE LookAndFeel rendering is used. This allows selective override.

5. **Component property injection:** In `callWithGraphics` (lines 2836-2857), all JUCE component properties (except those containing "jcclr") are copied to the obj. This is how custom floating tile properties become accessible in paint callbacks.

## LAF Registry Integration

When `hasScriptFunctions` is true, `callWithGraphics` calls `registry->markAsRendered(id)` (line 2776-2778). The LAF registry tracks which components have been rendered by the LAF system, enabling development-time warnings about unregistered components.
