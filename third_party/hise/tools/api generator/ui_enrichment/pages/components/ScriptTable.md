---
title: "Table"
description: "Interactive curve editor for velocity curves, CC response curves, waveshaping functions, and other transfer function mappings."
componentId: "ScriptTable"
componentType: "plugin-component"
screenshot: "/images/v2/reference/ui-components/table.png"
llmRef: |
  ScriptTable (UI component)
  Create via: Content.addTable("name", x, y)
  Scripting API: $API.ScriptTable$

  Table curve editor component for editing transfer curves (velocity, CC, waveshaping). Binds to ExternalData::Table via processorId and tableIndex. Supports internal ownership, processor-slot binding, and shared-table binding via referToData().

  Properties (component-specific):
    tableIndex: table slot index on connected processor (0-based)
    customColours: enable flat design with custom colours (hex colour, non-zero = enabled)

  Customisation:
    LAF: drawTableBackground, drawTablePath, drawTablePoint, drawTableMidPoint, drawTableRuler
    CSS: .scripttable with :hover, :active; .tablepoint, .tablemidpoint, .playhead sub-selectors; --tablePath, --playhead variables
    Filmstrip: no
seeAlso: []
commonMistakes:
  - title: "Passing raw MIDI values to getTableValue"
    wrong: "table.getTableValue(Message.getVelocity()) — input 0-127"
    right: "table.getTableValue(Message.getVelocity() / 127.0) — normalise to 0.0-1.0 first"
    explanation: "getTableValue() expects normalised input (0.0–1.0). Passing unnormalised MIDI values collapses the curve behaviour because only a tiny portion of the table is used."
  - title: "Calling registerAtParent in note callbacks"
    wrong: "Calling registerAtParent() inside onNoteOn or other real-time callbacks"
    right: "Call registerAtParent() once during onInit and cache the returned handle"
    explanation: "registerAtParent() is a setup operation that should run once during initialisation. Calling it repeatedly in note callbacks adds unnecessary overhead."
  - title: "Confusing tableIndex with parameterId"
    wrong: "Using parameterId to select a table slot"
    right: "Use tableIndex to select the table slot, processorId to select the processor"
    explanation: "Table binding uses the complex data path (processorId + tableIndex), not the normal parameter path (processorId + parameterId). parameterId is deactivated for ScriptTable."
  - title: "Setting colour properties without enabling customColours"
    wrong: "table.set('bgColour', 0xFF444444) — colours have no visible effect"
    right: "table.set('customColours', '0xFFFFFFFF'); table.set('bgColour', 0xFF444444);"
    explanation: "Colour properties (bgColour, itemColour, itemColour2) are only used when customColours is set to a non-zero value. The default 3D appearance ignores them entirely."
---

![Table](/images/v2/reference/ui-components/table.png)

ScriptTable is a curve editor component for editing transfer functions in HISE plugin interfaces. It is one of the most commonly used complex components, enabling users to shape velocity curves, CC response curves, waveshaping functions, and other mappings through an interactive drag-and-drop interface.

The table displays a curve defined by draggable control points with adjustable curvature. By default, it starts as a linear curve (y = x). Users click to add points, drag them to reshape the curve, adjust curvature with the mouse wheel, and right-click to remove points. The first and last points are fixed to the left and right edges.

Connect the table to a processor module via `processorId` and select the table slot with `tableIndex` (e.g., the Table Envelope has two tables: 0 = Attack, 1 = Release). For shared data workflows, use `registerAtParent()` to get a data handle for runtime lookups, or `referToData()` to bind to an external table source.

## Properties

Set properties with `ScriptTable.set(property, value)`.

### Component-specific properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| *`tableIndex`* | int | `0` | Selects which table slot on the connected processor to display and edit. Most processors have a single table (index 0), but some like the [Table Envelope](/hise-modules/modulators/envelopes/list/tableenvelope) have multiple (0 = Attack, 1 = Release). You may need to rebuild the interface after changing this value. |
| *`customColours`* | hex String | `"0x00000000"` | When set to a non-zero colour value, enables flat design rendering with the component's colour properties (`bgColour`, `itemColour`, `itemColour2`). When zero (transparent), uses the default 3D table appearance. |

> [!Warning:Enable customColours before setting colour properties] Setting `bgColour`, `itemColour`, or `itemColour2` has no visible effect unless `customColours` is set to a non-zero value first. The default 3D table appearance ignores all colour properties. Set `customColours` to any opaque colour (e.g., `"0xFFFFFFFF"`) to switch to flat design mode.

