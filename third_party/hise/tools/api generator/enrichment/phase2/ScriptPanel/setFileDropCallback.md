## setFileDropCallback

**Examples:**

```javascript:audio-file-drop-zone
// Title: Audio file drop zone with hover feedback and broadcaster coordination
// Context: A panel that accepts audio file drops and uses a broadcaster to
// coordinate hover/drop state across multiple panels. The drop callback
// distinguishes between hover (file dragged over), drop (file released),
// and exit (file dragged away) events.

const var DROP_EXIT = 0;
const var DROP_HOVER = 1;
const var DROP_COMPLETE = 2;

// Broadcaster coordinates drop state across UI
const var fileDrop = Engine.createBroadcaster({
    "id": "fileDropHandler",
    "args": ["dropType", "fileName"]
});

const var dropZone = Content.addPanel("DropZone", 0, 0);
dropZone.set("width", 200);
dropZone.set("height", 100);

dropZone.data.hovering = false;

inline function onFileDrop(event)
{
    local dropType;

    if (event.drop)
        dropType = DROP_COMPLETE;
    else if (event.hover)
        dropType = DROP_HOVER;
    else
        dropType = DROP_EXIT;

    fileDrop.sendAsyncMessage([dropType, event.fileName]);
    this.repaint();
};

dropZone.setFileDropCallback("Drop & Hover", "*.wav;*.aif;*.aiff", onFileDrop);

// Update visual state from broadcaster
fileDrop.addListener(dropZone, "update drop zone", function(dropType, fileName)
{
    this.data.hovering = (dropType == DROP_HOVER);
    this.repaint();

    if (dropType == DROP_COMPLETE)
    {
        Console.print("Dropped: " + fileName);
        // Load the dropped file into a sampler or audio player here
    }
});

dropZone.setPaintRoutine(function(g)
{
    g.fillAll(this.data.hovering ? 0xFF334455 : 0xFF222222);
    g.setColour(this.data.hovering ? 0xFFFFFFFF : 0xFF666666);
    g.drawRoundedRectangle(this.getLocalBounds(4), 6.0, 2.0);
    g.setFont("Arial", 14.0);
    g.drawAlignedText("Drop audio file here",
        this.getLocalBounds(0), "centred");
});

dropZone.repaint();
```
```json:testMetadata:audio-file-drop-zone
{
  "testable": false,
  "skipReason": "Requires external file drag-and-drop interaction"
}
```

**Pitfalls:**
- The `"Drop Only"` callback level does not fire hover events. Use `"Drop & Hover"` if you need visual feedback while files are being dragged over the panel.
- The wildcard filter (e.g. `"*.wav;*.aif"`) is checked before the callback fires. Files that don't match the wildcard are silently ignored with no callback.
