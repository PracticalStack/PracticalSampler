---
title: "AHDSRGraph"
description: "Real-time AHDSR envelope visualisation with animated playback position and stage highlighting."
contentType: "AHDSRGraph"
componentType: "floating-tile"
screenshot: "/images/v2/reference/ui-components/floating-tiles/ahdsrgraph.png"
llmRef: |
  AHDSRGraph (FloatingTile)
  ContentType string: "AHDSRGraph"
  Set via: FloatingTile.set("ContentType", "AHDSRGraph")

  Displays a real-time AHDSR envelope graph with an animated ball that tracks the current envelope position. Connect to an AHDSR modulator via ProcessorId.

  JSON Properties:
    ProcessorId: The ID of the connected AHDSR modulator
    Index: Unused for this panel (leave at default)

  Customisation:
    LAF: drawAhdsrBackground, drawAhdsrPath, drawAhdsrBall
    CSS: none

seeAlso: []
commonMistakes:
  - title: "Missing ProcessorId connection"
    wrong: "Adding an AHDSRGraph floating tile without setting the ProcessorId property"
    right: "Set ProcessorId to the ID of the AHDSR modulator you want to visualise"
    explanation: "Without a connected processor, the graph renders empty. The floating tile needs a valid AHDSR modulator reference to display envelope data."
  - title: "Not handling the dual drawAhdsrPath call"
    wrong: "Writing a single drawAhdsrPath implementation without checking obj.isActive"
    right: "Check obj.isActive to distinguish: false = full path outline, true = active section highlight"
    explanation: "drawAhdsrPath is called twice per repaint — once for the full envelope curve and once for the currently active section. Without the isActive check, both calls render identically and the active section highlight is lost."
---

![AHDSRGraph](/images/v2/reference/ui-components/floating-tiles/ahdsrgraph.png)

The AHDSRGraph floating tile displays a real-time visualisation of an AHDSR (Attack, Hold, Decay, Sustain, Release) envelope modulator. It renders the envelope curve as a path and shows an animated ball that tracks the current position within the envelope stages as notes are played.

Connect the floating tile to an AHDSR modulator by setting the `ProcessorId` property to the modulator's ID. The graph updates in real time, highlighting the currently active envelope section.

## Setup

```javascript
const var ft = Content.getComponent("FloatingTile1");

ft.set("ContentType", "AHDSRGraph");
ft.set("Data", JSON.stringify({
    "ProcessorId": "AHDSR Envelope1"
}));
```

## JSON Properties

Configure via the `Data` property as a JSON string, or set individual properties in the Interface Designer.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ProcessorId` | String | `""` | The ID of the connected AHDSR modulator |
| `Index` | int | `-1` | Unused for this content type |

The `ColourData` object can be used to set colours used by the default rendering:

| Colour ID | Description |
|-----------|-------------|
| `bgColour` | Background colour |
| `itemColour1` | Path fill colour |
| `itemColour2` | Active section colour |
| `itemColour3` | Ball / outline colour |

## LAF Customisation

Register a custom look and feel to control the rendering of this floating tile.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawAhdsrBackground` | Draws the background of the envelope graph |
| `drawAhdsrPath` | Draws the envelope path curve (called twice: once for the full path, once for the active section) |
| `drawAhdsrBall` | Draws the ball indicating the current envelope position |

### `obj` Properties (shared across all functions)

| Property | Type | Description |
|----------|------|-------------|
| `obj.id` | String | The component's ID |
| `obj.enabled` | bool | Whether the graph is enabled |
| `obj.area` | Array[x,y,w,h] | The graph bounds |
| `obj.bgColour` | int (ARGB) | Background colour |
| `obj.itemColour` | int (ARGB) | Fill colour |
| `obj.itemColour2` | int (ARGB) | Line colour |
| `obj.itemColour3` | int (ARGB) | Outline colour |
| `obj.parentType` | String | ContentType of parent FloatingTile (if any) |

### Additional `obj` properties per function

`drawAhdsrBackground` uses only the shared properties above.

#### `drawAhdsrPath`

| Property | Type | Description |
|----------|------|-------------|
| `obj.isActive` | bool | `false` for the full path, `true` for the active section |
| `obj.path` | Path | The envelope path to render |
| `obj.currentState` | int | Current envelope state index (attack, hold, decay, sustain, release) |

#### `drawAhdsrBall`

| Property | Type | Description |
|----------|------|-------------|
| `obj.position` | Array[x,y] | The ball position |
| `obj.currentState` | int | Current envelope state index |

### Example

```javascript
const var ft = Content.getComponent("FloatingTile1");
const var laf = Content.createLocalLookAndFeel();

// Draw the background
laf.registerFunction("drawAhdsrBackground", function(g, obj)
{
    g.setColour(obj.bgColour);
    g.fillRect(obj.area);
});

// Draw the envelope path (called twice: full path + active section)
laf.registerFunction("drawAhdsrPath", function(g, obj)
{
    if(obj.isActive)
    {
        // Draw the currently active section with a highlight colour
        g.setColour(obj.itemColour2);
        g.fillPath(obj.path, obj.area);
    }
    else
    {
        // Draw the full envelope path as an outline
        g.setColour(obj.itemColour);
        g.drawPath(obj.path, obj.area, 2.0);
    }
});

// Draw the ball at the current envelope position
laf.registerFunction("drawAhdsrBall", function(g, obj)
{
    g.setColour(obj.itemColour3);
    g.fillEllipse([obj.position[0] - 5, obj.position[1] - 5, 10, 10]);
});

ft.setLocalLookAndFeel(laf);
```

## Notes

- The `drawAhdsrPath` function is called **twice** per repaint: once with `obj.isActive = false` for the full envelope curve, and once with `obj.isActive = true` for only the currently active section. Use `obj.isActive` to distinguish between the two calls and render them differently (e.g., outlined vs filled).
- The `obj.currentState` property indicates which envelope stage is currently active. This is available in both `drawAhdsrPath` and `drawAhdsrBall`.
- The `Index` property is not used by this content type — it can be left at its default value.

**See also:** <!-- populated during cross-reference post-processing -->
