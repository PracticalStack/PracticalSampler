## getSecondsSinceLastPresetLoad

**Examples:**

```javascript:debounce-guard
// Title: Debouncing operations that conflict with preset loading
// Context: Some operations (like cleaning up unused data) should not run
// immediately after a preset load because the load process may still be
// settling state. Use getSecondsSinceLastPresetLoad() as a debounce guard
// to skip the operation if a preset was loaded very recently.

const var uph = Engine.createUserPresetHandler();

inline function cleanupUnusedData(obj)
{
    // Skip cleanup if a preset was loaded less than 500ms ago
    local elapsed = uph.getSecondsSinceLastPresetLoad();

    if (elapsed < 0.5)
    {
        Console.print("Skipping cleanup - preset loaded " +
                       parseInt(elapsed * 1000) + "ms ago");
        return;
    }

    // Safe to proceed with cleanup
    // ...
}

// Call from a timer callback or button handler
cleanupUnusedData({});
```
```json:testMetadata:debounce-guard
{
  "testable": false,
  "skipReason": "Requires a recent preset load to produce a meaningful elapsed time value."
}
```
