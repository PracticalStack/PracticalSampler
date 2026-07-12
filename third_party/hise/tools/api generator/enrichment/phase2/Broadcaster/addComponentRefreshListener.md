## addComponentRefreshListener

**Examples:**

```javascript:repaint-on-module-parameter-change
// Title: Triggering panel repaints when a module parameter changes
// Context: When a module parameter changes (e.g., bypass state toggling),
// a panel that draws a background reflecting that state needs to repaint.
// addComponentRefreshListener avoids writing a custom listener callback
// just to call sendRepaintMessage().

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "RefreshGain", 0, builder.ChainIndexes.FX);
builder.flush();
const var BackgroundPanel = Content.addPanel("BackgroundPanel", 0, 0);
BackgroundPanel.set("saveInPreset", false);
// --- end setup ---

const var bgBc = Engine.createBroadcaster({
    "id": "BypassWatcher",
    "args": ["processorId", "parameterId", "value"]
});

bgBc.attachToModuleParameter("RefreshGain", "Bypassed", "bypassState");

// The panel repaints automatically when the bypass state changes.
// No callback needed - the refresh type does the work.
bgBc.addComponentRefreshListener("BackgroundPanel", "repaint", "updateBg");
```
```json:testMetadata:repaint-on-module-parameter-change
{
  "testable": false,
  "skipReason": "addComponentRefreshListener with 'repaint' triggers a visual repaint that has no scriptable observable side-effect"
}
```

Use `"repaint"` to trigger the paint routine, `"changed"` to fire the control callback, and `"updateValueFromProcessorConnection"` to refresh a component's value from its connected processor parameter.
