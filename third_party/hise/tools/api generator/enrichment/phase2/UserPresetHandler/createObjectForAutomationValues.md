## createObjectForAutomationValues

**Examples:**

```javascript:channel-copy-paste
// Title: Channel copy/paste using automation value snapshots
// Context: In a multi-channel instrument, copying a channel's settings to
// another channel requires capturing all automation values for the source
// channel, then applying them to the target. createObjectForAutomationValues()
// provides the full snapshot; the script filters and remaps by channel.

const var uph = Engine.createUserPresetHandler();

inline function copyChannel(sourceIndex, targetIndex)
{
    local allValues = uph.createObjectForAutomationValues();
    // allValues is: [{"id": "Gain A1", "value": -6.0}, {"id": "Pan A1", "value": 0.5}, ...]

    local sourceTag = " " + (sourceIndex + 1);
    local targetTag = " " + (targetIndex + 1);

    for (slot in allValues)
    {
        // Find slots belonging to the source channel
        if (slot.id.indexOf(sourceTag) != -1)
        {
            // Remap the ID to the target channel
            local targetId = slot.id.replace(sourceTag, targetTag);
            local targetIdx = uph.getAutomationIndex(targetId);

            if (targetIdx >= 0)
                uph.setAutomationValue(targetIdx, slot.value);
        }
    }
}
```
```json:testMetadata:channel-copy-paste
{
  "testable": false,
  "skipReason": "Requires setCustomAutomation with per-channel automation slots."
}
```
