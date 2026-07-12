## setControlCallback

**Examples:**

```javascript:index-driven-slider-pack-callback
// Title: Use callback index to update step state
// Context: Step sequencer workflows treat the callback value as the edited step index.

const var stepPack = Content.addSliderPack("StepPack", 10, 10);
stepPack.set("sliderAmount", 8);
stepPack.setAllValues([1.0, 0.5, 0.0, 0.75, 0.25, 0.0, 1.0, 0.5]);

const var currentStepValues = [];
for (v in stepPack.getDataAsBuffer())
    currentStepValues.push(v);

reg lastEditedStep = -1;

inline function onStepPackControl(component, value)
{
    local stepIndex = parseInt(value);
    local stepValue = component.getSliderValueAt(stepIndex);

    currentStepValues[stepIndex] = stepValue;
    lastEditedStep = stepIndex;
}

stepPack.setControlCallback(onStepPackControl);
```
```json:testMetadata:index-driven-slider-pack-callback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "stepPack.setSliderAtIndex(3, 0.9) || true", "value": true},
    {"type": "REPL", "expression": "lastEditedStep", "value": 3},
    {"type": "REPL", "expression": "Math.round(currentStepValues[3] * 10)", "value": 9}
  ]
}
```

**Pitfalls:**
- In lane-edit patterns, callback `value` is used as an edited index. Fetch the lane value with `getSliderValueAt(index)` instead of assuming `value` is the lane amplitude.

**Cross References:**
- `ScriptSliderPack.getSliderValueAt`