### Common properties

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `text`, `tooltip` | Display text and hover tooltip |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `bgColour`, `itemColour`, `itemColour2` | Colour properties (used when `customColours` is enabled) |
| `parentComponent` | Parent component for layout nesting |
| `saveInPreset`, `useUndoManager`, `deferControlCallback` | Preset persistence, undo, and callback deferral |
| `processorId` | Connected processor module ID |

### Deactivated properties

The following properties are deactivated for ScriptTable:

`min`, `max`, `defaultValue`, `textColour`, `parameterId`, `macroControl`, `linkedTo`, `isMetaParameter`, `isPluginParameter`, `pluginParameterName`, `automationID`.

## LAF Customisation

Register a custom look and feel to control the rendering of all table elements. All five functions should typically be registered together on the same LAF object for a consistent appearance.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawTableBackground` | Draws the background of the table editor |
| `drawTablePath` | Draws the curve path |
| `drawTablePoint` | Draws each draggable control point (called per point) |
| `drawTableMidPoint` | Draws each mid-point curve control (called per segment) |
| `drawTableRuler` | Draws the vertical position ruler showing the last input value |

### `obj` Properties (shared across all functions)

| Property | Type | Description |
|----------|------|-------------|
| `obj.id` | String | The component's ID |
| `obj.enabled` | bool | Whether the table is enabled |
| `obj.bgColour` | int (ARGB) | Background colour |
| `obj.itemColour` | int (ARGB) | Fill colour |
| `obj.itemColour2` | int (ARGB) | Line colour |
| `obj.textColour` | int (ARGB) | Ruler colour |
| `obj.parentType` | String | ContentType of parent FloatingTile (if any) |

### Additional `obj` properties per function

#### `drawTableBackground`

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The table area |
| `obj.position` | double | The ruler position (0.0–1.0) |

#### `drawTablePath`

| Property | Type | Description |
|----------|------|-------------|
| `obj.path` | Path | The curve path object |
| `obj.area` | Array[x,y,w,h] | The table area |
| `obj.lineThickness` | double | The line thickness for drawing |

#### `drawTablePoint`

| Property | Type | Description |
|----------|------|-------------|
| `obj.tablePoint` | Array[x,y,w,h] | The point bounds |
| `obj.isEdge` | bool | Whether this is a start or end edge point (larger than mid points) |
| `obj.hover` | bool | Whether the mouse is over the point |
| `obj.clicked` | bool | Whether the point is being dragged |

#### `drawTableMidPoint`

| Property | Type | Description |
|----------|------|-------------|
| `obj.midPoint` | Array[x,y,w,h] | The mid-point bounds |
| `obj.hover` | bool | Whether the mouse is over the point |
| `obj.clicked` | bool | Whether the point is being dragged |

#### `drawTableRuler`

| Property | Type | Description |
|----------|------|-------------|
| `obj.area` | Array[x,y,w,h] | The table area |
| `obj.position` | double | The normalised ruler position (0.0–1.0) |
| `obj.lineThickness` | double | The line thickness for drawing |

### Example

```javascript
const var table = Content.addTable("Table1", 10, 10);
table.set("width", 300);
table.set("height", 100);

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawTableBackground", function(g, obj)
{
    g.setColour(obj.bgColour);
    g.fillRoundedRectangle(obj.area, 3.0);
});

laf.registerFunction("drawTablePath", function(g, obj)
{
    // Fill under the curve
    g.setColour(Colours.withAlpha(obj.itemColour, 0.3));
    g.fillPath(obj.path, obj.area);

    // Draw the curve line
    g.setColour(obj.itemColour);
    g.drawPath(obj.path, obj.area, obj.lineThickness);
});

laf.registerFunction("drawTablePoint", function(g, obj)
{
    g.setColour(obj.hover ? Colours.white : obj.itemColour2);
    g.fillEllipse(obj.tablePoint);

    if (obj.isEdge)
    {
        g.setColour(Colours.withAlpha(obj.itemColour2, 0.5));
        g.drawEllipse(obj.tablePoint, 1.0);
    }
});

laf.registerFunction("drawTableMidPoint", function(g, obj)
{
    g.setColour(obj.hover ? Colours.white : obj.itemColour2);
    g.fillEllipse(obj.midPoint);
});

laf.registerFunction("drawTableRuler", function(g, obj)
{
    var x = obj.position * obj.area[2] + obj.area[0];
    g.setColour(Colours.withAlpha(obj.textColour, 0.3));
    g.drawLine(x, x, obj.area[1], obj.area[1] + obj.area[3], 8.0);
    g.setColour(obj.textColour);
    g.drawLine(x, x, obj.area[1], obj.area[1] + obj.area[3], 1.0);
});

