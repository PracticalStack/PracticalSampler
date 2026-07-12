## addComponentPropertyListener

**Examples:**

```javascript:grey-out-on-bypass
// Title: Greying out controls when a module is bypassed via property binding
// Context: A common pattern where a module's bypass state drives the visual
// appearance of its associated controls. The transform callback converts the
// bypass state (0/1) into a colour value for the itemColour property.

// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "PropGain", 0, builder.ChainIndexes.FX);
builder.flush();
const var PropKnob1 = Content.addKnob("PropKnob1", 0, 0);
PropKnob1.set("saveInPreset", false);
const var PropKnob2 = Content.addKnob("PropKnob2", 150, 0);
PropKnob2.set("saveInPreset", false);
const var PropKnob3 = Content.addKnob("PropKnob3", 300, 0);
PropKnob3.set("saveInPreset", false);
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "BypassDisabler",
    "args": ["processorId", "parameterId", "value"]
});

bc.attachToModuleParameter("PropGain", "Bypassed", "bypassSource");

const var WHITE_ACTIVE = 0x88FFFFFF;
const var WHITE_DIMMED = 0x15FFFFFF;

const var controlKnobs = [Content.getComponent("PropKnob1"),
                          Content.getComponent("PropKnob2"),
                          Content.getComponent("PropKnob3")];

// The transform callback receives (targetIndex, processorId, parameterId, value)
// and must return the property value to set
bc.addComponentPropertyListener(controlKnobs, "itemColour",
    "dimOnBypass",
    function(targetIndex, processorId, parameterId, isBypassed)
{
    return isBypassed ? WHITE_DIMMED : WHITE_ACTIVE;
});
```
```json:testMetadata:grey-out-on-bypass
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "Content.getComponent(\"PropKnob1\").get(\"itemColour\") == WHITE_ACTIVE", "value": true},
    {"type": "REPL", "expression": "Content.getComponent(\"PropKnob3\").get(\"itemColour\") == WHITE_ACTIVE", "value": true}
  ]
}
```

```javascript:lock-icon-toggle
// Title: Updating a lock icon based on a toggle button's state
// Context: A button toggles between locked/unlocked states. A broadcaster
// watches the button value and updates an icon component's text property
// (used for icon path selection) via addComponentPropertyListener.

// --- setup ---
const var LockToggle = Content.addButton("LockToggle", 0, 0);
LockToggle.set("saveInPreset", false);
const var LockIcon = Content.addLabel("LockIcon", 150, 0);
LockIcon.set("saveInPreset", false);
// --- end setup ---

const var lockBc = Engine.createBroadcaster({
    "id": "LockIconUpdater",
    "args": ["component", "value"]
});

lockBc.attachToComponentValue("LockToggle", "lockState");

lockBc.addComponentPropertyListener("LockIcon", "text",
    "updateIcon",
    function(targetIndex, component, value)
{
    return value ? "lock_closed" : "lock_open";
});
```
```json:testMetadata:lock-icon-toggle
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Content.getComponent(\"LockIcon\").get(\"text\")", "value": "lock_open"}
  ]
}
```

**Pitfalls:**
- In the transform callback, the `targetIndex` is the index within the component array passed as the first parameter. When targeting a single component (not an array), `targetIndex` is always 0. Forgetting to account for this extra first parameter shifts all subsequent arguments.
