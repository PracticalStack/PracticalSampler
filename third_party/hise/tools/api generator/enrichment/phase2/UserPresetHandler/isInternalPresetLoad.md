## isInternalPresetLoad

**Examples:**

```javascript:parameter-lock
// Title: Parameter locking that respects DAW state recall
// Context: A "preset lock" feature preserves certain parameter values when
// the user manually changes presets, but allows DAW session recall to
// overwrite everything. isInternalPresetLoad() distinguishes these two cases.

const var uph = Engine.createUserPresetHandler();

// Assume lockButton and mixKnob are ScriptButton/ScriptSlider references
reg lockValue;

// In the post-callback: restore locked value only for user-initiated loads
uph.setPostCallback(function(presetFile)
{
    local isUserLoad = !uph.isInternalPresetLoad();
    local lockEnabled = lockButton.getValue();

    if (isUserLoad && lockEnabled && isDefined(lockValue))
    {
        // Restore the locked parameter to its pre-load value
        mixKnob.setValue(lockValue);
        mixKnob.changed();
    }
    else
    {
        // Capture the new value for future locking
        lockValue = mixKnob.getValue();
    }
});
```
```json:testMetadata:parameter-lock
{
  "testable": false,
  "skipReason": "Requires a preset load to trigger the post-callback with meaningful isInternalPresetLoad state."
}
```
