## setSliderAtIndex

**Examples:**

```javascript:fill-pack-from-sorted-measurements
// Title: Populate a slider pack from sorted measurement data
// Context: Utility workflows sort collected values, then write each entry into the matching slider.

const var valuePack = Content.addSliderPack("ValuePack", 10, 10);
const var valueData = Engine.createAndRegisterSliderPackData(1);

inline function sortByValue(a, b)
{
    if (a[1] < b[1]) return -1;
    if (a[1] > b[1]) return 1;
    return 0;
}

inline function applyMeasuredValues(measurements)
{
    Engine.sortWithFunction(measurements, sortByValue);

    valuePack.set("sliderAmount", measurements.length);
    valueData.setNumSliders(measurements.length);
    valueData.setAllValues(0.0);
    valuePack.referToData(valueData);

    local i = 0;
    for (entry in measurements)
        valuePack.setSliderAtIndex(i++, entry[1]);
}

applyMeasuredValues([[0, 0.6], [1, 0.2], [2, 0.9], [3, 0.4]]);
```
```json:testMetadata:fill-pack-from-sorted-measurements
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "valuePack.getSliderValueAt(0)", "value": 0.2},
    {"type": "REPL", "expression": "valuePack.getSliderValueAt(3)", "value": 0.9}
  ]
}
```

**Cross References:**
- `ScriptSliderPack.set`
