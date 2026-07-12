## clearAllConnections

**Examples:**

```javascript:context-menu-clear-all
// Title: Context menu to clear all modulation connections
// Context: A broadcaster-driven context menu on a button provides
// "Clear all connections" as a single-click action in the matrix UI.

const var mm = Engine.createModulationMatrix("Global Modulator Container0");

const var clearButton = Content.getComponent("MatrixOptions");

const var menuBroadcaster = Engine.createBroadcaster({
    "id": "matrixContextMenu",
    "args": ["component", "index"]
});

// Attach a context menu to the button. The second argument controls
// when items are enabled -- here, always enabled.
menuBroadcaster.attachToContextMenu(
    ["MatrixOptions"],
    function(state) { return state == "enabled"; },
    ["Clear all connections"],
    "",
    true
);

inline function onMenuSelect(component, index)
{
    // index 0 = "Clear all connections"
    if (index == 0)
        mm.clearAllConnections("");
};

menuBroadcaster.addListener(0, "clear matrix connections", onMenuSelect);
```

```json:testMetadata:context-menu-clear-all
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer and user interaction with the context menu"
}
```
