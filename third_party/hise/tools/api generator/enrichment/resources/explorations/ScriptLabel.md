Resources consulted:
- enrichment/resources/survey/class_survey.md
- enrichment/resources/survey/class_survey_data.json
- enrichment/phase1/Content/Readme.md
- enrichment/resources/explorations/ScriptComponent_base.md

# ScriptLabel -- C++ Source Exploration

## Class declaration and inheritance

Header: `hi_scripting/scripting/api/ScriptingApiContent.h`

`ScriptingApi::Content::ScriptLabel` inherits `ScriptComponent`.

Declaration summary:
- Overrides `getObjectName()`, `getValue()`, `setValue()`, `resetValueToDefault()`, `setScriptObjectPropertyWithChangeMessage()`, `handleDefaultDeactivatedProperties()`, `isClickable()`, `createComponentWrapper()`.
- Adds one API method: `setEditable(bool shouldBeEditable)`.
- Provides `getOptionsFor()` for property editors and `getJustification()` helper.
- Declares property enum `Properties` with component-specific properties.

Property enum:
```
enum Properties
{
    FontName = ScriptComponent::Properties::numProperties,
    FontSize,
    FontStyle,
    Alignment,
    Editable,
    Multiline,
    SendValueEachKeyPress,
    numProperties
};
```

Notes from ScriptComponent prerequisite:
- ScriptLabel inherits all ScriptComponent API methods and property system. Content is the prerequisite context for lifecycle and property rules.
- ScriptLabel overrides `getValue` and `setValue` to store and return text values (string) instead of numeric values.

## Constructor and registration

Constructor: `ScriptingApi::Content::ScriptLabel::ScriptLabel(...)` in `ScriptingApiContent.cpp`.

Property registration and type selectors:
- `fontName` (choice selector)
- `fontSize` (slider 1..200)
- `fontStyle` (choice selector)
- `alignment` (choice selector)
- `editable` (toggle selector)
- `multiline` (toggle selector)
- `updateEachKey` (toggle selector)

Default values set in constructor:
- `x`, `y` from creation args
- `width = 128`, `height = 28`
- `saveInPreset = false`
- `text = name.toString()`
- `bgColour = 0x00000000`
- `itemColour = 0x00000000`
- `textColour = 0xffffffff`
- `FontStyle = "plain"`
- `FontSize = 13.0f`
- `FontName = "Arial"`
- `Alignment = "centred"`
- `Editable = true`
- `Multiline = false`
- `SendValueEachKeyPress = false`

Constructor also:
- calls `handleDefaultDeactivatedProperties()`
- initializes the internal `text` property from value tree or default
- sets `value = var("internal")` (sentinel for internal value handling)

API method registration:
- `ADD_API_METHOD_1(setEditable)` only. No `ADD_TYPED_API_METHOD` and no `addConstant()`.

Wrapper registration:
- `struct ScriptLabel::Wrapper` uses `API_VOID_METHOD_WRAPPER_1(ScriptLabel, setEditable)`.

## Factory / obtainedVia

`Content.addLabel(labelName, x, y)` calls `addComponent<ScriptLabel>()`.
Location: `ScriptingApiContent.cpp` around `addLabel`.

## ScriptLabel property behaviors and options

### getOptionsFor()
`ScriptLabel::getOptionsFor(const Identifier& id)` supplies option lists for properties:
- `FontStyle`:
  - `Font::getAvailableStyles()` for the default font
  - Adds "Password" as a custom option
- `FontName`:
  - Adds built-in labels: "Default", "Oxygen", "Source Code Pro"
  - Adds custom fonts from `MainController::fillWithCustomFonts`
  - Adds all system typeface names via `Font::findAllTypefaceNames()`
- `Alignment`: uses `ApiHelpers::getJustificationNames()`
- Other IDs: falls back to `ScriptComponent::getOptionsFor(id)`

### getJustification()
Uses `ApiHelpers::getJustification()` to convert the `Alignment` string into a JUCE `Justification`.

### Deactivated base properties

`handleDefaultDeactivatedProperties()` disables some ScriptComponent properties for ScriptLabel:
- `defaultValue`
- `min`
- `max`
- `automationId`

These are removed from active property lists and should not be relied on for ScriptLabel.

## Value handling and overrides

### getValue()
Returns the `text` property value.

### setValue(var newValue)
Behavior:
- If `newValue.isString()` then sets the `text` property and triggers async UI update.
- Non-string values are ignored (no error, no change).
- `jassert(newValue != "internal")` to avoid sentinel recursion.

### resetValueToDefault()
Calls `setValue("")`.

### setScriptObjectPropertyWithChangeMessage()
If the property ID is `text`, it calls `setValue(newValue.toString())` before delegating to base class.
This forces text updates to go through the ScriptLabel string-only path.

### isClickable()
Returns `Editable && ScriptComponent::isClickable()`.
If not editable, the label will not accept clicks even if visible/enabled.

## UI wrapper: ScriptCreatedComponentWrappers::LabelWrapper

Wrapper class is in `ScriptComponentWrappers.h/.cpp`.

