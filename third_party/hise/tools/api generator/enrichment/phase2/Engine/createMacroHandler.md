## createMacroHandler

**Examples:**

```javascript:macro-handler-exclusive-broadcaster
// Title: Macro handler with exclusive mode and broadcaster updates
// Context: Plugins with macro/automation systems create the macro
// handler at init, enable exclusive mode (one connection per slot),
// and route updates through a broadcaster for UI synchronization.

// --- setup ---
Engine.setFrontendMacros(["Macro1", "Macro2", "Macro3", "Macro4"]);
// --- end setup ---

const var macroHandler = Engine.createMacroHandler();

// Exclusive mode: each macro slot can only control one parameter.
// Assigning a new connection to an occupied slot replaces the old one.
macroHandler.setExclusiveMode(true);

const var macroBroadcaster = Engine.createBroadcaster({
    "id": "macroBroadcaster",
    "args": ["obj"]
});

// Route macro update events through the broadcaster
macroHandler.setUpdateCallback(macroBroadcaster);

// Initialize with an empty connection list
macroHandler.setMacroDataFromObject([]);

// Listeners react to macro assignment changes
macroBroadcaster.addListener("ui", "update macro display", function(obj)
{
    Console.print("Macro connection changed");
});
```
```json:testMetadata:macro-handler-exclusive-broadcaster
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "typeof macroHandler", "value": "object"},
    {"type": "REPL", "expression": "Engine.getMacroName(1)", "value": "Macro1"}
  ]
}
```

**Cross References:**
- `Engine.setFrontendMacros` -- must be called before creating a macro handler to enable macros
