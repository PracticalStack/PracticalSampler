## killVoicesAndCall

**Examples:**

```javascript:mode-switching-kill-voices
// Title: Mode switching that reconfigures multiple processors
// Context: A multi-mode instrument switches between performance modes by
// changing bypass states, mic routing, and effect settings across many
// processors. killVoicesAndCall suspends audio output during reconfiguration,
// preventing glitches from mid-stream state changes.

const var modeTask = Engine.createBackgroundTask("ModeSwitch");

const var NUM_MODES = 3;
reg currentMode = -1;

const var reverbSend = Synth.getChildSynth("ReverbSend");
const var effectChain = Synth.getEffect("MainFX");

inline function applyMode()
{
    // Runs on the sample loading thread with all voices killed.
    // Safe to change bypass states, attributes, and routing here.
    reverbSend.setBypassed(currentMode == 1);
    effectChain.setBypassed(currentMode == 0);
};

inline function setMode(component, value)
{
    if (!value)
        return;

    local newMode = parseInt(component.get("text"));

    if (currentMode != newMode)
    {
        currentMode = newMode;
        modeTask.killVoicesAndCall(applyMode);
    }
};
```
```json:testMetadata:mode-switching-kill-voices
{
  "testable": false,
  "skipReason": "Requires KillStateHandler and module tree with named processors"
}
```
