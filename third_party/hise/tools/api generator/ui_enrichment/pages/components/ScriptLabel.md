---
title: "Label"
description: "Text display and input component with editable, multiline, and per-keystroke callback support."
componentId: "ScriptLabel"
componentType: "plugin-component"
screenshot: "/images/v2/reference/ui-components/label.png"
llmRef: |
  ScriptLabel (UI component)
  Create via: Content.addLabel("name", x, y)
  Scripting API: $API.ScriptLabel$

  Editable text label component for displaying and editing interface text.
  Its value is the label text itself (a string, not a number).

  Properties (component-specific):
    fontName: font family for the label text
    fontSize: font size in pixels
    fontStyle: font style (plain, bold, italic, bold italic)
    alignment: text alignment within the component bounds
    editable: whether the user can click to edit the text
    multiline: enable multi-line text with automatic line wrapping
    updateEachKey: fire callbacks on each keystroke (for live search)

  Customisation:
    LAF: none
    CSS: label, input, ::selection with :hover, :active, :disabled
    Filmstrip: no

seeAlso: []
commonMistakes:
  - title: "Calling setEditable() after onInit"
    wrong: "Calling setEditable() outside of the onInit callback"
    right: "Set the editable property during onInit only"
    explanation: "setEditable() reports a script error and has no effect when called after initialisation. The editable state must be configured at component creation time."
  - title: "Passing non-string values to setValue()"
    wrong: "label.setValue(42) or label.setValue(true)"
    right: "label.setValue(\"my text\") — always pass a string"
    explanation: "The label's value is a string (the displayed text). Non-string values are silently ignored."
  - title: "Updating text without calling changed()"
    wrong: "label.setValue(\"new text\") and expecting onControl to fire"
    right: "label.setValue(\"new text\"); label.changed();"
    explanation: "setValue() does not trigger the onControl callback. Call changed() explicitly when other logic should react to the text update."
  - title: "Display label text not persisting on DAW session reopen"
    wrong: "Using set(\"text\", Engine.getCurrentUserPresetName()) in onInit and expecting it to survive session reload"
    right: "Restore the label text in the onPresetLoaded callback or use saveInPreset with a dedicated display label"
    explanation: "Labels with saveInPreset=false lose their text when the DAW session is reopened. For display-only labels showing dynamic info (e.g. preset names), restore the text in an appropriate callback after state is reloaded."
---

![Label](/images/v2/reference/ui-components/label.png)

ScriptLabel is a text display and input component. Its value is the label text itself — a string, not a number — making it suitable for captions, editable text fields, search boxes, and other lightweight text input tasks.

The component ranges from purely declarative use (static captions with font and alignment settings) to interactive text input with per-keystroke callbacks for live search or incremental filtering. Set the font, alignment, and editability properties in the Interface Designer for the common case; use scripting only for dynamic text updates or to react to user edits.

## Properties

Set properties with `ScriptLabel.set(property, value)`.

### Component-specific properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| *`fontName`* | String | `"Arial"` | Font family for the label text. Use fonts registered in the project or system fonts. |
| *`fontSize`* | double | `13` | Font size in pixels. |
| *`fontStyle`* | String | `"plain"` | Font style: `"plain"`, `"bold"`, `"italic"`, or `"bold italic"`. |
| *`alignment`* | String | `"centred"` | Text alignment within the component bounds: `"left"`, `"right"`, `"centred"`, `"centredTop"`, `"centredBottom"`, `"topLeft"`, `"topRight"`, `"bottomLeft"`, `"bottomRight"`. |
| *`editable`* | bool | `false` | When enabled, the user can click the label to enter edit mode and type text. Must be set during `onInit` — calling `setEditable()` after `onInit` triggers a script error. |

> [!Warning:Text input position ignores alignment property] When an editable label enters edit mode, the text input jumps to a default top-left position regardless of the `alignment` setting. Use the CSS `input` selector with `text-align` and `padding` properties to align the text editor to match the label's visual alignment.

| *`multiline`* | bool | `false` | Enable multi-line text with automatic line wrapping. |
| *`updateEachKey`* | bool | `false` | When enabled, fires the control callback on each keystroke rather than only when editing is confirmed. Use for live-search or incremental filtering scenarios. |

