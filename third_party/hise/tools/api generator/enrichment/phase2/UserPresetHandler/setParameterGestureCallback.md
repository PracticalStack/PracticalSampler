## setParameterGestureCallback

**Examples:**

```javascript:gesture-smoothing
// Title: Enabling parameter smoothing during DAW automation gestures
// Context: When a DAW records automation, abrupt parameter changes can cause
// audible artifacts. The gesture callback detects when the DAW (or script)
// begins/ends a parameter interaction, allowing the plugin to enable
// parameter smoothing during the gesture and disable it afterward.

const var uph = Engine.createUserPresetHandler();
const var CUSTOM_AUTOMATION = 1;

// Map automation indices to their smoothing processor and parameter
// (populated during init based on automation slot layout)
const var SMOOTHERS = {};

inline function onParameterGesture(type, index, isStart)
{
    if (type == CUSTOM_AUTOMATION)
    {
        local s = SMOOTHERS[index];

        if (isDefined(s))
        {
            // Enable/disable smoothing on the associated gain module
            s.module.setAttribute(s.parameterIndex, isStart);
        }
    }
};

uph.setParameterGestureCallback(onParameterGesture);

// Also send gestures from script-driven UI controls (e.g., knob mouse events)
// so the same smoothing logic applies to both DAW and manual interaction.
// Assume gainKnob is a ScriptSlider with an "automationID" custom property.
gainKnob.setMouseCallback(function(event)
{
    if (event.clicked || event.mouseUp)
    {
        local id = this.get("automationID");
        local index = uph.getAutomationIndex(id);
        onParameterGesture(CUSTOM_AUTOMATION, index, event.clicked);
    }
});
```
```json:testMetadata:gesture-smoothing
{
  "testable": false,
  "skipReason": "Requires setCustomAutomation with registered slots and smoothing processor modules."
}
```
