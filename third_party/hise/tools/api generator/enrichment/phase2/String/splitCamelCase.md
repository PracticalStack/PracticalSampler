## splitCamelCase

**Examples:**

```javascript:decompose-component-ids
// Title: Decompose camelCase component IDs into structured data
// Context: Name components with a convention like "FilterAttack" where the
// first token is the envelope type and the second is the parameter. Then
// splitCamelCase lets a single callback handle all envelope controls.

// --- setup ---
Content.addKnob("FilterAttack", 0, 0);
Content.addKnob("FilterDecay", 100, 0);
Content.addKnob("PlayerAttack", 200, 0);
Content.addKnob("PlayerDecay", 300, 0);
// --- end setup ---

const var envelopeControls = [Content.getComponent("FilterAttack"),
                              Content.getComponent("FilterDecay"),
                              Content.getComponent("PlayerAttack"),
                              Content.getComponent("PlayerDecay")];

inline function createSortedControls()
{
    local obj = {};
    
    for (e in envelopeControls)
    {
        local tokens = e.getId().splitCamelCase();
        local envelopeType = tokens[0]; // "Filter" or "Player"
        local parameter = tokens[1];    // "Attack" or "Decay"
        
        if (!isDefined(obj[envelopeType]))
            obj[envelopeType] = {};
        
        obj[envelopeType][parameter] = e;
    }
    
    return obj;
}

// Result: { "Filter": { "Attack": ..., "Decay": ... },
//           "Player": { "Attack": ..., "Decay": ... } }
const var sorted = createSortedControls();
```
```json:testMetadata:decompose-component-ids
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "sorted.Filter.Attack.get(\"id\")", "value": "FilterAttack"},
    {"type": "REPL", "expression": "sorted.Player.Decay.get(\"id\")", "value": "PlayerDecay"}
  ]
}
```

```javascript:camelcase-to-label
// Title: Convert internal camelCase IDs to human-readable labels
// Context: NKS integration and display labels need space-separated
// words from camelCase module attribute names.

var attributeName = "SendFilterReverbPreDelay";

// Strip the "Send" prefix (4 characters), then split and rejoin with spaces
var label = attributeName.substring(4, 10000).splitCamelCase().join(" ");
Console.print(label); // Filter Reverb Pre Delay
```
```json:testMetadata:camelcase-to-label
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Filter Reverb Pre Delay"]}
  ]
}
```
