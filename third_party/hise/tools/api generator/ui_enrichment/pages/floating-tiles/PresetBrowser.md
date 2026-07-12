---
title: "PresetBrowser"
description: "Multi-column user preset browser with search, favourites, tagging, and save/load/rename/delete functionality."
contentType: "PresetBrowser"
componentType: "floating-tile"
screenshot: "/images/v2/reference/ui-components/floating-tiles/presetbrowser.png"
llmRef: |
  PresetBrowser (FloatingTile)
  ContentType string: "PresetBrowser"
  Set via: FloatingTile.set("ContentType", "PresetBrowser")

  A fully-featured preset management interface for loading, saving, renaming, and deleting user presets. Displays a multi-column browser (banks, categories, presets) with search, favourites, tags, and notes. Saves and recalls all interface elements where saveInPreset is true.

  JSON Properties:
    ShowFolderButton: Show the "more" options button (default: true)
    ShowSaveButton: Show the save preset button (default: true)
    ShowNotes: Show the notes bar (default: true)
    ShowEditButtons: Show add/rename/delete buttons (default: true)
    ShowAddButton: Show the add button individually (default: true)
    ShowRenameButton: Show the rename button individually (default: true)
    ShowDeleteButton: Show the delete button individually (default: true)
    ShowSearchBar: Show the search bar (default: true)
    ShowFavoriteIcon: Show the favourite star icon (default: true)
    ShowExpansionsAsColumn: Show expansions as an additional column (default: false)
    NumColumns: Number of columns 1-3 (default: 3)
    ColumnWidthRatio: Array of relative column widths (default: [0.333, 0.333, 0.333])
    EditButtonOffset: Vertical offset for edit buttons in pixels (default: 10)
    ListAreaOffset: Margin offsets for the list area [top, right, bottom, left] (default: [0,0,0,0])
    ColumnRowPadding: Padding between columns and rows [top, right, bottom, left] (default: [0,0,0,0])
    ButtonsInsideBorder: Place buttons inside the border (default: false)
    SearchBarBounds: Custom bounds for search bar [x, y, w, h] (default: auto)
    SaveButtonBounds: Custom bounds for save button [x, y, w, h] (default: auto)
    MoreButtonBounds: Custom bounds for more button [x, y, w, h] (default: auto)
    FavoriteButtonBounds: Custom bounds for favourite button [x, y, w, h] (default: auto)
    FullPathFavorites: Use full path for favourite matching (default: false)
    FavoriteIconOffset: Horizontal offset for the favourite icon (default: 0)

  Customisation:
    LAF: drawPresetBrowserBackground, drawPresetBrowserColumnBackground, drawPresetBrowserListItem, drawPresetBrowserDialog, drawPresetBrowserTag, drawPresetBrowserSearchBar, createPresetBrowserIcons
    CSS: Multiple selectors — see CSS diagrams for full map
seeAlso: []
commonMistakes:
  - title: "Forgetting to set saveInPreset on controls"
    wrong: "Adding a PresetBrowser but leaving saveInPreset as false on controls that should be recalled"
    right: "Set saveInPreset to true on every control whose state should be stored in user presets"
    explanation: "The preset browser saves and recalls the values of all interface elements where saveInPreset is enabled. Controls left at the default (false for panels, true for sliders/buttons) may not persist across preset changes."
  - title: "Using wrong column index in drawPresetBrowserListItem"
    wrong: "Assuming columnIndex 0 = banks, 1 = categories, 2 = presets when NumColumns < 3"
    right: "The preset column always has columnIndex 2 regardless of NumColumns. When NumColumns is 2, columnIndex 0 = categories and 2 = presets."
    explanation: "The column index mapping is fixed — the preset column is always index 2. When fewer columns are shown, earlier columns are hidden but the indices do not shift."
  - title: "Not handling createPresetBrowserIcons return value"
    wrong: "Registering createPresetBrowserIcons like a drawing callback with (g, obj) parameters"
    right: "Register it with a single (id) parameter and return a Path object"
    explanation: "Unlike all other PresetBrowser LAF functions, createPresetBrowserIcons is not a drawing callback. It receives an icon ID string and must return a Path that HISE will use to render the icon."
  - title: "Using setControlCallback to detect preset changes"
    wrong: "Setting a control callback on the preset browser FloatingTile to react when a preset is loaded"
    right: "Use UserPresetHandler.setPostCallback() to get notified after a preset is loaded"
    explanation: "The PresetBrowser floating tile does not fire a control callback when presets are loaded. To react to preset changes (e.g. updating a label with the current preset name), use UserPresetHandler.setPostCallback() instead."
---

