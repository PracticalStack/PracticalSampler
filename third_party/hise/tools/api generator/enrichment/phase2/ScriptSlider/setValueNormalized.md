## setValueNormalized

**Examples:**

```javascript:panel-gesture-to-normalized-slider
// Title: Drive a hidden parameter slider from custom panel gestures
// Context: A custom panel calculates pointer position, then forwards normalized values into a ScriptSlider.

const var hiddenThreshold = Content.addKnob("HiddenThreshold", 0, 0);
hiddenThreshold.set("visible", false);
hiddenThreshold.set("min", -48.0);
hiddenThreshold.set("max", 0.0);
reg lastThresholdValue = 0.0;

const var gesturePanel = Content.addPanel("ThresholdSurface", 0, 0);
gesturePanel.set("width", 160);
gesturePanel.set("height", 120);
gesturePanel.set("min", 0.0);
gesturePanel.set("max", 1.0);

inline function onSurfaceControl(component, value)
{
    local normalized = 1.0 - value;
    hiddenThreshold.setValueNormalized(normalized);
    lastThresholdValue = hiddenThreshold.getValue();
    hiddenThreshold.changed();
}

gesturePanel.setControlCallback(onSurfaceControl);

// --- test-only ---
inline function triggerThresholdSurface(value)
{
    onSurfaceControl(gesturePanel, value);
    return lastThresholdValue;
}
// --- end test-only ---
```
```json:testMetadata:panel-gesture-to-normalized-slider
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "triggerThresholdSurface(0.25)", "value": -12.0},
    {"type": "REPL", "expression": "lastThresholdValue", "value": -12.0},
    {"type": "REPL", "expression": "hiddenThreshold.getValue()", "value": -12.0}
  ]
}
```

**Pitfalls:**
- For axis-inverted UI (top means higher value), invert normalized input before calling `setValueNormalized()` to avoid reversed control feel.
