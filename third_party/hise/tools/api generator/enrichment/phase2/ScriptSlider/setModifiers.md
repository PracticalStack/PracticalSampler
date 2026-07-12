## setModifiers

**Examples:**

```javascript:disable-and-remap-slider-gestures
// Title: Disable or remap advanced slider gestures
// Context: A custom knob style removes fine-tune and reserves Ctrl/Cmd drag for modulation scaling.

const var amountKnob = Content.addKnob("Amount", 0, 0);
const var mods = amountKnob.createModifiers();

// Disable fine-tune gesture to keep drag response uniform.
amountKnob.setModifiers(mods.FineTune, mods.disabled);

// Keep modulation scaling on Ctrl/Cmd drag.
amountKnob.setModifiers(mods.ScaleModulation, mods.ctrlDown | mods.cmdDown);
```
```json:testMetadata:disable-and-remap-slider-gestures
{
  "testable": false,
  "skipReason": "Modifier mappings only affect interactive mouse gestures that cannot be programmatically triggered in the validator"
}
```

**Pitfalls:**
- If different sliders receive different modifier maps accidentally, drag behavior feels inconsistent; apply one shared mapping strategy per slider group.
