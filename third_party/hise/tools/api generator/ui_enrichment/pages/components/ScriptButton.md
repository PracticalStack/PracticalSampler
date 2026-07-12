---
title: "Button"
description: "Toggle or momentary button with filmstrip skins, radio group exclusion, and FloatingTile popup attachment."
componentId: "ScriptButton"
componentType: "plugin-component"
screenshot: "/images/v2/reference/ui-components/button.png"
llmRef: |
  ScriptButton (UI component)
  Create via: Content.addButton("name", x, y)
  Scripting API: $API.ScriptButton$

  Toggle or momentary button with filmstrip image support and radio group mutual exclusion. Binary value (0 or 1) with fixed range.

  Properties (component-specific):
    filmstripImage: filmstrip image path for custom button skin
    numStrips: number of filmstrip frames (typically 2 or 6)
    isVertical: filmstrip orientation
    scaleFactor: filmstrip display scale (for retina)
    radioGroup: radio group ID for mutual exclusion (0 = none)
    isMomentary: momentary mode (value resets to 0 on mouse release)
    setValueOnClick: set value directly on click instead of toggling
    enableMidiLearn: allow MIDI CC learn via right-click
    mouseCursor: mouse cursor style when hovering

  Customisation:
    LAF: drawToggleButton
    CSS: button with :hover, :active, :checked, :disabled
    Filmstrip: yes
seeAlso: []
commonMistakes:
  - title: "Using wrong numStrips value"
    wrong: "Setting numStrips to a value other than 2 or 6"
    right: "Use numStrips = 2 (on/off) or 6 (on/off × normal/hover/down) to match your filmstrip"
    explanation: "An incorrect numStrips value silently falls back to the default skin with no error, making it hard to diagnose."
  - title: "Expecting MIDI learn with saveInPreset disabled"
    wrong: "Setting enableMidiLearn to true while saveInPreset is false"
    right: "Keep saveInPreset enabled when using MIDI learn"
    explanation: "saveInPreset being false disables MIDI learn regardless of the enableMidiLearn setting."
  - title: "setValue without changed() expecting callback to fire"
    wrong: "btn.setValue(1) — callback does not execute"
    right: "btn.setValue(1); btn.changed(); — fires the control callback"
    explanation: "setValue() only updates the stored value. Call changed() afterwards to trigger the control callback, just as a user click would."
  - title: "Incomplete position array in setPopupData"
    wrong: "btn.setPopupData(ft, [10, 20])"
    right: "btn.setPopupData(ft, [10, 20, 200, 100]) — always pass [x, y, w, h]"
    explanation: "setPopupData requires a full [x, y, w, h] array. An incomplete array throws a script error."
---

![Button](/images/v2/reference/ui-components/button.png)

ScriptButton is a toggle or momentary button component with a binary on/off value (0 or 1). It supports filmstrip image skins for fully custom visual appearances, radio group mutual exclusion for option selection, and FloatingTile popup attachment for expandable UI sections.

The button operates in two modes: **toggle** (default), where clicking alternates between on and off, and **momentary** (`isMomentary = true`), where the value is 1 only while the mouse button is held. Use radio groups to create mutually exclusive button sets — all buttons sharing the same `radioGroup` ID automatically deselect when another is selected.

## Properties

Set properties with `ScriptButton.set(property, value)`.

### Component-specific properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| *`filmstripImage`* | String | `""` | Path to a filmstrip image from the project image pool. When set, the button renders using the filmstrip instead of the default skin. |
| *`numStrips`* | String | `"2"` | Number of frames in the filmstrip. Use 2 for simple on/off states, or 6 for on/off × normal/hover/down states. Frame order for 6-strip: off-normal, off-hover, off-down, on-normal, on-hover, on-down. |
| *`isVertical`* | bool | `true` | Filmstrip orientation. Set to `true` for vertically stacked frames, `false` for horizontal. |
| *`scaleFactor`* | double | `1` | Scale factor for the filmstrip image. Use `2` for retina/HiDPI filmstrips that are twice the component size. |
| *`radioGroup`* | int | `0` | Radio group ID. Buttons sharing the same non-zero ID form a mutually exclusive group — selecting one deselects the others. |

> [!Warning:Radio group exclusion cannot be toggled at runtime] The radio group is enforced at the C++ component level and cannot be dynamically disabled (e.g., for shift-click latch patterns). To implement conditional mutual exclusion, leave `radioGroup` at 0 and manage the toggle logic manually with `setValue()` and `changed()` in your callback.
| *`isMomentary`* | int | `0` | Momentary mode. When enabled, the button value is 1 only while the mouse is held down, returning to 0 on release. |
| *`setValueOnClick`* | bool | `false` | When enabled, sets the value directly on click instead of toggling. Useful for radio-group buttons that should not toggle off. |
| *`enableMidiLearn`* | bool | `true` | Allow MIDI CC learn via right-click context menu. Requires `saveInPreset` to be `true`. |
| *`mouseCursor`* | String | `"ParentCursor"` | Mouse cursor style when hovering. Options: `"ParentCursor"`, `"NormalCursor"`, `"PointingHandCursor"`, `"WaitCursor"`, `"CrosshairCursor"`, `"DraggingHandCursor"`, and others. |