![PresetBrowser](/images/v2/reference/ui-components/floating-tiles/presetbrowser.png)

The PresetBrowser floating tile provides a complete user preset management interface. It displays a multi-column browser with banks, categories, and presets, along with search, favourites, tagging, and a notes panel. Users can save, load, rename, and delete presets directly from the browser.

The browser saves and recalls all interface elements that have `saveInPreset` enabled. Connect it to the user preset system — no `ProcessorId` is required as it operates on the global preset state. For full control over its appearance, the preset browser supports seven LAF functions and comprehensive CSS styling.

## Setup

```javascript
const var ft = Content.getComponent("FloatingTile1");

ft.set("ContentType", "PresetBrowser");
ft.set("Data", JSON.stringify({
    "ShowFolderButton": true,
    "ShowSaveButton": true,
    "ShowNotes": false,
    "NumColumns": 3,
    "ColumnWidthRatio": [0.2, 0.3, 0.5]
}));
```

## JSON Properties

Configure via the `Data` property as a JSON string, or set individual properties in the Interface Designer.

### Visibility

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ShowFolderButton` | bool | `true` | Show the "more" options button |
| `ShowSaveButton` | bool | `true` | Show the save preset button |
| `ShowNotes` | bool | `true` | Show the notes bar below the columns |
| `ShowEditButtons` | bool | `true` | Show add/rename/delete buttons (master toggle) |
| `ShowAddButton` | bool | `true` | Show the add button individually |
| `ShowRenameButton` | bool | `true` | Show the rename button individually |
| `ShowDeleteButton` | bool | `true` | Show the delete button individually |
| `ShowSearchBar` | bool | `true` | Show the search bar |
| `ShowFavoriteIcon` | bool | `true` | Show the favourite star icon on presets |
| `ShowExpansionsAsColumn` | bool | `false` | Show expansions as an additional column |

### Layout

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `NumColumns` | int | `3` | Number of visible columns (1, 2, or 3) |
| `ColumnWidthRatio` | Array | `[0.333, 0.333, 0.333]` | Relative width of each column (must sum to 1.0) |
| `EditButtonOffset` | int | `10` | Vertical offset for the edit buttons in pixels |
| `ListAreaOffset` | Array[4] | `[0, 0, 0, 0]` | Margin offsets for the list area `[top, right, bottom, left]` |
| `ColumnRowPadding` | Array[4] | `[0, 0, 0, 0]` | Padding between columns and rows `[top, right, bottom, left]` |
| `ButtonsInsideBorder` | bool | `false` | Place action buttons inside the column border |
| `SearchBarBounds` | Array[4] | `[]` | Custom bounds for the search bar `[x, y, w, h]` (empty = auto) |
| `SaveButtonBounds` | Array[4] | `[]` | Custom bounds for the save button `[x, y, w, h]` (empty = auto) |
| `MoreButtonBounds` | Array[4] | `[]` | Custom bounds for the more button `[x, y, w, h]` (empty = auto) |
| `FavoriteButtonBounds` | Array[4] | `[]` | Custom bounds for the favourite button `[x, y, w, h]` (empty = auto) |

### Favourites

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `FullPathFavorites` | bool | `false` | Use the full preset path for favourite matching (instead of name only) |
| `FavoriteIconOffset` | int | `0` | Horizontal offset for the favourite icon in pixels |

The `ColourData` object can be used to set colours used by the default rendering:

| Colour ID | Description |
|-----------|-------------|
| `bgColour` | Background colour |
| `textColour` | Text colour |
| `itemColour1` | Highlight / accent colour |
| `itemColour2` | Modal background colour |

## LAF Customisation

Register a custom look and feel to control the rendering of this floating tile. The preset browser has seven LAF functions — six drawing callbacks and one icon factory function.

> [!Tip:drawPresetBrowserListItem replaces all columns] Registering `drawPresetBrowserListItem` overrides the default rendering for every column, not just one. If you only draw content for a specific `columnIndex` (e.g. expansion icons), the other columns will appear blank. Always handle all column indices in your function — use `if/else` branches on `obj.columnIndex` to draw each column appropriately.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawPresetBrowserBackground` | Draws the main background of the preset browser |
| `drawPresetBrowserColumnBackground` | Draws the background of each column (called per column) |
| `drawPresetBrowserListItem` | Draws each item in a column list (called per visible item) |
| `drawPresetBrowserDialog` | Draws the modal dialog for rename/delete/save confirmations |
| `drawPresetBrowserTag` | Draws each tag in the tag list (called per tag) |
| `drawPresetBrowserSearchBar` | Draws the search bar area |
| `createPresetBrowserIcons` | Returns a Path for a given icon ID (not a drawing callback) |

