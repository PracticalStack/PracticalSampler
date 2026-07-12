## getComponent

**Examples:**

```javascript:caching-component-refs
// Title: Caching component references at init time
// Context: getComponent performs a linear search. The standard practice is to
// cache all references as const var during onInit and use those variables
// everywhere else. This is the single most common API call in HiseScript.

Content.makeFrontInterface(900, 600);

// --- setup ---
Content.addKnob("GainKnob", 10, 10);
Content.addKnob("MixKnob", 150, 10);
Content.addButton("BypassBtn", 300, 10);
for (i = 0; i < 4; i++)
{
    Content.addKnob("Volume" + (i + 1), 10, 60 + i * 50);
    Content.addKnob("Pan" + (i + 1), 150, 60 + i * 50);
}
// --- end setup ---

// Cache references once at init
const var gainKnob = Content.getComponent("GainKnob");
const var mixKnob = Content.getComponent("MixKnob");
const var bypassBtn = Content.getComponent("BypassBtn");

// Build arrays of related components using a loop
const var NUM_CHANNELS = 4;
const var channelVolumes = [];
const var channelPans = [];

for (i = 0; i < NUM_CHANNELS; i++)
{
    channelVolumes.push(Content.getComponent("Volume" + (i + 1)));
    channelPans.push(Content.getComponent("Pan" + (i + 1)));
}

Console.print(channelVolumes.length); // 4
```
```json:testMetadata:caching-component-refs
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "gainKnob.get(\"id\")", "value": "GainKnob"},
    {"type": "REPL", "expression": "channelVolumes.length", "value": 4},
    {"type": "REPL", "expression": "channelVolumes[2].get(\"id\")", "value": "Volume3"}
  ]
}
```

**Pitfalls:**
- Calling `Content.getComponent()` inside callbacks, timer functions, or paint routines is a common performance mistake. Each call performs a linear search through all components. In a plugin with hundreds of components, this overhead is measurable. Always cache references as `const var` at init time.
