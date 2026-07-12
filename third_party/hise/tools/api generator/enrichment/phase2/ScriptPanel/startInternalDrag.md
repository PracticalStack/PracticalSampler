## startInternalDrag

**Examples:**

```javascript:modulation-source-drag
// Title: Modulation source drag initiation
// Context: A panel representing a modulation source (LFO, envelope, etc.)
// that initiates a drag operation when the user clicks its header area.
// The drag data includes a type identifier so drop targets can identify
// what kind of drag is in progress.

const var mod = Synth.getModulator("LFO1");

const var sourcePanel = Content.addPanel("ModSource", 0, 0);
sourcePanel.set("width", 200);
sourcePanel.set("height", 80);
sourcePanel.set("allowCallbacks", "Clicks, Hover & Dragging");

sourcePanel.data.modName = "LFO1";

sourcePanel.setMouseCallback(function(event)
{
    // Only start drag from the header area (top 30px)
    if (event.clicked && event.y < 30)
    {
        this.startInternalDrag({
            "Type": "ModulationDrag",
            "SourceId": this.data.modName
        });
    }
});

sourcePanel.setPaintRoutine(function(g)
{
    g.fillAll(0xFF333333);

    // Draw header bar
    g.setColour(0xFF555555);
    g.fillRect([0, 0, this.getWidth(), 30]);

    g.setColour(Colours.white);
    g.setFont("Arial", 14.0);
    g.drawAlignedText(this.data.modName,
        [8, 0, this.getWidth() - 16, 30], "left");
});

sourcePanel.repaint();
```
```json:testMetadata:modulation-source-drag
{
  "testable": false,
  "skipReason": "Requires LFO1 modulator in the signal chain and user mouse interaction for drag"
}
```

The drag data object can contain any properties. Drop targets receive this data through their mouse callback or through HISE's internal drag-and-drop routing system. Use a `"Type"` property to distinguish different drag operations in a complex UI.
