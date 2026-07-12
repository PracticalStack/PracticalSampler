## contains

**Examples:**

```javascript:case-insensitive-search
// Title: Case-insensitive preset search filtering
// Context: A preset browser filters its list as the user types.
// Both the search term and the preset name are lowercased before
// checking, since contains() is case-sensitive.

reg searchTerm = "";

inline function onSearchChanged(text)
{
    searchTerm = text.toLowerCase();
}

inline function shouldHidePreset(presetName)
{
    if (searchTerm.length == 0)
        return false;
    
    return !presetName.toLowerCase().contains(searchTerm);
}

onSearchChanged("warm");
Console.print(shouldHidePreset("Warm Pad"));    // 0 (false - matches)
Console.print(shouldHidePreset("Bright Lead")); // 1 (true - no match)
```
```json:testMetadata:case-insensitive-search
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["0", "1"]}
  ]
}
```

```javascript:dispatch-by-id-content
// Title: Dispatch logic based on component ID content
// Context: A single callback handles multiple component types by
// checking which keyword the ID contains, combined with
// getTrailingIntValue for the index.

inline function onComboBoxChanged(component, value)
{
    local id = component.get("id");
    local idx = id.getTrailingIntValue() - 1;
    
    if (id.contains("Unisono"))
        SynthGroups[idx].setAttribute(SynthGroups[idx].VoiceAmount, value);
    else if (id.contains("FilterType"))
        Filters[idx].setAttribute(Filters[idx].Type, value);
    else if (id.contains("Waveform"))
        WaveDisplays[idx].loadFile(waveforms[value - 1]);
}
```
```json:testMetadata:dispatch-by-id-content
{
  "testable": false,
  "skipReason": "Requires SynthGroups, Filters, and WaveDisplays module arrays that cannot be created standalone"
}
```
