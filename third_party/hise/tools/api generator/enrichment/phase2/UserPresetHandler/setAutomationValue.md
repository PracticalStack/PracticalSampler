## setAutomationValue

**Examples:**

```javascript:xy-pad-automation
// Title: Driving automation values from a custom XY pad control
// Context: A ScriptPanel acting as an XY pad uses setAutomationValue
// to push its position into the automation system. This routes the
// values through all connections (processor parameters, cables, meta
// connections) and notifies any attached callbacks.

const var uph = Engine.createUserPresetHandler();

// Cache automation indices at init time for performance.
// These IDs must match slots registered via setCustomAutomation.
const var xIndex = uph.getAutomationIndex("Mixer 1 X");
const var yIndex = uph.getAutomationIndex("Mixer 1 Y");

inline function onPadChange(component, value)
{
    // value is a 2-element array [x, y] from the XY pad
    uph.setAutomationValue(xIndex, value[0]);
    uph.setAutomationValue(yIndex, value[1]);
}

// Reset to center position
inline function resetPad()
{
    uph.setAutomationValue(xIndex, 0.5);
    uph.setAutomationValue(yIndex, 0.0);
}
```
```json:testMetadata:xy-pad-automation
{
  "testable": false,
  "skipReason": "Requires setCustomAutomation with Mixer automation slots registered."
}
```
