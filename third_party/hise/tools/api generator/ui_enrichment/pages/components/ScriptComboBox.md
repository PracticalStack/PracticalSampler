---
title: "ComboBox"
description: "Drop-down selector with a newline-separated items list, custom popup rendering, and 1-based index value."
componentId: "ScriptComboBox"
componentType: "plugin-component"
screenshot: "/images/v2/reference/ui-components/combobox.png"
llmRef: |
  ScriptComboBox (UI component)
  Create via: Content.addComboBox("name", x, y)
  Scripting API: $API.ScriptComboBox$

  Drop-down list component for selecting from named items using 1-based integer indexing. Supports custom popup menus with submenus, headers, separators, and disabled items.

  Properties (component-specific):
    items: newline-separated list of selectable items
    fontName: font family for the displayed text
    fontSize: font size in pixels
    fontStyle: font style (plain, bold, italic, bold italic)
    enableMidiLearn: allow MIDI CC learn via right-click
    popupAlignment: popup menu position (bottom, top, topRight, bottomRight)
    useCustomPopup: enable advanced popup syntax with headers, submenus, separators

  Customisation:
    LAF: drawComboBox (closed box only — popup uses drawPopupMenuBackground, drawPopupMenuItem)
    CSS: select with :hover, :active, :disabled; .popup, .popup-item, hr for dropdown
    Filmstrip: no
seeAlso: []
commonMistakes:
  - title: "Using value directly as array index"
    wrong: "array[value] — combo box values are 1-based floats"
    right: "array[parseInt(value) - 1] for 0-based array access"
    explanation: "ComboBox values start at 1 (not 0) and arrive as floats in callbacks. Use parseInt() and subtract 1 for array indexing."
  - title: "Setting value to 0 to select first item"
    wrong: "cb.setValue(0) — value 0 means nothing selected"
    right: "cb.setValue(1) to select the first item"
    explanation: "Value 0 shows the placeholder text property. The first selectable item is index 1."
  - title: "Using comma-separated items"
    wrong: "cb.set(\"items\", \"A,B,C\")"
    right: "cb.set(\"items\", \"A\\nB\\nC\") — items must be newline-separated"
    explanation: "Comma-separated text creates a single item containing the full string. Use newline characters to separate items."
  - title: "Preset breaks after adding items to the list"
    wrong: "Adding new items to a combobox that has saveInPreset enabled"
    right: "For stable lists, only append new items at the end. For dynamic lists, set saveInPreset = false and persist by name"
    explanation: "Presets store the numeric index, not the item text. Inserting or reordering items shifts all indices, causing existing presets to select the wrong item."
  - title: "Styling only drawComboBox without popup functions"
    wrong: "Registering only drawComboBox and expecting the dropdown to match"
    right: "Also register drawPopupMenuBackground and drawPopupMenuItem for consistent styling"
    explanation: "The popup menu uses separate LAF functions. Without them, the dropdown renders with the default skin while the closed box uses your custom style."
---

![ComboBox](/images/v2/reference/ui-components/combobox.png)

ScriptComboBox is a drop-down list component for selecting from a set of named items. Its value is a 1-based integer index corresponding to the selected item. Items are stored as a newline-separated string in the `items` property.

The component supports dynamic item management via `addItem()`, host automation via plugin parameters, and an advanced popup syntax for creating structured menus with section headers, submenus, separators, and disabled entries. When `useCustomPopup` is enabled, the item string supports special markup:

- `**HeaderText**` — non-selectable section header
- `___` — separator line
- `Category::ItemName` — submenu grouping
- `~~DisabledItem~~` — greyed-out disabled entry

Headers and separators do not consume selection indices — the selected value counts only real selectable items.

## Properties

Set properties with `ScriptComboBox.set(property, value)`.

### Component-specific properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| *`items`* | String | `""` | Newline-separated list of selectable items. Setting this auto-updates the value range (`max` is set to the item count). |

> [!Warning:Preset indices break when items change between versions] The combobox saves only the numeric index in presets. If you add, remove, or reorder items between plugin updates, existing presets will select the wrong item. For dynamically populated lists (e.g., expansion content, user samples), set `saveInPreset = false` and persist the selection by name using a separate mechanism.
| *`fontName`* | String | `"Default"` | Font family for the displayed text. Use fonts registered in the project or system fonts. |
| *`fontSize`* | double | `13` | Font size in pixels. |
| *`fontStyle`* | String | `"plain"` | Font style: `"plain"`, `"bold"`, `"italic"`, or `"bold italic"`. |
| *`enableMidiLearn`* | bool | `false` | Allow MIDI CC learn via right-click context menu. |
| *`popupAlignment`* | String | `"bottom"` | Controls where the popup menu appears relative to the combobox: `"bottom"`, `"top"`, `"topRight"`, `"bottomRight"`. |
| *`useCustomPopup`* | bool | `false` | Enable advanced popup syntax for headers (`**Header**`), separators (`___`), submenus (`Category::Item`), and disabled items (`~~Item~~`). |

