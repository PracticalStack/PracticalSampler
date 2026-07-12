## attachToRadioGroup

**Examples:**

```javascript:page-navigation-radio-buttons
// Title: Page navigation with radio buttons
// Context: The most common broadcaster pattern - radio buttons drive page visibility.
// attachToRadioGroup eliminates manual radio group state tracking.

// --- setup ---
const var PageBtn1 = Content.addButton("PageBtn1", 0, 0);
PageBtn1.set("radioGroup", 2);
PageBtn1.set("saveInPreset", false);
const var PageBtn2 = Content.addButton("PageBtn2", 130, 0);
PageBtn2.set("radioGroup", 2);
PageBtn2.set("saveInPreset", false);
const var PageBtn3 = Content.addButton("PageBtn3", 260, 0);
PageBtn3.set("radioGroup", 2);
PageBtn3.set("saveInPreset", false);
PageBtn1.setValue(1);
// --- end setup ---

const var pageBc = Engine.createBroadcaster({
    "id": "PageSelector2",
    "args": ["selectedIndex"]
});

var navLog = [];

pageBc.attachToRadioGroup(2, "pageButtons");

const var pageNames = ["Sound", "Effects", "Settings"];

pageBc.addListener("", "switchPage", function(selectedIndex)
{
    navLog.push(pageNames[selectedIndex]);
});
```
```json:testMetadata:page-navigation-radio-buttons
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "navLog.length", "value": 1},
    {"type": "REPL", "expression": "navLog[0]", "value": "Sound"}
  ]
}
```

The broadcaster fires immediately on attachment with the current selection index, so listeners are synchronized without a manual initial call.
