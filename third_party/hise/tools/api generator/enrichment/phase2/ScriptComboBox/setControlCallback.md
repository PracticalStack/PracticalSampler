## setControlCallback

**Examples:**

```javascript:sample-map-selector
// Title: Sample map selector with combo box callback
// Context: A combo box populated from the sample map list drives sample loading.
// The callback uses value-1 to convert from 1-based combo index to 0-based array index.

const var sampleMapList = Sampler.getSampleMapList();

const var cbSampleMap = Content.addComboBox("SampleMapSelector", 0, 0);
cbSampleMap.set("items", sampleMapList.join("\n"));
cbSampleMap.set("saveInPreset", false);

inline function onSampleMapChanged(component, value)
{
    // value arrives as a float (e.g. 1.0), so convert to int for array indexing
    Console.print("Loading map index: " + (parseInt(value) - 1));
}

cbSampleMap.setControlCallback(onSampleMapChanged);
```
```json:testMetadata:sample-map-selector
{
  "testable": false,
  "skipReason": "Requires a sampler module and sample maps in the project"
}
```

```javascript:multi-combo-dispatch
// Title: Multi-purpose callback dispatching by component ID
// Context: A single callback handles multiple combo boxes created in a loop.
// The component's ID encodes its role, enabling one function to drive different behaviors.

var dispatchLog = [];

inline function onComboChanged(component, value)
{
    local id = component.getId();
    local idx = parseInt(value) - 1;

    if (id.contains("FilterType"))
        dispatchLog.push("filter:" + idx);
    else if (id.contains("Waveform"))
        dispatchLog.push("waveform:" + idx);
}

// Create combo boxes in a loop, all sharing the same callback
for (i = 1; i <= 2; i++)
{
    local cb = Content.addComboBox("FilterType" + i, (i - 1) * 200, 0);
    cb.set("items", "LP\nHP\nBP");
    cb.set("saveInPreset", false);
    cb.setControlCallback(onComboChanged);
}
```
```json:testMetadata:multi-combo-dispatch
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Content.getComponent(\"FilterType1\").setValue(2) || Content.getComponent(\"FilterType1\").changed()", "value": false},
    {"type": "REPL", "expression": "dispatchLog[0]", "value": "filter:1"}
  ]
}
```