### Common properties

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `text`, `tooltip` | Display text (shown as placeholder when nothing selected) and hover tooltip |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Colour properties |
| `defaultValue` | Default value (1-based item index) |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `isPluginParameter`, `pluginParameterName`, `pluginParameterGroup`, `automationID` | DAW automation |
| `macroControl`, `isMetaParameter`, `linkedTo` | Macro control and parameter linking |
| `processorId`, `parameterId` | Module parameter connection |

### Deactivated properties

The following properties are deactivated for ScriptComboBox:

`min` (fixed at 1), `max` (auto-managed from item count).

## LAF Customisation

Register a custom look and feel to control the rendering of the closed combobox. The popup menu requires separate LAF functions.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawComboBox` | Draws the closed combobox (the selector bar) |
| `drawPopupMenuBackground` | Draws the background of the dropdown menu (shared global function) |
| `drawPopupMenuItem` | Draws each item in the dropdown menu (shared global function) |

### `obj` Properties — `drawComboBox`

| Property | Type | Description |
|----------|------|-------------|
| `obj.id` | String | The component's ID |
| `obj.area` | Array[x,y,w,h] | The component bounds |
| `obj.text` | String | The currently displayed text (selected item or placeholder) |
| `obj.active` | bool | Whether an item is selected |
| `obj.enabled` | bool | Whether the combobox is enabled and has items |
| `obj.hover` | bool | Whether the mouse is over the component or popup is active |
| `obj.down` | bool | Whether the mouse button is pressed |
| `obj.bgColour` | int (ARGB) | Background colour |
| `obj.itemColour1` | int (ARGB) | First item colour |
| `obj.itemColour2` | int (ARGB) | Second item colour |
| `obj.textColour` | int (ARGB) | Text colour |
| `obj.parentType` | String | ContentType of parent FloatingTile (if any) |

### Example

```javascript
const var cb = Content.addComboBox("ComboBox1", 10, 10);
cb.set("items", "Option A\nOption B\nOption C");

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawComboBox", function(g, obj)
{
    // Draw background
    g.setColour(obj.bgColour);
    g.fillRoundedRectangle(obj.area, 5.0);

    // Draw text
    g.setColour(Colours.withAlpha(obj.textColour, (obj.enabled && obj.active) ? 1.0 : 0.2));
    g.setFont("Arial Bold", 11.0);

    var a = obj.area;
    g.drawAlignedText(obj.text, [a[0] + 10, a[1], a[2] - 10, a[3]], "left");

    // Draw drop-down arrow
    var h = a[3];
    g.fillTriangle([a[0] + a[2] - h/3 - 10, a[1] + h/3, h/3, h/3], Math.PI);
});

cb.setLocalLookAndFeel(laf);
```

## CSS Styling

CSS provides full control over the combobox appearance, including the dropdown popup menu. The combobox uses the `select` tag selector, while the popup menu uses `.popup` and `.popup-item` class selectors.

> **Note:** Popup menu styles (`.popup`, `.popup-item`, `hr`) also apply to right-click menus of other components that share the same CSS look-and-feel object.

> [!Tip:Popup menu width is determined by content, not the combobox] The dropdown popup width is calculated by JUCE based on the widest item text, not constrained to the combobox width. There is no built-in property to limit the popup width. Use CSS `.popup { max-width: ... }` for approximate control, or use a ScriptPanel context menu for full layout control.

### Selectors

| Selector | Type | Description |
|----------|------|-------------|
| `select` | HTML tag | Selects all combobox elements |
| `.scriptcombobox` | Class | Default class selector for ScriptComboBox |
| `#ComboBox1` | ID | Targets a specific combobox by component name |

### Pseudo-states

| State | Description |
|-------|-------------|
| `:hover` | Mouse is over the combobox |
| `:active` | Mouse button is pressed |
| `:disabled` | Component is disabled |

### Pseudo-elements

| Element | Description |
|---------|-------------|
| `::before` | Typically used for the drop-down arrow icon |
| `::after` | Additional pseudo-element |

### CSS Variables

| Variable | Description |
|----------|-------------|
| `--bgColour` | Background colour from the `bgColour` property |
| `--itemColour` | From the `itemColour` property |
| `--itemColour2` | From the `itemColour2` property |
| `--textColour` | Text colour from the `textColour` property |

### Popup menu selectors

The dropdown menu is styled using these additional selectors:

