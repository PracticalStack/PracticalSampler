## setUseHighResolutionForPanels

**Examples:**

```javascript:hidpi-panel-rendering
// Title: Enabling HiDPI rendering for ScriptPanels
// Context: Call this immediately after makeFrontInterface if any ScriptPanel
// in your project uses a custom paint routine. Without it, panels render
// at 1x resolution and appear blurry on Retina/HiDPI displays.

Content.makeFrontInterface(900, 600);
Content.setUseHighResolutionForPanels(true);

// Now all ScriptPanels render at 2x resolution
const var panel = Content.addPanel("WaveformPanel", 10, 10);
panel.set("width", 400);
panel.set("height", 200);

panel.setPaintRoutine(function(g)
{
    // This will render at 2x resolution for crisp edges
    g.setColour(0xFF445566);
    g.fillRoundedRectangle(this.getLocalBounds(0), 4.0);
});
```
```json:testMetadata:hidpi-panel-rendering
{
  "testable": false,
  "skipReason": "Paint routine rendering is visual-only and cannot be verified via REPL"
}
```