table.setLocalLookAndFeel(laf);
```

> [!Tip:Use Broadcaster for table change notifications] The ScriptTable control callback does not fire when the user edits the curve — it only fires on compilation. To react to user edits in real time, use `Broadcaster.attachToComplexData("Table.Content", processorId, tableIndex)` and register a listener function.

## CSS Styling

CSS provides full control over the table appearance including the curve path, control points, mid-points, playhead, and text overlay. The table uses the `.scripttable` class selector (not the `table` HTML tag, which is reserved for Viewport table mode).

### Selectors

| Selector | Type | Description |
|----------|------|-------------|
| `.scripttable` | Class | Default class selector for table components |
| `#Table1` | ID | Targets a specific table by component name |

### Pseudo-states

| State | Description |
|-------|-------------|
| `:hover` | Mouse is over the table |
| `:active` | Mouse button is pressed |

### Pseudo-elements

| Element | Description |
|---------|-------------|
| `::before` | Used for rendering the table path via `background-image: var(--tablePath)` |

### CSS Variables

| Variable | Description |
|----------|-------------|
| `--tablePath` | Base64-encoded path of the table curve — use as `background-image` |
| `--playhead` | Normalised x-position (0.0–1.0) of the position ruler |
| `--bgColour` | Background colour from component properties |
| `--itemColour` | From the `itemColour` property |
| `--itemColour2` | From the `itemColour2` property |
| `--textColour` | Text colour from component properties |

### Sub-selectors

| Selector | Description |
|----------|-------------|
| `.tablepoint` | Draggable control points on the curve |
| `.tablemidpoint` | Curve control mid-points between control points |
| `.playhead` | Position indicator (use `::before` with `--playhead` variable) |
| `label` | Text overlay popup |

### Point pseudo-states

| State | Description |
|-------|-------------|
| `.tablepoint:hover` | Mouse over a control point |
| `.tablepoint:active` | Dragging a control point |
| `.tablepoint:first-child` | The first (left edge) point |
| `.tablepoint:last-child` | The last (right edge) point |

### Example Stylesheet

```javascript
const var t = Content.addTable("Table1", 10, 10);
t.set("width", 300);
t.set("height", 100);

const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("

.scripttable
{
	background: #444;
	border-radius: 3px;
}

.scripttable::before
{
	content: '';
	background-image: var(--tablePath);
	background-color: #aaa;
	box-shadow: inset 0px 2px 4px rgba(0,0,0, 0.5);
}

.tablepoint
{
	background: rgba(255,255,255, 0.5);
	border-radius: 50%;
	margin: 2px;
	box-shadow: 0px 1px 2px rgba(0, 0, 0, 0.3);
}

.tablepoint:hover
{
	background: white;
}

.tablemidpoint
{
	border-radius: 50%;
	background-color: rgba(255, 255, 255, 0.4);
	margin: 2px;
}

.tablemidpoint:hover
{
	background-color: white;
}

label
{
	background: rgba(255, 255, 255, 0.9);
	color: #222;
	border: 1px solid #aaa;
	padding: 5px;
}

.playhead::before
{
	content: '';
	width: 2px;
	left: calc(calc(var(--playhead) * 100%) - 1px);
	background: white;
	box-shadow: 0px 0px 4px black;
}
");

t.setLocalLookAndFeel(laf);
```

## Mouse Handling Properties

The table editor's drag interaction behaviour can be extensively customised using `setMouseHandlingProperties()`. Pass a JSON object with any combination of the properties below. This was introduced to allow detailed customisation of point editing, edge locking, snap grids, and curve control without breaking backwards compatibility — all properties are optional and unrecognised keys are silently ignored.

