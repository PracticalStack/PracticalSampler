## getTrailingIntValue

**Examples:**

```javascript:map-id-to-array-index
// Title: Map numbered component IDs to array indices in a shared callback
// Context: When multiple UI controls share a single callback, the trailing
// number in the component ID identifies which channel or slot to update.
// This avoids writing separate callbacks for each control.

const var NUM_CHANNELS = 4;
const var Filters = [];

for (i = 0; i < NUM_CHANNELS; i++)
    Filters[i] = Synth.getEffect("Filter" + (i + 1));

inline function onComboBox(component, value)
{
    local id = component.get("id");
    
    // "FilterType1" -> 0, "FilterType2" -> 1, etc.
    local idx = id.getTrailingIntValue() - 1;
    
    if (id.contains("FilterType"))
        Filters[idx].setAttribute(Filters[idx].Type, value);
    else if (id.contains("FilterFreq"))
        Filters[idx].setAttribute(Filters[idx].Frequency, value);
}
```
```json:testMetadata:map-id-to-array-index
{
  "testable": false,
  "skipReason": "Requires Filter1-Filter4 effect modules in the signal chain"
}
```

```javascript:rewrite-channel-index
// Title: Rewrite a component ID to target a different channel index
// Context: When copying parameter state between channels, replace
// the trailing number in the automation ID to point at the new channel.

var id = "PlayerVolume3";
var currentIndex = id.getTrailingIntValue(); // 3

// Replace the trailing "3" with the new channel index
var newId = id.replace(currentIndex, 5);
Console.print(newId); // PlayerVolume5
```
```json:testMetadata:rewrite-channel-index
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["PlayerVolume5"]}
  ]
}
```
