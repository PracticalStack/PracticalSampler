## showYesNoWindow

**Examples:**

```javascript:confirmation-dialog-destructive
// Title: Confirmation dialog before a destructive operation
// Context: Use showYesNoWindow for any irreversible action like
// deleting presets, clearing data, or resetting settings. The
// callback receives true for Yes, false for No.

inline function onDeletePreset(presetName)
{
    // Capture the preset name for use inside the callback
    Engine.showYesNoWindow(
        "Delete Preset",
        "Are you sure you want to delete **" + presetName + "**?",
        function [presetName](ok)
        {
            if (ok)
            {
                Console.print("Deleting: " + presetName);
                // ... perform deletion
            }
        }
    );
}
```
```json:testMetadata:confirmation-dialog-destructive
{
  "testable": false,
  "skipReason": "showYesNoWindow opens a modal dialog requiring user interaction to dismiss."
}
```

**Pitfalls:**
- The callback runs asynchronously on the message thread. Do not read or write audio-thread state from inside the callback without proper synchronization.
- The markdown message body supports basic formatting: **bold**, *italic*, and line breaks.