### Component instantiation
- Uses `MultilineLabel` as the underlying UI component.
- Adds itself as `LabelListener` and `TextEditor::Listener`.
- Calls `initAllProperties()` and `updateValue(getValue())`.

### Property updates
`updateComponent(int propertyIndex, var newValue)` handles:
- `tooltip`: `Label::setTooltip`
- `bgColour`, `itemColour`, `itemColour2`, `textColour`: `updateColours()`
- `FontName`, `FontSize`, `FontStyle`, `Alignment`: `updateFont()`
- `Editable`: `updateEditability()`
- `Multiline`: `Label::setMultiline(newValue)`
- `SendValueEachKeyPress`: updates internal `sendValueEachKey` flag

### updateFont()
Behavior based on `FontName` and `FontStyle`:
- If `FontName` is "Oxygen" or "Default":
  - "Bold" uses `GLOBAL_BOLD_FONT()`
  - otherwise `GLOBAL_FONT()`
- If `FontName` is "Source Code Pro": uses `GLOBAL_MONOSPACE_FONT()`
- Otherwise: tries `MainController::getFont(fontName)` (custom fonts). If missing, constructs `Font(fontName, fontStyle, fontSize)`.
- If `FontStyle` is "Password", `MultilineLabel::setUsePasswordCharacter(true)` is enabled.
- Always applies justification from `ScriptLabel::getJustification()`.

### updateEditability()
- Updates text, intercepts mouse clicks, sets editable and multiline.
- `MultilineLabel::setEditable(editable)` controls text editor state.

### Value change callbacks
`labelTextChanged(Label* l)`:
- Calls `ScriptLabel::setValue(l->getText())`.
- Calls `ProcessorWithScriptingContent::controlCallback(component, value)`.

### Send value on each key press
`SendValueEachKeyPress` property drives a `ValueChecker` timer:
- When the editor is shown and `sendValueEachKey` is true, a `ValueChecker` is created.
- `ValueChecker` runs every 300 ms, compares editor text, and triggers `setValue()` and `controlCallback()` on change.
- On editor hidden, key listener is removed and `valueChecker` is cleared.

### Focus grab behavior
`wantsToGrabFocus()` checks `Editable` and then calls:
- `MultilineLabel::showEditor()`
- `MultilineLabel::grabKeyboardFocusAsync()`

## Preset serialization

### ScriptLabel::exportAsValueTree()
Adds a `value` property with `getValue()` to the base ScriptComponent tree.

### ScriptLabel::restoreFromValueTree()
Calls `setValue(v.getProperty("value", ""))`.

### Content::restoreAllControlsFromPreset()
Special handling for ScriptLabel:
- `allowStrings = (component is ScriptLabel)` is passed to `Helpers::getCleanedComponentValue()`.
- After restore, ScriptLabel always calls `controlCallback(component, v)` rather than `setAttribute()`.

This is the main integration point where ScriptLabel string values are preserved in presets.

## Threading and lifecycle constraints

`setEditable(bool)`:
- Checks `parent->allowGuiCreation` and reports a script error if called after onInit.
- Enforces the same Content lifecycle rules used by component creation.

Other base class constraints from ScriptComponent still apply, but ScriptLabel adds the string-only value semantics for `setValue` and `getValue`.

## Helper classes and utilities

- `ApiHelpers::getJustificationNames()` and `ApiHelpers::getJustification()` for alignment strings.
- `MainController::fillWithCustomFonts()` and `MainController::getFont()` for custom font registration.
- `MultilineLabel` and `TextEditor` from UI layer are the actual widget implementation.
- `ProcessorWithScriptingContent::controlCallback()` used for label value change dispatch.

## Preprocessor guards

No ScriptLabel-specific preprocessor guards found in the examined sources.

## Constants and enums

No `addConstant()` calls and no typed constant registration. ScriptLabel only defines the local `Properties` enum for property indices.

## Upstream data providers

- `Content` owns ScriptLabel instances and enforces GUI creation window (`allowGuiCreation`).
- `ScriptContentComponent` hosts the UI component wrappers and routes property updates to `LabelWrapper`.
- `MainController` provides the font registry used for `FontName` resolution and custom fonts.
- Preset ValueTree data is supplied by `Content::restoreAllControlsFromPreset()`; ScriptLabel opts into string-safe restore via `allowStrings` and uses `controlCallback` for event propagation.

## Enum and mode behavioral tracing

`FontStyle` value "Password":
- Consumed by `LabelWrapper::updateFont()` which calls `setUsePasswordCharacter(true)`.
- Other style strings are passed to JUCE `Font` style selection.

`FontName` values:
- "Default" and "Oxygen" map to global app fonts.
- "Source Code Pro" maps to `GLOBAL_MONOSPACE_FONT()`.
- Other strings resolve to custom or system fonts via `MainController::getFont()` and `Font(fontName, fontStyle, fontSize)` fallback.

`Alignment` string values:
- Mapped to JUCE `Justification` via `ApiHelpers::getJustification()` and applied in `updateFont()`.

`SendValueEachKeyPress`:
- When true, the wrapper attaches a timer that periodically reads the editor text and dispatches control callbacks while editing.

`Editable`:
- Gates `isClickable()` and controls whether the wrapper opens a text editor or intercepts mouse clicks.
