## killVoicesAndCall

**Examples:**

```javascript:suspend-audio-for-mode-switch
// Title: Suspend audio to reconfigure multiple processors during mode switch
// Context: When switching between playback modes, multiple processors need their
// bypass state, gain, and routing updated atomically. Wrapping in killVoicesAndCall
// ensures no audio glitches from partial state changes.

const var Reverb = Synth.getEffect("Reverb");
const var Chorus = Synth.getEffect("Chorus");
const var MainGain = Synth.getEffect("MainGain");

reg currentMode = 0;

inline function applyModeSettings()
{
    // Reconfigure multiple processors while audio is suspended
    Reverb.setBypassed(currentMode != 1);
    Chorus.setBypassed(currentMode != 2);
    MainGain.setAttribute(MainGain.Gain, currentMode == 0 ? -6.0 : 0.0);
};

inline function onModeChanged(component, value)
{
    local newMode = parseInt(value) - 1;

    if (currentMode != newMode)
    {
        currentMode = newMode;
        Threads.killVoicesAndCall(applyModeSettings);
    }
};
```
```json:testMetadata:suspend-audio-for-mode-switch
{
  "testable": false,
  "skipReason": "References project-specific effects (Reverb, Chorus, MainGain) and killVoicesAndCall defers callback to SampleLoadingThread after onInit completes"
}
```

**Pitfalls:**
- In practice, `BackgroundTask.killVoicesAndCall(fn)` is more commonly used than `Threads.killVoicesAndCall(fn)` because background tasks provide progress tracking and abort support. Use the `Threads` version for simple, one-shot operations where a full BackgroundTask is unnecessary.