| Selector | Description |
|----------|-------------|
| `.popup` | The popup menu background |
| `.popup-item` | Individual items in the popup menu |
| `.popup-item:hover` | Hovered popup menu item |
| `.popup-item:active` | Currently selected popup menu item |
| `.popup-item:disabled` | Disabled popup menu item |
| `hr` | Separator line between items |

### Example Stylesheet

```javascript
const var cb = Content.addComboBox("ComboBox1", 10, 10);

cb.set("items", "Item1\nItem2\nItem3");

const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("

/** Style the appearance of the combobox. */
select
{
	background: #333;
	border-radius: 3px;
	color: white;
	letter-spacing: 1px;
	font-weight: bold;
	text-align: left;
	padding: 10px;
}

/** Draw the drop down arrow. */
select::before
{
	content: '';
	background-image: \"84.t0lavsBQ76.tCwF..VDQX+9fCw1WJBDQnj.cCwFp5YBQ3NhqCwly0w.QzMCcCwF..d.QTV.gCwFD6YBQpsevCwVtvsBQn.AtCwlavsBQ76.tCMVY\";
	background-color: rgba(255,255,255, 0.4);
	position: absolute;
	width: 100vh;
	margin: 8px;
	right: 0px;
}

select::before:hover
{
	background-color: white;
}

/** Style the popup menu background. */
.popup
{
	background: #333;
}

/** Style individual popup items. */
.popup-item
{
	background: transparent;
	color: #999;
	padding: 10px;
}

.popup-item:hover
{
	background: rgba(255,255,255, 0.2);
}

.popup-item:active
{
	color: white;
	font-weight: bold;
}

.popup-item:disabled
{
	color: #555;
}

/** Style separator lines. */
hr
{
	margin: -10px;
	border: 1px solid #444;
}
");

cb.setLocalLookAndFeel(laf);
```

## Custom Popup Syntax

When `useCustomPopup` is enabled, the items string supports special markup for structured menus:

| Syntax | Effect | Selectable |
|--------|--------|:----------:|
| `Item Name` | Normal item | Yes |
| `**Header**` | Section header (bold, non-clickable) | No |
| `___` | Separator line (three underscores) | No |
| `~~Disabled Item~~` | Greyed-out disabled entry | No |
| `Category::Item` | Item inside a submenu named "Category" | Yes |
| `Category::**Header**` | Header inside a submenu | No |
| `Category::~~Item~~` | Disabled item inside a submenu | No |

Headers and separators do not consume selection indices — the value counts only real selectable items. `getItemText()` strips the submenu prefix, returning only the part after `::`.

### Example

```javascript
const var cb = Content.addComboBox("FxSelector", 0, 0);
cb.set("useCustomPopup", true);
cb.set("saveInPreset", false);

// Build a structured popup with headers, separators, and submenus.
// Headers and separators are skipped when counting selection indices.
cb.set("items", [
    "**Filters**",
    "Filters::LowPass",
    "Filters::HighPass",
    "Filters::BandPass",
    "___",
    "**Dynamics**",
    "Dynamics::Compressor",
    "Dynamics::Gate",
    "___",
    "**Spatial**",
    "Spatial::Chorus",
    "Spatial::Delay",
    "Spatial::Reverb"
].join("\n"));

// Value 1 = "LowPass" (first selectable item, not the header)
// Value 4 = "Compressor" (headers and separators are skipped)
cb.setValue(1);
Console.print(cb.getItemText()); // "LowPass" — submenu prefix stripped
```

## Notes

> [!Tip:Use getItemText() for samplemap loading] The most common combobox pattern is `Sampler.loadSampleMap(samplemaps[parseInt(value) - 1])`. Use `getItemText()` when you need the display name directly, as it handles `useCustomPopup` prefix stripping automatically.

- **Value is 1-based.** The first item has value 1, not 0. Value 0 means no item is selected and shows the `text` property as placeholder text.
- **Use `parseInt(value) - 1` for array indexing.** Values arrive as floats in callbacks (e.g. `1.0`). Use `parseInt(value) - 1` to convert to a 0-based array index.
- **Use `getItemText()` to get the selected item's name** rather than maintaining a parallel array.
- **Clear items before rebuilding.** When populating dynamically with `addItem()`, first clear with `set("items", "")` to remove stale entries from the Interface Designer.
- **Set `saveInPreset = false` for dynamic file lists.** File-scanned item lists differ between machines, so persisting the index is meaningless. Persist the file reference or processor state instead.
- **Call `changed()` after rebuilding dependent combos.** When programmatically updating items and values of dependent comboboxes, call `changed()` to trigger the callback chain.
- **Advanced popup syntax** requires `useCustomPopup = true`. Without it, markup like `**Header**` is displayed literally as item text.

**See also:** {placeholder — populated during cross-reference post-processing}