### Common properties

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `text` | The displayed text (also the component's value) |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `tooltip` | Hover tooltip text |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup` | DAW automation |
| `macroControl`, `isMetaParameter`, `linkedTo` | Macro control and parameter linking |
| `processorId`, `parameterId` | Module parameter connection |

### Deactivated properties

The following properties are deactivated for ScriptLabel and have no effect:

`defaultValue`, `min`, `max`, `automationID`.

## CSS Styling

CSS provides full control over the label's appearance, including the text editor and selection styles. The primary selector is `label`, but additional selectors target the editing and selection states.

> [!Tip:CSS is the only custom styling path for labels] ScriptLabel has no LAF (Look and Feel) functions — this is a recurring source of confusion. Use CSS with the `label`, `input`, and `::selection` selectors for all visual customisation beyond the built-in properties.

> **Note:** The `label` selector is shared globally — it also styles popup text overlays on Sliders, Tables, and SliderPacks. The `input` selector also styles text input elements on other components (e.g. shift-clicking a Slider). Keep this in mind when using a shared look-and-feel object.

### Selectors

| Selector | Type | Description |
|----------|------|-------------|
| `label` | HTML tag | Selects all label elements (including popup overlays on other components) |
| `.scriptlabel` | Class | Default class selector for ScriptLabel |
| `#Label1` | ID | Targets a specific label by component name |
| `input` | HTML tag | Styles the text editor while the user is typing |

### Pseudo-states

| State | Description |
|-------|-------------|
| `:hover` | Mouse is over the component |
| `:active` | Mouse button is pressed |
| `:disabled` | Component is disabled |

### Pseudo-elements

| Element | Description |
|---------|-------------|
| `::selection` | Styles the text selection highlight (background and text colour) |

### CSS Variables

| Variable | Description |
|----------|-------------|
| `--bgColour` | Background colour from the `bgColour` property |
| `--textColour` | Text colour from the `textColour` property |
| `--itemColour` | From the `itemColour` property |
| `--itemColour2` | From the `itemColour2` property |

### Example Stylesheet

```javascript
const var l = Content.addLabel("Label1", 10, 10);

l.set("textColour", 0xFFEEEEEE);
l.set("bgColour", Colours.darkgoldenrod);

const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("

*
{
	letter-spacing: 2px;
	font-weight: bold;
}

/** Render the default appearance. */
label
{
	background-color: var(--bgColour);
	color: var(--textColour);	
	border-radius: 3px;
}

/** If you edit the text, it will use this selector. */
input
{
	background: red;
	text-align: center;
	padding-top: 0.5px;
	padding-left: 2px;
	caret-color: blue;
}

/** Style the text selection with this selector. */
::selection
{
	background: green;
	color: white;
}
");

l.setLocalLookAndFeel(laf);
```

## Notes

> [!Warning:Do not set label text from audio callbacks] Calling `set("text", ...)` from `onNoteOn` or `onController` triggers an "Illegal operation in audio thread: String creation" error because label values are strings and string allocation is forbidden on the audio thread. Use `Synth.deferCallbacks(true)` or set the text from a deferred context (e.g. a timer callback or control callback).

- **Use `grabFocus()` for immediate keyboard input.** Call `grabFocus()` on an editable label to give it keyboard focus programmatically — e.g. after a button press opens a rename dialog. The user can start typing immediately without clicking the label first.
- **The label's value is a string.** Unlike other components, `getValue()` returns the displayed text, not a number. Passing non-string values to `setValue()` will be ignored.
- **`setEditable()` must be called during `onInit`.** Calling it after initialisation reports a script error and has no effect.
- **Call `changed()` after programmatic text updates** when other logic should react as if the user edited the field. Without `changed()`, the `onControl` callback does not fire.
- **Use `updateEachKey` sparingly.** Only enable it for live-search or incremental filtering scenarios to avoid unnecessary callback overhead.
- **Use `saveInPreset = false` for transient inputs** like search fields. Keep it `true` only for text that should persist across preset changes.
- **Use `alignment = "left"` for editable text fields** — centred text in an editable label can feel awkward during editing.

**See also:** {placeholder — populated during cross-reference post-processing}
