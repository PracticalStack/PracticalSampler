## addComponentValueListener

**Examples:**

```javascript:sync-knob-to-display
// Title: Syncing FX knob values to visual display positions
// Context: An FX page has knob controls and a custom panel that draws
// visual indicators at the knob positions. When a knob value changes
// via a broadcaster, addComponentValueListener with a transform callback
// can map the value to a different range for the display component.

// --- setup ---
const var FXKnob1 = Content.addKnob("FXKnob1", 0, 0);
FXKnob1.set("saveInPreset", false);
const var FXKnob2 = Content.addKnob("FXKnob2", 150, 0);
FXKnob2.set("saveInPreset", false);
const var FXKnob3 = Content.addKnob("FXKnob3", 300, 0);
FXKnob3.set("saveInPreset", false);
const var Display1 = Content.addKnob("Display1", 0, 50);
Display1.set("saveInPreset", false);
const var Display2 = Content.addKnob("Display2", 150, 50);
Display2.set("saveInPreset", false);
const var Display3 = Content.addKnob("Display3", 300, 50);
Display3.set("saveInPreset", false);
// --- end setup ---

const var fxBc = Engine.createBroadcaster({
    "id": "FXKnobSync",
    "args": ["component", "value"]
});

fxBc.attachToComponentValue(
    ["FXKnob1", "FXKnob2", "FXKnob3"],
    "fxKnobValues"
);

// In callback mode, targetIndex identifies which display component
// is being updated. The callback must return the value to set.
const var displaySliders = [Content.getComponent("Display1"),
                            Content.getComponent("Display2"),
                            Content.getComponent("Display3")];

fxBc.addComponentValueListener(displaySliders, "syncDisplays",
    function(targetIndex, component, value)
{
    // Map the knob range to the display range
    return value * 0.5;
});
```
```json:testMetadata:sync-knob-to-display
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Content.getComponent(\"FXKnob1\").setValue(0.8) || Content.getComponent(\"FXKnob1\").changed()", "value": false},
    {"type": "REPL", "delay": 300, "expression": "Content.getComponent(\"Display1\").getValue()", "value": 0.4}
  ]
}
```
