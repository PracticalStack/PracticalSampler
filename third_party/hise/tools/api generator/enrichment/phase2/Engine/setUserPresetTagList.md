## setUserPresetTagList

**Examples:**

```javascript:preset-browser-tags
// Title: Configuring preset browser tags at initialization
// Context: Call setUserPresetTagList during onInit to define the
// tag categories that appear in the preset browser filter panel.
// Tags should reflect the sonic categories of the preset library.

Engine.setUserPresetTagList([
    "Bass",
    "Lead",
    "Pad",
    "Sequenced",
    "Mono",
    "Orchestral",
    "Synth",
    "Dark",
    "Epic"
]);
```
```json:testMetadata:preset-browser-tags
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Engine.getUptime() >= 0", "value": true}
  ]
}
```