### Common properties

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `text`, `tooltip` | Display text and hover tooltip |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `automationID` | DAW automation |
| `macroControl`, `isMetaParameter`, `linkedTo` | Macro control and parameter linking |
| `processorId`, `parameterId` | Module parameter connection |

### Deactivated properties

The following properties are deactivated for ScriptButton and have no effect:

`min`, `max` (button value is always 0 or 1).

## LAF Customisation

Register a custom look and feel to fully control the rendering of this component.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawToggleButton` | Draws the button in all states (on/off, hover, pressed) |

### `obj` Properties

| Property | Type | Description |
|----------|------|-------------|
| `obj.id` | String | The component's ID |
| `obj.area` | Array[x,y,w,h] | The component bounds |
| `obj.enabled` | bool | Whether the button is enabled |
| `obj.text` | String | The button text |
| `obj.over` | bool | Whether the mouse is over the button |
| `obj.down` | bool | Whether the mouse button is pressed |
| `obj.value` | bool | The toggle state (true = on, false = off) |
| `obj.bgColour` | int (ARGB) | Background colour |
| `obj.itemColour1` | int (ARGB) | First item colour |
| `obj.itemColour2` | int (ARGB) | Second item colour |
| `obj.textColour` | int (ARGB) | Text colour |
| `obj.parentType` | String | ContentType of parent FloatingTile (if any) |

### Example

```javascript
const var btn = Content.addButton("Button1", 10, 10);
btn.set("text", "Enable");

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawToggleButton", function(g, obj)
{
    // Draw background
    var alpha = obj.over ? 0.8 : 0.6;
    g.setColour(Colours.withAlpha(obj.bgColour, alpha));
    g.fillRoundedRectangle(obj.area, 4.0);

    // Draw border
    if (obj.down)
    {
        g.setColour(0x33FFFFFF);
        g.fillRoundedRectangle(obj.area, 4.0);
    }

    // Draw text with brightness based on toggle state
    g.setColour(Colours.withAlpha(obj.textColour, obj.value ? 1.0 : 0.4));
    g.setFont("Arial Bold", 12.0);
    g.drawAlignedText(obj.text, obj.area, "centred");
});

btn.setLocalLookAndFeel(laf);
```

## CSS Styling

CSS provides a straightforward way to style buttons using standard pseudo-states. The `:checked` state maps directly to the button's toggle value.

### Selectors

| Selector | Type | Description |
|----------|------|-------------|
| `button` | HTML tag | Selects all button elements |
| `.scriptbutton` | Class | Default class selector for ScriptButton |
| `#Button1` | ID | Targets a specific button by component name |

### Pseudo-states

| State | Description |
|-------|-------------|
| `:hover` | Mouse is over the button |
| `:active` | Mouse button is pressed |
| `:checked` | Button value is true (on) |
| `:disabled` | Component is disabled |

### Pseudo-elements

| Element | Description |
|---------|-------------|
| `::before` | Pseudo-element before the content |
| `::after` | Pseudo-element after the content |

### CSS Variables

| Variable | Description |
|----------|-------------|
| `--bgColour` | Background colour from the `bgColour` property |
| `--itemColour` | From the `itemColour` property |
| `--itemColour2` | From the `itemColour2` property |
| `--textColour` | Text colour from the `textColour` property |

### Example Stylesheet

```javascript
const var b = Content.addButton("Button1", 10, 10);

const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("
button
{
	background: blue;
	border-radius: 3px;
	color: white;
	transition: background-color 0.5s ease-in;
}

button:hover
{
	font-weight: bold;
}

button:active
{
	margin: 2px;
}

button:checked
{
	background: white;
	color: black;
}
");

b.setLocalLookAndFeel(laf);
```

> [!Tip:Call changed() after setValue() to trigger the callback] `setValue()` updates the button's value but does not fire the control callback. Call `btn.changed()` immediately after `btn.setValue(1)` if you need the callback to execute — this is the most common source of "my button setValue isn't working" issues.

## Notes

- **Radio groups** provide mutual exclusion. Set `radioGroup` to the same non-zero integer on multiple buttons to make them behave like radio buttons — selecting one automatically deselects the others. When exposed as plugin parameters, radio group buttons are automatically flagged as meta parameters (because toggling one affects the others in the group).
- **Momentary mode** (`isMomentary = true`) makes the button act like a push button: value is 1 only while the mouse is held, returning to 0 on release.
- **`setValueOnClick`** is useful for radio group buttons that should always activate when clicked, rather than toggling off.
- **Filmstrip images** replace the default skin entirely. A filmstrip is a single image containing multiple frames stacked vertically (or horizontally if `isVertical` is false). Use `numStrips = 2` for on/off, or `numStrips = 6` for on/off × normal/hover/down states. Use `scaleFactor = 2` for retina filmstrips. A custom LAF (`setLocalLookAndFeel`) takes priority over filmstrip rendering — if both are set, only the LAF drawing is used.
- **FloatingTile popups** can be attached via `setPopupData(floatingTile, [x, y, w, h])` to show a floating tile panel when the button is toggled on.
- **`mouseCursor`** changes the cursor when hovering, useful for indicating clickability with `"PointingHandCursor"`.

**See also:** {placeholder — populated during cross-reference post-processing}
