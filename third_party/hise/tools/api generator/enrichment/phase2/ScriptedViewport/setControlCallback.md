## setControlCallback

**Examples:**

```javascript:using-setcontrolcallback-to-restore
// Title: Using setControlCallback to restore serialized state in a modulation table
// Context: A modulation matrix table stores its connection data as a
//          base64 string in the component's value. When a preset loads,
//          the control callback receives the restored value and rebuilds
//          the modulation matrix from it.

const var modTable = Content.addViewport("ModTable", 0, 0);

// ... setTableMode, setTableColumns, setTableCallback in onInit ...

inline function onModTableRestore(component, value)
{
    // value is the restored preset data (e.g., a base64-encoded string in an array)
    if (value.length == 1)
    {
        Console.print("Restoring modulation state from preset");
        // Decode and rebuild the modulation connections
    }
};

modTable.setControlCallback(onModTableRestore);

```
```json:testMetadata:using-setcontrolcallback-to-restore
{
  "testable": false
}
```
