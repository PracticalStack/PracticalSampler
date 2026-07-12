## store

**Examples:**

```javascript:laf-broadcaster-mailbox
// Title: Bridging a LAF paint callback to a Broadcaster listener
// Context: A ScriptedViewport table needs to show the hovered row's text
// in a label, but LAF paint callbacks cannot modify component state.
// ThreadSafeStorage acts as a mailbox between the two execution contexts.

// --- setup ---
Content.addViewport("Viewport1", 0, 0);
Content.addPanel("HoverLabel", 200, 0);
// --- end setup ---

const var tss = Engine.createThreadSafeStorage();
const var laf = Content.createLocalLookAndFeel();

// Inside the paint callback: store the hovered text
laf.registerFunction("drawTableCell", function(g, obj)
{
    if (obj.hover)
        tss.store(obj.text);

    g.setColour(obj.hover ? 0xFFFFFFFF : 0xAAFFFFFF);
    g.drawAlignedText(obj.text, obj.area, "left");
});

const var Viewport1 = Content.getComponent("Viewport1");
Viewport1.setLocalLookAndFeel(laf);

// A label panel that displays the hovered text
const var HoverLabel = Content.getComponent("HoverLabel");

const var bc = Engine.createBroadcaster({
    "id": "viewportHover",
    "args": ["component", "event"]
});

// Attach to mouse events on the viewport
bc.attachToComponentMouseEvents("Viewport1", "Hover", "");

// Outside the paint callback: read from the storage
bc.addListener(HoverLabel, "update label", function(component, event)
{
    if (event.hover)
    {
        this.data.text = tss.load();
        this.repaint();
    }

    this.set("visible", event.hover);
});
```
```json:testMetadata:laf-broadcaster-mailbox
{
  "testable": false,
  "skipReason": "Requires mouse hover events on a rendered table viewport to trigger the LAF paint callback and broadcaster"
}
```
