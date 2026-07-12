---
title: "MidiOverlayPanel"
description: "MIDI file drag-and-drop area connected to a MIDI Player module for loading and exporting MIDI content."
contentType: "MidiOverlayPanel"
componentType: "floating-tile"
llmRef: |
  MidiOverlayPanel (FloatingTile)
  ContentType string: "MidiOverlayPanel"
  Set via: FloatingTile.set("ContentType", "MidiOverlayPanel")

  MIDI file drag-and-drop area that connects to a MIDI Player module. Supports loading MIDI files via drag-and-drop and dragging the current MIDI content to external targets (e.g., DAWs).

  JSON Properties:
    ProcessorId: The ID of the connected MIDI Player
    Index: Usually 0

  Customisation:
    LAF: drawMidiDropper
    CSS: none

seeAlso: []
commonMistakes:
  - title: "Missing ProcessorId connection"
    wrong: "Adding a MidiOverlayPanel floating tile without setting the ProcessorId property"
    right: "Set ProcessorId to the ID of the MIDI Player you want to connect"
    explanation: "Without a connected processor, the dropper has no MIDI data to display or export. Set ProcessorId to a valid MIDI Player ID."
  - title: "Confusing with ScriptAudioWaveform"
    wrong: "Expecting drawMidiDropper to be part of the ScriptAudioWaveform LAF"
    right: "drawMidiDropper is a MidiOverlayPanel LAF function — set it on the FloatingTile's LAF, not the AudioWaveform's"
    explanation: "Although the laf_style_guide.json groups drawMidiDropper under ScriptAudioWaveform, it is architecturally part of MidiOverlayPanel (MidiFileDragAndDropper). Register it on the LAF applied to the FloatingTile containing the MidiOverlayPanel."
---

The MidiOverlayPanel is a FloatingTile content type that provides a MIDI file drag-and-drop area. It connects to a MIDI Player module and allows users to:

- **Drop** MIDI files onto the panel to load them into the connected MIDI Player
- **Drag** the current MIDI content from the panel to an external target (e.g., a DAW track)

## Setup

```javascript
const var ft = Content.getComponent("FloatingTile1");

ft.set("ContentType", "MidiOverlayPanel");
ft.set("Data", JSON.stringify({
    "ProcessorId": "MIDI Player1",
    "Index": 0
}));
```

## JSON Properties

Configure via the `Data` property as a JSON string, or set individual properties in the Interface Designer.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ProcessorId` | String | `""` | The ID of the connected MIDI Player module |
| `Index` | int | `0` | The MIDI file slot index |

The `ColourData` object can be used to set colours used by the default rendering:

| Colour ID | Description |
|-----------|-------------|
| `bgColour` | Background colour |
| `itemColour1` | Highlight / outline colour |
| `textColour` | Text colour |

## LAF Customisation

Register a custom look and feel to control the rendering of the MIDI dropper area.

### LAF Functions

| Function | Description |
|----------|-------------|
| `drawMidiDropper` | Draws the MIDI file drag-and-drop area |

### `obj` Properties

| Property | Type | Description |
|----------|------|-------------|
| `obj.id` | String | The component's ID |
| `obj.area` | Array[x,y,w,h] | The component area |
| `obj.text` | String | The default display text |
| `obj.hover` | bool | `true` if a drag operation is active — either an item hovers over the dropper or an external drag is in progress |
| `obj.active` | bool | `true` if a MIDI sequence is loaded into the connected MIDI Player |
| `obj.externalDrag` | bool | `true` if an external drag operation is in progress (dragging MIDI content out) |
| `obj.bgColour` | int (ARGB) | Background colour |
| `obj.itemColour1` | int (ARGB) | Highlight / outline colour |
| `obj.textColour` | int (ARGB) | Text colour |

### Example

```javascript
const var ft = Content.getComponent("FloatingTile1");

ft.set("ContentType", "MidiOverlayPanel");
ft.set("Data", JSON.stringify({
    "ProcessorId": "MIDI Player1",
    "Index": 0
}));

const var laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawMidiDropper", function(g, obj)
{
    // Background
    g.setColour(obj.bgColour);
    g.fillRoundedRectangle(obj.area, 5.0);

    // Highlight border when dragging
    if (obj.hover)
    {
        g.setColour(obj.itemColour1);
        g.drawRoundedRectangle(obj.area, 5.0, 3.0);
    }

    // Status text
    g.setColour(obj.textColour);
    g.setFont("Arial", 14.0);

    if (obj.active)
        g.drawAlignedText(obj.text, obj.area, "centred");
    else
        g.drawAlignedText("Drop MIDI file here", obj.area, "centred");
});

ft.setLocalLookAndFeel(laf);
```

## Notes

- **Drag direction is bidirectional.** The panel supports both inward drops (loading MIDI files) and outward drags (exporting current MIDI content to a DAW or other target).
- **`obj.hover` is `true` during both inward and outward drags.** Use `obj.externalDrag` to distinguish between a file hovering over the dropper (inward) and the user dragging content out (outward).
- **`obj.active` reflects loaded state.** It is `true` when a MIDI sequence is loaded into the connected MIDI Player, regardless of playback state.
- **Colour property naming:** The old `custom_lookandfeel.md` documentation references `obj.itemColour` (without the `1` suffix), but the LAF style guide and C++ implementation use `obj.itemColour1`. Both may work depending on HISE version — prefer `obj.itemColour1` for consistency with other FloatingTile LAF functions.

**See also:** <!-- populated during cross-reference post-processing -->
