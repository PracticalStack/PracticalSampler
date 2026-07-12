## attachAutomationCallback

**Examples:**

```javascript:dynamic-callback-rebinding
// Title: Dynamic automation callback binding for a switchable XY pad
// Context: When a control switches which channel it displays (e.g., an XY pad
// that follows the selected channel), the automation callbacks need to be
// rebound. First remove old callbacks by passing an empty string, then
// attach new ones for the current channel's automation IDs.

const var uph = Engine.createUserPresetHandler();

// On channel switch: rebind the XY pad to the new channel's automation slots
inline function onChannelSwitch(panel, channelIndex)
{
    // Remove previous callbacks by passing a non-function value
    if (isDefined(panel.data.xId))
        uph.attachAutomationCallback(panel.data.xId, "", false);

    if (isDefined(panel.data.yId))
        uph.attachAutomationCallback(panel.data.yId, "", false);

    // Build new automation IDs for the selected channel
    panel.data.xId = "Mixer " + (channelIndex + 1) + " X";
    panel.data.yId = "Mixer " + (channelIndex + 1) + " Y";

    // Attach async callbacks that update the panel display
    uph.attachAutomationCallback(panel.data.xId, function [panel](index, value)
    {
        local v = panel.getValue();
        v[0] = value;
        panel.setValue(v);
        panel.repaint();
    }, AsyncNotification);

    uph.attachAutomationCallback(panel.data.yId, function [panel](index, value)
    {
        local v = panel.getValue();
        v[1] = value;
        panel.setValue(v);
        panel.repaint();
    }, AsyncNotification);

    // Read current values to initialize the display
    local allValues = uph.createObjectForAutomationValues();
    local xIdx = uph.getAutomationIndex(panel.data.xId);
    local yIdx = uph.getAutomationIndex(panel.data.yId);

    panel.setValue([allValues[xIdx].value, allValues[yIdx].value]);
    panel.repaint();
}
```
```json:testMetadata:dynamic-callback-rebinding
{
  "testable": false,
  "skipReason": "Requires setCustomAutomation with Mixer automation slots and a panel component."
}
```

**Pitfalls:**
- When switching which automation slot a callback listens to, always remove the old callback first by passing a non-function value (e.g., `""` or `false`). Only one callback per automation ID is allowed - attaching a new one silently replaces the old one, but if the old ID is different, the old callback remains active and continues firing.
