## makeFrontInterface

**Examples:**

```javascript:init-scaffold
// Title: Standard interface initialization scaffold
// Context: This is always the very first line of the main interface script.
// It sets the interface size and registers this script processor as the
// front interface. Everything else follows after this call.

Content.makeFrontInterface(900, 600);

// Optional: enable HiDPI for custom-drawn panels
Content.setUseHighResolutionForPanels(true);

// Create the UI components
const var mainPanel = Content.addPanel("MainPanel", 0, 0);
mainPanel.set("width", 900);
mainPanel.set("height", 600);

const var gainKnob = Content.addKnob("GainKnob", 10, 10);
const var bypassBtn = Content.addButton("BypassBtn", 150, 10);

const var size = Content.getInterfaceSize();
Console.print(size[0]); // 900
Console.print(size[1]); // 600
```
```json:testMetadata:init-scaffold
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "size[0]", "value": 900},
    {"type": "REPL", "expression": "size[1]", "value": 600}
  ]
}
```

This method must be called during `onInit`. It sets both dimensions atomically and registers the script processor as the front interface in a single call - prefer this over separate `setWidth`/`setHeight` calls.
