## loadSampleMap

**Examples:**

```javascript:deferred-loading
// Title: Timer-deferred sample map loading for dual-layer instrument
// Context: Loading sample maps directly from UI callbacks can cause audio glitches.
// A timer polls for changes at 50ms intervals and loads only when needed.

const var sampler1 = Synth.getSampler("Layer1");
const var sampler2 = Synth.getSampler("Layer2");
const var sampleMaps = Sampler.getSampleMapList();

reg layer1_last = 0;
reg layer1_current = 0;
reg layer2_last = 0;
reg layer2_current = 0;

const var loadTimer = Engine.createTimerObject();

loadTimer.setTimerCallback(function()
{
    if (layer1_last != layer1_current)
    {
        sampler1.loadSampleMap(sampleMaps[layer1_current]);
        layer1_last = layer1_current;
    }

    if (layer2_last != layer2_current)
    {
        sampler2.loadSampleMap(sampleMaps[layer2_current]);
        layer2_last = layer2_current;
    }
});

loadTimer.startTimer(50);

// UI callback just updates the target index - never calls loadSampleMap directly
inline function onLayerSelectorControl(component, value)
{
    if (value)
        layer1_current = parseInt(value - 1);
};
```

```json:testMetadata:deferred-loading
{
  "testable": false,
  "skipReason": "Requires sampler module tree with sample maps"
}
```

The timer-deferred pattern avoids calling `loadSampleMap()` on the message thread during a control callback. The 50ms poll interval is a good default - fast enough for responsive UI, slow enough to batch rapid changes.

**Pitfalls:**
- ComboBox values arrive as floats (e.g., `1.0`). Always use `parseInt(value - 1)` when converting to an array index, not just `value - 1`.
