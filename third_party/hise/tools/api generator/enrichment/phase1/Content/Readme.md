# Content -- Class Analysis

## Brief
Top-level UI factory and namespace for creating components, layout objects, and managing the script interface.

## Purpose
Content is the root factory for the entire HiseScript UI system. It provides methods to create all visual components (buttons, knobs, labels, panels, etc.) during onInit, factory methods for utility drawing objects (Path, Shader, SVG, MarkdownRenderer, LookAndFeel), and interface-level configuration (size, tooltips, key press callbacks, value popups). Unlike most HISE API classes, Content inherits DynamicObject rather than ConstScriptingObject, meaning it has no typed API method registrations or constants -- all methods use native function wrappers.

## Details

### Initialization Lifecycle

Content enforces a strict two-phase lifecycle:

1. **During onInit**: Component creation methods (`addButton`, `addKnob`, etc.) are allowed. Calling these after onInit raises a script error.
2. **After onInit**: Component creation is blocked, but async utilities (`callAfterDelay`, `showModalTextInput`, etc.) become available.

This is controlled by `allowGuiCreation` (true during onInit, false after) and `allowAsyncFunctions` (false during onInit, true after).

### Component Creation Methods

Content provides 14 component creation methods, one per component type:

| Method | Returns | Component Type |
|--------|---------|---------------|
| `addKnob` | ScriptSlider | Rotary knob / slider |
| `addButton` | ScriptButton | Toggle button |
| `addLabel` | ScriptLabel | Text input label |
| `addComboBox` | ScriptComboBox | Dropdown selector |
| `addTable` | ScriptTable | Table curve editor |
| `addImage` | ScriptImage | Static image display |
| `addPanel` | ScriptPanel | Drawable panel (paint routines) |
| `addAudioWaveform` | ScriptAudioWaveform | Audio waveform display |
| `addSliderPack` | ScriptSliderPack | Multi-slider array |
| `addViewport` | ScriptedViewport | Scrollable viewport |
| `addFloatingTile` | ScriptFloatingTile | Floating tile (preset browser, keyboard, etc.) |
| `addWebView` | ScriptWebView | Embedded web view |
| `addMultipageDialog` | ScriptMultipageDialog | Multi-page dialog |
| `addDynamicContainer` | ScriptDynamicContainer | Dynamic child container |

### Component Creation Semantics

All `addXXX()` methods are **idempotent**: if a component with the same name already exists, the existing component is returned (with optional position update). This means the same `Content.addButton("MyBtn", 10, 20)` call runs safely on every recompile without creating duplicates.

Each `addXXX()` method accepts either 1 argument (name only) or 3 arguments (name, x, y).

The position update on existing components is controlled by `setUpdateExistingPosition(bool)`. When set to `false`, re-calling `addXXX()` with an existing name will return the existing component without modifying its x/y coordinates. This is useful when positions are managed dynamically by layout scripts and should not be reset on recompile. The default is `true` (positions are updated).

### Additional Utility Methods

- `componentExists(name)` - checks whether a component with the given name exists
- `setPropertiesFromJSON(name, jsonData)` - bulk-sets properties on a component by name
- `storeAllControlsAsPreset(fileName)` - saves all control values to an XML file (counterpart to `restoreAllControlsFromPreset`)

### Method Registration Architecture

Content uses `DynamicObject::setMethod()` for all method registrations, not `ADD_API_METHOD_N` / `ADD_TYPED_API_METHOD_N`. This means:
- No forced parameter types exist on any Content method
- No constants are registered via `addConstant()`
- All type checking happens inside manual NativeFunctionArgs wrappers

### Value Popup Configuration

See `setValuePopupData()` for the full JSON schema and usage.

### Key Press System

Content supports two levels of key press handling:
1. **Interface-level** via `setKeyPressCallback()` -- registers a (keyPress, callback) pair at the Content level
2. **Component-level** via `ScriptComponent.setKeyPressCallback()` -- per-component key handling

See `setKeyPressCallback()` for the full key press description format and callback properties.

### Modal Text Input

See `showModalTextInput()` for the full properties JSON schema and callback signature.

### Visual Guides

See `addVisualGuide()` for the full guide types and usage.

### Deprecated Method

`setToolbarProperties()` is fully deprecated. See its method entry for details.

## obtainedVia
Built-in namespace -- available as `Content` in every script processor's onInit callback.

## minimalObjectToken


## Constants
None. Content uses DynamicObject method registration and has no `addConstant()` calls.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Content.addButton("Btn1", 10, 20)` in onControl | `Content.addButton("Btn1", 10, 20)` in onInit | Component creation is only allowed during onInit. Calling addXXX after initialization throws a script error. |
| `Content.setToolbarProperties({...})` | (remove the call) | setToolbarProperties has been deprecated since 2017 and always throws a script error. |

## codeExample
```javascript
// Content is a built-in namespace, no variable creation needed
Content.makeFrontInterface(900, 600);
const var knob1 = Content.addKnob("Volume", 10, 10);
const var btn1 = Content.addButton("Bypass", 150, 10);
```

## Alternatives
- `Engine` manages the audio engine and global state, while Content manages the visual interface.
- `ScriptPanel` is a specific component you draw on with paint routines, created by Content.

## Related Preprocessors
- `USE_BACKEND` -- Enables LafRegistry, component definition navigation, ValueTree rebuild on change
- `USE_FRONTEND` -- Suspends update dispatcher, uses embedded ValueTree for preset restore
- `HISE_SUPPORT_GLSL_LINE_NUMBERS` -- Optional shader debug line numbers in createShader

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Content methods are mostly factories and configuration setters with clear runtime errors for misuse (e.g., adding components after onInit). The only deprecated method (setToolbarProperties) already throws at runtime. No silent-failure patterns that would benefit from parse-time diagnostics.
