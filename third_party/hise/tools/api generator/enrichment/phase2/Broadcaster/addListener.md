## addListener

**Examples:**

```javascript:centralized-event-bus
// Title: Centralized event bus - one broadcaster notifies multiple independent modules
// Context: In large plugins, a single broadcaster with listeners in many separate scripts
// decouples feature modules. This pattern replaces direct function calls between scripts.

// --- setup ---
const var Page1 = Content.addPanel("Page1", 0, 50);
Page1.set("saveInPreset", false);
const var Page2 = Content.addPanel("Page2", 0, 150);
Page2.set("saveInPreset", false);
const var Page3 = Content.addPanel("Page3", 0, 250);
Page3.set("saveInPreset", false);
const var HeaderLabel = Content.addLabel("HeaderLabel", 0, 0);
HeaderLabel.set("saveInPreset", false);
const var PB1 = Content.addButton("PB1", 0, 350);
PB1.set("radioGroup", 9);
PB1.set("saveInPreset", false);
const var PB2 = Content.addButton("PB2", 130, 350);
PB2.set("radioGroup", 9);
PB2.set("saveInPreset", false);
const var PB3 = Content.addButton("PB3", 260, 350);
PB3.set("radioGroup", 9);
PB3.set("saveInPreset", false);
PB1.setValue(1);
// --- end setup ---

const var pageBroadcaster = Engine.createBroadcaster({
    "id": "PageSelector",
    "args": ["index"]
});

pageBroadcaster.attachToRadioGroup(9, "pageButtons");

// Listener 1: Toggle page visibility
const var pages = [Content.getComponent("Page1"),
                   Content.getComponent("Page2"),
                   Content.getComponent("Page3")];

pageBroadcaster.addListener(pages, "showPage", function(index)
{
    for (i = 0; i < this.length; i++)
        this[i].set("visible", i == index);
});

// Listener 2: Update header label (independent module, same broadcaster)
const var headerLabel = Content.getComponent("HeaderLabel");

pageBroadcaster.addListener(headerLabel, "updateHeader", function(index)
{
    local names = ["Sound", "Effects", "Settings"];
    this.set("text", names[index]);
});
```
```json:testMetadata:centralized-event-bus
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "Content.getComponent(\"Page1\").get(\"visible\")", "value": true},
    {"type": "REPL", "expression": "Content.getComponent(\"Page2\").get(\"visible\")", "value": false},
    {"type": "REPL", "expression": "Content.getComponent(\"Page3\").get(\"visible\")", "value": false},
    {"type": "REPL", "expression": "Content.getComponent(\"HeaderLabel\").get(\"text\")", "value": "Sound"}
  ]
}
```

```javascript:object-as-this-reference
// Title: Using the object parameter as a lookup table inside the callback
// Context: Passing a JSON object or array as the `object` parameter replaces `this`
// inside the callback, giving clean access to related data without external variables.

const var speedBroadcaster = Engine.createBroadcaster({
    "id": "PlaybackSpeed",
    "args": ["ratio"]
});

var lastRatio = 0;

speedBroadcaster.addListener({"scale": 2.0}, "updatePlayers", function(ratio)
{
    lastRatio = ratio * this.scale;
});

speedBroadcaster.sendSyncMessage(1.5);
```
```json:testMetadata:object-as-this-reference
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastRatio", "value": 3.0}
  ]
}
```

**Pitfalls:**
- When using the `object` parameter as `this` inside the callback, remember that arrays and objects are passed by reference. If the array is modified after `addListener` is called, the callback sees the modified version. Use `.clone()` if you need a snapshot.