### `obj` Properties (shared across drawing functions)

These properties are available in all six drawing callbacks:

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The component bounds |
| `obj.bgColour` | int (ARGB) | Background colour |
| `obj.itemColour` | int (ARGB) | Highlight colour |
| `obj.itemColour2` | int (ARGB) | Modal background colour |
| `obj.textColour` | int (ARGB) | Text colour |

### Additional `obj` properties per function

`drawPresetBrowserBackground` uses only the shared properties above.

#### `drawPresetBrowserColumnBackground`

| Property | Type | Description |
|----------|------|-------------|
| `obj.columnIndex` | int | Column index (0 = banks, 1 = categories, 2 = presets) |
| `obj.text` | String | Placeholder text when the column is empty (e.g. "Add a Category") |

#### `drawPresetBrowserListItem`

> [!Warning:Favourite icon overlaps custom text] When `ShowFavoriteIcon` is enabled, HISE draws the favourite star icon separately on top of the preset column (columnIndex 2). Your custom LAF must add left padding (~30px) to text in the preset column to avoid overlap. Check `obj.columnIndex == 2` and offset accordingly — applying the padding to all columns will waste space in the bank and category columns.

| Property | Type | Description |
|----------|------|-------------|
| `obj.columnIndex` | int | Column index (0 = banks, 1 = categories, 2 = presets) |
| `obj.rowIndex` | int | Row index within the column (starting at 0) |
| `obj.text` | String | The item name (folder or preset name) |
| `obj.selected` | bool | Whether the item is currently selected |
| `obj.hover` | bool | Whether the mouse is over the item |

#### `drawPresetBrowserDialog`

| Property | Type | Description |
|----------|------|-------------|
| `obj.labelArea` | Array[x,y,w,h] | The area for the text input field (inside `obj.area`) |
| `obj.title` | String | The action title (e.g. "Delete Preset") |
| `obj.text` | String | The dialog message text |

#### `drawPresetBrowserSearchBar`

| Property | Type | Description |
|----------|------|-------------|
| `obj.icon` | Path | The search icon as a Path object (ready for `g.fillPath()`) |

#### `drawPresetBrowserTag`

| Property | Type | Description |
|----------|------|-------------|
| `obj.text` | String | The tag name |
| `obj.hover` | bool | Whether the mouse is over the tag |
| `obj.blinking` | bool | Whether the tag is blinking (during tag editing mode) |
| `obj.value` | bool | Whether the current preset has this tag |
| `obj.selected` | bool | Whether the tag is selected as a filter |

#### `createPresetBrowserIcons`

This function is different from the other LAF functions. It takes a single `id` parameter (not `g, obj`) and must return a Path object.

| Parameter | Type | Description |
|-----------|------|-------------|
| `id` | String | The icon identifier: `"favorite_on"`, `"favorite_off"`, or `"searchIcon"` |

**Returns:** A `Path` object that will be used to render the icon.

### Example

```javascript
const var ft = Content.getComponent("FloatingTile1");
const var laf = Content.createLocalLookAndFeel();

// Draw the main background
laf.registerFunction("drawPresetBrowserBackground", function(g, obj)
{
    g.setColour(obj.bgColour);
    g.fillRect(obj.area);
});

// Draw each column background
laf.registerFunction("drawPresetBrowserColumnBackground", function(g, obj)
{
    g.setColour(Colours.withAlpha(obj.bgColour, 0.5));
    g.fillRoundedRectangle(obj.area, 3.0);
    
    // Show placeholder text if the column is empty
    if (obj.text != "")
    {
        g.setColour(Colours.withAlpha(obj.textColour, 0.3));
        g.setFont("Arial", 14.0);
        g.drawAlignedText(obj.text, obj.area, "centred");
    }
});

// Draw each list item
laf.registerFunction("drawPresetBrowserListItem", function(g, obj)
{
    if (obj.selected)
    {
        g.setColour(obj.itemColour);
        g.fillRoundedRectangle(obj.area, 3.0);
    }
    
    if (obj.hover)
    {
        g.setColour(0x10FFFFFF);
        g.fillRoundedRectangle(obj.area, 3.0);
    }
    
    g.setColour(obj.textColour);
    g.setFont("Arial", 14.0);
    g.drawAlignedText(obj.text, [obj.area[0] + 10, obj.area[1], obj.area[2] - 20, obj.area[3]], "left");
});

// Draw the modal dialog (rename, delete, save)
laf.registerFunction("drawPresetBrowserDialog", function(g, obj)
{
    // Darken the background
    g.setColour(0x88000000);
    g.fillRect(obj.area);
    
    // Draw dialog box
    g.setColour(obj.itemColour2);
    g.fillRoundedRectangle(obj.labelArea, 5.0);
    
    g.setColour(obj.textColour);
    g.setFont("Arial Bold", 16.0);
    g.drawAlignedText(obj.title, obj.area, "centredTop");
});

// Draw each tag
laf.registerFunction("drawPresetBrowserTag", function(g, obj)
{
    g.setColour(obj.selected ? obj.itemColour : 0x11FFFFFF);
    g.fillRoundedRectangle(obj.area, 3.0);
    
    if (obj.value)
    {
        g.setColour(obj.textColour);
        g.drawRoundedRectangle(obj.area, 3.0, 1.0);
    }
    
    g.setColour(obj.textColour);
    g.setFont("Arial", 13.0);
    g.drawAlignedText(obj.text, obj.area, "centred");
});

// Draw the search bar
laf.registerFunction("drawPresetBrowserSearchBar", function(g, obj)
{
    g.setColour(0x11FFFFFF);
    g.fillRoundedRectangle(obj.area, 5.0);
    g.setColour(obj.textColour);
    g.fillPath(obj.icon, [obj.area[0] + 5, obj.area[1] + 5, 20, 20]);
});

// Return custom icon paths (optional)
laf.registerFunction("createPresetBrowserIcons", function(id)
{
    // Return a custom path for each icon ID, or undefined to use the default
    // Supported ids: "favorite_on", "favorite_off", "searchIcon"
    return undefined;
});

ft.setLocalLookAndFeel(laf);
```

