## sendParameterGesture

**Examples:**

```javascript:gesture-xy-pad
// Title: Wrapping XY pad mouse interaction with DAW gesture notifications
// Context: For a DAW to correctly record automation, parameter changes must
// be bracketed by begin/end gesture messages. When a custom ScriptPanel
// control (like an XY pad) is dragged, send gesture begin on mouseDown
// and gesture end on mouseUp.

const var uph = Engine.createUserPresetHandler();
const var CUSTOM_AUTOMATION = 1; // automationType for CustomAutomation slots

// Assume xyPad is a ScriptPanel with data.xIndex and data.yIndex
// set to the automation slot indices for its X and Y parameters
xyPad.setMouseCallback(function(event)
{
    if (event.clicked && !event.drag && !event.rightClick)
    {
        // Begin gesture for both X and Y parameters
        uph.sendParameterGesture(CUSTOM_AUTOMATION, this.data.xIndex, true);
        uph.sendParameterGesture(CUSTOM_AUTOMATION, this.data.yIndex, true);
    }

    if (event.mouseUp && !event.rightClick)
    {
        // End gesture for both parameters
        uph.sendParameterGesture(CUSTOM_AUTOMATION, this.data.xIndex, false);
        uph.sendParameterGesture(CUSTOM_AUTOMATION, this.data.yIndex, false);
    }

    if (event.drag)
    {
        // Update values between gesture begin/end
        local area = this.getLocalBounds(0);
        local normX = Math.range((event.x - area[0]) / area[2], 0.0, 1.0);
        local normY = Math.range((event.y - area[1]) / area[3], 0.0, 1.0);

        uph.setAutomationValue(this.data.xIndex, normX);
        uph.setAutomationValue(this.data.yIndex, normY);
    }
});
```
```json:testMetadata:gesture-xy-pad
{
  "testable": false,
  "skipReason": "Requires setCustomAutomation with registered slots and a DAW host to observe gesture behavior."
}
```