### Supported Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `margin` | double | `0` | Adds a margin (in pixels) around the table area so that edge points are not visually clipped |
| `fixLeftEdge` | double | `-1` | Lock the first point's y-value to a normalised position. Values > -0.5 enable the lock (e.g., `0.0` = bottom, `0.5` = middle, `1.0` = top). Set to `-1` to allow free dragging |
| `fixRightEdge` | double | `-1` | Lock the last point's y-value to a normalised position. Same semantics as `fixLeftEdge` |
| `syncStartEnd` | bool | `false` | When `true`, dragging one edge point mirrors the y-value change to the opposite edge |
| `allowSwap` | bool | `true` | When `true`, points can cross each other on the x-axis when dragged. Set to `false` to constrain each point between its neighbours |
| `numSteps` | int | `-1` | Build an evenly spaced snap grid with this many steps (including positions at 0.0 and 1.0). Set to `-1` to disable the grid. Overrides any snap positions set via `setSnapValues()` |
| `snapWidth` | double | `10` | Snap capture tolerance in pixels — how close a dragged point must be to a grid line before it snaps. Only effective when `numSteps` > 0 or custom snap values are set |
| `endPointSize` | int | `12` | Size of the edge point handles (first and last points) in pixels |
| `dragPointSize` | int | `18` | Size of regular drag point handles in pixels |
| `midPointSize` | int | `0` | Size of mid-point (curve control) handles in pixels. The default `0` hides them entirely — set to a positive value to show draggable curve handles between control points |
| `useMouseWheelForCurve` | bool | `false` | When `true`, the mouse wheel adjusts curve tension. This replaces the compile-time `HISE_USE_MOUSE_WHEEL_FOR_TABLE_CURVE` flag (the default value is picked up from that constant for backwards compatibility) |
| `closePath` | bool | `true` | When `true`, the path is closed to the baseline for filled area rendering. Set to `false` for a line-only curve display |

### Example

```javascript
const var table = Content.addTable("Table1", 10, 10);
table.set("width", 300);
table.set("height", 100);

// Configure a step-locked curve editor with visible mid-points
table.setMouseHandlingProperties({
    margin: 10,
    fixLeftEdge: 0.5,
    fixRightEdge: 0.5,
    syncStartEnd: false,
    endPointSize: 12,
    dragPointSize: 18,
    midPointSize: 12,
    allowSwap: true,
    numSteps: 12,
    snapWidth: 10,
    useMouseWheelForCurve: false,
    closePath: false
});
```

> [!Warning:Mouse wheel curve editing is off by default in exported plugins] The `useMouseWheelForCurve` property defaults to `false` in exported plugins. Without it, users cannot adjust segment curvature with the mouse wheel — a frequent source of confusion. In the HISE workspace, Cmd+Mousewheel (macOS) or Ctrl+Mousewheel (Windows) works regardless of this setting, but exported plugins require explicit opt-in via `setMouseHandlingProperties({ useMouseWheelForCurve: true })`.

### Behaviour Notes

- When both `fixLeftEdge` and `fixRightEdge` are set (> -0.5), the table internally calls `Table.setStartAndEndY()` to lock both endpoints.
- `numSteps` replaces the current snap list with an evenly spaced grid. For custom non-uniform snap positions, use `setSnapValues()` instead.
- `midPointSize` defaults to `0`, meaning curve control nodes are hidden. Setting it to a positive value reveals draggable mid-points between control points for adjusting curvature. Double-clicking a mid-point resets its curve to 0.5.
- When `allowSwap` is `false`, each point is constrained to the x-range between its neighbours — useful for preventing accidental reordering in step-sequencer-style curves.
- Unknown keys in the JSON object are silently ignored — no error is reported, so check spelling carefully.

## Notes

- **Table interaction:** Click anywhere on the curve to add a point. Drag points to reshape the curve. Hover over a point and use the mouse wheel to adjust curvature (in the HISE main workspace, use Cmd + Mousewheel to avoid accidental changes while scrolling). Right-click on a point to remove it.
- **Normalised lookup domain.** `getTableValue()` expects input normalised to 0.0–1.0 and returns output in the same range. Always normalise MIDI values before lookup (e.g., `velocity / 127.0`).
- **Complex data binding** uses `processorId` + `tableIndex`, not the normal `processorId` + `parameterId` path. The `parameterId` property is deactivated for this component.
- **Register data handles during init.** Call `registerAtParent()` once in `onInit` and cache the returned `ScriptTableData` handle. Use this handle's `getTableValueNormalised()` for efficient lookups in note callbacks, rather than calling the component's `getTableValue()` directly.

> [!Warning:Avoid linking multiple processors to one ScriptTable via linkTo] Chaining `referToData()` on the ScriptTable with `linkTo()` on multiple table processors (e.g., linking 3 Table Envelopes to one shared table) can cause crashes during MIDI events or preset saving. Instead, connect the ScriptTable to one processor via `processorId`, and use `Broadcaster.attachToComplexData` to propagate curve changes to the other processors programmatically.

- **Customise interaction behaviour** — see **Mouse Handling Properties** above for the full reference on configuring drag behaviour, edge locking, snap grids, and curve control handles.
- **Suppress drag popups** in dense editor layouts by calling `setTablePopupFunction(false)` or providing an empty formatter function.

**See also:** <!-- populated during cross-reference post-processing -->