## CSS Styling

The preset browser and its sub-components can be fully styled using the CSS renderer. Assign a CSS look and feel to the floating tile to enable CSS rendering.

### Selector Map

The preset browser uses the same selectors as the corresponding plugin components for its internal elements. Refer to the CSS diagrams for the full selector map:

**Preset Browser:**

![Preset Browser CSS](/images/custom/preset_css.png)

**Modal Popup:**

![Preset Browser Modal CSS](/images/custom/preset_modal_css.png)

### Key Selectors

| Selector | Description |
|----------|-------------|
| `.modal` | The modal popup overlay (use `padding` to control background size) |
| `button` | Action buttons (save, rename, delete, etc.) — styled like ScriptButton |
| `label` | Text labels — styled like ScriptLabel |
| `.popup` | Context menu from the "More" button — styled like ComboBox popup |
| `.popup-item` | Individual context menu items |

### Notes

- Most CSS positioning properties are ignored — the preset browser handles its own layout. The notable exception is the `padding` property on `.modal`, which controls the modal popup background size.
- Internal buttons and labels follow the same CSS rules as their plugin component equivalents (`button`, `label`).
- The context menu of the **More** button can be styled using the `.popup` and `.popup-item` selectors, just like the ComboBox popup.

### Example Stylesheet

```javascript
const var ft = Content.getComponent("FloatingTile1");
const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("
.modal
{
    background: rgba(0, 0, 0, 0.6);
    padding: 40px;
}

button
{
    background: #555;
    color: white;
    border-radius: 3px;
    padding: 5px 10px;
}

button:hover
{
    background: #666;
}

button:active
{
    background: #444;
}
");

ft.setLocalLookAndFeel(laf);
```

## Notes

- The preset browser operates on the global user preset system — no `ProcessorId` connection is needed.

> [!Tip:Reacting to preset changes] The PresetBrowser does not fire a control callback when presets are loaded. To update your UI when the user selects a preset (e.g. displaying the preset name in a label), use `UserPresetHandler.setPostCallback()` instead of `setControlCallback()` on the floating tile.

- Use `Engine.setUserPresetTagList(["Tag1", "Tag2"])` to define tags before they appear in the browser. This only defines the list of available tags — it does not assign tags to presets. Tags are assigned to individual presets through the browser's "More" menu: click "Edit Tags", then toggle tags on each preset. Tag assignments are stored per-preset in the user preset XML files.
- The `NumColumns` property controls how many columns are visible (1, 2, or 3). When set to 1, only the preset list is shown. When set to 2, categories and presets are shown.
- The `columnIndex` in `drawPresetBrowserListItem` is fixed: the preset column is always index 2, regardless of how many columns are visible.
- The `ColumnWidthRatio` array must have the same number of entries as `NumColumns`, and the values should sum to 1.0.
- Custom bounds properties (`SearchBarBounds`, `SaveButtonBounds`, `MoreButtonBounds`, `FavoriteButtonBounds`) accept `[x, y, w, h]` arrays for pixel-precise positioning. Pass an empty array `[]` for automatic layout.

**See also:** <!-- populated during cross-reference post-processing -->
