## clone

**Examples:**

```javascript:clone-template-object
// Title: Clone a template object before modifying
// Context: When creating multiple parameter descriptors from a
// template, clone() produces an independent deep copy. Without
// it, all entries would share the same object reference and
// the last modification would overwrite all previous ones.

const var template = {
    "ID": "",
    "min": -100.0,
    "max": 0.0,
    "mode": "Decibel",
    "defaultValue": -100.0,
    "connections": [{ "processorId": "", "parameterId": "Gain" }]
};

const var params = [];

// Each clone is independent -- modifying one doesn't affect others
for(i = 0; i < 4; i++)
{
    var p = template.clone();
    var name = "Send " + (i + 1);
    p.ID = name;
    p.connections[0].processorId = name;
    params.push(p);
}

Console.print(params[0].ID);  // "Send 1"
Console.print(params[3].ID);  // "Send 4"
```
```json:testMetadata:clone-template-object
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Send 1", "Send 4"]}
  ]
}
```
