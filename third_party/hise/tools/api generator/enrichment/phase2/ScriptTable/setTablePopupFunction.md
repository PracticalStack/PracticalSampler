## setTablePopupFunction

**Examples:**

```javascript:toggle-table-popup-verbosity
// Title: Switch between detailed popup text and compact editing mode
// Context: Dense editors often hide drag popup text to keep nearby controls visible.

const var lfoTable = Content.addTable("LfoTable", 10, 10);
const var compactMode = Content.addButton("CompactMode", 10, 130);

inline function formatPopup(x, y)
{
    return parseInt(x * 100) + "% | " + parseInt(y * 100) + "%";
}

inline function hidePopup(x, y)
{
    return "";
}

lfoTable.setTablePopupFunction(formatPopup);

inline function onCompactModeControl(component, value)
{
    if (value)
        lfoTable.setTablePopupFunction(hidePopup);
    else
        lfoTable.setTablePopupFunction(false);
}

compactMode.setControlCallback(onCompactModeControl);
```
```json:testMetadata:toggle-table-popup-verbosity
{
  "testable": false,
  "skipReason": "Table popup callbacks are only evaluated during interactive point dragging"
}
```

**Pitfalls:**
- Returning `null` or `""` from the popup formatter suppresses visible popup text. Use `setTablePopupFunction(false)` when you want to restore the default formatter.

**Cross References:**
- `ScriptTable.setMouseHandlingProperties`
